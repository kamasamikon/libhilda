/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>

#include <hilda/kbuf.h>

#define alloc_nr(x) (((x) + 16) * 3 / 2)

#define MEM_INC(x, nr, alloc) \
	do { \
		if ((nr) > alloc) { \
			if (alloc_nr(alloc) < (nr)) \
				alloc = (nr); \
			else \
				alloc = alloc_nr(alloc); \
			x = realloc((x), alloc * sizeof(*(x))); \
		} \
	} while (0)


#define SWAP_16(n) (((((uint16_t)n) & 0xff) << 8) | ((((uint16_t)n) >> 8) & 0xff))

#define SWAP_32(n) ( \
        (((uint32_t)n) >> 24) | \
		((((uint32_t)n) & 0x00ff0000) >> 8)  | \
		((((uint32_t)n) & 0x0000ff00) << 8)  | \
		(((uint32_t)n) << 24) \
        )

static char kbuf_slopbuf[1];

static size_t kbuf_avail(const kbuf_s *kb)
{
	return kb->alloc ? kb->alloc - kb->len - 1 : 0;
}

void kbuf_setlen(kbuf_s *kb, size_t len)
{
	if (len > (kb->alloc ? kb->alloc - 1 : 0)) {
		fprintf(stderr, "fatal! kbuf_setlen: len = %lu\n", (unsigned long)len);
		exit(-1);
	}
	kb->len = len;
	kb->buf[len] = '\0';
}

void kbuf_init(kbuf_s *kb, size_t hint)
{
	kb->alloc = kb->len = 0;
	kb->buf = kbuf_slopbuf;
	if (hint)
		kbuf_grow(kb, hint);
}

void kbuf_release(kbuf_s *kb)
{
	if (kb->alloc) {
		free(kb->buf);
		kbuf_init(kb, 0);
	}
}

char *kbuf_detach(kbuf_s *kb, size_t *size)
{
	char *res = kb->alloc ? kb->buf : NULL;

	if (size)
		*size = kb->len;
	kbuf_init(kb, 0);
	return res;
}

void kbuf_attach(kbuf_s *kb, void *buf, size_t len, size_t alloc)
{
	kbuf_release(kb);
	kb->buf = buf;
	kb->len = len;
	kb->alloc = alloc;
	kbuf_grow(kb, 0);
	kb->buf[kb->len] = '\0';
}

void kbuf_grow(kbuf_s *kb, size_t extra)
{
	int new_buf = !kb->alloc;

	if (new_buf)
		kb->buf = NULL;
	MEM_INC(kb->buf, kb->len + extra + 1, kb->alloc);
	if (new_buf)
		kb->buf[0] = '\0';
}

void kbuf_addat(kbuf_s *kb, size_t offset, const void *data, size_t len)
{
	size_t endpos = offset + len;

	if (endpos > kb->alloc) {
		kbuf_grow(kb, endpos - kb->alloc);
	}
	memcpy(kb->buf + offset, data, len);
	if (endpos > kb->len) {
		kbuf_setlen(kb, endpos);
	}
}

void kbuf_add(kbuf_s *kb, const void *data, size_t len)
{
	kbuf_grow(kb, len);
	memcpy(kb->buf + kb->len, data, len);
	kbuf_setlen(kb, kb->len + len);
}

void kbuf_add8(kbuf_s *kb, unsigned char data)
{
	kbuf_add(kb, &data, 1);
}

void kbuf_add16(kbuf_s *kb, unsigned short data, int swap)
{
	if (swap) {
		data = SWAP_16(data);
	}
	kbuf_add(kb, &data, 2);
}

void kbuf_add32(kbuf_s *kb, unsigned int data, int swap)
{
	if (swap) {
		data = SWAP_32(data);
	}
	kbuf_add(kb, &data, 4);
}

void kbuf_adds(kbuf_s *kb, const char *str)
{
	kbuf_add(kb, str, strlen(str));
}

void kbuf_addf(kbuf_s *kb, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	kbuf_vaddf(kb, fmt, ap);
	va_end(ap);
}

void kbuf_vaddf(kbuf_s *kb, const char *fmt, va_list ap)
{
	int len;
	va_list cp;

	if (!kbuf_avail(kb))
		kbuf_grow(kb, 64);
	va_copy(cp, ap);
	len = vsnprintf(kb->buf + kb->len, kb->alloc - kb->len, fmt, cp);
	va_end(cp);
	if (len < 0) {
		fprintf(stderr, "fatal when vsnprintf\n");
		exit(-1);
	}
	if ((size_t)len > kbuf_avail(kb)) {
		kbuf_grow(kb, len);
		len = vsnprintf(kb->buf + kb->len, kb->alloc - kb->len, fmt, ap);
		if ((size_t)len > kbuf_avail(kb)) {
			fprintf(stderr, "fatal when vsnprintf\n");
			exit(-1);
		}
	}
	kbuf_setlen(kb, kb->len + len);
}

size_t kbuf_fread(kbuf_s *kb, size_t size, FILE *fp, int *err)
{
	size_t bytes;
	int error = 0;

	kbuf_grow(kb, size);

	bytes = fread(kb->buf + kb->len, 1, size, fp);
	if (bytes > 0)
		kbuf_setlen(kb, kb->len + bytes);
	if (bytes < size) {
		if (ferror(fp))
			error = 1;
	}

	if (err)
		*err = error;
	return bytes;
}

void kbuf_dump(kbuf_s *kb, const char *banner, char *dat, int len, int width)
{
	int i, line, offset = 0, blen;
	char cache[2048], *pbuf, *p;

	if (width <= 0)
		width = 16;

	/* 6 => "%04x  "; 3 => "%02x ", 1 => "%c", 2 => " |", 2 => "|\n" */
	blen = 6 + (3 + 1) * width + 2 + 2;
	if ((size_t)blen > sizeof(cache))
		pbuf = (char*)malloc(blen);
	else
		pbuf = cache;

	kbuf_addf(kb, "\n%s\n", banner);
	kbuf_addf(kb, "Data:%p, Length:%d\n", dat, len);

	while (offset < len) {
		p = pbuf;

		p += sprintf(p, "%04x  ", offset);
		line = len - offset;

		if (line > width)
			line = width;

		for (i = 0; i < line; i++) {
			unsigned char c = (unsigned char)dat[i];

			if (i % 2 == 0) {
				p += sprintf(p, "%02X ", c);
			} else {
				p += sprintf(p, "\033[0;33m%02X\033[0m ", c);
			}
			if (i % 4 == 3) {
				p += sprintf(p, " ");
			}
		}
		for (; i < width; i++) {
			p += sprintf(p, "  ");
			if (i % 4 == 3) {
				p += sprintf(p, " ");
			}
		}

		p += sprintf(p, " |");
		for (i = 0; i < line; i++) {
			unsigned char c;

			if ((unsigned char)dat[i] >= 0x20 && (unsigned char)dat[i] < 0x7f)
				c = (unsigned char)dat[i];
			else
				c = (unsigned char)'.';

			if (i % 2 == 0) {
				p += sprintf(p, "%c", c);
			} else {
				p += sprintf(p, "\033[0;33m%c\033[0m", c);
			}
		}
		p += sprintf(p, "|\n");
		kbuf_addf(kb, "%s", pbuf);

		offset += line;
		dat += width;
	}

	if (pbuf != cache)
		free(pbuf);
}

