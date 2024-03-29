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
#include <unistd.h>
#include <errno.h>

#if defined(DEBUG)
#include <string.h>
#endif

#include "inotify-daemon.h"

extern pid_t in_forcefork(size_t);

pid_t in_forcefork(size_t retries)
{
	int    sverr;
	pid_t  child;
	size_t retry;

	IN_CODE_DEBUG("Entering (%lu)", retries);

	for(retry = 0; retry < retries; retry += 1)
	{
		if(-1 != (child = fork()))
			break;
		sverr = errno;
		IN_CODE_DEBUG("Fork failed, errno = %d (%s) -- Waiting 100ms", sverr, strerror(sverr));
		usleep(100000);	/* Waiting 100ms */
		errno = sverr;
	}
	IN_CODE_DEBUG("pid = %d, Return %d", getpid(), child);
	return child;
}
