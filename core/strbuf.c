#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>

#include <hilda/klog.h>
#include <hilda/strbuf.h>

/*
 * Used as the default ->buf value, so that people can always assume
 * buf is non NULL and ->buf is NUL terminated even for a freshly
 * initialized strbuf.
 */
char strbuf_slopbuf[1];


/*----- strbuf size related -----*/
static size_t strbuf_avail(const struct strbuf *sb)
{
	return sb->alloc ? sb->alloc - sb->len - 1 : 0;
}

void strbuf_setlen(struct strbuf *sb, size_t len)
{
	if (len > (sb->alloc ? sb->alloc - 1 : 0)) {
		fprintf(stderr, "fatal! strbuf_setlen: len = %lu\n", (unsigned long)len);
		exit(-1);
	}
	sb->len = len;
	sb->buf[len] = '\0';
}

void strbuf_init(struct strbuf *sb, size_t hint)
{
	sb->alloc = sb->len = 0;
	sb->buf = strbuf_slopbuf;
	if (hint)
		strbuf_grow(sb, hint);
}

void strbuf_release(struct strbuf *sb)
{
	if (sb->alloc) {
		free(sb->buf);
		strbuf_init(sb, 0);
	}
}

char *strbuf_detach(struct strbuf *sb, size_t *size)
{
	char *res = sb->alloc ? sb->buf : NULL;

	if (size)
		*size = sb->len;
	strbuf_init(sb, 0);
	return res;
}

void strbuf_attach(struct strbuf *sb, void *buf, size_t len, size_t alloc)
{
	strbuf_release(sb);
	sb->buf = buf;
	sb->len = len;
	sb->alloc = alloc;
	strbuf_grow(sb, 0);
	sb->buf[sb->len] = '\0';
}

void strbuf_grow(struct strbuf *sb, size_t extra)
{
	int new_buf = !sb->alloc;

	if (new_buf)
		sb->buf = NULL;
	ALLOC_GROW(sb->buf, sb->len + extra + 1, sb->alloc);
	if (new_buf)
		sb->buf[0] = '\0';
}

void strbuf_add(struct strbuf *sb, const void *data, size_t len)
{
	strbuf_grow(sb, len);
	memcpy(sb->buf + sb->len, data, len);
	strbuf_setlen(sb, sb->len + len);
}

void strbuf_addf(struct strbuf *sb, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	strbuf_vaddf(sb, fmt, ap);
	va_end(ap);
}

void strbuf_vaddf(struct strbuf *sb, const char *fmt, va_list ap)
{
	int len;
	va_list cp;

	if (!strbuf_avail(sb))
		strbuf_grow(sb, 64);
	va_copy(cp, ap);
	len = vsnprintf(sb->buf + sb->len, sb->alloc - sb->len, fmt, cp);
	va_end(cp);
	if (len < 0) {
		fprintf(stderr, "fatal when vsnprintf\n");
		exit(-1);
	}
	if ((size_t)len > strbuf_avail(sb)) {
		strbuf_grow(sb, len);
		len = vsnprintf(sb->buf + sb->len, sb->alloc - sb->len, fmt, ap);
		if ((size_t)len > strbuf_avail(sb)) {
			fprintf(stderr, "fatal when vsnprintf\n");
			exit(-1);
		}
	}
	strbuf_setlen(sb, sb->len + len);
}

size_t strbuf_fread(struct strbuf *sb, size_t size, FILE *fp, int *err)
{
	size_t bytes;

	*err = 0;

	strbuf_grow(sb, size);

	bytes = fread(sb->buf + sb->len, 1, size, fp);
	if (bytes > 0)
		strbuf_setlen(sb, sb->len + bytes);
	if (bytes < size) {
		if (ferror(fp))
			*err = 1;
	}

	return bytes;
}

void strbuf_dump(struct strbuf *sb, const char *banner, char *dat, int len, int width)
{
	int i, line, offset = 0, blen;
	char cache[2048], *pbuf, *p;

	if (width <= 0)
		width = 16;

	/* 6 => "%04x  "; 3 => "%02x ", 1 => "%c", 2 => " |", 2 => "|\n" */
	blen = 6 + (3 + 1) * width + 2 + 2;
	if ((size_t)blen > sizeof(cache))
		pbuf = (char*)malloc(blen * sizeof(char));
	else
		pbuf = cache;

	strbuf_addf(sb, "\n%s\n", banner);
	strbuf_addf(sb, "Data:%p, Length:%d\n", dat, len);

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
		strbuf_addf(sb, "%s", pbuf);

		offset += line;
		dat += width;
	}

	if (pbuf != cache)
		free(pbuf);
}

