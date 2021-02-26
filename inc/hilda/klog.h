/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_LOG_H__
#define __K_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include <hilda/sysdeps.h>
#include <hilda/xtcool.h>

/*-----------------------------------------------------------------------
 * Embedded variable used by klog_xxx
 */
static char VAR_UNUSED *__kl_file_name = NULL;
static char VAR_UNUSED *__kl_prog_name = NULL;

/*-----------------------------------------------------------------------
 * Normal and Raw Logger
 */
typedef void (*KNLOGGER)(char *content, int len);
typedef void (*KRLOGGER)(unsigned char type, unsigned int mask, char *prog, char *modu, char *file, char *func, int ln, const char *fmt, va_list ap);

/*-----------------------------------------------------------------------
 * Define KLOG_MODU_NAME if someone forgot it.
 */
#ifndef KLOG_MODU_NAME
#define KLOG_MODU_NAME  "?"
#endif

/*-----------------------------------------------------------------------
 * KLog switch mask
 */
/* Copied from syslog */
#define KLOG_FATAL      0x00000001 /* 0:f(fatal): system is unusable */
#define KLOG_ALERT      0x00000002 /* 1:a: action must be taken immediately */
#define KLOG_CRIT       0x00000004 /* 2:c: critical conditions */
#define KLOG_ERR        0x00000008 /* 3:e: error conditions */
#define KLOG_WARNING    0x00000010 /* 4:w: warning conditions */
#define KLOG_NOTICE     0x00000020 /* 5:n: normal but significant condition */
#define KLOG_INFO       0x00000040 /* 6:i:l(log): informational */
#define KLOG_DEBUG      0x00000080 /* 7:d:t(trace): debug-level messages */
#define KLOG_TYPE_ALL   0x000000ff

#define KLOG_LOG        KLOG_INFO
#define KLOG_TRC        KLOG_DEBUG

#define KLOG_RTM        0x00000100 /* s: Relative Time, in MS, 'ShiJian' */
#define KLOG_ATM        0x00000200 /* S: ABS Time, in MS, 'ShiJian' */

#define KLOG_PID        0x00001000 /* j: Process ID, 'JinCheng' */
#define KLOG_TID        0x00002000 /* x: Thread ID, 'XianCheng' */

#define KLOG_PROG       0x00010000 /* P: Process Name */
#define KLOG_MODU       0x00020000 /* M: Module Name */
#define KLOG_FILE       0x00040000 /* F: File Name */
#define KLOG_FUNC       0x00080000 /* H: Function Name, 'HanShu' */
#define KLOG_LINE       0x00100000 /* N: Line Number */

#define KLOG_ALL        0xffffffff
#define KLOG_DFT        (KLOG_FATAL | KLOG_ALERT | KLOG_CRIT | KLOG_ERR | KLOG_WARNING | KLOG_NOTICE | KLOG_ATM | KLOG_PROG | KLOG_MODU | KLOG_FILE | KLOG_LINE)

/*-----------------------------------------------------------------------
 * Embedded variable used by klog_xxx
 */
#define KLOG_INNER_VAR_DEF() \
	static int VAR_UNUSED __kl_ver_sav = -1; \
	static char VAR_UNUSED *__kl_modu_name = NULL; \
	static char VAR_UNUSED *__kl_func_name = NULL; \
	static int VAR_UNUSED __kl_mask = 0; \
	int VAR_UNUSED __kl_ver_get = klog_touches()

#define KLOG_SETUP_NAME(modu, file, func) do { \
	if (unlikely(__kl_file_name == NULL)) { \
		__kl_file_name = klog_file_name_add((char*)file); \
	} \
	if (unlikely(__kl_prog_name == NULL)) { \
		__kl_prog_name = klog_prog_name_add(NULL); \
	} \
	if (unlikely(__kl_modu_name == NULL)) { \
		__kl_modu_name = klog_modu_name_add((char*)modu); \
	} \
	if (unlikely(__kl_func_name == NULL)) { \
		__kl_func_name = klog_func_name_add((char*)func); \
	} \
} while (0)

#define KLOG_CHK_AND_CALL(mask, indi, modu, file, func, line, fmt, ...) do { \
	KLOG_INNER_VAR_DEF(); \
	if (unlikely(__kl_ver_get > __kl_ver_sav)) { \
		__kl_ver_sav = __kl_ver_get; \
		KLOG_SETUP_NAME(modu, file, func); \
		__kl_mask = klog_calc_mask(__kl_prog_name, __kl_modu_name, __kl_file_name, __kl_func_name, line); \
		if (!(__kl_mask & (mask))) { \
			__kl_mask = 0; \
		} \
	} \
	if (__kl_mask) { \
		klog_f(indi, __kl_mask, __kl_prog_name, modu, __kl_file_name, (char*)func, line, fmt, ##__VA_ARGS__); \
	} \
} while (0)

