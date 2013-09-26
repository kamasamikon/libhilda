/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __CRC_32_H__
#define __CRC_32_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <hilda/sysdeps.h>

unsigned long crc32_calc(unsigned char *buf, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif /* __CRC_32_H__ */

