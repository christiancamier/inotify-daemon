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
#include <sys/param.h>
#include <errno.h>
#include <limits.h>
#include <paths.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

#include "inotify-daemon.h"
#include "ind_tunables.h"
#include "ind_opts.h"

#define VERSION		"0.1"

extern void in_reread_configuration(void);
extern int  main(int, char **);

static const char *usage = ""
	"CC %P %V, File notification daemon\n\n"
	"Usage: %p [options] file...\n"
	"\n"
	"Options:\n%O\n";

static const char *version = ""
	"CC %P %V\n"
	"\n"
	"Copyright (c) 2010\n"
	"\tChristian CAMIER <chcamier@free.fr>\n"
	"\n"
	"Permission to use, copy, modify, and distribute this software for any\n"
	"purpose with or without fee is hereby granted, provided that the above\n"
	"copyright notice and this permission notice appear in all copies.\n"
	"\n"
	"THE SOFTWARE IS PROVIDED \"AS IS\" AND THE AUTHOR DISCLAIMS ALL WARRANTIES\n"
	"WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF\n"
	"MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR\n"
	"ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES\n"
	"WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN\n"
	"ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF\n"
	"OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.\n";

static struct in_option options_defs[] = {
	IN_OPT_ENTRY(
		'C', 'C', "configuration",
		IN_OPTARG_REQUIRED,
		NULL, 0, IN_OPTARG_OPENONE,
		"Configuration file"),
	IN_OPT_ENTRY(
		'f', 'f', "foreground",
		IN_OPTARG_NONE,
		NULL, 0, IN_OPTARG_OPENONE,
		"Don't fork, stay foreground"),
	IN_OPT_ENTRY(
		'h', 'h', "help",
		IN_OPTARG_NONE,
		NULL, 0, IN_OPTARG_HELP,
		"display this help"),
	IN_OPT_ENTRY(
		'L', 'L', "logdriver",
		IN_OPTARG_REQUIRED,
		NULL, 0, IN_OPTARG_OPENONE,
		"set log driver"),
	IN_OPT_ENTRY(
		'l', 'l', "loglevel",
		IN_OPTARG_REQUIRED,
		NULL, 0, IN_OPTARG_OPENONE,
		"set log level"),
	IN_OPT_ENTRY(
		'o', 'o', "logoption",
		IN_OPTARG_REQUIRED,
		NULL, 0, IN_OPTARG_OPENONE,
		"set log option"),
	IN_OPT_ENTRY(
		'p', 'p', "pidfile",
		IN_OPTARG_REQUIRED,
		NULL, 0, IN_OPTARG_OPENONE,
		"set pid file"),
	IN_OPT_ENTRY(
		'R', 'R', "reload",
		IN_OPTARG_NONE,
		NULL, 0, IN_OPTARG_OPENONE,
		"Reload daemon configuration"),
	IN_OPT_ENTRY(
		'S', 'S', "stop",
		IN_OPTARG_NONE,
		NULL, 0, IN_OPTARG_OPENONE,
		"Stop daemon"),
	IN_OPT_ENTRY(
		's', 's', "status",
		IN_OPTARG_NONE,
		NULL, 0, IN_OPTARG_OPENONE,
		"Test if daemon is running (outputs are info level)"),
	IN_OPT_ENTRY(
		'T', 'T', "configtest",
		IN_OPTARG_NONE,
		NULL, 0, IN_OPTARG_OPENONE,
		"Do not run, just test configuration file"),
	IN_OPT_ENTRY(
		'V', 'V', "version",
		IN_OPTARG_NONE,
		NULL, 0, IN_OPTARG_VERSION,
		"display programm version"),
};

static unsigned int cliopts = 0;
static char        pid_file [MAXPATHLEN + 1];
static char        conf_file[MAXPATHLEN + 1];
static const char *logdriver = "default";

static jmp_buf ind_jmp_env;

static void logging_init(void)
{
	IN_CODE_DEBUG("Entering ()");

	(void)in_log_set_option ("timestamp", DEFAULT_TIMESTAMP);

	(void)in_log_set_drv_opt("default", "channel",  "stderr");

	(void)in_log_set_drv_opt("syslog",  "facility", "user");
	(void)in_log_set_drv_opt("syslog",  "options",  "pid|perror");
	(void)in_log_set_drv_opt("syslog",  "ident",    DEFAULT_IDENT);

	(void)in_log_set_drv_opt("file",    "filename",    DEFAULT_LOG_FILE);
	(void)in_log_set_drv_opt("file",    "timestamped", "no");
	(void)in_log_set_drv_opt("file",    "stayopen",    "no");

	(void)in_log_set_driver("default");
	IN_CODE_DEBUG("Return");
	return;
}

