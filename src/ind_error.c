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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "inotify-daemon.h"

extern const char *in_strerror(in_status_t);

const char *in_strerror(in_status_t status)
{
	static char  syserrbuf[128];

	switch(status)
	{
	case IN_ST_SYSTEM_ERROR:
		snprintf(syserrbuf, sizeof(syserrbuf), "System error %d (%s)", errno, strerror(errno));
		return syserrbuf;
	case IN_ST_OK:				return "No error";
	case IN_ST_INTERNAL:			return "Internal error";
	case IN_ST_SYNTAX_ERROR:		return "Syntax error";
	case IN_ST_PREMATURE_END_OF_FILE:	return "Premature end of file";
	case IN_ST_LINE_TOO_LONG:		return "Line too long";
	case IN_ST_END_OF_FILE:			return "End of file";
	case IN_ST_VALUE_ERROR:			return "Bad value";
	case IN_ST_NOT_DIRECTORY:		return "Not a directory";
	case IN_ST_NOT_FOUND:			return "Not found";
	case IN_ST_EXISTS:			return "Already exists";
	}
	return "Unknown error";
}

