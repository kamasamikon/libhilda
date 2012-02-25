/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __SYSDEPS_H__
#define __SYSDEPS_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#define kinline
#define kexport __declspec(dllexport)
#define VAR_UNUSED
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

#endif /* __SYSDEPS_H__ */

