/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_XTCOOL_H__
#define __K_XTCOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <hilda/sysdeps.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include <stdarg.h>
#include <stdio.h>

#include <hilda/kmem.h>

typedef void* SPL_HANDLE;

/**
 * \brief ErrorCode
 */
#define SPL_EC_OK               0x00000000      /**< Success */
#define SPL_EC_NG               0x80000001      /**< General error */
#define SPL_EC_PARAM            0x80000002      /**< Bad parameters */
#define SPL_EC_HANDLE           0x80000003      /**< Bad Handle */
#define SPL_EC_FORBID           0x80000004      /**< Operation is forbidden */
#define SPL_EC_TIMEOUT          0x80000005      /**< Timeout */
#define SPL_EC_OWNER            0x80000006      /**< Already owner the object */
#define SPL_EC_NOT_OWNER        0x80000007      /**< Not own the object */
#define SPL_EC_NOT_SUPPRTED     0x80000008      /**< Not own the object */

enum SPL_PRIORITY { XXX, XXXX };

/**
 * @brief Create an event instance
 *
 * @param signaled 1 for signal the event by default, 0 for non-signel
 *
 * @return A handle for instance, 0 for error
 */
SPL_HANDLE spl_event_create(int signaled);

/**
 * @brief Signal the event
 *
 * @param h Handle of event instance
 *
 * @return SPL_EC_OK: Success
 *         SPL_EC_HANDLE: Illegal event handle
 */
int spl_event_set(SPL_HANDLE h);
int spl_event_clear(SPL_HANDLE h);
int spl_event_wait(SPL_HANDLE h, unsigned int ms);
int spl_event_destroy(SPL_HANDLE h);

/**
 * \brief lock
 */
SPL_HANDLE spl_lck_new(void);
int spl_lck_del(SPL_HANDLE a_lck);
void spl_lck_get(SPL_HANDLE a_lck);
void spl_lck_rel(SPL_HANDLE a_lck);

/**
 * \brief Mutex
 */
SPL_HANDLE spl_mutex_create(void);
int spl_mutex_lock(SPL_HANDLE h);
int spl_mutex_unlock(SPL_HANDLE h);
int spl_mutex_destroy(SPL_HANDLE h);

/**
 * \brief semaphone
 */
SPL_HANDLE spl_sema_new(int init_val);
int spl_sema_del(SPL_HANDLE h);
int spl_sema_get(SPL_HANDLE h, int timeout);
int spl_sema_rel(SPL_HANDLE h);


/**
 * \brief Thread
 */
SPL_HANDLE spl_thread_create(void *(*entry)(void *), void *param, int prio);
SPL_HANDLE spl_thread_current();
int spl_thread_suspend(SPL_HANDLE h);
int spl_thread_resume(SPL_HANDLE h);
int spl_thread_set_priority(SPL_HANDLE h, int prio);
int spl_thread_wait(SPL_HANDLE h);

void spl_thread_exit(void);
void spl_thread_detach(SPL_HANDLE h);
int spl_thread_kill(SPL_HANDLE h, int signo);
int spl_thread_destroy(SPL_HANDLE h);

/**
 * \brief Process
 */
SPL_HANDLE spl_process_create(int prio, char *const argv[]);
int spl_process_set_priority(SPL_HANDLE h, int prio);
int spl_process_wait(SPL_HANDLE h);
int spl_process_kill(SPL_HANDLE h, int signo);
SPL_HANDLE spl_process_current();

/**
 * \brief Other support
 */
/**
 * @brief Sleep the current thread
 *
 * @param ms Millisecond to sleep
 */
void spl_sleep(int ms);

/**
 * @brief Get a tick timestamp
 *
 * @return tick in millisecond
 */
unsigned long spl_get_ticks();
unsigned long long int spl_time_get_usec(void);

char *spl_get_cmdline(int *size);

char kvfs_path_sep(kvoid);
kbool kvfs_exist(const char *a_path);

#define KVFS_A_REG      0x01        /**< regular file */
#define KVFS_A_DEV      0x02        /**< device file */
#define KVFS_A_DIR      0x04        /**< directory */
#define KVFS_A_LNK      0x08        /**< symbol link */
#define KVFS_A_SKT      0x10        /**< socket */
#define KVFS_A_FIFO     0x20        /**< fifo or pipe */
#define KVFS_A_UNK      0x80        /**< unknown */

typedef struct _KVFS_FINDDATA_ {
	kuint attrib;               /**< OR-ed of KVFS_A_XXX */
	char name[2048];           /**< Null-terminated name */
} KVFS_FINDDATA;

/**
 * find directory with path == a_fspec not /xxx/ *.* and return each found files
 */
kbean kvfs_findfirst(const char *a_fspec, KVFS_FINDDATA *a_finfo);
int kvfs_findnext(kbean a_find, KVFS_FINDDATA *a_finfo);
int kvfs_findclose(kbean a_find);

int kvfs_chdir(const char *dir);
char *kvfs_getcwd(char *buf, int size);

/**
 * \brief timer with callback.
 */

/**
 * \brief Callback function when timer hit.
 *
 * \param id \c retid come from \c spl_timer_add().
 * \param userdata \c userdata set by user when call \c spl_timer_add().
 *
 * \return ktrue: continue run
 * kfalse: cancelled the timer.
 */
typedef int (*TIMER_CBK)(void *id, void *userdata);
int spl_timer_add(void **retid, int interval, TIMER_CBK callback, void *userdata);
int spl_timer_del(void *id);

/**
 * \brief read result from popen.
 */
char* popen_read(const char *cmd, char *buffer, int buflen);

void *spl_lib_load(const char *path, int flags);
int spl_lib_unload(void *handle);
void *spl_lib_getsym(void *lib, const char *name);

int wlogf(const char *fmt, ...);
int wlog(const char *msg);

/**
 * \brief Get full path of exe application.
 * This must be called before any chdir()
 */
void spl_exedir(char *argv[], char *exedir);
int spl_mkdir(const char *path, unsigned int mode);

int spl_sock_close(void *s);
int spl_sock_err();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __K_XTCOOL_H__ */

