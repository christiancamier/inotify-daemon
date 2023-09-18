/* 
 * Copyright: Christian CAMIER & Quentin PERIDON 2022
 * 
 * christian.c@promethee.services
 * 
 * This software is a computer program whose purpose is to manage filesystems
 * events on defined directories.
 * 
 * This software is governed by the CeCILL-B license under French law and
 * abiding by the rules of distribution of free software.  You can  use, 
 * modify and/ or redistribute the software under the terms of the CeCILL-B
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info". 
 * 
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability. 
 * 
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or 
 * data to be ensured and,  more generally, to use and operate it in the 
 * same conditions as regards security. 
 * 
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-B license and that you accept its terms.
 */

#include "ind_config.h"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glob.h>
#include <pwd.h>
#include <grp.h>

#include "inotify-daemon.h"
#include "ind_tunables.h"

#define IN_BUFFER_REALSIZE IN_BUFFER_ROUNDUP(IN_BUFSIZE + 1)

extern in_status_t in_configuration_read(const char *, unsigned int);

struct event_st;
struct context_st;
struct keyword_st;

typedef in_status_t (*kw_func_t)(struct context_st *, struct keyword_st *, char *, char *);

typedef struct context_st {
	int             ct_is_top;
        int             ct_filedesc;
        const char     *ct_filename;
        size_t          ct_lineno;
	void           *ct_data;
        char           *ct_bufpos;
        char           *ct_buffre;
        unsigned long   ct_buffer[IN_BUFFER_REALSIZE];
} context_t;

typedef struct event_st {
	const char *ev_name;
	uint32_t    ev_mask;
	int         ev_cplx;
} event_t;
	;
typedef struct keyword_st {
        const char *kw_name;    /* Keyword name        */
        kw_func_t   kw_func;    /* Associated function */
} keyword_t;

#define KW_FUNC_NULL (kw_func_t)NULL
#define KW_FUNC_END  (kw_func_t)-1

static in_status_t process_lines(context_t *, const keyword_t *);
static in_status_t read_from_file(const char *);
static in_status_t subprocess_lines(context_t *, const keyword_t *);
static size_t      split(char **, char **, size_t, int);

static in_status_t error            (context_t *, in_status_t, const char *, ...);
static in_status_t malformed        (context_t *, const char *);
static in_status_t warning          (context_t *, in_status_t, const char *, ...);

static in_status_t kw_main_directory(context_t *, keyword_t *, char *, char *);
static in_status_t kw_main_include  (context_t *, keyword_t *, char *, char *);
static in_status_t kw_main_settings (context_t *, keyword_t *, char *, char *);
static in_status_t kw_dire_event    (context_t *, keyword_t *, char *, char *);
static in_status_t kw_dire_group    (context_t *, keyword_t *, char *, char *);
static in_status_t kw_dire_shell    (context_t *, keyword_t *, char *, char *);
static in_status_t kw_dire_user     (context_t *, keyword_t *, char *, char *);
static in_status_t kw_logg_level    (context_t *, keyword_t *, char *, char *);
static in_status_t kw_logg_set      (context_t *, keyword_t *, char *, char *);
static in_status_t kw_sett_logging  (context_t *, keyword_t *, char *, char *);
static in_status_t kw_sett_pidfile  (context_t *, keyword_t *, char *, char *);

static keyword_t kw_main[] = {
	{ "directory", kw_main_directory },
	{ "include",   kw_main_include   },
	{ "settings",  kw_main_settings  },
	{ NULL,        NULL              }
};

static keyword_t kw_dire[] = {
	{ "end",       KW_FUNC_END       },
	{ "event",     kw_dire_event     },
	{ "group",     kw_dire_group     },
	{ "shell",     kw_dire_shell     },
	{ "user",      kw_dire_user      },
	{ NULL,        NULL              }
};

static keyword_t kw_logg[] = {
	{ "end",       KW_FUNC_END       },
	{ "level",     kw_logg_level     },
	{ "set",       kw_logg_set       },
	{ NULL,        NULL              }
};

