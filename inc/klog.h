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
#define LOG_TM_REL      0x00000008
#define LOG_TM_ABS      0x00000010
#define LOG_FUNC        0x00000020
#define LOG_LINE        0x00000040

/* content ends with NUL, len should be equal to strlen(content) */
/* Normal and Raw Logger */
typedef int (*KNLOGGER)(const char *content, int len);
typedef int (*KRLOGGER)(unsigned char type, unsigned int flg, const char *fn, int ln, const char *fmt, va_list ap);

#if defined(CFG_KLOG_DO_NOTHING)
#define klog_init(deflev, argc, argv) do {} while (0)
#define klog_attach(logcc) do {} while (0)
#define klog_getflg(a_file) 0

#define LOG_HIDE_LOG    1
#define LOG_HIDE_ERR    1
#define LOG_HIDE_FAT    1
#define LOG_HIDE_ASSERT 1

#else
void *klog_init(kuint deflev, int argc, char **argv);
void *klog_cc(void);
void *klog_attach(void *logcc);

int klog_add_logger(KNLOGGER logger);
int klog_del_logger(KNLOGGER logger);

int klog_add_rlogger(KRLOGGER logger);
int klog_del_rlogger(KRLOGGER logger);

kuint klog_getflg(const kchar *fn);
void klog_setflg(const char *cmd);
kinline int klog_touches(void);
int klogf(unsigned char type, unsigned int flg, const char *fn, int ln, const char *fmt, ...);

static kint VAR_UNUSED __g_klog_touches = -1;
static kuint VAR_UNUSED __gc_klog_level;

/* KLOG Full */
int klogf(unsigned char type, unsigned int flg, const char *fn, int ln, const char *fmt, ...);

/* KLOG Short */
#define klogs(fmt, ...) do { \
	klogf(0, 0, "", 0, fmt, ##__VA_ARGS__); \
} while (0)

#define GET_LOG_LEVEL() do { \
	int touches = klog_touches(); \
	if (__g_klog_touches < touches) { \
		__g_klog_touches = touches; \
		__gc_klog_level = klog_getflg((const kchar*)__FILE__); \
	} \
} while (0)

#if defined(LOG_HIDE_LOG) || defined(LOG_HIDE_CUR_LOG)
#define klog(fmt, ...) do {} while (0)
#else
#define klog(fmt, ...) do { \
	GET_LOG_LEVEL(); \
	if (__gc_klog_level & LOG_LOG) { \
		klogf('L', __gc_klog_level, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
	} \
} while (0)
#endif

#if defined(LOG_HIDE_ERR) || defined(LOG_HIDE_CUR_ERR)
#define kerror(fmt, ...) do {} while (0)
#else
#define kerror(fmt, ...) do { \
	GET_LOG_LEVEL(); \
	if (__gc_klog_level & LOG_ERR) { \
		klogf('E', __gc_klog_level, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
	} \
} while (0)
#endif

#if defined(LOG_HIDE_FAT) || defined(LOG_HIDE_CUR_FAT)
#define kfatal(fmt, ...) do {} while (0)
#else
#define kfatal(fmt, ...) do { \
	GET_LOG_LEVEL(); \
	if (__gc_klog_level & LOG_FAT) { \
		klogf('F', __gc_klog_level, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
	} \
} while (0)
#endif

#if defined(LOG_HIDE_ASSERT) || defined(LOG_HIDE_ASSERT)
#define kassert(x) do {} while (0)
#elif defined(LOG_ASSERT_AS_ERR)
#define kassert(_x_) \
	do { \
		if (!(_x_)) { \
			klogf(0, 0, "", 0, "\n\n\tkassert failed!!!\n\t[%s], \n\tFILE:%s, LINES:%d, FUNC:%s\n\n", #_x_, __FILE__, __LINE__, __FUNCTION__); \
		} \
	} while (0)
#else
#define kassert(_x_) \
	do { \
		if (!(_x_)) { \
			klogf(0, 0, "", 0, "\n\n\tkassert failed!!!\n\t[%s], \n\tFILE:%s, LINES:%d, FUNC:%s\n\n", #_x_, __FILE__, __LINE__, __FUNCTION__); \
			/* klogbrk(); kbacktrace(); */ \
		} \
	} while (0)
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif /* __K_LOG_H__ */