#define KLOG_CHK_AND_CALL_AP(mask, indi, modu, file, func, line, fmt, ap) do { \
	KLOG_INNER_VAR_DEF(); \
	if (unlikely(__kl_ver_get > __kl_ver_sav)) { \
		__kl_ver_sav = __kl_ver_get; \
		KLOG_SETUP_NAME(modu, file, func); \
		__kl_mask = klog_calc_mask(__kl_prog_name, __kl_modu_name, __kl_file_name, __kl_func_name, line); \
		if (!(__kl_mask & (mask))) { \
			__kl_mask = 0; \
		} \
	} \
	if (__kl_mask) { \
		klog_vf(indi, __kl_mask, __kl_prog_name, modu, __kl_file_name, (char*)func, line, fmt, ap); \
	} \
} while (0)

/*-----------------------------------------------------------------------
 * klog_xxx
 */
#define klogs(fmt, ...) do { \
	klog_f(0, 0, NULL, NULL, NULL, NULL, 0, fmt, ##__VA_ARGS__); \
} while (0)

#define kfatal(fmt, ...)       KLOG_CHK_AND_CALL(KLOG_FATAL,   'F', KLOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define kalert(fmt, ...)       KLOG_CHK_AND_CALL(KLOG_ALERT,   'A', KLOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define kcritical(fmt, ...)    KLOG_CHK_AND_CALL(KLOG_CRIT,    'C', KLOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define kerror(fmt, ...)       KLOG_CHK_AND_CALL(KLOG_ERR,     'E', KLOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define kwarning(fmt, ...)     KLOG_CHK_AND_CALL(KLOG_WARNING, 'W', KLOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define knotice(fmt, ...)      KLOG_CHK_AND_CALL(KLOG_NOTICE,  'N', KLOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define kinfo(fmt, ...)        KLOG_CHK_AND_CALL(KLOG_INFO,    'I', KLOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define kdebug(fmt, ...)       KLOG_CHK_AND_CALL(KLOG_DEBUG,   'D', KLOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define klog(fmt, ...)         KLOG_CHK_AND_CALL(KLOG_DEBUG,   'D', KLOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define kassert(expr, msg) do { \
	if (!(expr)) { \
		KLOG_INNER_VAR_DEF(); \
		if (__kl_ver_get > __kl_ver_sav) { \
			__kl_ver_sav = __kl_ver_get; \
			KLOG_SETUP_NAME(KLOG_MODU_NAME, __FILE__, __func__); \
		} \
		klog_f('!', KLOG_ALL, __kl_prog_name, KLOG_MODU_NAME, __kl_file_name, (char*)__FUNCTION__, __LINE__, \
				"ASSERT FAILED: %s\n", msg); \
	} \
} while (0)

/*-----------------------------------------------------------------------
 * Functions:
 */
int klog_touches(void);

char *klog_info(void);

char *klog_file_name_add(char *name);
char *klog_modu_name_add(char *name);
char *klog_prog_name_add(char *name);
char *klog_func_name_add(char *name);

unsigned int klog_calc_mask(char *prog, char *modu, char *file, char *func, int line);

void klog_bt(const char *fmt, ...);
int klog_f(unsigned char type, unsigned int mask, char *prog, char *modu, char *file, char *func, int ln, const char *fmt, ...) __attribute__ ((format (printf, 8, 9)));
int klog_vf(unsigned char type, unsigned int mask, char *prog, char *modu, char *file, char *func, int ln, const char *fmt, va_list ap);

int klog_add_logger(KNLOGGER logger);
int klog_del_logger(KNLOGGER logger);

int klog_add_rlogger(KRLOGGER logger);
int klog_del_rlogger(KRLOGGER logger);

void *klog_attach(void *logcc);
void klog_touch(void);

void klog_set_default_mask(unsigned int mask);
void *klog_init(int argc, char **argv);

void klog_rule_add(char *rule);
void klog_rule_del(unsigned int idx);
void klog_rule_clr(void);

#ifdef __cplusplus
}
#endif
#endif /* __N_LIB_LOG_H__ */