static keyword_t kw_sett[] = {
	{ "end",       KW_FUNC_END       },
	{ "logging",   kw_sett_logging   },
	{ "pidfile",   kw_sett_pidfile   },
};

static const char ifs[]     = " \t\n\r";

static unsigned int cliopts = 0;
static unsigned int unicity = 0;
#define UNICITY_SETTINGS (1 << 0)
#define UNICITY_LOGGING  (1 << 1)
#define UNICITY_PIDFILE  (1 << 2)

/*
 * misc functions
 */

static in_status_t error(context_t *context, in_status_t status, const char *format, ...)
{
	char    bu[128];
	va_list ap;
	int     er;

	er = errno;
	va_start(ap, format);
	vsnprintf(bu, sizeof(bu), format, ap);
	va_end(ap);
	in_log_error("Error %s [%lu]: %s\n", IN_IFNULL(char, context->ct_filename, ""), (unsigned long)(context->ct_lineno), bu);
	errno = er;
	return status;
}

static in_status_t malformed(context_t *context, const char *directive)
{
	return error(context, IN_ST_SYNTAX_ERROR, "%s: malformed directive", directive);
}


static in_status_t warning(context_t *context, in_status_t status, const char *format, ...)
{
	char    bu[128];
	va_list ap;
	int     er;

	er = errno;
	va_start(ap, format);
	vsnprintf(bu, sizeof(bu), format, ap);
	va_end(ap);
	in_log_warning("Warning %s [%lu]: %s\n", IN_IFNULL(char, context->ct_filename, ""), (unsigned long)(context->ct_lineno), bu);
	errno = er;
	return status;
}

/*
 * main keywords processing
 */

static in_status_t kw_main_directory(context_t *context, keyword_t *keyword, char *arg0, char *args)
{
	size_t          argc;
	char           *argv[1];
	char           *path;
	in_directory_t *dire;
	in_status_t     status;

	IN_CODE_DEBUG("Entering (%p, %p, %s, %s)", context, keyword, arg0, args);
	if(0 == (argc = split(&args, argv, 1, 1)))
		return malformed(context, arg0);
	path = *argv;
	if(IN_ST_OK != (status = in_directory_create(path, &dire)))
	{
		IN_CODE_DEBUG("Return %d", (int)status);
		return error(context, status, "%s: cannot allocate new directory", arg0);
	}
	context->ct_data = (void *)dire;
	status = subprocess_lines(context, kw_dire);
	context->ct_data = NULL;
	if(-1 == dire->dir_uid) dire->dir_uid = 0;
	if(-1 == dire->dir_gid) dire->dir_gid = 0;
	IN_CODE_DEBUG("Return %d", (int)status);
	return status;
}

static int glob_error(const char *epath, int eerrno)
{
	IN_PROTECT_ERRNO((void)in_log_warning("%s: %s", epath, strerror(eerrno)));
	return 1;
}

static in_status_t kw_main_include(context_t *context, keyword_t *keyword, char *arg0, char *args)
{
	int              gret;
	glob_t           gval;
	char            *argv[1];
	size_t           argc;
	in_status_t      status;
	size_t           pathc;
	char           **pathv;

	IN_CODE_DEBUG("Entering (%p, %p, %s, %s)", context, keyword, arg0, args);
	status = IN_ST_OK;
	if(0 == (argc = split(&args, argv, 1, 1)))
	{
		IN_CODE_DEBUG("Return %d", (int)status);
		return malformed(context, arg0);
	}
	gval.gl_pathc = 0;
	gval.gl_pathv = NULL;
	gval.gl_offs  = 0;
	if(0 != (gret = glob(*argv, GLOB_ERR | GLOB_BRACE | GLOB_TILDE, glob_error, &gval)))
	{
		switch(gret)
		{
		case GLOB_NOSPACE:
			IN_CODE_DEBUG("Return %d", IN_ST_SYSTEM_ERROR);
			return error(context, IN_ST_SYSTEM_ERROR, "%s: insufisant memory space", arg0);
		case GLOB_ABORTED:
		case GLOB_NOMATCH:
			break;
		default:
			IN_CODE_DEBUG("Return %d", IN_ST_INTERNAL);
			return warning(context, IN_ST_INTERNAL, "%s: unknown glob(3) error %d", arg0, gret);
		}
	}

	if(0 == gret)
	{
		for(pathv = gval.gl_pathv, pathc = 0; pathc < gval.gl_pathc; pathc += 1, pathv += 1)
		{
			if(IN_ST_OK != (status = read_from_file(*pathv)))
			{
				(void)warning(context, status, "%s: cannot process file %s", arg0, *pathv);
				break;
			}
		}
		IN_PROTECT_ERRNO(globfree(&gval));
	}
	IN_CODE_DEBUG("Return %d", (int)status);
	return status;
}

