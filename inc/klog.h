/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */ #ifndef __K_LOG_H__
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
#define LOG_ALL         0xffffffff

#define LOG_LOG         0x00000001 /* l: Log */
#define LOG_ERR         0x00000002 /* e: Error */
#define LOG_FAT         0x00000004 /* f: Fatal Error */
#define LOG_TYPE_ALL    0x0000000f

#define LOG_RTM		0x00000100 /* t: Relative Time */
#define LOG_ATM		0x00000200 /* T: ABS Time, in MS */

#define LOG_PID		0x00001000 /* p: Process ID */
#define LOG_TID		0x00002000 /* P: Thread ID */

#define LOG_LINE        0x00010000 /* N: Line Number */
#define LOG_FILE        0x00020000 /* F: File Name */
#define LOG_MODU        0x00040000 /* M: Module Name */

/*
 * Normal and Raw Logger
 */
/* content ends with NUL, len should be equal to strlen(content) */
typedef int (*KNLOGGER)(const char *content, int len);
/* modu = MODULE, file = FileName, ln = Line */
typedef int (*KRLOGGER)(unsigned char type, unsigned int flg, const char *modu, const char *file, int ln, const char *fmt, va_list ap);

#if defined(CFG_KLOG_DO_NOTHING)
#define klog_init(deflev, argc, argv) do {} while (0)
#define klog_attach(logcc) do {} while (0)
#define klog_getflg(file) 0

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

kuint klog_getflg(const kchar *file);
void klog_setflg(const char *cmd);
kinline int klog_touches(void);

static kint VAR_UNUSED __g_klog_touches = -1;
static kuint VAR_UNUSED __gc_klog_level;

/* KLOG Full */
/* modu = MODULE, file = FileName, ln = Line */
int klogf(unsigned char type, unsigned int flg, const char *modu, const char *file, int ln, const char *fmt, ...);

/* KLOG Short */
#define klogs(fmt, ...) do { \
	klogf(0, 0, NULL, NULL, 0, fmt, ##__VA_ARGS__); \
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
#ifdef KLOG_MODU
#define klog(fmt, ...) do { \
	GET_LOG_LEVEL(); \
	if (__gc_klog_level & LOG_LOG) { \
		klogf('L', __gc_klog_level, KLOG_MODU, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} \
} while (0)
#else
#define klog(fmt, ...) do { \
	GET_LOG_LEVEL(); \
	if (__gc_klog_level & LOG_LOG) { \
		klogf('L', __gc_klog_level, NULL, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} \
} while (0)
#endif
#endif

#if defined(LOG_HIDE_ERR) || defined(LOG_HIDE_CUR_ERR)
#define kerror(fmt, ...) do {} while (0)
#else
#ifdef KLOG_MODU
#define kerror(fmt, ...) do { \
	GET_LOG_LEVEL(); \
	if (__gc_klog_level & LOG_ERR) { \
		klogf('E', __gc_klog_level, KLOG_MODU, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} \
} while (0)
#else
#define kerror(fmt, ...) do { \
	GET_LOG_LEVEL(); \
	if (__gc_klog_level & LOG_ERR) { \
		klogf('E', __gc_klog_level, NULL, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} \
} while (0)
#endif
#endif

#if defined(LOG_HIDE_FAT) || defined(LOG_HIDE_CUR_FAT)
#define kfatal(fmt, ...) do {} while (0)
#else
#ifdef KLOG_MODU
#define kfatal(fmt, ...) do { \
	GET_LOG_LEVEL(); \
	if (__gc_klog_level & LOG_FAT) { \
		klogf('F', __gc_klog_level, KLOG_MODU, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} \
} while (0)
#else
#define kfatal(fmt, ...) do { \
	GET_LOG_LEVEL(); \
	if (__gc_klog_level & LOG_FAT) { \
		klogf('F', __gc_klog_level, NULL, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} \
} while (0)
#endif
#endif

#if defined(LOG_HIDE_ASSERT) || defined(LOG_HIDE_ASSERT)
#define kassert(x) do {} while (0)
#else
#ifdef KLOG_MODU
#define kassert(_x_) \
	do { \
		if (!(_x_)) { \
			klogf(0, 0, KLOG_MODU, __FILE__, __LINE__, "\n\tKASSERT FAILED: [%s]\n\n", #_x_); \
			/* klogbrk(); kbacktrace(); */ \
		} \
	} while (0)
#else
#define kassert(_x_) \
	do { \
		if (!(_x_)) { \
			klogf(0, 0, NULL, __FILE__, __LINE__, "\n\tKASSERT FAILED: [%s]\n\n", #_x_); \
			/* klogbrk(); kbacktrace(); */ \
		} \
	} while (0)
#endif
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif /* __K_LOG_H__ */

