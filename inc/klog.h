/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_LOG_H__
#define __K_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdarg.h>

#include <sysdeps.h>
#include <xtcool.h>

/*-----------------------------------------------------------------------
 * Embedded variable used by ktrace, klog, kerror, kfatal etc
 */
static int VAR_UNUSED __kl_file_name_id_ = -1;
static char VAR_UNUSED *__kl_file_name_ = NULL;

static int VAR_UNUSED __kl_prog_name_id_ = -1;
static char VAR_UNUSED *__kl_prog_name_ = NULL;

static int VAR_UNUSED __kl_modu_name_id_ = -1;


/*-----------------------------------------------------------------------
 * Normal and Raw Logger
 */
typedef void (*KNLOGGER)(const char *content, int len);
typedef void (*KRLOGGER)(unsigned char type, unsigned int mask, const char *prog, const char *modu, const char *file, const char *func, int ln, const char *fmt, va_list ap);


/*-----------------------------------------------------------------------
 * Define KMODU_NAME if someone forgot it.
 */
#ifndef KMODU_NAME
#define KMODU_NAME  "?"
#endif


/*-----------------------------------------------------------------------
 * KLog switch mask
 */
#define KLOG_TRC         0x00000001 /* t: Trace */
#define KLOG_LOG         0x00000002 /* l: Log */
#define KLOG_ERR         0x00000004 /* e: Error */
#define KLOG_FAT         0x00000008 /* f: Fatal Error */
#define KLOG_TYPE_ALL    0x0000000f

#define KLOG_RTM         0x00000100 /* s: Relative Time, in MS, 'ShiJian' */
#define KLOG_ATM         0x00000200 /* S: ABS Time, in MS, 'ShiJian' */

#define KLOG_PID         0x00001000 /* j: Process ID, 'JinCheng' */
#define KLOG_TID         0x00002000 /* x: Thread ID, 'XianCheng' */

#define KLOG_PROG        0x00010000 /* P: Process Name */
#define KLOG_MODU        0x00020000 /* M: Module Name */
#define KLOG_FILE        0x00040000 /* F: File Name */
#define KLOG_FUNC        0x00080000 /* H: Function Name, 'HanShu' */
#define KLOG_LINE        0x00100000 /* N: Line Number */

#define KLOG_ALL         0xffffffff
#define KLOG_DFT         (KLOG_LOG | KLOG_ERR | KLOG_FAT | KLOG_RTM | KLOG_FILE | KLOG_LINE)


/*-----------------------------------------------------------------------
 * Embedded variable used by ktrace, klog, kerror, kfatal etc
 */
#define KLOG_INNER_VAR_DEF() \
	static int VAR_UNUSED __kl_ver_sav = -1; \
	static int VAR_UNUSED __kl_func_name_id = -1; \
	static int VAR_UNUSED __kl_mask = 0; \
	int VAR_UNUSED __kl_ver_get = klog_touches()

#define KLOG_SETUP_NAME_AND_ID() do { \
	if (__kl_file_name_id_ == -1) { \
		__kl_file_name_ = klog_get_name_part(__FILE__); \
		__kl_file_name_id_ = klog_file_name_add(__kl_file_name_); \
	} \
	if (__kl_prog_name_id_ == -1) { \
		__kl_prog_name_ = klog_get_prog_name(); \
		__kl_prog_name_id_ = klog_prog_name_add(__kl_prog_name_); \
	} \
	if (__kl_modu_name_id_ == -1) { \
		__kl_modu_name_id_ = klog_modu_name_add(KMODU_NAME); \
	} \
	if (__kl_func_name_id == -1) { \
		__kl_func_name_id = klog_func_name_add(__func__); \
	} \
} while (0)


/*-----------------------------------------------------------------------
 * ktrace, klog, kerror, kfatal etc
 */
#define klogs(fmt, ...) do { \
	klog_f(0, 0, NULL, NULL, NULL, NULL, 0, fmt, ##__VA_ARGS__); \
} while (0)

#define ktrace(fmt, ...) do { \
	KLOG_INNER_VAR_DEF(); \
	if (__kl_ver_get > __kl_ver_sav) { \
		__kl_ver_sav = __kl_ver_get; \
		KLOG_SETUP_NAME_AND_ID(); \
		__kl_mask = klog_calc_mask(__kl_prog_name_id_, __kl_modu_name_id_, __kl_file_name_id_, __kl_func_name_id, __LINE__, (int)spl_process_current()); \
		if (!(__kl_mask & KLOG_TRC)) \
			__kl_mask = 0; \
	} \
	if (__kl_mask) \
		klog_f('T', __kl_mask, __kl_prog_name_, KMODU_NAME, __kl_file_name_, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
} while (0)

