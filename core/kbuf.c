/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>

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
	size_t newlen = offset + len;
	if (newlen > kb->alloc) {
		kbuf_grow(kb, newlen - kb->alloc);
	}
	memcpy(kb->buf + offset, data, len);
	kbuf_setlen(kb, newlen);
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

void kbuf_add16(kbuf_s *kb, unsigned short data)
{
	kbuf_add(kb, &data, 2);
}

void kbuf_add32(kbuf_s *kb, unsigned int data)
{
	kbuf_add(kb, &data, 4);
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

		for (i = 0; i < line; i++)
			p += sprintf(p, "%02x ", (unsigned char)dat[i]);
		for (; i < width; i++)
			p += sprintf(p, "   ");

		p += sprintf(p, " |");
		for (i = 0; i < line; i++)
			if ((unsigned char)dat[i] >= 0x20 && (unsigned char)dat[i] < 0x7f)
				p += sprintf(p, "%c",  (unsigned char)dat[i]);
			else
				p += sprintf(p, ".");
		p += sprintf(p, "|\n");
		kbuf_addf(kb, "%s", pbuf);

		offset += line;
		dat += width;
	}

	if (pbuf != cache)
		free(pbuf);
}

