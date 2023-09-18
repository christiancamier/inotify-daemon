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
#include <sys/stat.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

#include "inotify-daemon.h"

static int  file_sopt (const char *, const char *, int);
static void file_open (void);
static void file_close(void);
static void file_log  (int, const char *, va_list);

static void do_open   (void);
static void do_close  (void);

#define FOPT_TIMESTAMPED	0x01	/* Filename is timestamped */
#define FOPT_STAYOPEN		0x02	/* File stay opened */

static char  filename[MAXPATHLEN + 1] = "/var/log/inotify-daemon-%Y%m.log";
static char  filelast[MAXPATHLEN + 1] = "/var/log/inotify-daemon-000000.log";
static FILE *filedesc                 = NULL;
static int   fileopts                 = FOPT_TIMESTAMPED | FOPT_STAYOPEN;
static int   filemode                 = 0600;

static int file_sopt (const char *option, const char *value, int simu)
{
	int onoff = -1;

	if(0 == strcasecmp("filename", option))
	{
		if(!simu)
		{
			char   *fpt = filename;
			size_t  fsz = sizeof(filename) - 1;
			in_fmt_string(&fpt, &fsz, value);
			*fpt = '\0';
			(void)strcpy(filelast, filename);
		}
		return IN_LOG_OK;
	}

	if(0 == strcasecmp("mode", option))
	{
		long  nmod;
		char *eptr;

		nmod = strtol(value, &eptr, 8);
		if((eptr && *eptr) || (nmod < 0) || (nmod > 0777))
			return IN_LOG_BADOPTVAL;
		if(!simu)
			filemode = nmod;
		return IN_LOG_OK;
	}

	if(-1 == (onoff = in_str2bool(value, 1, 0, -1)))
		return IN_LOG_BADOPTVAL;

	if(0 == strcasecmp(option, "timestamped"))
	{
		if(!simu)
		{
			if(onoff) fileopts |=  FOPT_TIMESTAMPED;
			else      fileopts &= ~FOPT_TIMESTAMPED;
		}
		return IN_LOG_OK;
	}

	if(0 == strcasecmp(option, "stayopen"))
	{
		if(!simu)
		{
			if(onoff) fileopts |=  FOPT_STAYOPEN;
			else      fileopts &= ~FOPT_STAYOPEN;
		}
		return IN_LOG_OK;
	}

	return IN_LOG_BADOPTION;
}

static void file_open (void)
{
	do_open();
	do_close();
	return;
}

static void file_close(void)
{
	if(NULL != filedesc)
	{
		(void)fclose(filedesc);
		filedesc = NULL;
	}
	return;
}

static void file_log  (int level, const char *format, va_list ap)
{
	char    buffer[1024];
	char   *bufpos;
	size_t  bufsiz;

	do_open();

	if(NULL != filedesc)
	{
		bufpos = buffer;
		bufsiz = sizeof(buffer);
	
		in_fmt_timestamp(&bufpos, &bufsiz, in_log_timestamp());
		in_fmt_string   (&bufpos, &bufsiz, " - [");
		in_fmt_string   (&bufpos, &bufsiz, in_log_level_name(level));
		in_fmt_string   (&bufpos, &bufsiz, "] - ");
		(void)vsnprintf(bufpos, bufsiz, format, ap);
		(void)fprintf(filedesc, "%s\n", buffer);
	}

	do_close();

	return;
}

static void do_open(void)
{
	if(FOPT_TIMESTAMPED == (fileopts & FOPT_TIMESTAMPED))
	{
		static char  newfilename[MAXPATHLEN + 1];
		char        *fnptr;
		size_t       fnsiz;
		fnsiz = sizeof(newfilename) - 1;
		fnptr = newfilename;
		in_fmt_timestamp(&fnptr, &fnsiz, filename);
		*fnptr = '\0';
		if(0 != strcmp(newfilename, filelast))
		{
			if(NULL != filedesc)
			{
				(void)fclose(filedesc);
				filedesc = NULL;
			}
			(void)strcpy(filelast, newfilename);
		}
	}
	if(NULL == filedesc)
	{
		if(NULL == (filedesc = fopen(filelast, "a")))
			perror(filelast);
		else
			(void)fchmod(fileno(filedesc), filemode);
	}
	return;
}

static void do_close(void)
{
	if((FOPT_STAYOPEN != (fileopts & FOPT_STAYOPEN)) && (NULL != filedesc))
	{
		fclose(filedesc);
		filedesc = NULL;
	}
	return;
}

IN_LOG_REGISTER(
	file,
	"file logging driver",
	file_sopt,
	file_open,
	file_close,
	file_log);
