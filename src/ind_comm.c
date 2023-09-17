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

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/inotify.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <paths.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <CCR/cc_fmt.h>
#include <CCR/cc_forcefork.h>
#include <CCR/cc_log.h>

#include "inotify-daemon.h"
#include "ind_tunables.h"

extern size_t      in_cmd_count (struct pollfd *);
extern void        in_cmd_exited(pid_t);
extern int         in_cmd_log   (struct pollfd *, size_t);
extern in_status_t in_cmd_run   (struct in_event_st *);

static const uid_t nobody  = 65534;
static const gid_t nogroup = 65534;

struct in_cmd {
	struct in_cmd *prv;
	struct in_cmd *nxt;
	pid_t          pid;
	int            pfdE;
	int            pfdS;
	char           bufE[256];
	char           bufS[256];
};

static cc_log_level_t stdoutlvl = CC_LOG_DEBUG;
static cc_log_level_t stderrlvl = CC_LOG_NOTICE;

/* Initial freelist */
static struct in_cmd  cmd_firstfree[] = {
	{ NULL,              cmd_firstfree + 1, -1, -1, -1, "" },
	{ cmd_firstfree + 0, cmd_firstfree + 2, -1, -1, -1, "" },
	{ cmd_firstfree + 1, cmd_firstfree + 3, -1, -1, -1, "" },
	{ cmd_firstfree + 2, cmd_firstfree + 4, -1, -1, -1, "" },
	{ cmd_firstfree + 3, cmd_firstfree + 5, -1, -1, -1, "" },
	{ cmd_firstfree + 4, cmd_firstfree + 6, -1, -1, -1, "" },
	{ cmd_firstfree + 5, cmd_firstfree + 7, -1, -1, -1, "" },
	{ cmd_firstfree + 6, NULL,              -1, -1, -1, "" }
};

static struct in_cmd *cmd_runnings = NULL;
static struct in_cmd *cmd_freelist = cmd_firstfree;
static size_t         cmd_runcnt   = 0;

static int cmd_alloc(pid_t pid, int pfdE, int pfdS)
{
	struct in_cmd *new;

	CC_LOG_DBGCOD("Entering cmd_alloc(%d, %d, %d)", pid, pfdE, pfdS);

	if(NULL == cmd_freelist)
	{
		CC_LOG_DBGCOD("Allocating new in_cmd structure");
		if(NULL == (new = (struct in_cmd *)malloc(sizeof(struct in_cmd))))
		{
			cc_log_error("Cannot allocate %lu bytes", sizeof(struct in_cmd));
			return -1;
		}
	}
	else
	{
		CC_LOG_DBGCOD("Taking in_cmd structure in freelist");
		new          = cmd_freelist;
		cmd_freelist = cmd_freelist->nxt;
	}
	new->prv = NULL;
	new->nxt = cmd_runnings;
	if(NULL != cmd_runnings) cmd_runnings->prv = new;
	if(cmd_runnings)
		cmd_runnings->prv = new;
	cmd_runnings = new;
	new->pfdE = pfdE;
	new->pfdS = pfdS;
	new->pid  = pid;
	*(new->bufE) = '\0';
	*(new->bufS) = '\0';
	cmd_runcnt += 1;
	return 0;
}

static size_t uint32_arg(char **bufptr, size_t *bufsiz, uint32_t value, int format)
{
	size_t   inc = 0;
	size_t   bit;
	uint32_t msk;

	switch(format)
	{
	case 'b':
		for(bit = 32; bit > 0;)
		{
			bit -= 1;
			msk = (uint32_t)1 << bit;
			(void)cc_fmt_char(bufptr, bufsiz, msk == (value & msk) ? '1' : '0');
		}
		inc = 2;
		break;
	case 'd': (void)cc_fmt_fmt(bufptr, bufsiz, "%u",    value); inc = 2; break;
	case 'o': (void)cc_fmt_fmt(bufptr, bufsiz, "%011o", value); inc = 2; break;
	case 'X': (void)cc_fmt_fmt(bufptr, bufsiz, "%08X",  value); inc = 2; break;
	case 'x': (void)cc_fmt_fmt(bufptr, bufsiz, "%08x",  value); inc = 2; break;
	default:  (void)cc_fmt_fmt(bufptr, bufsiz, "%u",    value); inc = 1; break;
	}
	return inc;
}

