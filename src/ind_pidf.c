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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "inotify-daemon.h"

static char pidfile[MAXPATHLEN + 1];

extern int          ind_ctl_pidfile(const char *);
extern const char  *ind_get_pidfile(void);
extern int          ind_set_pidfile(const char *);
extern pid_t        ind_pidfile    (const char *, int);

static int
pid_is_running(pid_t pid)
{
	int retval = 1;
	IN_CODE_DEBUG("Entering (%d)", pid);

	if(-1 == kill(pid, 0))
	{
#if defined(DEBUG)
		int s64_save_errno;
		s64_save_errno = errno;
		IN_CODE_DEBUG("errno = %d, ESRCH = %d", errno, ESRCH);
		errno = s64_save_errno;
#endif
		retval = ESRCH != errno;
	}
	IN_CODE_DEBUG("Return %d", retval);
	return retval;
}

int in_ctl_pidfile(const char *new_pidfile)
{
	size_t pflen;
	int    retv;

	IN_CODE_DEBUG("Entering (%s)", new_pidfile);
	retv  = 0;
	pflen = strlen(new_pidfile);

	if('/' != new_pidfile[0])
	{
		in_log_error("pidfile must be absolute");
		retv = -1;
	}
	if(pflen < 3)
	{
		in_log_error("pidfile name too short");
		retv = -1;
	}
	if(pflen > MAXPATHLEN)
	{
		in_log_error("pidfile name too long");
		retv = -1;
	}
	IN_CODE_DEBUG("Return %d", retv);
	return retv;
}

const char *in_get_pidfile(void)
{
	return pidfile;
}

int in_set_pidfile(const char *new_pidfile)
{
	int retv;
	IN_CODE_DEBUG("Entering (%s)", new_pidfile);
	if(-1 != (retv = in_ctl_pidfile(new_pidfile)))
	{
		(void)strcpy(pidfile, new_pidfile);
	}
	IN_CODE_DEBUG("Return %d", retv);
	return retv;
}

pid_t in_pidfile(const char *filename, int docreate)
{
	int    pfpid;
	pid_t  mypid;
	FILE  *fdesc;

	IN_CODE_DEBUG("Entering - filename = %s, docreate = %d", filename ? filename : "(NULL)", docreate);
	mypid = getpid();

	if(NULL == filename)
	{
		filename = pidfile;
	}

	IN_CODE_DEBUG("Stage 1 - Using '%s' as pid file name", filename);
	if(NULL == (fdesc = fopen(filename, "r")))
	{
		if(ENOENT != errno)
		{
			int sverr = errno;
			in_log_perror(filename);
			errno = sverr;
			pfpid = -1;
		}
		else
		{
			pfpid = 0;
		}
	}
	else
	{
		if(-1 == fscanf(fdesc, "%d", &pfpid))
		{
			in_log_warning("%s: Misformed %s pid file", filename);
			if( 0 == docreate)
			{
				in_log_notice("Assuming there is no other daemon running");
			}
			pfpid = 0;
		}
		fclose(fdesc);
	}

	/* If not create, just return the running pid (0 if not running) */
	if(0 == docreate)
	{
		if((pfpid <= 0) || (0 == pid_is_running(pfpid)))
			pfpid = 0;
		IN_CODE_DEBUG("Return %d", pfpid);
		return pfpid;
	}

	IN_CODE_DEBUG("Stage 2");
	IN_CODE_DEBUG("pfpid =%d, mypid = %d, is_running = %d", pfpid, mypid, pid_is_running(pfpid));

	if((pfpid > 0) && ((pid_t)pfpid != mypid) && (0 != pid_is_running((pid_t)pfpid)))
	{
		in_log_error("There is another daemon running (pid = %d, %s)", pfpid, mypid);
		IN_CODE_DEBUG("Return %d", pfpid);
		return pfpid;
	}

	if(docreate && 0 == pfpid)
	{
		if(-1 == unlink(filename) && ENOENT != errno)
		{
			in_log_perror(filename);
			pfpid = -1;
			IN_CODE_DEBUG("Return %d", pfpid);
			return pfpid;
		}		

		do {
			const char *pslash;

			if(NULL != (pslash = (const char *)strrchr(filename, '/')))
			{
				char        dirnam[pslash - filename + 1];
				const char *psrc;
				char       *ptgt;

				for(ptgt = dirnam, psrc = filename; psrc < pslash; *(ptgt++) = *(psrc++));
				*(ptgt) = '\0';
				if(-1 == in_mkpath(dirnam, 0700))
				{
					in_log_perror(dirnam);
					pfpid = -1;
					IN_CODE_DEBUG("Return %d", pfpid);
					return pfpid;
				}
			}
		} while(0);

		if(NULL == (fdesc = fopen(filename, "w")))
		{
			in_log_perror(filename);
			pfpid = -1;
			IN_CODE_DEBUG("Return %d", pfpid);
			return pfpid;
		}

		(void)fprintf(fdesc, "%d", (int)mypid);
		(void)fclose(fdesc);

		do {
			char   *bufptr = pidfile;
			size_t  bufsiz = sizeof(pidfile) - 1;
			in_fmt_string(&bufptr, &bufsiz, filename);
			*bufptr = '\0';
		} while(0);
	}	
	IN_CODE_DEBUG("Return 0");
	return 0;
}
