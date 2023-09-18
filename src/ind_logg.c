#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

extern void            in_log_add_option(const char *, int(*)(const char *, const char *, void *, int), void *);
extern int             in_log_driver_exists(const char *);
extern size_t          in_log_driver_list  (struct in_log_driver_info *, size_t);
extern in_log_level_t  in_log_level_by_name(const char *);
extern const char     *in_log_level_name   (in_log_level_t);
extern void            in_log_register     (struct in_logger_st *);
extern in_log_level_t  in_log_set_level    (in_log_level_t);
extern int             in_log_set_driver   (const char *);
extern int             in_log_set_option   (const char *, const char *);
extern int             in_log_set_drv_opt  (const char *, const char *, const char *);
extern const char     *in_log_timestamp    (void);

extern void            in_log_alert        (const char *, ...);
extern void            in_log_crit         (const char *, ...);
extern void            in_log_debug        (const char *, ...);
extern void            in_log_emerg        (const char *, ...);
extern void            in_log_error        (const char *, ...);
extern void            in_log_info         (const char *, ...);
extern void            in_log_notice       (const char *, ...);
extern void            in_log_warning      (const char *, ...);
extern void            in_log_log          (in_log_level_t, const char *, ...);
extern void            in_log_vlog         (in_log_level_t, const char *, va_list);
extern void            in_log_dbgcod       (const char *, size_t, const char *, ...);

extern void            in_log_panic          (const char *, ...) __attribute__((noreturn));
extern void            in_log_perror	     (const char *);

static struct in_logger_st *find_driver(const char *);

static int  dfl_sop(const char *, const char *, int);
static void dfl_log(in_log_level_t, const char *, va_list);

struct logger_opt_st {
	struct logger_opt_st  *next;
	const char            *option;
	int                  (*setter)(const char *, const char *, void *, int);
	void                  *data;
};

static struct logger_opt_st *options = NULL;

static struct in_logger_st     defdrv[1] = {
	{
		.next    = NULL,
		.name    = "default",
		.desc    = "default logging driver (stderr)",
		.sopt    = dfl_sop,
		.open    = NULL,
		.close   = NULL,
		.log     = dfl_log
	}
};
		
static const char *lvl_names[] = {
	"EMERG",	"ALERT",	"CRIT",		"ERROR",
	"WARNING",	"NOTICE",	"INFO",		"DEBUG"
};

static struct in_logger_st    *drivers    = defdrv;
static struct in_logger_st    *curdrv     = defdrv;
static in_log_level_t          curlev     = IN_LOG_INFO;
static char                    tistmp[64] = "%d/%m/%Y %H:%M:%S";

/*
 * NAME
 *     in_log_add_option -- Add a global logger option
 * SYNOPSIS
 *     void in_log_add_option(const char *opt, int (*set)(const char *, const char *, void *, int), void *dat)
 * ARGUMENTS
 *     * opt : Option name
 *     * set : Setter callback. This setter take (in order) the following options :
 *             * The option name
 *             * The option value
 *             * The user defined data
 *             * The simulation indicator. If set, the option setting must be just validated.
 *             The setter function can return one of the following values :
 *             * IN_LOG_OK
 *             * IN_LOG_BADOPTION
 *             * IN_LOG_BADOPTVAL
 *     * dat : Attached user defined data.
 */

void in_log_add_option(const char *opt, int (*set)(const char *, const char *, void *, int), void *dat)
{
	struct logger_opt_st *newopt;
	if(NULL == (newopt = (struct logger_opt_st *)malloc(sizeof(struct logger_opt_st))))
	{
		in_log_panic("Cannot allocate %lu bytes for log option", sizeof(struct logger_opt_st));
	}
	newopt->next   = options;
	newopt->option = opt;
	newopt->setter = set;
	newopt->data   = dat;
	options = newopt;
	return;
}