static void paths_init(const char *name)
{
	char   *nptr = strrchr(name, '/');
	char   *pptr = pid_file;
	size_t  psiz = sizeof(pid_file) - 1;

	IN_CODE_DEBUG("Entering (%s)", name);

	if(NULL == nptr)
		nptr = (char *)name;
	else
		nptr += 1;

	in_fmt_string(&pptr, &psiz, _PATH_VARRUN);
	in_fmt_string(&pptr, &psiz, nptr);
	in_fmt_string(&pptr, &psiz, ".pid");
	*pptr = '\0';

	pptr = conf_file;
	in_fmt_string(&pptr, &psiz, SYSCONFDIR);
	in_fmt_string(&pptr, &psiz, "/");
	in_fmt_string(&pptr, &psiz, nptr);
	in_fmt_string(&pptr, &psiz, ".conf");
	*pptr = '\0';

	(void)in_set_pidfile(pid_file);
	IN_CODE_DEBUG("Return");
	return;
}

static int idf_callback(in_directory_t *dir, void *dat)
{
	char         mskstr[128];
	in_action_t *p;
	size_t       i;

	IN_CODE_DEBUG("Entering (%p (%s), %p)", dir, dir->dir_name, dat);

	(void)dat;
	(void)in_log_set_level(IN_LOG_EMERG);
	in_events2str(mskstr, sizeof(mskstr), dir->dir_mask);
	(void)in_log_set_level(IN_LOG_DEBUG);
	in_log_debug("  Directory %s:", dir->dir_name);
	in_log_debug("    dir_dev : %d", dir->dir_dev);
	in_log_debug("    dir_ino : %d", dir->dir_ino);
	in_log_debug("    dir_uid : %d", dir->dir_uid);
	in_log_debug("    dir_gid : %d", dir->dir_gid);
	in_log_debug("    dir_msk : %08X (%s)", dir->dir_mask, mskstr);
	in_log_debug("    # acts  : %lu", dir->dir_nactions);
	for(p = dir->dir_actions, i = 0; i < dir->dir_nactions; p += 1, i += 1)
	{
		(void)in_log_set_level(IN_LOG_EMERG);
		in_events2str(mskstr, sizeof(mskstr), p->act_mask);
		(void)in_log_set_level(IN_LOG_DEBUG);
		in_log_debug("      [%02d]: mask = %08X (%s), command = %s", (int)i, p->act_mask, mskstr, p->act_command);
	}

	IN_CODE_DEBUG("Return 1");

	return 1;
}

static int inform_daemon_for_action(int action)
{
	pid_t           dmnpid;
	int             signum;
	int             retval;
	in_log_level_t  loglvl;

	IN_CODE_DEBUG("Entering (%d (%c))", action, action);

	retval = 0;
	dmnpid = in_pidfile(in_get_pidfile(), 0);

	if(0 != dmnpid)
	{
		switch(action)
		{
		case 'R':	signum = SIGUSR1;	loglvl = IN_LOG_ERR;	break;
		case 'S':	signum = SIGTERM;	loglvl = IN_LOG_ERR;	break;
		default:	signum = 0;		loglvl = IN_LOG_INFO;	break;
		}
		if(-1 == kill(dmnpid, signum))
		{
			if(ESRCH == errno)
			{
				in_log_log(loglvl, "Daemon id not running");
			}
			else
			{
				in_log_perror("daemon");
			}
			retval = 1;
		}
	}
	else
	{
		loglvl = 's' == action ? IN_LOG_INFO : IN_LOG_ERR;
		in_log_log(loglvl, "Daemon id not running");
		retval = 1;
	}
		
	IN_CODE_DEBUG("Return %s", retval);
	return retval;
}

void in_reread_configuration(void)
{
	IN_CODE_DEBUG("Entering");
	longjmp(ind_jmp_env, 1);
	IN_CODE_DEBUG("HOUSTON WE HAVE A PROBLEM");
	exit(1);
}

