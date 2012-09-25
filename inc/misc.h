/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_MISC_H__
#define __K_MISC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sysdeps.h>
#include <ktypes.h>

/* 0 for success, 1 for v buffer too small, -1 for not found */
int url_getv(const char *str, const char *k, int klen, char v[], int len);
int split_ipaddr(const char *addr, char outip[128], unsigned short *outport);

#ifdef __cplusplus
}
#endif
#endif /* __K_MISC_H__ */
