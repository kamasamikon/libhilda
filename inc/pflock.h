/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __PFLOCK_H__
#define __PFLOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int pflck_get(const char *pidfile, int *retlck);
int pflck_rel(int lck);

#ifdef __cplusplus
}
#endif
#endif /* __PFLOCK_H__ */