#define klog(fmt, ...) do { \
	KLOG_INNER_VAR_DEF(); \
	if (__kl_ver_get > __kl_ver_sav) { \
		__kl_ver_sav = __kl_ver_get; \
		KLOG_SETUP_NAME_AND_ID(); \
		__kl_mask = klog_calc_mask(__kl_prog_name_id_, __kl_modu_name_id_, __kl_file_name_id_, __kl_func_name_id, __LINE__, (int)spl_process_current()); \
		if (!(__kl_mask & KLOG_LOG)) \
			__kl_mask = 0; \
	} \
	if (__kl_mask) \
		klog_f('L', __kl_mask, __kl_prog_name_, KMODU_NAME, __kl_file_name_, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
} while (0)

#define kerror(fmt, ...) do { \
	KLOG_INNER_VAR_DEF(); \
	if (__kl_ver_get > __kl_ver_sav) { \
		__kl_ver_sav = __kl_ver_get; \
		KLOG_SETUP_NAME_AND_ID(); \
		__kl_mask = klog_calc_mask(__kl_prog_name_id_, __kl_modu_name_id_, __kl_file_name_id_, __kl_func_name_id, __LINE__, (int)spl_process_current()); \
		if (!(__kl_mask & KLOG_ERR)) \
			__kl_mask = 0; \
	} \
	if (__kl_mask) \
		klog_f('E', __kl_mask, __kl_prog_name_, KMODU_NAME, __kl_file_name_, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
} while (0)

#define kfatal(fmt, ...) do { \
	KLOG_INNER_VAR_DEF(); \
	if (__kl_ver_get > __kl_ver_sav) { \
		__kl_ver_sav = __kl_ver_get; \
		KLOG_SETUP_NAME_AND_ID(); \
		__kl_mask = klog_calc_mask(__kl_prog_name_id_, __kl_modu_name_id_, __kl_file_name_id_, __kl_func_name_id, __LINE__, (int)spl_process_current()); \
		if (!(__kl_mask & KLOG_FAT)) \
			__kl_mask = 0; \
	} \
	if (__kl_mask) \
		klog_f('F', __kl_mask, __kl_prog_name_, KMODU_NAME, __kl_file_name_, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
} while (0)

#define kassert(_x_) do { \
	if (!(_x_)) { \
		KLOG_INNER_VAR_DEF(); \
		if (__kl_ver_get > __kl_ver_sav) { \
			__kl_ver_sav = __kl_ver_get; \
			KLOG_SETUP_NAME_AND_ID(); \
		} \
		klog_f('A', KLOG_ALL, __kl_prog_name_, KMODU_NAME, __kl_file_name_, __FUNCTION__, __LINE__, \
				"\n\t ASSERT NG: \"%s\"\n\n", #_x_); \
	} \
} while (0)


/*-----------------------------------------------------------------------
 * Functions:
 */
void *klog_init(unsigned int deflev, int argc, char **argv);
kinline void *klog_cc(void) VAR_UNUSED;
void *klog_attach(void *logcc);

kinline void klog_touch(void) VAR_UNUSED;
kinline int klog_touches(void) VAR_UNUSED;

char *klog_get_name_part(char *name);
char *klog_get_prog_name();

int klog_file_name_add(const char *name);
int klog_modu_name_add(const char *name);
int klog_prog_name_add(const char *name);
int klog_func_name_add(const char *name);

void klog_rule_add(const char *rule);
void klog_rule_del(int index);
void klog_rule_clr();

unsigned int klog_calc_mask(int prog, int modu, int file, int func, int line, int pid);

int klog_f(unsigned char type, unsigned int mask, const char *prog, const char *modu, const char *file, const char *func, int ln, const char *fmt, ...);
int klog_vf(unsigned char type, unsigned int mask, const char *prog, const char *modu, const char *file, const char *func, int ln, const char *fmt, va_list ap);

int klog_add_logger(KNLOGGER logger);
int klog_del_logger(KNLOGGER logger);
int klog_add_rlogger(KRLOGGER logger);
int klog_del_rlogger(KRLOGGER logger);


#ifdef __cplusplus
}
#endif
#endif /* __K_LOG_H__ */

