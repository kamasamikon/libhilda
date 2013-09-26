#include <errno.h>
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
		kerror("BUG: strbuf_setlen() beyond buffer");
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

char *strbuf_detach(struct strbuf *sb, size_t *sz)
{
	char *res = sb->alloc ? sb->buf : NULL;
	if (sz)
		*sz = sb->len;
	strbuf_init(sb, 0);
	return res;
}

void strbuf_attach(struct strbuf *sb, void *buf, size_t len, size_t alloc)
{
	strbuf_release(sb);
	sb->buf   = buf;
	sb->len   = len;
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
		kerror("BUG: your vsnprintf is broken (returned %d)", len);
		exit(-1);
	}
	if (len > strbuf_avail(sb)) {
		strbuf_grow(sb, len);
		len = vsnprintf(sb->buf + sb->len, sb->alloc - sb->len, fmt, ap);
		if (len > strbuf_avail(sb)) {
			kerror("BUG: your vsnprintf is broken (insatiable)");
			exit(-1);
		}
	}
	strbuf_setlen(sb, sb->len + len);
}

size_t strbuf_fread(struct strbuf *sb, size_t size, FILE *f)
{
	size_t res;
	size_t oldalloc = sb->alloc;

	strbuf_grow(sb, size);
	res = fread(sb->buf + sb->len, 1, size, f);
	if (res > 0)
		strbuf_setlen(sb, sb->len + res);
	else if (oldalloc == 0)
		strbuf_release(sb);
	return res;
}

