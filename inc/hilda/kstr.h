/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_STR_H__
#define __K_STR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <hilda/sysdeps.h>

#include <hilda/ktypes.h>

kint kstr_cmp(const kchar *a_str0, const kchar *a_str1, kint a_len);
kint kstr_icmp(const kchar *a_str0, const kchar *a_str1, kint a_len);
kchar *kstr_dup(const kchar *a_str);
kchar *kstr_subs(kchar *a_str, kchar a_from, kchar a_to);
kint kstr_escxml(const kchar *a_is, kchar **a_os, kint *a_ol);
kint kstr_toint(const kchar *s, kint *ret);

char *kstr_trim_left(char *str);
char *kstr_trim_right(char *str);
char *kstr_trim(char *str);

#ifdef __cplusplus
}
#endif
#endif /* __K_STR_H__ */
