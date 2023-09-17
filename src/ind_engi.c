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
#include <errno.h>
#include <poll.h>
#include <stdlib.h>

#include <CCR/cc_log.h>
#include <CCR/cc_pidfile.h>

#include "inotify-daemon.h"

extern void in_engine(void);

__attribute__((noreturn)) void in_engine(void)
{
	int                  evtfd;
	int                  sigfd;
	int                  poret;

	const char          *pidfile = cc_get_pidfile();

	CC_LOG_DBGCOD("Entering in_engine()");

	do {
		CC_LOG_DBGCOD("%s", pidfile);
		pid_t pidret = cc_pidfile(pidfile, 1);

		if((pid_t)-1 == pidret)
		{
			cc_log_error("Problem with pidfile %s", pidfile);
			exit(1);
		}

		if((pid_t) 0 != pidret)
		{
			cc_log_error("Another similar engine is running with pid = %d", (int)pidret);
			exit(1);
		}
	} while(0);
	
	if(-1 == (evtfd = in_events_init())) goto end;
	if(-1 == (sigfd = in_signal_init())) goto end;

	while(1)
	{
		nfds_t nfds = (nfds_t)in_cmd_count(NULL) + 2;
		do {
			struct pollfd polfds[nfds];

			polfds[0].fd      = evtfd;	polfds[1].fd      = sigfd;
			polfds[0].events  = POLLIN;	polfds[1].events  = POLLIN;
			polfds[0].revents = 0;		polfds[1].revents = 0;

			(void)in_cmd_count(polfds + 2);

			poret = poll(polfds, nfds, -1);
			cc_log_debug("in_engine/poll returns %d", poret);

			switch(poret)
			{
			case -1:
				if(EINTR != errno)
				{
					cc_log_perror("in_engine/poll");
					goto end;
				}
			case  0:	/* Intentionaly fall into */
				continue;
			default:
				break;
			}
			
			if(POLLIN == (polfds[0].revents & POLLIN))
			{
				cc_log_debug("New event(s)");
				in_events_process();
				continue;
			}

			if(0 < in_cmd_log(polfds + 2, (size_t)(nfds - 2)))
				continue;

			if(POLLIN == (polfds[1].revents & POLLIN))
			{
				cc_log_debug("New signal(s)");
				in_signal_process();
			}
		} while(0);
	}

end:
	in_signal_terminate();
	in_events_terminate();
	CC_LOG_DBGCOD("Exiting in_engine()");
	exit(1);
}
