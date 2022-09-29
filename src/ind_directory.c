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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <CCR/cc_log.h>

#include "inotify-daemon.h"

extern in_status_t in_directory_create    (const char *, in_directory_t **);
extern in_status_t in_directory_getbyname (const char *, in_directory_t **);
extern in_status_t in_directory_getbywatch(int,          in_directory_t **);
extern void        in_directory_foreach   (int (*)(in_directory_t *, void *), void *);
extern void        in_directory_purge     (void);

static in_directory_t *dir_first = NULL;

static in_status_t getbyname(const char *, in_directory_t **, struct stat *);

static in_status_t getbyname(const char *pathname, in_directory_t **retd, struct stat *rets)
{
	in_directory_t *pt;
	struct stat     di[1];

	CC_LOG_DBGCOD("Entering getbyname(%s, %p, %p)", pathname, retd, rets);
	if(-1 == stat(pathname, di))
		return IN_ST_SYSTEM_ERROR;

	if(rets)
		(void)memcpy(rets, di, sizeof(di));

	if((di->st_mode & S_IFMT) != S_IFDIR)
		return IN_ST_NOT_DIRECTORY;
	for(pt = dir_first; pt != NULL; pt = pt->dir_next)
	{
		if((pt->dir_dev == di->st_dev) && (pt->dir_ino == di->st_ino))
		{
			if(retd) *retd = pt;
			return IN_ST_OK;
		}
	}
	return IN_ST_NOT_FOUND;
}

in_status_t in_directory_create(const char *pathname, in_directory_t **retv)
{
	in_status_t     status;
	struct stat     dinfo[1];
	in_directory_t *newdir;
	char           *dname;

	CC_LOG_DBGCOD("Entering in_directory_create(%s, %p)", pathname, retv);
	switch(status = getbyname(pathname, NULL, dinfo))
	{
	case IN_ST_NOT_FOUND:
		break;
	case IN_ST_OK:
		status = IN_ST_EXISTS;
	default:		/* Intentionaly fall into */
		return status;
	}
	
	if(NULL == (newdir = (in_directory_t *)malloc(sizeof(in_directory_t))))
		return IN_ST_SYSTEM_ERROR;
	if(NULL == (dname = strdup(pathname)))
		return IN_ST_SYSTEM_ERROR;

	(void)memset(newdir, 0, sizeof(in_directory_t));
	newdir->dir_name     = dname;
	newdir->dir_shell    = NULL;
	newdir->dir_dev      = dinfo->st_dev;
	newdir->dir_ino      = dinfo->st_ino;
	newdir->dir_uid      = -1;
	newdir->dir_gid      = -1;
	newdir->dir_wd       = -1;
	newdir->dir_mask     =  0;
	newdir->dir_nactions =  0;
	newdir->dir_next     = dir_first;
	dir_first            = newdir;

	*retv = newdir;

	return IN_ST_OK;
}

in_status_t in_directory_getbyname(const char *pathname, in_directory_t **retv)
{
	CC_LOG_DBGCOD("Entering in_directory_getbyname(%s, %p)", pathname, retv);
	return getbyname(pathname, retv, NULL);
}

in_status_t in_directory_getbywatch(int wd, in_directory_t **retd)
{
	in_directory_t *ptr;

	CC_LOG_DBGCOD("Entering in_directory_getbywatch(%d, %p)", wd, retd);
	for(ptr = dir_first; ptr; ptr = ptr->dir_next)
	{
		if(wd == ptr->dir_wd)
		{
			if(retd)
				*retd = ptr;
			return IN_ST_OK;
		}
	}
	return IN_ST_NOT_FOUND;
}

void in_directory_foreach(int (*callback)(in_directory_t *, void *), void *data)
{
	in_directory_t *ptr;

	CC_LOG_DBGCOD("Entering in_directory_foreach(%p, %p)", callback, data);
	for(ptr = dir_first; ptr; ptr = ptr->dir_next)
	{
		if(!callback(ptr, data))
			break;
	}
	return;
}

void in_directory_purge(void)
{
	in_directory_t *cptr;
	in_directory_t *nptr;

	CC_LOG_DBGCOD("Entering in_directory_purge()");
	if(NULL == dir_first)
		return;

	for(cptr = dir_first, nptr = cptr->dir_next; cptr; cptr = nptr)
	{
		size_t       n;
		in_action_t *p = cptr->dir_actions + IN_ARRAY_COUNT(cptr->dir_actions);
		for(p -= 1, n = 0; n < IN_ARRAY_COUNT(cptr->dir_actions); n += 1, p -= 1)
		{
			if(p->act_command) free((void *)p->act_command);
		}
		if(cptr->dir_shell)	free((void *)cptr->dir_shell);
		if(cptr->dir_name)	free((void *)cptr->dir_name);
		nptr = cptr->dir_next;
		free(cptr);
	}
	dir_first = NULL;
	return;
}
