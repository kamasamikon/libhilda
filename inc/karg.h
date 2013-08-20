/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_ARG_H__
#define __K_ARG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sysdeps.h>

char **build_argv_nul(const char *ibuf, int ilen, int *arg_c, char ***arg_v);
char **build_argv(const char *input, int *arg_c, char ***arg_v);
void free_argv(char **vector);

int arg_find(int argc, char **argv, const char *opt, int fullmatch);

#ifdef __cplusplus
}
#endif

#endif /* __K_ARG_H__ */

