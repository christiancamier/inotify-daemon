/*-
 * Copyright (c) 2022
 * 	Christian CAMIER <christian.c@promethee.services>
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "ind_config.h"

#include <sys/types.h>

#include <stdarg.h>
#include <stdlib.h>
#include <strings.h>
#include <syslog.h>

#include <CCR/cc_fmt.h>
#include <CCR/cc_log.h>
#include <CCR/cc_nextopt.h>

static int  slog_sopt (const char *, const char *, int);
static void slog_open (void);
static void slog_close(void);
static void slog_log  (int, const char *, va_list);

static int slog_setval_facility(const char *, int);
static int slog_setval_ident   (const char *, int);
static int slog_setval_options (const char *, int);

static char slog_ident[128] = "unnamed";
static int  slog_options    = LOG_PID;
static int  slog_facility   = LOG_USER;

struct slog_sopts_st {
	const char  *opt_name;
	int        (*opt_setval)(const char *, int);
};

struct slog_values_st {
	const char *v_name;
	int         v_val;
};

static struct slog_sopts_st slog_sopts[] = {
	{ "facility", slog_setval_facility },
	{ "ident",    slog_setval_ident    },
	{ "options",  slog_setval_options  },
	{ NULL,       NULL                 }
};

static struct slog_values_st slog_vfacilities[] = {
	{ "auth",	LOG_AUTH	},
	{ "authpriv",	LOG_AUTHPRIV	},
	{ "cron",	LOG_CRON	},
	{ "daemon",	LOG_DAEMON	},
	{ "ftp",	LOG_FTP		},
	{ "kern",	LOG_KERN	},
	{ "local0",	LOG_LOCAL0	},
	{ "local1",	LOG_LOCAL1	},
	{ "local2",	LOG_LOCAL2	},
	{ "local3",	LOG_LOCAL3	},
	{ "local4",	LOG_LOCAL5	},
	{ "local5",	LOG_LOCAL5	},
	{ "local6",	LOG_LOCAL6	},
	{ "local7",	LOG_LOCAL7	},
	{ "lpr",	LOG_LPR		},
	{ "mail",	LOG_MAIL	},
	{ "news",	LOG_NEWS	},
	{ "syslog",	LOG_SYSLOG	},
	{ "user",	LOG_USER	},
	{ "uucp",	LOG_UUCP	},
	{ NULL,		0		}
};

static struct slog_values_st slog_voptions[] = {
	{ "cons",	LOG_CONS	},
	{ "ndelay",	LOG_NDELAY	},
//	{ "none",       0               },
	{ "nowait",	LOG_NOWAIT	},
	{ "odelay",	LOG_ODELAY	},
	{ "perror",	LOG_PERROR	},
	{ "pid",	LOG_PID		},
	{ NULL,		0		}
};

static int slog_val_search(const char *name, const struct slog_values_st *values)
{
	const struct slog_values_st *pva;

	for(pva = values; pva->v_name; pva += 1)
	{
		if(0 == strcasecmp(pva->v_name, name))
			return pva->v_val;
	}
	return CC_LOG_BADOPTVAL;
}

static int slog_setval_facility(const char *value, int simu)
{
	int  ret = CC_LOG_BADOPTVAL;
	int  val;

	if(CC_LOG_BADOPTVAL != (val = slog_val_search(value, slog_vfacilities)))
	{
		if(!simu)
			slog_facility = val;
		ret = CC_LOG_OK;
	}
	return ret;
}

static int slog_setval_ident(const char *value, int simu)
{
	if(!simu)
	{
		char   *ipt = slog_ident;
		size_t  isz = sizeof(slog_ident) - 1;
		(void)cc_fmt_string(&ipt, &isz, value);
		*ipt = '\0';
	}
	return CC_LOG_OK;
}

static int slog_setval_options(const char *value, int simu)
{
	char        buff[64];
	const char *popt;
	const char *pval = value;
	int         copt;
	int         opts = 0;
	int         retv = CC_LOG_OK;

	if(strcasecmp("none", value))
	{
		slog_options = opts;
		return retv;
	}
	
	while(NULL != (popt = cc_next_opt((char **)&pval, buff, sizeof(buff), ",|")))
	{
		if(CC_LOG_BADOPTVAL == (copt = slog_val_search(popt, slog_voptions)))
		{
			retv = CC_LOG_BADOPTVAL;
			break;
		}
		if(!simu)
			opts |= copt;
	}
	if(CC_LOG_OK != retv)
		slog_options = opts;

	return retv;
}

static int slog_sopt(const char *option, const char *value, int simu)
{
	struct slog_sopts_st *pop;
	int                   ret = CC_LOG_BADOPTION;
	for(pop = slog_sopts; pop->opt_name; pop += 1)
	{
		if(0 == strcasecmp(pop->opt_name, option))
		{
			ret = pop->opt_setval(value, simu);
			break;
		}
	}
	return ret;
}

static void slog_open(void)
{
	openlog(slog_ident, slog_options, slog_facility);
	return;
}

static void slog_close(void)
{
	closelog();
	return;
}

static int cclog2syslog_levels[] = {
	LOG_EMERG,	LOG_ALERT,	LOG_CRIT,	LOG_ERR,
	LOG_WARNING,	LOG_NOTICE,	LOG_INFO,	LOG_DEBUG
};

static void slog_log(int level, const char *format, va_list ap)
{
	vsyslog(slog_facility | cclog2syslog_levels[level & 7], format, ap);
	return;
}

CC_LOG_REGISTER(
	syslog,
	"syslog logging driver",
	slog_sopt,
	slog_open,
	slog_close,
	slog_log);
