/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_STR_H__
#define __K_STR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <hilda/sysdeps.h>

#include <hilda/ktypes.h>

int kstr_cmp(const char *a_str0, const char *a_str1, int a_len);
int kstr_icmp(const char *a_str0, const char *a_str1, int a_len);
char *kstr_dup(const char *a_str);
char *kstr_subs(char *a_str, char a_from, char a_to);
int kstr_escxml(const char *a_is, char **a_os, int *a_ol);
int kstr_toint(const char *s, int *ret);

char *kstr_trim_left(char *str);
char *kstr_trim_right(char *str);
char *kstr_trim(char *str);

#ifdef __cplusplus
}
#endif
#endif /* __K_STR_H__ */
