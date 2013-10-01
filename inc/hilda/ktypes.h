/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

/*---------------------------------------------------------------------------

FileName:

ivtype.h

Abstract:

Override some data type to provide independance between some platform.

The assumption for the data length is:
sizeof(kllint) >= 8 byte
sizeof(klong) >= 4 byte
sizeof(kint) >= 4 byte
sizeof(kshort) >= 2 byte
sizeof(kchar) >= 1 byte

----------------------------------------------------------------------------*/

#ifndef __K_TYPE_H__
#define __K_TYPE_H__

typedef signed long long int kllint;
typedef unsigned long long int kullint;

typedef signed long klong;
typedef unsigned long kulong;

typedef signed int kint;
typedef unsigned int kuint;

typedef signed short kshort;
typedef unsigned short kushort;

typedef signed char kchar;
typedef unsigned char kuchar;

typedef signed short kwchar;
typedef unsigned short kuwchar;

typedef kint kbool;

typedef void *kbean;

#define kvoid void

#define kfalse ((kbool)0)
#define ktrue (!kfalse)

#ifndef knull
#ifdef  __cplusplus
#define knull 0
#else
#define knull ((void *)0)
#endif
#endif

#define knul '\0'
#define knil knull

#define EC_OK           0x00000000
#define EC_DEFAULT      0x00000001
#define EC_IGNORE       0x00000002
#define EC_SKIP         0x00000003

#define EC_NG           -1
#define EC_NOIMPL       0x80010000
#define EC_BAD_TYPE     0x80020000
#define EC_RECUR        0x80030000
#define EC_NOINIT       0x80040000
#define EC_NOTFOUND     0x80050000
#define EC_FORBIDEN     0x80060000
#define EC_COMMAND      0x80070000
#define EC_NOTHING      0x80080000
#define EC_EXIST        0x80090000
#define EC_BAD_PARAM    0x800a0000
#define EC_CANCEL       0x800b0000
#define EC_CONNECT      0x800c0000

#define EC_ERR(c)   ((c) & 0xffff0000)

#endif /* __K_TYPE_H__ */

