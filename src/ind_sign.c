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
#include <sys/signalfd.h>
#include <sys/wait.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <CCR/cc_log.h>

#include "inotify-daemon.h"

extern int  in_signal_init(void);
extern void in_signal_process(void);
extern void in_signal_terminate(void);

static int in_signal_fd = -1;

int in_signal_init(void)
{
	sigset_t sigmsk;
	int      sigfds;

	CC_LOG_DBGCOD("Entering in_signal_init");

	if(-1 != in_signal_fd)
		(void)close(in_signal_fd);

	(void)sigemptyset(&sigmsk);
	(void)sigaddset  (&sigmsk, SIGINT);
	(void)sigaddset  (&sigmsk, SIGTERM);
	(void)sigaddset  (&sigmsk, SIGCHLD);
	(void)sigaddset  (&sigmsk, SIGUSR1);
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
			in_reread_configuration();
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

