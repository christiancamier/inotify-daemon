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

typedef enum in_log_level_en {
	IN_LOG_GET      = -1,
	IN_LOG_EMERG	=  0,
	IN_LOG_ALERT    =  1,
	IN_LOG_CRIT     =  2,
	IN_LOG_ERR      =  3,
	IN_LOG_WARNING  =  4,
	IN_LOG_NOTICE   =  5,
	IN_LOG_INFO     =  6,
	IN_LOG_DEBUG    =  7
} in_log_level_t;

struct in_log_driver_info {
	const char *name;
	const char *desc;
};

#if defined(_STDARG_H) || defined(_ANSI_STDARG_H_) || defined(_VA_LIST_DECLARED)
struct in_logger_st {
	struct in_logger_st  *next;
	const char           *name;
	const char           *desc;
	int                 (*sopt)(const char *, const char *, int);
	void                (*open )(void);
	void                (*close)(void);
	void                (*log  )(int, const char *, va_list);
};

extern void            in_log_register       (struct in_logger_st *);
#define IN_LOG_REGISTER(aname, adesc, asopt, aopen, aclose, alog)	\
	static struct in_logger_st drv_##aname = { .next = NULL, .name = #aname, .desc = adesc, .sopt = asopt, .open = aopen, .close = aclose, .log = alog }; \
	static __attribute__((constructor)) void ctor_##aname(void) { in_log_register(&drv_##aname); }
#else
struct in_logger_st;
#endif	

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

/* ind_comm.c */
extern size_t      in_cmd_count (struct pollfd *);
extern void        in_cmd_exited(pid_t);
extern int         in_cmd_log   (struct pollfd *, size_t);
extern in_status_t in_cmd_run   (struct in_event_st *);

/* ind_conf.c */
extern in_status_t in_configuration_read(const char *, unsigned int);

/* ind_dire.c */
extern in_status_t in_directory_create    (const char *, in_directory_t **);
extern in_status_t in_directory_getbyname (const char *, in_directory_t **);
extern in_status_t in_directory_getbywatch(int,          in_directory_t **);
extern void        in_directory_foreach   (int (*)(in_directory_t *, void *), void *);
extern void        in_directory_purge     (void);

/* ind_engi.c */
extern void in_engine(void) __attribute__((noreturn));

/* ind_erro.c */
extern const char *in_strerror(in_status_t);

/* ind_forl.c */
extern  pid_t in_forcefork(size_t);

/* ind_form.c */
extern void in_fmt_char  (char **, size_t *, char);
extern void in_fmt_fmt   (char **, size_t *, const char *, ...);
extern void in_fmt_string(char **, size_t *, const char *);
#if defined(_STDARG_H) || defined(_ANSI_STDARG_H_) || defined(_VA_LIST_DECLARED)
extern void in_fmt_vfmt  (char **, size_t *, const char *, va_list);
#endif

extern void    in_fmt_timestamp_setgmt(void);
extern void    in_fmt_timestamp_setloc(void);
extern ssize_t in_fmt_timestamp       (char **, size_t *, const char *);


/* ind_inot.c */
extern in_status_t in_str2events(char *, uint32_t *);
extern void        in_events2str(char *, size_t, uint32_t);
extern int         in_events_init(void);
extern void        in_events_process(void);
extern in_status_t in_events_terminate(void);

/* ind_logg.c */
#define IN_LOG_OK         0
#define IN_LOG_BADOPTION -1
#define IN_LOG_BADOPTVAL -2
#define IN_LOG_BADDRIVER -3

extern void            in_log_add_option(const char *, int(*)(const char *, const char *, void *, int), void *);
extern int             in_log_driver_exists(const char *);
extern size_t          in_log_driver_list  (struct in_log_driver_info *, size_t);
extern in_log_level_t  in_log_level_by_name(const char *);
extern const char     *in_log_level_name   (in_log_level_t);
extern void            in_log_register     (struct in_logger_st *);
extern in_log_level_t  in_log_set_level    (in_log_level_t);
extern int             in_log_set_driver   (const char *);
extern int             in_log_set_option   (const char *, const char *);
extern int             in_log_set_drv_opt  (const char *, const char *, const char *);
extern const char     *in_log_timestamp    (void);

extern int             in_log_tst_option   (const char *, const char *);
extern int             in_log_tst_drv_opt  (const char *, const char *, const char *);

extern void            in_log_alert        (const char *, ...);
extern void            in_log_crit         (const char *, ...);
extern void            in_log_debug        (const char *, ...);
extern void            in_log_emerg        (const char *, ...);
extern void            in_log_error        (const char *, ...);
extern void            in_log_info         (const char *, ...);
extern void            in_log_notice       (const char *, ...);
extern void            in_log_warning      (const char *, ...);
extern void            in_log_log          (in_log_level_t, const char *, ...);
#if defined(_STDARG_H) || defined(_ANSI_STDARG_H_) || defined(_VA_LIST_DECLARED)
extern void            in_log_vlog         (in_log_level_t, const char *, va_list);
#endif
extern void            in_log_dbgcod       (const char *, size_t, const char *, ...);

extern void            in_log_panic          (const char *, ...) __attribute__((noreturn));
extern void            in_log_perror	     (const char *);

#if defined(DEBUG)
extern void in_log_code_debug(const char *, size_t, const char *, const char *, ...);
#define IN_CODE_DEBUG(...)	in_log_code_debug(__FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define IN_CODE_DEBUG(...)
#endif

/* ind_main.c */
extern void in_reread_configuration(void);
extern int  main(int, char **);

/* misc.c */
extern int in_str2bool(const char *, int, int, int);

/* ind_nopt.c */
extern char *in_next_option(char **, char *, size_t, const char *);

/* ind_path.c */
extern int    in_mkpath(const char *, int);

/* ind_sign.c */
extern int  in_signal_init(void);
extern void in_signal_process(void);
extern void in_signal_terminate(void);

/* ind_pidf.c */
extern int          in_ctl_pidfile(const char *);
extern const char  *in_get_pidfile(void);
extern int          in_set_pidfile(const char *);
extern pid_t        in_pidfile    (const char *, int);

#endif /* !__INOTIFY_DAEMON_H__ */
