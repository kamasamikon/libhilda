/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_ARG_H__
#define __K_ARG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <hilda/sysdeps.h>

char **karg_build_nul(const char *ibuf, int ilen, int *arg_c, char ***arg_v);
char **karg_build(const char *input, int *arg_c, char ***arg_v);
void karg_free(char **vector);

int karg_find(int argc, char **argv, const char *opt, int fullmatch);

#ifdef __cplusplus
}
#endif

#endif /* __K_ARG_H__ */

