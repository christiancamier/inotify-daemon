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
#include <sys/inotify.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "inotify-daemon.h"

extern in_status_t in_str2events(char *, uint32_t *);
extern void        in_events2str(char *, size_t, uint32_t);
extern int         in_events_init(void);
extern in_status_t in_events_terminate(void);

extern int in_events_fd;
int in_events_fd = -1;

static struct in_renamed_st ren_firsts[] = {
	{ NULL,           ren_firsts + 1, NULL, 0, 0, "" },
	{ ren_firsts + 0, ren_firsts + 2, NULL, 0, 0, "" },
	{ ren_firsts + 1, ren_firsts + 3, NULL, 0, 0, "" },
	{ ren_firsts + 2, ren_firsts + 4, NULL, 0, 0, "" },
	{ ren_firsts + 3, ren_firsts + 5, NULL, 0, 0, "" },
	{ ren_firsts + 4, ren_firsts + 6, NULL, 0, 0, "" },
	{ ren_firsts + 5, ren_firsts + 7, NULL, 0, 0, "" },
	{ ren_firsts + 6, NULL,           NULL, 0, 0, "" }
};
static struct in_renamed_st *ren_free = ren_firsts;

struct event_st {
	const char *ev_name;
	uint32_t    ev_mask;
	int         ev_cplx;
};
	;
static struct event_st events[] = {
	{ "access",		  IN_ACCESS,        0 },
	{ "attrib",		  IN_ATTRIB,        0 },
	{ "close",                IN_CLOSE,         1 },
	{ "close_write",	  IN_CLOSE_WRITE,   0 },
	{ "close_nowrite",	  IN_CLOSE_NOWRITE, 0 },
	{ "create",		  IN_CREATE,        0 },
	{ "delete",		  IN_DELETE,        0 },
	{ "delete_self",	  IN_DELETE_SELF,   0 },
	{ "modify",		  IN_MODIFY,        0 },
	{ "move_self",		  IN_MOVE_SELF,     0 },
	{ "open",		  IN_OPEN,          0 },
	{ "rename",		  IN_MOVE,          0 },
	{ NULL,                   0,                0 }
};

in_status_t in_str2events(char *eventstr, uint32_t *rmask)
{
	char             estr[strlen(eventstr) + 1];
	char             buff[64];
	const char      *pevt;
	struct event_st *pevd;
	uint32_t         mask;
	in_status_t      retv = IN_ST_OK;
	int              fnd = 0;

	IN_CODE_DEBUG("Entering (%s, %p)", eventstr, rmask);
	(void)strcpy(estr, eventstr);
	mask = 0;
	while(NULL != (pevt = in_next_option(&eventstr, buff, sizeof(buff), ",|")))
	{
		for(pevd = events; pevd->ev_name; pevd += 1)
		{
			if(0 == strcasecmp(pevd->ev_name, pevt))
			{
				mask |= pevd->ev_mask;
				fnd   = 1;
				break;
			}
		}
		if(0 == fnd)
		{
			retv = IN_ST_VALUE_ERROR;
		}
		fnd = 0;
	}
	if(NULL != rmask && IN_ST_OK == retv)
	{
		*rmask = mask;
	}
	IN_CODE_DEBUG("Return %d", retv);
	return retv;
}


void in_events2str(char *buffer, size_t bufsiz, uint32_t mask)
{
	const char      *sep = "";
	char            *pos = buffer;
	size_t           rst = bufsiz - 1;
	struct event_st *pev;

	IN_CODE_DEBUG("Entering (%p, %lu, %X)", buffer, bufsiz, mask);
	for(pev = events; pev->ev_name; pev += 1)
	{
		if(pev->ev_cplx)
			continue;
		if(pev->ev_mask & mask)
		{
			(void)in_fmt_string(&pos, &rst, sep);
			(void)in_fmt_string(&pos, &rst, pev->ev_name);
			sep = "|";
		}
	}
	*pos = '\0';
	IN_CODE_DEBUG("Return");
	return;
}

static int callback(in_directory_t *dir, void *dat)
{
	int evtfd = *((int *)dat);

	IN_CODE_DEBUG("Entering (%p (%s), %p (%d))", dir, dir->dir_name, dat, evtfd);

	if(-1 == (dir->dir_wd = inotify_add_watch(evtfd, dir->dir_name, dir->dir_mask)))
	{
		in_log_perror("inotify_add_watch");
		in_log_error("dir_name = %s, dir_mask = %04X", dir->dir_name, dir->dir_mask);
	}

	IN_CODE_DEBUG("Return 1");
	return 1;
}