static in_status_t kw_main_settings(context_t *context, keyword_t *keyword, char *arg0, char *args)
{
	char *rem;
#if defined(DEBUG)
	in_status_t retval;
#endif

	IN_CODE_DEBUG("Entering(%p, %p, %s, %s)", context, keyword, arg0, args);

	if(args && NULL != strtok_r(args, ifs, &rem))
	{
		IN_CODE_DEBUG("Return IN_ST_SYNTAX_ERROR");
		return malformed(context, arg0);
	}
	if(UNICITY_SETTINGS == (unicity & UNICITY_SETTINGS))
	{
		IN_CODE_DEBUG("Return IN_ST_SYNTAX_ERROR");
		return error(context, IN_ST_SYNTAX_ERROR, "%s: duplicate directive", keyword->kw_name);
	}
#if defined(DEBUG)
	retval = subprocess_lines(context, kw_sett);
	IN_CODE_DEBUG("Return %d", retval);
#else
	return subprocess_lines(context, kw_sett);
#endif
}
/*
 * directory definition keywords
 */
static in_status_t kw_dire_event(context_t *context, keyword_t *keyword, char *arg0, char *args)
{
	char           *evs;
	char           *rem;
	char           *cmd;
	uint32_t        msk;
	in_status_t     sta;
	size_t          pos;
	in_directory_t *dir;
	in_action_t    *act;

	IN_CODE_DEBUG("Entering (%p, %p, %s, %s)", context, keyword, arg0, args);
	dir = (in_directory_t *)context->ct_data;
	evs = strtok_r(args, ifs, &rem);
	if(NULL == evs || '\0' == *evs || NULL == rem || '\0' == *rem)
		return error(context, IN_ST_VALUE_ERROR, "%s: incomplete directive", arg0);
	if(IN_ST_OK != (sta = in_str2events(evs, &msk)))
		return error(context, IN_ST_VALUE_ERROR, "Bad event definition '%s'", evs);
	if(0 != (msk & dir->dir_mask))
	{
		char evtstr[128];
		IN_CODE_DEBUG("msk = %X, dir msk = %X", msk, dir->dir_mask);
		in_events2str(evtstr, sizeof(evtstr), msk & dir->dir_mask);
		return error(context, IN_ST_VALUE_ERROR, "Duplicate events : `%s'", evtstr);
	}
	while('\0' != *rem && isspace(*rem)) rem += 1;
	for(act = dir->dir_actions, pos = 0; pos < dir->dir_nactions; act += 1, pos += 1)
	{
		if(0 == strcmp(rem, act->act_command))
		{
			act->act_mask |= msk;
			goto event_found;
		}
	}
	if(NULL == (cmd = strdup(rem)))
		return error(context, IN_ST_SYSTEM_ERROR, "Cannot duplicate string '%s'", rem);
	pos = dir->dir_nactions++;
	dir->dir_actions[pos].act_mask    = msk;
	dir->dir_actions[pos].act_command = cmd;
event_found:
	dir->dir_mask |= msk;
	return IN_ST_OK;
}

static in_status_t str2integer(const char *string, long *retval)
{
	long  value;
	char *endpt;
	endpt = NULL;
	value = strtol(string, &endpt, 0);
	if(value < 0 || NULL != endpt || '\0' != *endpt)
		return IN_ST_VALUE_ERROR;
	if(NULL != retval)
		*retval = value;
	return IN_ST_OK;
}

