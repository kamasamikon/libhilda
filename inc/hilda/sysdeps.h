/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_SYSDEPS_H__
#define __K_SYSDEPS_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#define kinline
#define kexport __declspec(dllexport)
#define VAR_UNUSED
typedef long long int int64_t;
#endif

#ifdef __GNUC__
#define kinline inline
#define kexport
#define __int64 long long
#define VAR_UNUSED __attribute__ ((unused))
#endif

#ifdef __cplusplus
}
#endif

#endif /* __K_SYSDEPS_H__ */

