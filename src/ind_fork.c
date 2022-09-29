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
#include <unistd.h>
#include <errno.h>

#include "inotify-daemon.h"

extern pid_t in_forcefork(size_t);

pid_t in_forcefork(size_t retries)
{
	int    sverr;
	pid_t  child;
	size_t retry;

	for(retry = 0; retry < retries; retry += 1)
	{
		if(-1 != (child = fork()))
			break;
		sverr = errno;
		usleep(100000);	/* Sleep 100ms */
		errno = sverr;
	}
	return child;
}