int in_events_init(void)
{
	int evtfd = 0;

	IN_CODE_DEBUG("Entering ()");

	if(-1 != in_events_fd)
		(void)close(in_events_fd);

	if(-1 == (evtfd = inotify_init1(IN_NONBLOCK)))
	{
		in_log_perror("in_events_init/inotify_init1(IN_NONBLOCK)");
	}
	else
	{
		in_directory_foreach(callback, (void *)&evtfd);
		in_events_fd = evtfd;
	}

	IN_CODE_DEBUG("Return %d", evtfd);
	return evtfd;
}

void in_events_process(void)
{
	static char buffer[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));

	struct in_directory_st *evdir;
	struct in_event_st      inevt[1];
	struct inotify_event   *event;
	char                   *evptr;
	ssize_t                 rdlen;

	IN_CODE_DEBUG("Entering ()");

	while(1)
	{
		if(-1 == (rdlen = read(in_events_fd, &buffer, sizeof(buffer))))
		{
			if(errno != EAGAIN)
			{
				in_log_perror("in_events_fd/read");
			}
			break;
		}
		for(evptr = buffer; evptr < buffer + rdlen; evptr += sizeof(struct inotify_event) + event->len)
		{
			event = (struct inotify_event *)evptr;
			if(IN_ST_OK != in_directory_getbywatch(event->wd, &evdir))
			{
				in_log_error("Unknown watch descriptor wd=%d", event->wd);
				continue;
			}
			inevt->dir     = evdir;
			inevt->wd      = event->wd;
			inevt->mask    = event->mask;
			inevt->cookie  = event->cookie;
			inevt->len     = event->len;
			inevt->evtname = event->len > 0 ? (const char *)event->name : "";
			inevt->oldname = "";
			if(event->mask == IN_MOVED_FROM || event->mask == IN_MOVED_TO)
			{
				int                   fnd;
				struct in_renamed_st *pmv;

				in_log_debug("%s caught", IN_MOVED_FROM == event->mask ? "IN_MOVED_FROM" : "IN_MOVED_TO");

				for(fnd = 0, pmv = evdir->dir_renamed; pmv; pmv = pmv->ren_nxt)
				{
					if(inevt->cookie == pmv->ren_cok)
					{
						if(pmv->ren_prv) pmv->ren_prv->ren_nxt = pmv->ren_nxt;
						else             evdir->dir_renamed    = pmv->ren_nxt;
						if(pmv->ren_nxt) pmv->ren_nxt->ren_prv = pmv->ren_prv;
						pmv->ren_nxt = ren_free;
						ren_free     = pmv;

						if(event->mask == IN_MOVED_TO)
						{
							inevt->oldname = (const char *)pmv->ren_nam;
						}
						else
						{
							inevt->oldname = (const char *)inevt->evtname;
							inevt->evtname = (const char *)pmv->ren_nam;
						}
						fnd = 1;
						break;
					}
				}
				if(!fnd)
				{
					if(NULL == (pmv = ren_free))
					{
						in_log_debug("Allocating memory for in_rename_st");
						if(NULL == (pmv = (struct in_renamed_st *)malloc(sizeof(struct in_renamed_st))))
						{
							in_log_error("Cannot allocate %lu bytes for in_rename_st", sizeof(struct in_renamed_st));
							continue;
						}
					}
					else
					{
						ren_free = pmv->ren_nxt;
					}
					(void)memset(pmv, 0, sizeof(struct in_renamed_st));
					pmv->ren_prv = NULL;
					pmv->ren_nxt = evdir->dir_renamed;
					pmv->ren_dir = evdir;
					pmv->ren_evt = inevt->mask;
					pmv->ren_cok = inevt->cookie;
					if(inevt->len)
						(void)strncpy(pmv->ren_nam, inevt->evtname, MAXPATHLEN);
					evdir->dir_renamed = pmv;
					continue;
				}
			}
			(void)in_cmd_run(inevt);
		}
	}
	IN_CODE_DEBUG("Return");
	return;
}

in_status_t in_events_terminate(void)
{
	IN_CODE_DEBUG("Entering in_events_terminate");
	if(-1 != in_events_fd)
		(void)close(in_events_fd);
	in_events_fd = -1;
	IN_CODE_DEBUG("Return IN_ST_OK");
	return IN_ST_OK;
}
