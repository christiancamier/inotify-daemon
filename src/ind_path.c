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
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#include "inotify-daemon.h"

extern int    in_mkpath(const char *, int);

int in_mkpath(const char *path, int mode)
{
	char         bpath[strlen(path) + 1];
	const char  *srcpt;
	char        *tgtpt;
	struct stat  statb[1];

	srcpt = path;
	tgtpt = bpath;

	if('/' == *srcpt)
		*(tgtpt++) = *(srcpt++);
	while('/' == *srcpt)
		srcpt += 1;
	while(*srcpt)
	{
		while('/' != *srcpt && *srcpt)
			*(tgtpt++) = *(srcpt++);
		*tgtpt  = '\0';
		if(-1 == stat(bpath, statb))
		{
			if(ENOENT == errno)
			{
				if(-1 == mkdir(bpath, mode))
					return -1;
			}
			else
			{
				return -1;
			}
		}
		if('/' == *(srcpt))
			*(tgtpt++) = *(srcpt++);
	}

	return 0;
}

