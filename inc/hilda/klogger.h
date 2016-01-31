/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_LOG_H__
#define __K_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

void klog_add_stdout_logger(void);
void klog_del_stdout_logger(void);


void klog_add_stderr_logger(void);
void klog_del_stderr_logger(void);

void klog_add_file_logger(const char *pathfmt, unsigned int size,
		unsigned int time, unsigned int when);
void klog_del_file_logger(void);


void klog_add_network_logger(const char *addr, unsigned short port);
void klog_del_network_logger(void);

#ifdef __cplusplus
}
#endif
#endif /* __N_LIB_LOG_H__ */

