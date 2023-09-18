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
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "inotify-daemon.h"

extern void in_fmt_char  (char **, size_t *, char);
extern void in_fmt_fmt   (char **, size_t *, const char *, ...);
extern void in_fmt_string(char **, size_t *, const char *);
extern void in_fmt_vfmt  (char **, size_t *, const char *, va_list);

extern void    in_fmt_timestamp_setgmt(void);
extern void    in_fmt_timestamp_setloc(void);
extern ssize_t in_fmt_date            (char **, size_t *, const char *, struct tm *);
extern ssize_t in_fmt_timestamp       (char **, size_t *, const char *);

static struct tm *(*xxtime)(const time_t *, struct tm *) = localtime_r;

void
in_fmt_char(
	char       **buffer,
	size_t      *bufsiz,
	char         character
	)
{
	size_t      s = *bufsiz;
	char       *p = *buffer;
	
	if(s > 0)
	{
		*p =  character;
		*buffer = p + 1;
		*bufsiz = s - 1;
	}
	return;
}

void
in_fmt_fmt(
	char       **buffer,
	size_t      *bufsiz,
	const char  *format,
	...
	)
{
	va_list	ap;
	va_start(ap, format),
	in_fmt_vfmt(buffer, bufsiz, format, ap);
	va_end(ap);
	return;
}

void
in_fmt_string(
	char       **buffer,
	size_t      *bufsiz,
	const char  *string
	)
{
	const char *sp = string;
	char       *pb = *buffer;
	size_t      bs = *bufsiz;

	while('\0' != *sp)
	{
		if(0 == bs)
			break;
		*(pb++) = *(sp++), bs -= 1;
	}
	*buffer =  pb;
	*bufsiz =  bs;
	return;
}

void
in_fmt_vfmt(
	char       **buffer,
	size_t      *bufsiz,
	const char  *format,
	va_list      ap
	)
{
	size_t  bs = *bufsiz;
	size_t  rs;

	rs = vsnprintf(*buffer, bs, format, ap);
	if(rs > bs)
		rs = bs;
	*buffer += rs;
	*bufsiz -= rs;
	return;
}

ssize_t in_fmt_date(char **buffer, size_t *bufsiz, const char *format, struct tm *tm)
{
	char   *buf = *buffer;
	size_t  bsz = *bufsiz;
	size_t  nbw;

	nbw = strftime(buf, bsz, format, tm);
	if(nbw > 0)
	{
		*bufsiz -= nbw;
		*buffer += nbw;
	}

	return (ssize_t)nbw;
}

void in_fmt_timestamp_setgmt(void) { xxtime = gmtime_r;    return; }
void in_fmt_timestamp_setloc(void) { xxtime = localtime_r; return; }

ssize_t in_fmt_timestamp(char **buffer, size_t *bufsiz, const char *format)
{
	time_t    ti[1];
	struct tm tm[1];
	(void)time(ti);
	(void)xxtime(ti, tm);
	return in_fmt_date(buffer, bufsiz, format, tm);
}

