/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_BUF_H__
#define __K_BUF_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

typedef struct _kbuf_s kbuf_s;
struct _kbuf_s {
	size_t alloc;
	size_t len;
	char *buf;
};

#define KBNULL { .alloc = 0, .len = 0, .buf = NULL }

void kbuf_setlen(kbuf_s *kb, size_t len);

void kbuf_init(kbuf_s *kb, size_t hint);
void kbuf_release(kbuf_s *kb);

char *kbuf_detach(kbuf_s *kb, size_t *size);
void kbuf_attach(kbuf_s *kb, void *buf, size_t len, size_t alloc);

void kbuf_grow(kbuf_s *kb, size_t extra);

void kbuf_addat(kbuf_s *kb, size_t offset, const void *data, size_t len);
void kbuf_add(kbuf_s *kb, const void *data, size_t len);
void kbuf_add8(kbuf_s *kb, unsigned char data);
void kbuf_add16(kbuf_s *kb, unsigned short data, int swap);
void kbuf_add32(kbuf_s *kb, unsigned int data, int swap);
void kbuf_adds(kbuf_s *kb, const char *str);
void kbuf_addf(kbuf_s *kb, const char *fmt, ...);
void kbuf_vaddf(kbuf_s *kb, const char *fmt, va_list ap);

size_t kbuf_fread(kbuf_s *kb, size_t size, FILE *fp, int *err);
void kbuf_dump(kbuf_s *kb, const char *banner, char *dat, int len, int width);

#endif /* __K_BUF_H__ */

