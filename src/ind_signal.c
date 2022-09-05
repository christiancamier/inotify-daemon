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

#include <sys/types.h>
#include <sys/signalfd.h>
#include <sys/wait.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cc_log.h"

#include "inotify-daemon.h"
//#include "ind_tunables.h"

extern int  in_signal_init(void);
extern void in_signal_process(void);
extern void in_signal_terminate(void);

extern jmp_buf ind_jmp_env;
extern int in_signal_fd;
int in_signal_fd = -1;

int in_signal_init(void)
{
	sigset_t sigmsk;
	int      sigfds;

	CC_LOG_DBGCOD("Entering in_signal_init");

	if(-1 != in_signal_fd)
		(void)close(in_signal_fd);

	sigemptyset(&sigmsk);
	sigaddset  (&sigmsk, SIGINT);
	sigaddset  (&sigmsk, SIGTERM);
	sigaddset  (&sigmsk, SIGCHLD);
	sigaddset  (&sigmsk, SIGUSR1);
	if(-1 == sigprocmask(SIG_BLOCK, &sigmsk, NULL))
	{
		cc_log_perror("in_signal_init/sigprocmask");
		exit(1);
	}
	if(-1 == (sigfds = signalfd(-1, &sigmsk, SFD_NONBLOCK)))
	{
		cc_log_perror("in_signal_init/signalfd");
		exit(1);
	}
	in_signal_fd = sigfds;
	return sigfds;
}

void in_signal_process(void)
{
	struct signalfd_siginfo siginfo;

	CC_LOG_DBGCOD("Entering in_signal_process");
	
	while(sizeof(siginfo) == read(in_signal_fd, &siginfo, sizeof(siginfo)))
	{
		cc_log_debug("Receveid signal %d", siginfo.ssi_signo);
		switch(siginfo.ssi_signo)
		{
		case SIGINT:
		case SIGTERM:		// Terminate service
			cc_log_notice("%s Caught", strsignal(siginfo.ssi_signo));
			cc_log_notice("Exiting ...");
			in_signal_terminate();
			in_events_terminate();
			in_directory_purge();
			exit(0);
			break;
		case SIGUSR1:		// Reread configuration file
			cc_log_notice("SIGUSR1 caught");
			longjmp(ind_jmp_env, 1);
			break;
		case SIGCHLD:		// Child terminated
			cc_log_debug("SIGCHLD caught");
			do {
				pid_t pid;
				int   sta;
				while(-1 != (pid = waitpid(-1, &sta, WNOHANG)))
				{
					if(0 == pid)
						break;
					/****/
					/* Insert here command exit control ***/
					/****/
					in_cmd_exited(pid);
					if(WIFEXITED(sta))
					{
						cc_log_notice("Subprocess %d exited with status %d", pid, WEXITSTATUS(sta));
					}
					else if (WIFSIGNALED(sta))
					{
						int sig = WTERMSIG(sta);
						cc_log_notice("Subprocess %d killed by signal %d (%s)", pid, sig, strsignal(sig));
						if(WCOREDUMP(sta))
							cc_log_notice("Core dump");
					}
					else if (WIFSTOPPED(sta))
					{
						int sig = WSTOPSIG(sta);
						cc_log_notice("Subprocess %d stopped by signal %d (%s)", pid, sig, strsignal(sig));
					}
				}
			} while(0);
			break;
		default:		// WTF
			cc_log_warning("Unexpected signal %d (%s) caught", siginfo.ssi_signo, strsignal(siginfo.ssi_signo));
			break;
		}
	}
	return;
}

void in_signal_terminate(void)
{
	CC_LOG_DBGCOD("Entering in_signal_terminate");
	if(-1 != in_signal_fd)
		(void)close(in_signal_fd);
	return;
}

