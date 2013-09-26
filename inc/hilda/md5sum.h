/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __MD5SUM_H__
#define __MD5SUM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <hilda/sysdeps.h>

void md5_calculate(char rethash[32], const char *dat, int len);

#ifdef __cplusplus
}
#endif
#endif /* __MD5SUM_H__ */