int in_log_driver_exists(const char *name)
{
	return (NULL == find_driver(name)) ? IN_LOG_BADDRIVER : IN_LOG_OK;
}

size_t in_log_driver_list(struct in_log_driver_info *drvnfos, size_t drvnb)
{
	size_t                     ndrvfnd;
	struct in_log_driver_info *pdrvnfo;
	struct in_logger_st       *pdrv;

	for(ndrvfnd = 0, pdrv = drivers, pdrvnfo = drvnfos; pdrv; pdrv = pdrv->next, ndrvfnd += 1)
	{
		if(pdrvnfo && ndrvfnd < drvnb)
		{
			pdrvnfo->name = pdrv->name;
			pdrvnfo->desc = pdrv->desc;
			pdrvnfo += 1;
		}
	}
	return ndrvfnd;
}


void in_log_register(struct in_logger_st *driver)
{
	driver->next = drivers;
	drivers = driver;
	return;
}

in_log_level_t  in_log_level_by_name  (const char *name)
{
	int          level;
	const char **plevl;
	for(level = 0, plevl = lvl_names; level < IN_ARRAY_COUNT(lvl_names); plevl += 1, level += 1)
		if(0 == strcasecmp(name, *plevl))
			return (in_log_level_t)level;
	return (in_log_level_t)-1;
}

const char *in_log_level_name(in_log_level_t level)
{
	int lvl = (int)level;
	if(lvl < 0 || lvl > 7)
		return "(unk)";
	return lvl_names[lvl];
}

in_log_level_t in_log_set_level(in_log_level_t new)
{
	in_log_level_t ret = curlev;
	if(new >= IN_LOG_EMERG && new <= IN_LOG_DEBUG)
	{
		curlev = new;
	}
	return ret;
}

int in_log_set_driver(const char *name)
{
	struct in_logger_st *drvptr;

	if(NULL != (drvptr = find_driver(name)))
	{
		if(drvptr->open ) drvptr->open ();
		if(curdrv->close) curdrv->close();
		curdrv = drvptr;
		return 0;
	}
	return IN_LOG_BADDRIVER;
}

static int set_option(struct in_logger_st *driver, const char *option, const char *value, int simu)
{
	struct logger_opt_st *curopt;

	if(0 == strcasecmp("timestamp", option))
	{
		if(!simu)
		{
			char   *tipos = tistmp;
			size_t  tisiz = sizeof(tistmp) - 1;
		
			(void)in_fmt_string(&tipos, &tisiz, value);
			*(tipos) = '\0';
		}
		return 0;
	}

	for(curopt = options; curopt; curopt = curopt->next)
	{
		if(0 == strcasecmp(option, curopt->option))
			return curopt->setter(option, value, curopt->data, simu);
	}

	if(driver->sopt)
		return driver->sopt(option, value, simu);

	return IN_LOG_BADOPTION;
}

int in_log_set_option(const char *option, const char *value)
{
	return set_option(curdrv, option, value, 0);
}

int in_log_set_drv_opt(const char *driver, const char *option, const char *value)
{
	struct in_logger_st *drv = find_driver(driver);
	if(NULL == drv)
		return IN_LOG_BADDRIVER;
	return set_option(drv, option, value, 0);
}

int in_log_tst_option(const char *option, const char *value)
{
	return set_option(curdrv, option, value, 1);
}

int in_log_tst_drv_opt(const char *driver, const char *option, const char *value)
{
	struct in_logger_st *drv = find_driver(driver);
	if(NULL == drv)
		return IN_LOG_BADDRIVER;
	return set_option(drv, option, value, 1);
}

const char *in_log_timestamp(void)
{
	return tistmp;
}

void in_log_alert(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	in_log_vlog(IN_LOG_ALERT, format, ap);
	va_end(ap);
	return;
}

void in_log_crit(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	in_log_vlog(IN_LOG_CRIT, format, ap);
	va_end(ap);
	return;
}