static in_status_t kw_dire_group(context_t *context, keyword_t *keyword, char *arg0, char *args)
{
	size_t          argc;
	char           *argv[1];
	long            value;
	in_directory_t *dire;

	IN_CODE_DEBUG("Entering (%p, %p, %s, %s)", context, keyword, arg0, args);
	if(0 == (argc = split(&args, argv, 1, 1)))
		return malformed(context, arg0);
	dire = (in_directory_t *)context->ct_data;
	if(IN_ST_OK == str2integer(*argv, &value))
	{
		dire->dir_gid = (gid_t)value;
	}
	else
	{
		struct group *grent;
		setgrent();
		grent = getgrnam(*argv);
		endgrent();
		if(NULL == grent)
			return error(context, IN_ST_NOT_FOUND, "%s: unknown group `%s'", arg0, *argv);
		dire->dir_gid = grent->gr_gid;
	}
	return IN_ST_OK;
}

static in_status_t kw_dire_shell(context_t *context, keyword_t *keyword, char *arg0, char *args)
{
	size_t          argc;
	char           *argv[1];
	char           *shell;
	in_directory_t *dire;

	IN_CODE_DEBUG("Entering (%p, %p, %s, %s)", context, keyword, arg0, args);
	if(0 == (argc = split(&args, argv, 1, 1)))
		return malformed(context, arg0);
	dire = (in_directory_t *)context->ct_data;
	if(NULL == (shell = strdup(*argv)))
	{
		return error(context, IN_ST_SYSTEM_ERROR, "Cannot duplicate string '%s'", shell);
	}
	dire->dir_shell = shell;
	return IN_ST_OK;
}

static in_status_t kw_dire_user(context_t *context, keyword_t *keyword, char *arg0, char *args)
{
	size_t          argc;
	char           *argv[1];
	long            value;
	in_directory_t *dire;

	IN_CODE_DEBUG("Entering (%p, %p, %s, %s)", context, keyword, arg0, args);
	if(0 == (argc = split(&args, argv, 1, 1)))
		return malformed(context, arg0);
	dire = (in_directory_t *)context->ct_data;
	if(IN_ST_OK == str2integer(*argv, &value))
	{
		dire->dir_uid = (gid_t)value;
	}
	else
	{
		struct passwd *pwent;
		setpwent();
		pwent = getpwnam(*argv);
		endpwent();
		if(NULL == pwent)
			return error(context, IN_ST_NOT_FOUND, "%s: unknown user `%s'", arg0, *argv);
		dire->dir_uid = pwent->pw_uid;
	}
	return IN_ST_OK;
}

static in_status_t kw_logg_level(context_t *context, keyword_t *keyword, char *arg0, char *args)
{
	char            *argv[1];
	size_t           argc;
	in_log_level_t   llev;

	IN_CODE_DEBUG("Entering (%p, %p, %s, %s)", context, keyword, arg0, args);
	if(0 == (argc = split(&args, argv, 1, 1)))
		return malformed(context, arg0);
	if((in_log_level_t )-1 == (llev = in_log_level_by_name(*argv)))
		return error(context, IN_ST_VALUE_ERROR, "%s: bad level name `%s'", arg0, *argv);
	if(0 == (cliopts & CL_OPT_LOGLVL))
		in_log_set_level(llev);
	return IN_ST_OK;
}

static in_status_t kw_logg_set(context_t *context, keyword_t *keyword, char *arg0, char *args)
{
	char            *ldrv;
	char            *argv[2];
	size_t           argc;

	IN_CODE_DEBUG("Entering (%p, %p, %s, %s)", context, keyword, arg0, args);
	if(0 == (argc = split(&args, argv, 2, 1)))
		return malformed(context, arg0);
	ldrv = (char *)context->ct_data;
	switch(in_log_tst_drv_opt(ldrv, argv[0], argv[1]))
	{
	case IN_LOG_BADOPTION:
		return error(context, IN_ST_NOT_FOUND, "%s: Log driver `%s' does not have `%s' as option", arg0, ldrv, argv[0]);
	case IN_LOG_BADOPTVAL:
		return error(context, IN_ST_VALUE_ERROR, "%s: Log driver `%s', option `%s', bad value `%s'", arg0, ldrv, argv[0], argv[1]);
	case IN_LOG_BADDRIVER:
		return error(context, IN_ST_INTERNAL, "%s: Unknown log driver `%s'", arg0, ldrv);
	default:
		break;
	}
	if(0 == (cliopts & CL_OPT_LOGOPT))
		(void)in_log_set_drv_opt(ldrv, argv[0], argv[1]);
	return IN_ST_OK;
}