static int execute_command(in_directory_t *dir, in_action_t *act, struct in_event_st *evt, int ofdE, int ofdS)
{
	static char   buffer[16 * IN_BUFSIZE];
	char         *bufptr;
	size_t        bufsiz = sizeof(buffer);
	const char   *cmdptr = act->act_command;
	const char   *cmdshl = _PATH_BSHELL;
	char          evtbuf[16];

	CC_LOG_DBGCOD("Entering execute_command(%p, %p, %p)", dir, act, evt);

	(void)memset(buffer, 0, bufsiz);
	bufptr  = buffer;
	bufsiz -= 1;
	while(*cmdptr)
	{
		if('%' == *cmdptr)
		{
			switch(*(cmdptr + 1))
			{
			case '%': /* percent sign */
				(void)cc_fmt_char(&bufptr, &bufsiz, '%');
				cmdptr += 1;
				break;
			case 'c': /* Event cookie */
				cmdptr += uint32_arg(&bufptr, &bufsiz, evt->cookie, *(cmdptr + 2));
				break;
			case 'D': /* Event directory */
				(void)cc_fmt_string(&bufptr, &bufsiz, dir->dir_name);
				cmdptr += 1;
				break;
			case 'E': /* Event name */
				(void)memset(evtbuf, 0, sizeof(evtbuf));
				(void)in_events2str(evtbuf, sizeof(evtbuf), evt->mask & IN_ALL_EVENTS);
				(void)cc_fmt_string(&bufptr, &bufsiz, evtbuf);
				cmdptr += 1;
				break;
			case 'e': /* Full event mask */
				cmdptr += uint32_arg(&bufptr, &bufsiz, evt->mask, *(cmdptr + 2));
				break;
			case 'F': /* Filename */
				(void)cc_fmt_string(&bufptr, &bufsiz, evt->evtname);
				cmdptr += 1;
				break;
			case 'O': /* Old filename (RENAME) */
				(void)cc_fmt_string(&bufptr, &bufsiz, evt->oldname);
				cmdptr += 1;
				break;
			default:
				(void)cc_fmt_char(&bufptr, &bufsiz, '%');
				break;
			}
		}
		else
		{
			cc_fmt_char(&bufptr, &bufsiz, *cmdptr);
		}
		cmdptr += 1;
	}
	*bufptr = '\0';
	if(dir->dir_shell)
		cmdshl = dir->dir_shell;
	CC_LOG_DBGCOD("Command to execute: %s -c `%s'", cmdshl, buffer);
	cc_log_info("Subprocess %d: Lauchning command %s", getpid(), buffer);

	if(-1 == dir->dir_gid) dir->dir_gid = nogroup;
	if(-1 != dir->dir_uid) dir->dir_uid = nobody;

	if((-1 == setgroups(1, &(dir->dir_gid))) || (-1 == setregid(dir->dir_gid, dir->dir_gid)))
	{
		cc_log_perror("Cannot change groups for command");
		cc_log_error("Exiting command");
		exit(1);
	}
	if(-1 == setreuid(dir->dir_uid, dir->dir_uid))
	{
		cc_log_perror("Cannot change groups for command");
		cc_log_error("Exiting command");
		exit(1);
	}

	if(-1 != ofdE)
	{
		(void)close(2);
		if(-1 == dup2(ofdE, 2))
			cc_log_perror("dup2 on stdout");
		(void)close(ofdE);
	}
	if(-1 != ofdS)
	{
		(void)close(1);
		if(-1 == dup2(ofdS, 1))
			cc_log_perror("dup2 on stdout");
		(void)close(ofdS);
	}
	(void)execl(cmdshl, cmdshl, "-c", buffer, NULL);
	perror(buffer);
	return 1;
}

size_t in_cmd_count(struct pollfd *pfds)
{
	CC_LOG_DBGCOD("Entering in_cmd_count(%p)", pfds);

	if(NULL != pfds && NULL != cmd_runnings)
	{
		struct in_cmd *ccmd = cmd_runnings;
		struct pollfd *cpfd = pfds;
		for(ccmd = cmd_runnings, cpfd = pfds; NULL != ccmd; ccmd = ccmd->nxt)
		{
			cpfd->revents = 0;
			cpfd->events  = POLLIN;
			cpfd->fd      = ccmd->pfdE;
			cpfd += 1;
			cpfd->revents = 0;
			cpfd->events  = POLLIN;
			cpfd->fd      = ccmd->pfdS;
			cpfd += 1;
		}
	}
	return 2 * cmd_runcnt;
}

void in_cmd_exited(pid_t pid)
{
	struct in_cmd *cmd;

	CC_LOG_DBGCOD("Entering in_cmd_exited(%d)", pid);
	for(cmd = cmd_runnings; NULL != cmd; cmd = cmd->nxt)
	{
		if(pid == cmd->pid)
		{
			if('\0' != cmd->bufS[0]) cc_log_log(stdoutlvl, "Subprocess %d: %s", cmd->pid, cmd->bufS);
			if('\0' != cmd->bufE[0]) cc_log_log(stderrlvl, "Subprocess %d: %s", cmd->pid, cmd->bufE);
			if(NULL != cmd->prv) cmd->prv->nxt = cmd->nxt;
			else                 cmd_runnings  = cmd->nxt;
			if(NULL != cmd->nxt) cmd->nxt->prv = cmd->prv;
			(void)close(cmd->pfdE);
			(void)close(cmd->pfdS);
			cmd->pid     = (pid_t)-1;
			cmd->pfdE    = -1;
			cmd->pfdS    = -1;
			(void)memset(cmd->bufE, 0,sizeof(cmd->bufE));
			(void)memset(cmd->bufS, 0,sizeof(cmd->bufS));
			cmd->prv     = NULL;
			cmd->nxt     = cmd_freelist;
			cmd_freelist = cmd;
			cmd_runcnt -= 1;
			break;
		}
	}
	return;
}

