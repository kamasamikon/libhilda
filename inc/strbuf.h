#ifndef STRBUF_H
#define STRBUF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kmem.h>

extern char strbuf_slopbuf[];
struct strbuf {
	size_t alloc;
	size_t len;
	char *buf;
};

#define STRBUF_INIT  { 0, 0, strbuf_slopbuf }

#define alloc_nr(x) (((x)+16)*3/2)

/*
 * Realloc the buffer pointed at by variable 'x' so that it can hold
 * at least 'nr' entries; the number of entries currently allocated
 * is 'alloc', using the standard growing factor alloc_nr() macro.
 *
 * DO NOT USE any expression with side-effect for 'x', 'nr', or 'alloc'.
 */
#define ALLOC_GROW(x, nr, alloc) \
	do { \
		if ((nr) > alloc) { \
			if (alloc_nr(alloc) < (nr)) \
				alloc = (nr); \
			else \
				alloc = alloc_nr(alloc); \
			x = kmem_realloc((x), alloc * sizeof(*(x))); \
		} \
	} while (0)


/*----- strbuf life cycle -----*/
extern void strbuf_init(struct strbuf *, size_t);
extern void strbuf_release(struct strbuf *);
extern char *strbuf_detach(struct strbuf *, size_t *);
extern void strbuf_attach(struct strbuf *, void *, size_t, size_t);

extern void strbuf_grow(struct strbuf *, size_t);

void strbuf_setlen(struct strbuf *sb, size_t len);
#define strbuf_reset(sb)  strbuf_setlen(sb, 0)

extern void strbuf_add(struct strbuf *, const void *, size_t);

extern void strbuf_addf(struct strbuf *sb, const char *fmt, ...);
extern void strbuf_vaddf(struct strbuf *sb, const char *fmt, va_list ap);

extern size_t strbuf_fread(struct strbuf *, size_t, FILE *);

#endif /* STRBUF_H */
