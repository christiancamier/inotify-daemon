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

#ifndef __INOTIFY_DAEMON_H__
#define __INOTIFY_DAEMON_H__

#include <sys/types.h>
#include <stdint.h>

struct pollfd;
struct inotify_event;

typedef struct in_action_st {
	uint32_t              act_mask;
	const char           *act_command;
} in_action_t;

#if defined(MAXPATHLEN)
struct in_directory_st;
struct in_renamed_st {
	struct in_renamed_st   *ren_prv;
	struct in_renamed_st   *ren_nxt;
	struct in_directory_st *ren_dir;
	uint32_t                ren_evt;
	uint32_t                ren_cok;
	char                    ren_nam[MAXPATHLEN + 1];
};
#else
struct in_renamed_st;
#endif

typedef struct in_directory_st {
	struct in_directory_st  *dir_next;
	const char              *dir_name;
	const char              *dir_shell;
	dev_t                    dir_dev;
	ino_t                    dir_ino;
	uid_t                    dir_uid;
	gid_t                    dir_gid;
	int                      dir_wd;
	uint32_t                 dir_mask;
	size_t                   dir_nactions;
	struct in_action_st      dir_actions[16];
	struct in_renamed_st    *dir_renamed;
} in_directory_t;

typedef struct in_event_st {
	struct in_directory_st *dir;
	int                     wd;
	uint32_t                mask;
	uint32_t                cookie;
	uint32_t                len;
	const char             *evtname; /* Filename */
	const char             *oldname; /* For rename */
} in_event_t;

typedef enum in_status_en {
	IN_ST_SYSTEM_ERROR           = -1,
	IN_ST_OK                     =  0,
	IN_ST_INTERNAL,
	IN_ST_SYNTAX_ERROR,
	IN_ST_PREMATURE_END_OF_FILE,
	IN_ST_LINE_TOO_LONG,
	IN_ST_END_OF_FILE,
	IN_ST_VALUE_ERROR,
	IN_ST_NOT_DIRECTORY,
	IN_ST_NOT_FOUND,
	IN_ST_EXISTS
} in_status_t;

/*
 * Helper macros
 */
#define IN_MIN(a, b) ((a) < (b) ? (a) : (b))
#define IN_MAX(a, b) ((a) > (b) ? (a) : (b))

#define IN_PROTECT_ERRNO(code)	\
	{			\
		int e = errno;	\
		code;		\
		errno = e;	\
	}

#define IN_STRUCT_OFFSET_OF(T, F) (size_t)(&((T *)0)->F)

#define IN_IFNULL(T, V, D)	(NULL == (V) ? (D) : (V))
#define IN_ARRAY_COUNT(A)	(sizeof(A) / sizeof(A[0]))
#define IN_BUFFER_ROUNDUP(S) (((S) + sizeof(unsigned long) - 1) / sizeof(unsigned long))


/*
 * Command line arguments masks
 */

#define CL_OPT_PIDFILE	(1 << 0)
#define CL_OPT_LOGDRV	(1 << 1)
#define CL_OPT_LOGLVL	(1 << 2)
#define CL_OPT_LOGOPT	(1 << 3)
#define CL_OPT_ALL	(CL_OPT_PIDFILE | CL_OPT_LOGDRV | CL_OPT_LOGLVL | CL_OPT_LOGOPT)

/* ind_command.c */
extern size_t      in_cmd_count (struct pollfd *);
extern void        in_cmd_exited(pid_t);
extern int         in_cmd_log   (struct pollfd *, size_t);
extern in_status_t in_cmd_run   (struct in_event_st *);

/* ind_directory.c */
extern in_status_t in_directory_create    (const char *, in_directory_t **);
extern in_status_t in_directory_getbyname (const char *, in_directory_t **);
extern in_status_t in_directory_getbywatch(int,          in_directory_t **);
extern void        in_directory_foreach   (int (*)(in_directory_t *, void *), void *);
extern void        in_directory_purge     (void);

/* ind_engine.c */
extern void in_engine(void) __attribute__((noreturn));

/* ind_error.c */
extern const char *in_strerror(in_status_t);

/* ind_configuration.c */
extern in_status_t in_configuration_read(const char *, unsigned int);

/* ind_inotify.c */
extern in_status_t in_str2events(char *, uint32_t *);
extern void        in_events2str(char *, size_t, uint32_t);
extern int         in_events_init(void);
extern void        in_events_process(void);
extern in_status_t in_events_terminate(void);

/* ind_signal.c */
extern int  in_signal_init(void);
extern void in_signal_process(void);
extern void in_signal_terminate(void);

#endif /* !__INOTIFY_DAEMON_H__ */
