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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "inotify-daemon.h"

#if 0
extern const char *in_strerror(in_status_t);
#endif

#if defined(DEBUG)
extern const char *in_strstatus(in_status_t);
#endif

#if 0
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
#endif

#if defined(DEBUG)
const char *in_strstatus(in_status_t status)
{
	static char errbuf[128];
	switch(status)
	{
	case IN_ST_SYSTEM_ERROR:		return "IN_ST_SYSTEM_ERROR";
	case IN_ST_OK:				return "IN_ST_OK";
	case IN_ST_INTERNAL:			return "IN_ST_INTERNAL";
	case IN_ST_SYNTAX_ERROR:		return "IN_ST_SYNTAX_ERROR";
	case IN_ST_PREMATURE_END_OF_FILE:	return "IN_ST_PREMATURE_END_OF_FILE";
	case IN_ST_LINE_TOO_LONG:		return "IN_ST_LINE_TOO_LONG";
	case IN_ST_END_OF_FILE:			return "IN_ST_END_OF_FILE";
	case IN_ST_VALUE_ERROR:			return "IN_ST_VALUE_ERROR";
	case IN_ST_NOT_DIRECTORY:		return "IN_ST_NOT_DIRECTORY";
	case IN_ST_NOT_FOUND:			return "IN_ST_NOT_FOUND";
	case IN_ST_EXISTS:			return "IN_ST_EXISTS";
	}
	(void)snprintf(errbuf, sizeof(errbuf), "Unknown error %d", (int)status);
	return errbuf;
}
#endif
