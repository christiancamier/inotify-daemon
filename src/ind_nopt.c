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

#include <stdlib.h>

#include "inotify-daemon.h"

extern char *in_next_option(char **, char *, size_t, const char *);

static inline int is_separator(int c, const char *separators)
{
	const char *p;
	for(p = separators; *p && *p != c; p += 1);
	return *p == c;
}

char *
in_next_option(
	char         **source,
	char          *buffer,
	size_t         bufsiz,
	const char    *separators
	)
{
	size_t  bsz;
	char   *src;
	char   *dst;
	char   *ret;

	ret = NULL;

	IN_CODE_DEBUG("Entering source = %p (%s), separators = '%s'", source, *source, separators);
	if('\0' != **source)
	{
		for(src = *source, ret = dst = buffer, bsz = 0; '\0' != *src; src = src + 1)
		{
			if(is_separator(*src, separators))
				break;
			if(bsz < bufsiz)
			{
				*dst = *src;
				bsz  = bsz + 1;
				dst  = dst + 1;
			}
		}
		if('\0' != *src)
			*source = src + 1;
		else
			*source = src;
		*dst = '\0';
	}
	IN_CODE_DEBUG("Return %s", ret);
	return ret;
}
