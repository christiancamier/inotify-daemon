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
