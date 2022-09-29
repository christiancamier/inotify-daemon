/*
 * Copyright (c) 2022
 *      Christian CAMIER <christian.c@promethee.services>
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

#include <CCR/cc_fmt.h>
#include <CCR/cc_forcefork.h>
#include <CCR/cc_log.h>
#include <CCR/cc_options.h>
#include <CCR/cc_pidfile.h>

#include "inotify-daemon.h"
#include "ind_tunables.h"

#define VERSION		"0.1"

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

static struct cc_option options_defs[] = {
	CC_OPT_ENTRY(
		'C', 'C', "configuration",
		CC_OPTARG_REQUIRED,
		NULL, 0, CC_OPTARG_OPENONE,
		"Configuration file"),
	CC_OPT_ENTRY(
		'f', 'f', "foreground",
		CC_OPTARG_NONE,
		NULL, 0, CC_OPTARG_OPENONE,
		"Don't fork, stay foreground"),
	CC_OPT_ENTRY(
		'h', 'h', "help",
		CC_OPTARG_NONE,
		NULL, 0, CC_OPTARG_HELP,
		"display this help"),
	CC_OPT_ENTRY(
		'L', 'L', "logdriver",
		CC_OPTARG_REQUIRED,
		NULL, 0, CC_OPTARG_OPENONE,
		"set log driver"),
	CC_OPT_ENTRY(
		'l', 'l', "loglevel",
		CC_OPTARG_REQUIRED,
		NULL, 0, CC_OPTARG_OPENONE,
		"set log level"),
	CC_OPT_ENTRY(
		'o', 'o', "logoption",
		CC_OPTARG_REQUIRED,
		NULL, 0, CC_OPTARG_OPENONE,
		"set log option"),
	CC_OPT_ENTRY(
		'p', 'p', "pidfile",
		CC_OPTARG_REQUIRED,
		NULL, 0, CC_OPTARG_OPENONE,
		"set pid file"),
	CC_OPT_ENTRY(
		'R', 'R', "reload",
		CC_OPTARG_NONE,
		NULL, 0, CC_OPTARG_OPENONE,
		"Reload daemon configuration"),
	CC_OPT_ENTRY(
		'S', 'S', "stop",
		CC_OPTARG_NONE,
		NULL, 0, CC_OPTARG_OPENONE,
		"Stop daemon"),
	CC_OPT_ENTRY(
		'T', 'T', "test",
		CC_OPTARG_NONE,
		NULL, 0, CC_OPTARG_OPENONE,
		"Do not run, just test configuration file"),
	CC_OPT_ENTRY(
		'V', 'V', "version",
		CC_OPTARG_NONE,
		NULL, 0, CC_OPTARG_VERSION,
		"display programm version"),
};

static unsigned int cliopts = 0;
static char pid_file [MAXPATHLEN + 1];
static char conf_file[MAXPATHLEN + 1];

static const char *logdriver = "default";

extern jmp_buf ind_jmp_env;
jmp_buf ind_jmp_env;

static void logging_init(void)
{
	CC_LOG_DBGCOD("Entering logging_init()");

	(void)cc_log_set_option ("timestamp", DEFAULT_TIMESTAMP);

	(void)cc_log_set_drv_opt("default", "channel",  "stderr");

	(void)cc_log_set_drv_opt("syslog",  "facility", "user");
	(void)cc_log_set_drv_opt("syslog",  "options",  "pid|perror");
	(void)cc_log_set_drv_opt("syslog",  "ident",    DEFAULT_IDENT);

	(void)cc_log_set_drv_opt("file",    "filename",    DEFAULT_LOG_FILE);
	(void)cc_log_set_drv_opt("file",    "timestamped", "yes");
	(void)cc_log_set_drv_opt("file",    "stayopen",    "no");

	(void)cc_log_set_driver("default");
	return;
}

static void paths_init(const char *name)
{
	char   *nptr = strrchr(name, '/');
	char   *pptr = pid_file;
	size_t  psiz = sizeof(pid_file) - 1;

	if(NULL == nptr)
		nptr = (char *)name;
	else
		nptr += 1;

	(void)cc_fmt_string(&pptr, &psiz, _PATH_VARRUN);
	(void)cc_fmt_string(&pptr, &psiz, nptr);
	(void)cc_fmt_string(&pptr, &psiz, ".pid");
	*pptr = '\0';

	pptr = conf_file;
	(void)cc_fmt_string(&pptr, &psiz, SYSCONFDIR);
	(void)cc_fmt_string(&pptr, &psiz, "/");
	(void)cc_fmt_string(&pptr, &psiz, nptr);
	(void)cc_fmt_string(&pptr, &psiz, ".conf");
	*pptr = '\0';

	(void)cc_set_pidfile(pid_file);
	return;
}

static int idf_callback(in_directory_t *dir, void *dat)
{
	char         mskstr[128];
	in_action_t *p;
	size_t       i;

	(void)dat;
	(void)cc_log_set_level(CC_LOG_EMERG);
	in_events2str(mskstr, sizeof(mskstr), dir->dir_mask);
	(void)cc_log_set_level(CC_LOG_DEBUG);
	cc_log_debug("  Directory %s:", dir->dir_name);
	cc_log_debug("    dir_dev : %d", dir->dir_dev);
	cc_log_debug("    dir_ino : %d", dir->dir_ino);
	cc_log_debug("    dir_uid : %d", dir->dir_uid);
	cc_log_debug("    dir_gid : %d", dir->dir_gid);
	cc_log_debug("    dir_msk : %08X (%s)", dir->dir_mask, mskstr);
	cc_log_debug("    # acts  : %lu", dir->dir_nactions);
	for(p = dir->dir_actions, i = 0; i < dir->dir_nactions; p += 1, i += 1)
	{
		(void)cc_log_set_level(CC_LOG_EMERG);
		in_events2str(mskstr, sizeof(mskstr), p->act_mask);
		(void)cc_log_set_level(CC_LOG_DEBUG);
		cc_log_debug("      [%02d]: mask = %08X (%s), command = %s", (int)i, p->act_mask, mskstr, p->act_command);
	}
	return 1;
}

static int signal_daemon(int signal)
{
	pid_t running_pid = cc_pidfile(cc_get_pidfile(), 0);
	if(0 == running_pid)
		goto nodaemon;

	if(-1 == kill(running_pid, signal))
	{
		if(ESRCH == errno)
			goto nodaemon;
		cc_log_perror("daemon");
		return 1;
	}
	return 0;
nodaemon:
	cc_log_error("There is no running daemon");
	return 1;
}

static int do_reload(void) { return signal_daemon(SIGUSR1); }
static int do_stop  (void) { return signal_daemon(SIGTERM); }

int main(int argc, char **argv)
{
	int foreground = 0;
	int action     = 0;
	int running    = 0;

	CC_LOG_DBGCOD("Entering main(%d, %p)", argc, argv);

	paths_init(*argv);
	logging_init();

	cc_opts_prepare(*argv, VERSION, usage, version, options_defs, IN_ARRAY_COUNT(options_defs));
	argc -= 1, argv += 1;
	do {
		char *aa;
		int   to;

		aa = NULL;
		while(0 != (to = cc_opts_next(&argc, &argv, &aa)))
		{
			switch(to)
			{
			case 'C':
				do {
					size_t  cfsiz = sizeof(conf_file) - 1;
					char   *cfpos = conf_file;
					(void)cc_fmt_string(&cfpos, &cfsiz, aa);
					*cfpos = '\0';
				} while(0);
				break;
			case 'f':
				foreground = 1;
				break;
			case 'L':
				if(CC_LOG_OK != cc_log_driver_exists(aa))
				{
					cc_log_error("Unknown logging driver `%s'\n", aa);
					exit(1);
				}
				logdriver = (const char *)aa;
				cliopts |= CL_OPT_LOGDRV;
				break;
			case 'l':
				do {
					cc_log_level_t lvl;
					if((cc_log_level_t)-1 == (lvl = cc_log_level_by_name(aa)))
					{
						cc_log_error("Unknown logging level `%s'\n", aa);
						exit(1);
					}
					(void)cc_log_set_level(lvl);
				} while(0);
				cliopts |= CL_OPT_LOGLVL;
				break;
			case 'o':
				do {
					char *p = strrchr(aa, '=');
					if(!p || p == aa)
					{
						cc_log_error("Bad logging option format `%s'\n", aa);
						exit(1);
					}
					*(p - 1) = '\0';
					switch(cc_log_set_drv_opt(logdriver, aa, p))
					{
					case CC_LOG_BADOPTION:
						cc_log_error("Bad logging `%s' does not have the option `%s'\n", logdriver, aa);
						exit(1);
					case CC_LOG_BADOPTVAL:
						cc_log_error("Bad logging `%s' bas option `%s' value `%s'\n", logdriver, aa, p);
						exit(1);
					case CC_LOG_BADDRIVER:
						CC_LOG_DBGCOD("INTERNAL ERRROR -- unknown driver `%s'", logdriver);
						exit(1);
					}
				} while(0);
				cliopts |= CL_OPT_LOGOPT;
				break;
			case 'p':
				if(cc_set_pidfile(aa))
					exit(1);
				cliopts |= CL_OPT_PIDFILE;
				(void)strcpy(pid_file, aa);
				break;
			case 'R':
				action = 'R';
				break;
			case 'S':
				action = 'S';
				break;
			case 'T':
				action = 'T';
				break;
			default:
				cc_opts_usage(200);
				break;
			}
		}
	} while(0);

	if(!action)
	{
		cc_log_set_driver(logdriver);
	}

	if(setjmp(ind_jmp_env))
	{
		cc_log_notice("Reinitialization: reading new configuration");
	}

	in_signal_terminate();
	in_events_terminate();

	if(IN_ST_OK != in_configuration_read(conf_file, 'T' == action ? CL_OPT_ALL : cliopts))
	{
		cc_log_error("Configuration error");
		return 1;
	}

	if(CC_LOG_DEBUG == cc_log_set_level(CC_LOG_GET))
	{
		cc_log_debug("Configuration:");
		in_directory_foreach(idf_callback, NULL);
	}

	if(action)
	{
		int rts = 0;
		switch(action)
		{
		case 'R':	rts = do_reload();			break;
		case 'S':	rts = do_stop  ();			break;
		case 'T':	cc_log_info("Configuration OK");	break;
		}
		return rts;
	}

	if(foreground || running)
	{
		if(foreground)
			cc_log_info("Running foreground");
		in_engine();
	}
	else
	{
		pid_t pid = cc_forcefork(10);
		switch(pid)
		{
		case -1:
			cc_log_perror("main/fork");
			cc_log_error("Cannot daemonize");
			return 1;
		case  0:
			running = 1;
			(void)setpgrp();
			in_engine();
			break;
		default:
			cc_log_info("Daemon started with pid = %d", pid);
			sleep(1);
			break;
		}
	}

	return 0;
}
