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

#ifndef __IN_TUNABLES_H__
#define __IN_TUNABLES_H__

/* Read buffer size for configuration file */
#if !defined(IN_BUFSIZE)
#define IN_BUFSIZE 1024
#endif

#if !defined(DEFAULT_TIMESTAMP)
#define DEFAULT_TIMESTAMP "%d/%m/%Y %H:%M:%S"
#endif

#if !defined(DEFAULT_IDENT)
#define DEFAULT_IDENT     "inotify-daemon"
#endif

#if !defined(DEFAULT_LOG_FILE)
#define DEFAULT_LOG_FILE  "/var/log/inotify-daemon.log"
#endif

#if !defined(SYSCONFDIR)
#define SYSCONFDIR	  "/etc"
#endif

#endif /* !__IN_TUNABLES_H__ */