void in_log_debug(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	in_log_vlog(IN_LOG_DEBUG, format, ap);
	va_end(ap);
	return;
}

void in_log_emerg(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	in_log_vlog(IN_LOG_EMERG, format, ap);
	va_end(ap);
	return;
}

void in_log_error(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	in_log_vlog(IN_LOG_ERR, format, ap);
	va_end(ap);
	return;
}

void in_log_info(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	in_log_vlog(IN_LOG_INFO, format, ap);
	va_end(ap);
	return;
}

void in_log_notice(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	in_log_vlog(IN_LOG_NOTICE, format, ap);
	va_end(ap);
	return;
}

void in_log_warning(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	in_log_vlog(IN_LOG_WARNING, format, ap);
	va_end(ap);
	return;
}

void in_log_log(in_log_level_t level, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	in_log_vlog(level, format, ap);
	va_end(ap);
	return;
}

void in_log_vlog(in_log_level_t level, const char *format, va_list ap)
{
	if(level <= curlev)
		curdrv->log(level, format, ap);
	return;
}

void in_log_dbgcod(const char *srcfile, size_t srcline, const char *format, ...)
{
	if(curlev >= IN_LOG_DEBUG)
	{
		int   sverrno = errno;
		char  buff[128];
		char *srcbasename;
		va_list ap;
		va_start(ap, format);
		(void)vsnprintf(buff, sizeof(buff), format, ap);
		va_end(ap);
		if(NULL == (srcbasename = strrchr(srcfile, '/')))
			srcbasename = (char *)srcfile;
		else
			srcbasename += 1;
		//printf("%s [%lu]: %s", srcbasename, srcline, buff);
		in_log_debug("%s [%lu]: %s", srcbasename, srcline, buff);
		errno = sverrno;
	}
	return;
}

void in_log_perror(const char *message)
{
	int sverrno = errno;
	in_log_error("%s: error %d (%s)", message, errno, strerror(errno));
	errno = sverrno;
	return;
}

__attribute__((noreturn)) void in_log_panic(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	in_log_vlog(IN_LOG_ALERT, format, ap);
	va_end(ap);
	exit(1);
}

static struct in_logger_st *find_driver(const char *name)
{
	struct in_logger_st *drv;
	for(drv = drivers; drv; drv = drv->next)
	{
		if(0 == strcasecmp(name, drv->name))
		{
			return drv;
		}
	}
	return NULL;
}

/*
 * The default output driver
 */

static FILE *dfl_channel = NULL;

static int  dfl_sop(const char *option, const char *value, int simu)
{
	int rep = IN_LOG_BADOPTION;
	if(0 == strcasecmp(option, "channel"))
	{
		rep = IN_LOG_BADOPTVAL;
		if(0 == strcasecmp(value, "stdin"))
		{
			if(!simu)
				dfl_channel = stdin;
			rep = IN_LOG_OK;
		}
		if(0 == strcasecmp(value, "stderr"))
		{
			if(!simu)
				dfl_channel = stderr;
			rep = IN_LOG_OK;
		}
	}
	
	return rep;
}

static void dfl_log(in_log_level_t level, const char *format, va_list ap)
{
	static char  buffer[1024];
	char        *bufpos = buffer;
	size_t       bufsiz = sizeof(buffer);
	// fprintf(stderr, "**** %p %p %p\n", dfl_channel, stdout, stderr);
	(void)in_fmt_timestamp(&bufpos, &bufsiz, tistmp);
	(void)in_fmt_string   (&bufpos, &bufsiz, " - [");
	(void)in_fmt_string   (&bufpos, &bufsiz, in_log_level_name(level));
	(void)in_fmt_string   (&bufpos, &bufsiz, "] - ");
	(void)vsnprintf(bufpos, bufsiz, format, ap);
	fprintf(dfl_channel, "%s\n", buffer);
	return;
}

static __attribute__((constructor)) void ctor_default(void)
{
	dfl_channel = stderr;
	return;
}