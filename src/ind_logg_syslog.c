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

#include <sys/types.h>

#include <stdarg.h>
#include <stdlib.h>
#include <strings.h>
#include <syslog.h>

#include "inotify-daemon.h"

static int  slog_sopt (const char *, const char *, int);
static void slog_open (void);
static void slog_close(void);
static void slog_log  (int, const char *, va_list);

static int slog_setval_facility(const char *, int);
static int slog_setval_ident   (const char *, int);
static int slog_setval_options (const char *, int);

static char slog_ident[128] = "inotify-daemon";
static int  slog_options    = LOG_PID;
static int  slog_facility   = LOG_DAEMON;

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
	return IN_LOG_BADOPTVAL;
}

static int slog_setval_facility(const char *value, int simu)
{
	int  ret = IN_LOG_BADOPTVAL;
	int  val;

	if(IN_LOG_BADOPTVAL != (val = slog_val_search(value, slog_vfacilities)))
	{
		if(!simu)
			slog_facility = val;
		ret = IN_LOG_OK;
	}
	return ret;
}

static int slog_setval_ident(const char *value, int simu)
{
	if(!simu)
	{
		char   *ipt = slog_ident;
		size_t  isz = sizeof(slog_ident) - 1;
		in_fmt_string(&ipt, &isz, value);
		*ipt = '\0';
	}
	return IN_LOG_OK;
}

static int slog_setval_options(const char *value, int simu)
{
	char        buff[64];
	const char *popt;
	const char *pval = value;
	int         copt;
	int         opts = 0;
	int         retv = IN_LOG_OK;

	if(strcasecmp("none", value))
	{
		slog_options = opts;
		return retv;
	}
	
	while(NULL != (popt = in_next_option((char **)&pval, buff, sizeof(buff), ",|")))
	{
		if(IN_LOG_BADOPTVAL == (copt = slog_val_search(popt, slog_voptions)))
		{
			retv = IN_LOG_BADOPTVAL;
			break;
		}
		if(!simu)
			opts |= copt;
	}
	if(IN_LOG_OK != retv)
		slog_options = opts;

	return retv;
}

static int slog_sopt(const char *option, const char *value, int simu)
{
	struct slog_sopts_st *pop;
	int                   ret = IN_LOG_BADOPTION;
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

IN_LOG_REGISTER(
	syslog,
	"syslog logging driver",
	slog_sopt,
	slog_open,
	slog_close,
	slog_log);