/*
 * Settings directives
 */
static in_status_t kw_sett_logging(context_t *context, keyword_t *keyword, char *arg0, char *args)
{
	in_status_t  status;
	char        *driver;
	char        *rest;

	IN_CODE_DEBUG("Entering (%p, %p, %s, %s)", context, keyword, arg0, args);
	driver = strtok_r(args, ifs, &rest);
	if(NULL == driver || '\0' == *driver || (NULL != rest && '\0' != *rest))
		return malformed(context, arg0);
	if(IN_LOG_BADDRIVER == in_log_driver_exists(driver))
		return error(context, IN_ST_VALUE_ERROR, "Logging driver '%s' does not exists", driver);
	context->ct_data = (void *)driver;
	if(IN_ST_OK == (status = subprocess_lines(context, kw_logg)))
	{
		if(0 == (cliopts & CL_OPT_LOGDRV))
			(void)in_log_set_driver(driver);
	}
	context->ct_data = NULL;
	return status;
}

static in_status_t kw_sett_pidfile(context_t *context, keyword_t *keyword, char *arg0, char *args)
{
	size_t          argc;
	char           *argv[1];

	IN_CODE_DEBUG("Entering (%p, %p, %s, %s)", context, keyword, arg0, args);
	if(0 == (argc = split(&args, argv, 1, 1)))
		return malformed(context, arg0);
	if(-1 == in_ctl_pidfile(*argv))
		return error(context, IN_ST_VALUE_ERROR, "%s: bad pidfile definition '%s'", arg0, *argv);
	if((0 == (cliopts & CL_OPT_PIDFILE)) && (-1 == in_set_pidfile(*argv)))
		return error(context, IN_ST_VALUE_ERROR, "%s: bad pidfile definition '%s'", arg0, *argv);
	
	return IN_ST_OK;
}

/*
 * Configuration file interpretation
 */

static size_t split(char **line, char **args, size_t nargs, int strict)
{
	char    *li;
	char    *re;
	char   **ca;
	char    *na;
	size_t   nb;

	for(li = na = *line, ca = args, nb = 0; na && nb < nargs; li = NULL, nb += 1, ca += 1)
	{
		na = strtok_r(li, ifs, &re);
		if(NULL == na)
			break;
		*ca = na;
	}
	*line = re;
	if(strict && (nb < nargs || (re && *re != '\0')))
	{
		return 0;
	}
	return nb;
}

static in_status_t get_one_line(context_t *context, char **retv)
{
	char *rval;
	char *cpos;
	char *epos;

	epos = context->ct_buffre;
	for(rval = context->ct_bufpos; rval < epos && isspace(*rval) && *rval != '\n'; rval += 1);
	for(cpos = rval; cpos < epos; cpos += 1)
	{
		if(*cpos == '\n')
		{
			*(cpos) = '\0';
			context->ct_bufpos  = cpos + 1;
			context->ct_lineno += 1;
			while(cpos > rval && isspace(*cpos)) *(cpos--) = '\0';
			*retv = rval;
			return IN_ST_OK;
		}
	}
	return IN_ST_LINE_TOO_LONG;
}

