/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_LOG_H__
#define __K_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sysdeps.h>

#include <stdio.h>

#include <ktypes.h>
#include <xtcool.h>

/*----------------------------------------------------------------------------
 * debug message mask
 */
#define LOG_LOG         0x00000001
#define LOG_ERR         0x00000002
#define LOG_FAT         0x00000004
#define LOG_TIME        0x00000080

#if defined(CFG_KLOG_DO_NOTHING)
#define klog_init(deflev, argc, argv) do {} while (0)
#define klog_attach(logcc) do {} while (0)
#define klog_getlevel(a_file) 0

#define LOG_HIDE_LOG    1
#define LOG_HIDE_ERR    1
#define LOG_HIDE_FAT    1
#define LOG_HIDE_ASSERT 1

#else
void *klog_init(kuint deflev, int argc, char **argv);
void *klog_cc(void);
void *klog_attach(void *logcc);

/**
 * \brief Save log to a file.
 * \param yesno Yes => save, No => Don't save.
 * \param max_size Max size of output file, if equal -1, no limit.
 * \param get_path A callback to get log path name.
 *
 * \return Old yesno setting.
 */
int klog_to_file(int yesno, int max_size, int (*get_path)(char path[1024]));
int klog_to_wlog(int yesno);

kuint klog_getlevel(const kchar *a_file);
kinline int klog_touches(void);
void klog_set_level(const char *cmd);
int kprintf(const char *fmt, ...);

static kint VAR_UNUSED __g_klog_touches = -1;
static kuint VAR_UNUSED __gc_klog_level;

#endif

#define GET_LOG_LEVEL() do { \
	int touches = klog_touches(); \
	if (__g_klog_touches < touches) { \
		__g_klog_touches = touches; \
		__gc_klog_level = klog_getlevel((const kchar*)__FILE__); \
	} \
} while (0)

#if defined(LOG_HIDE_LOG) || defined(LOG_HIDE_CUR_LOG)
#define klog(x) do {} while (0)
#else
#define klog(x) do { \
	GET_LOG_LEVEL(); \
	if (__gc_klog_level & LOG_LOG) { \
		if (__gc_klog_level & LOG_TIME) { \
			kprintf("[klog:%lu]-", spl_get_ticks()); \
		} else { \
			kprintf("[klog]-"); \
		} \
		kprintf x ; \
	} \
} while (0)
#endif

#if defined(LOG_HIDE_ERR) || defined(LOG_HIDE_CUR_ERR)
#define kerror(x) do {} while (0)
#else
#define kerror(x) do { \
	GET_LOG_LEVEL(); \
	if (__gc_klog_level & LOG_ERR) { \
		if (__gc_klog_level & LOG_TIME) { \
			kprintf("[kerr:%lu]-", spl_get_ticks()); \
		} else { \
			kprintf("[kerr]-"); \
		} \
		kprintf x ; \
	} \
} while (0)
#endif

#if defined(LOG_HIDE_FAT) || defined(LOG_HIDE_CUR_FAT)
#define kfatal(x) do {} while (0)
#else
#define kfatal(x) do { \
	GET_LOG_LEVEL(); \
	if (__gc_klog_level & LOG_FAT) { \
		if (__gc_klog_level & LOG_TIME) { \
			kprintf("[kfat:%lu]-", spl_get_ticks()); \
		} else { \
			kprintf("[kfat]-"); \
		} \
		kprintf x ; \
	} \
} while (0)
#endif

#if defined(LOG_HIDE_ASSERT) || defined(LOG_HIDE_ASSERT)
#define kassert(x) do {} while (0)
#elif defined(LOG_ASSERT_AS_ERR)
#define kassert(_x_) \
	do { \
		if (!(_x_)) { \
			kprintf("\n\n\tkassert failed!!!\n\t[%s], \n\tFILE:%s, LINES:%d\n\n", #_x_, __FILE__, __LINE__); \
		} \
	} while (0)
#else
#define kassert(_x_) \
	do { \
		if (!(_x_)) { \
			kprintf("\n\n\tkassert failed!!!\n\t[%s], \n\tFILE:%s, LINES:%d\n\n", #_x_, __FILE__, __LINE__); \
			/* klogbrk(); kbacktrace(); */ \
		} \
	} while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __K_LOG_H__ */