static void cmd_do_log(struct in_cmd *cmd, char *buf, int fd, cc_log_level_t lvl)
{
	ssize_t  nbrds;
	char     rbuf[256];
	char    *bstr;
	char    *estr;
	
	if(-1 == (nbrds = read(fd, rbuf, sizeof(rbuf) - 1)))
	{
		cc_log_notice("Subprocess %d: read error %d (%s)", errno, strerror(errno));
		return;
	}
	rbuf[nbrds] = '\0';
	for(estr = bstr = rbuf; nbrds > 0; nbrds -= 1, estr += 1)
	{
		if('\n' == *estr)
		{
			*(estr++) = '\0';
			cc_log_log(lvl, "Subprocess %d: %s%s", cmd->pid, buf, bstr);
			buf[0] = '\0';
			bstr = estr;
		}
	}
	if(bstr < estr)
		strcpy(buf, bstr);
	return;
}

int in_cmd_log(struct pollfd *pfds, size_t nfds)
{
	struct pollfd *pfd;
	struct in_cmd *cmd;
	int            ret = 0;

	CC_LOG_DBGCOD("Entering in_cmd_log(%p, %lu)", pfds, nfds);
	for(pfd = pfds; nfds > 0; pfd += 1, nfds -= 1)
	{
		if(pfd->revents & POLLIN)
		{
			ret += 1;
			for(cmd = cmd_runnings; NULL != cmd; cmd = cmd->nxt)
			{
				if(pfd->fd == cmd->pfdE)
				{
					cmd_do_log(cmd, cmd->bufE, pfd->fd, stderrlvl);
					break;
				}
				if(pfd->fd == cmd->pfdS)
				{
					cmd_do_log(cmd, cmd->bufS, pfd->fd, stdoutlvl);
					break;
				}
			}
		}
	}
	return ret;
}

in_status_t in_cmd_run(struct in_event_st *event)
{
	uint32_t         msk = event->mask & IN_ALL_EVENTS;
	pid_t            pid;
	in_action_t     *pac;
	size_t           cnt;
	in_directory_t  *dir;
	in_status_t      sta = IN_ST_OK;

	CC_LOG_DBGCOD(
		"Entering in_cmd_run(%p [wd: %d mask: %04X cookie: %04X len: %u evtname: %s oldname: %s)",
		event, event->wd, event->mask, event->cookie, event->len, event->evtname, event->oldname
		);
	dir = event->dir;
	for(pac = dir->dir_actions, cnt = 0; cnt < dir->dir_nactions; pac += 1, cnt += 1)
	{
		if(msk == (pac->act_mask & msk))
		{
			int prtE = -1;
			int prtS = -1;
			int pfdE[2];
			int pfdS[2];

			if(-1 == (prtE = pipe2(pfdE, O_DIRECT)))
				cc_log_perror("pipe2 (stderr)");
			if(-1 == (prtS = pipe2(pfdS, O_DIRECT)))
				cc_log_perror("pipe2 (stderr)");
			switch(pid = cc_forcefork(10))
			{
			case -1:
				cc_log_perror("in_execute/fork");
				break;
			case  0:
				if(-1 != prtE) close(pfdE[0]);
				if(-1 != prtS) close(pfdS[0]);
				exit(execute_command(dir, pac, event, -1 == prtE ? -1 : pfdE[1], -1 == prtS ? -1 : pfdS[1]));
			default:
				if(-1 != prtE && -1 != prtS)
				{
					close(pfdE[1]);
					close(pfdS[1]);
					if(-1 == cmd_alloc(pid, pfdE[0],pfdS[0]))
					{
						cc_log_info("Subprocess pid = %d outputs will not be logged", pid);
						close(pfdE[0]);
						close(pfdS[0]);
					}
				}
				else
				{
					if(-1 != prtE) close(pfdE[0]);
					if(-1 != prtS) close(pfdS[0]);
				}					
				goto end;
			}
		}
	}
	cc_log_error("Directory %s: event %X not defined", dir->dir_name, event->mask);
	sta = IN_ST_INTERNAL;
end:
	return sta;
}

static int log_setopt(const char *opt, const char *val, void *dat, int sim)
{
	cc_log_level_t  newlevel;
	cc_log_level_t *lvlvar = NULL;

	(void)dat;

	switch((long)dat)
	{
	case  1: lvlvar = &stdoutlvl; break;
	case  2: lvlvar = &stderrlvl; break;
	default: return CC_LOG_BADOPTION;
	}
	if((cc_log_level_t)-1 == (newlevel = cc_log_level_by_name(val)))
		return CC_LOG_BADOPTVAL;

	if(!sim)
		*lvlvar = newlevel;
	return CC_LOG_OK;
}

static __attribute__((constructor)) void ctor_command(void)
{
	cc_log_add_option("act_out_lvl", log_setopt, (void *)1);
	cc_log_add_option("act_err_lvl", log_setopt, (void *)2);
	return;
}