static in_status_t refill_buffer(context_t *context)
{
	size_t      nbytes;
	ssize_t     nread;
	in_status_t status;

	status = IN_ST_OK;

	if(context->ct_bufpos < context->ct_buffre)
	{
		size_t used = context->ct_buffre - context->ct_bufpos;
		(void)memcpy(context->ct_buffer, context->ct_bufpos, used);
		nbytes = IN_BUFSIZE - used;
		context->ct_bufpos = (char *)context->ct_buffer;
		context->ct_buffre = context->ct_bufpos + used;
	}
	else
	{
		context->ct_bufpos = (char *)context->ct_buffer;
		context->ct_buffre = (char *)context->ct_buffer;
		nbytes = IN_BUFSIZE;
	}
	nread = read(context->ct_filedesc, context->ct_buffre, nbytes);
	switch(nread)
	{
	case  0:
		status = IN_ST_END_OF_FILE;
		break;
	case -1:
		status = IN_ST_SYSTEM_ERROR;
		break;
	default:
		context->ct_buffre += nread;
		*(context->ct_buffre) = '\0';
	}
	return status;
}

static in_status_t readline(context_t *context, char **retv)
{
	in_status_t status;
	if(IN_ST_OK == get_one_line(context, retv))
		return IN_ST_OK;
	if(IN_ST_OK != (status = refill_buffer(context)))
	{
		if(IN_ST_END_OF_FILE == status && context->ct_bufpos < context->ct_buffre)
		{
			*retv = context->ct_bufpos;
			return IN_ST_OK;
		}
		return status;
	}
	return get_one_line(context, retv);
}

static in_status_t process_lines(context_t *context, const keyword_t *keywords)
{
	char           *line;
	char           *rest;
	char           *word;
	in_status_t     status;
	keyword_t      *keyword;

	while(IN_ST_OK == (status = readline(context, &line)))
	{
		word = strtok_r(line, ifs, &rest);
		if(!word || *word == '\0' || *word == '#')
			continue;
		for(keyword = (keyword_t *)keywords; keyword->kw_name; keyword += 1)
			if(0 == strcasecmp(word, keyword->kw_name))
				goto found;
		return warning(context, IN_ST_SYNTAX_ERROR, "unknown keyword %s", word);
	found:
		if(KW_FUNC_END == keyword->kw_func)
		{
			if(context->ct_is_top)
				return warning(context, IN_ST_SYNTAX_ERROR, "misplaced %s directive", keyword->kw_name);
			return IN_ST_OK;
		}
		if(IN_ST_OK != (status = keyword->kw_func(context, keyword, word, rest)))
			return status;
	}
	return (context->ct_is_top) ? IN_ST_OK : warning(context, IN_ST_PREMATURE_END_OF_FILE, "premature end of file");
}

static in_status_t subprocess_lines(context_t *context, const keyword_t *keywords)
{
	int         is_top;
	in_status_t status;
	is_top = context->ct_is_top;
	context->ct_is_top = 0;
	status = process_lines(context, keywords);
	context->ct_is_top = is_top;
	return status;
}

static in_status_t read_from_file(const char *filename)
{
	context_t    context;
	int          filedesc;
	in_status_t  status;
	struct stat  finfo;

	IN_CODE_DEBUG("Entering (%s)\n", filename);

	status = IN_ST_SYSTEM_ERROR;

	if(-1 == stat(filename, &finfo))
	{
		IN_PROTECT_ERRNO(in_log_perror(filename));
		return status;
	}

	if(-1 == (filedesc = open(filename, O_RDONLY)))
	{
		IN_PROTECT_ERRNO(in_log_perror(filename));
		return IN_ST_SYSTEM_ERROR;
	}

	context.ct_is_top    = 1;
	context.ct_filedesc  = filedesc;
	context.ct_filename  = filename;
	context.ct_lineno    = 0;
	context.ct_data      = NULL;
	context.ct_bufpos    = (char *)context.ct_buffer;
	context.ct_buffre    = (char *)context.ct_buffer;

	status = process_lines(&context, kw_main);
	IN_PROTECT_ERRNO(close(filedesc));
	return status;
}

in_status_t in_configuration_read(const char *filename, unsigned int optmask)
{
	IN_CODE_DEBUG("Entering (%s)\n", filename);

	unicity = 0;
	cliopts = optmask;
	in_directory_purge();
	return read_from_file(filename);
}
