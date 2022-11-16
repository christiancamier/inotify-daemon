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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

#include <CCR/cc_fmt.h>
#include <CCR/cc_log.h>
#include <CCR/cc_str2bool.h>

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
			(void)cc_fmt_string(&fpt, &fsz, value);
			*fpt = '\0';
			(void)strcpy(filelast, filename);
		}
		return CC_LOG_OK;
	}

	if(0 == strcasecmp("mode", option))
	{
		long  nmod;
		char *eptr;

		nmod = strtol(value, &eptr, 8);
		if((eptr && *eptr) || (nmod < 0) || (nmod > 0777))
			return CC_LOG_BADOPTVAL;
		if(!simu)
			filemode = nmod;
		return CC_LOG_OK;
	}

	if(-1 == (onoff = cc_str2bool(value, 1, 0, -1)))
		return CC_LOG_BADOPTVAL;

	if(0 == strcasecmp(option, "timestamped"))
	{
		if(!simu)
		{
			if(onoff) fileopts |=  FOPT_TIMESTAMPED;
			else      fileopts &= ~FOPT_TIMESTAMPED;
		}
		return CC_LOG_OK;
	}

	if(0 == strcasecmp(option, "stayopen"))
	{
		if(!simu)
		{
			if(onoff) fileopts |=  FOPT_STAYOPEN;
			else      fileopts &= ~FOPT_STAYOPEN;
		}
		return CC_LOG_OK;
	}

	return CC_LOG_BADOPTION;
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
	
		(void)cc_fmt_timestamp(&bufpos, &bufsiz, cc_log_timestamp());
		(void)cc_fmt_string   (&bufpos, &bufsiz, " - [");
		(void)cc_fmt_string   (&bufpos, &bufsiz, cc_log_level_name(level));
		(void)cc_fmt_string   (&bufpos, &bufsiz, "] - ");
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
		cc_fmt_timestamp(&fnptr, &fnsiz, filename);
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

CC_LOG_REGISTER(
	file,
	"file logging driver",
	file_sopt,
	file_open,
	file_close,
	file_log);