int main(int argc, char **argv)
{
	int foreground = 0;
	int action     = 0;

	IN_CODE_DEBUG("Entering (%d, %p)", argc, argv);

	paths_init(*argv);
	logging_init();

	in_opts_prepare(*argv, VERSION, usage, version, options_defs, IN_ARRAY_COUNT(options_defs));
	argc -= 1, argv += 1;
	{
		char *aa;
		int   to;

		aa = NULL;
		while(0 != (to = in_opts_next(&argc, &argv, &aa)))
		{
			switch(to)
			{
			case 'C':
				if(strlen(aa) > MAXPATHLEN)
				{
					in_log_error("%s: File name too long\n", aa);
					IN_CODE_DEBUG("Exit(1)");
					exit(1);
				}
				{
					size_t  cfsiz = sizeof(conf_file) - 1;
					char   *cfpos = conf_file;
					(void)in_fmt_string(&cfpos, &cfsiz, aa);
					*cfpos = '\0';
				}
				break;
			case 'f':
				foreground = 1;
				break;
			case 'L':
				if(IN_LOG_OK != in_log_driver_exists(aa))
				{
					in_log_error("Unknown logging driver `%s'\n", aa);
					IN_CODE_DEBUG("Exit(1)");
					exit(1);
				}
				logdriver = (const char *)aa;
				cliopts |= CL_OPT_LOGDRV;
				break;
			case 'l':
				;
				{
					in_log_level_t lvl;
					if((in_log_level_t)-1 == (lvl = in_log_level_by_name(aa)))
					{
						in_log_error("Unknown logging level `%s'\n", aa);
						IN_CODE_DEBUG("Exit(1)");
						exit(1);
					}
					(void)in_log_set_level(lvl);
				}
				cliopts |= CL_OPT_LOGLVL;
				break;
			case 'o':
				;
				{
					char *p = strrchr(aa, '=');
					if(!p || p == aa)
					{
						in_log_error("Bad logging option format `%s'\n", aa);
						IN_CODE_DEBUG("Exit(1)");
						exit(1);
					}
					*(p - 1) = '\0';
					switch(in_log_set_drv_opt(logdriver, aa, p))
					{
					case IN_LOG_BADOPTION:
						in_log_error("Bad logging `%s' does not have the option `%s'\n", logdriver, aa);
						IN_CODE_DEBUG("Exit(1)");
						exit(1);
					case IN_LOG_BADOPTVAL:
						in_log_error("Bad logging `%s' bas option `%s' value `%s'\n", logdriver, aa, p);
						IN_CODE_DEBUG("Exit(1)");
						exit(1);
					case IN_LOG_BADDRIVER:
						in_log_error("INTERNAL ERRROR -- unknown driver `%s'", logdriver);
						IN_CODE_DEBUG("Exit(1)");
						exit(1);
					}
				} 
				cliopts |= CL_OPT_LOGOPT;
				break;
			case 'p':
				if(in_set_pidfile(aa))
				{
					IN_CODE_DEBUG("Exit(1)");
					exit(1);
				}
				cliopts |= CL_OPT_PIDFILE;
				(void)strcpy(pid_file, aa);
				break;
			case 'R': /* Intentionaly fall into */
			case 'S': /* Intentionaly fall into */
			case 'T': /* Intentionaly fall into */
			case 't': /* Intentionaly fall into */
				action = to;
				break;
			default:
				in_opts_usage(200);
				break;
			}
		}
	}

	if(0 != action)
	{
		int rts = 0;
		switch(action)
		{
		case 'R':
		case 'S':	/* Intentionaly fall into */
		case 's':	/* Intentionaly fall into */
			rts = inform_daemon_for_action(action);
			break;
		case 'T':
			if(IN_ST_OK == in_configuration_read(conf_file, CL_OPT_ALL))
			{
				in_log_info("configuration ok");
				rts = 0;
			}
			else
			{
				in_log_error("There are configurations error");
				rts = 1;
			}
		}
		IN_CODE_DEBUG("Exit(%d)", rts);
		exit(rts);
	}

	if(foreground)
	{
		in_log_notice("Running foreground");
	}
	else
	{
		pid_t pid = in_forcefork(10);
#if defined(DEBUG)
		pid_t pgr;
#endif
		switch(pid)
		{
		case -1:
			in_log_perror("main/fork");
			in_log_error("Cannot daemonize");
			IN_CODE_DEBUG("Exit(1)");
			exit(1);
		case  0:
#if defined(DEBUG)
			pgr = setpgrp();
#else
			(void)setpgrp();
#endif			
			IN_CODE_DEBUG("New process group : %d", pgr);
			break;
		default:
			in_log_info("Daemon started with pid = %d", pid);
			sleep(1);
			IN_CODE_DEBUG("Child launched, Exit(0)");
			exit(0);
		}
	}

	if(IN_LOG_BADDRIVER == in_log_set_driver(logdriver))
	{
		in_log_error("Bad log driver `%s'", logdriver);
		IN_CODE_DEBUG("Exit(1)");
		exit(1);
	}

	if(setjmp(ind_jmp_env))
	{
		in_log_notice("Reinitialization: reading new configuration");
	}

	in_signal_terminate();
	in_events_terminate();

	if(IN_ST_OK != in_configuration_read(conf_file, cliopts))
	{
		in_log_error("Configuration error");
		IN_CODE_DEBUG("Exit(1)");
		exit(1);
	}

	if(IN_LOG_DEBUG == in_log_set_level(IN_LOG_GET))
	{
		in_log_debug("Configuration:");
		in_directory_foreach(idf_callback, NULL);
	}


	{
		pid_t pidret = in_pidfile(pid_file, 1);

		IN_CODE_DEBUG("pidfile = %s, pidret = %d", pid_file, pidret);

		if((pid_t)-1 == pidret)
		{
			in_log_error("Problem with pidfile %s", pid_file);
			IN_CODE_DEBUG("Exit(1)");
			exit(1);
		}

		if((pid_t) 0 != pidret)
		{
			in_log_error("Another similar engine is running with pid = %d", (int)pidret);
			IN_CODE_DEBUG("Exit(1)");
			exit(1);
		}
	}
	
	in_engine();
	return 0;
}
