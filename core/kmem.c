/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <hilda/kmem.h>
#include <hilda/klog.h>
#include <hilda/xtcool.h>
#include <hilda/klog.h>

#ifdef MEM_STAT
static kuint __g_mempeak = 0;
static kuint __g_memusage = 0;
static kuint __g_alloc_cnt = 0;
static kuint __g_free_cnt = 0;

static kuint __g_dump_loop = (kuint)-1;
#endif

#ifdef MEM_STAT
static void mem_set_size(void *usrptr, kuint size)
{
	kuint *p = (kuint*)usrptr;

	p--;
	*p = size;
}

static kuint mem_get_size(void *usrptr)
{
	kuint *p = (kuint*)usrptr;

	p--;
	return *p;
}

static void *mem_usr_to_raw(void *usrptr)
{
	kuint *p = (kuint*)usrptr;

	p--;
	return (void*)p;
}

static void *mem_raw_to_usr(void *rawptr)
{
	kuint *p = (kuint*)rawptr;

	p++;
	return (void*)p;
}
#endif

kint kmem_init(kuint a_flg)
{
	return 0;
}

kint kmem_final(kvoid)
{
	return 0;
}

kvoid *kmem_get(kuint size)
{
#ifdef MEM_STAT
	kuint *rawptr = malloc(size + sizeof(kuint));
	kvoid *usrptr = mem_raw_to_usr(rawptr);

	mem_set_size(usrptr, size);

	__g_memusage += size;
	__g_alloc_cnt++;

	if (__g_memusage > __g_mempeak)
		__g_mempeak = __g_memusage;
#else
	kvoid *usrptr = malloc(size);
#endif
	return usrptr;
}

kvoid *kmem_get_z(kuint size)
{
	kvoid *usrptr = kmem_get(size);

	if (usrptr)
		memset(usrptr, 0, size);
	return usrptr;
}

kvoid *kmem_reget(kvoid *usrptr, kuint size)
{
#ifdef MEM_STAT
	kvoid *old_usrptr = usrptr, *new_usrptr;
	kuint old_size;

	if (!old_usrptr)
		return kmem_get(size);

	old_size = mem_get_size(old_usrptr);

	new_usrptr = kmem_get(size);
	if (size > old_size)
		kmem_move(new_usrptr, old_usrptr, old_size);
	else
		kmem_move(new_usrptr, old_usrptr, size);
	kmem_rel(old_usrptr);
#else
	kvoid *new_usrptr;

	new_usrptr = realloc(usrptr, size);
#endif
	return new_usrptr;
}

kvoid kmem_rel(kvoid *usrptr)
{
#ifdef MEM_STAT
	kvoid *old_usrptr = usrptr;
	kvoid *old_rawptr = mem_usr_to_raw(old_usrptr);
	kuint old_size = mem_get_size(old_usrptr);

	__g_free_cnt++;
	__g_memusage -= old_size;

	if (__g_dump_loop == (kuint)-1) {
		char *env = getenv("KMEM_DUMP_LOOP");
		if (env)
			__g_dump_loop = strtoul(env, NULL, 10);

		if (__g_dump_loop == 0)
			__g_dump_loop = 1;
		else if (__g_dump_loop == (kuint)-1)
			__g_dump_loop = (kuint)-2;
	}

	if (!(__g_free_cnt % __g_dump_loop))
		klogs("\n\n#### MEMORY ####: ac:%d, fc:%d, now:%d\n\n\n",
				__g_alloc_cnt, __g_free_cnt, __g_memusage);

	free(old_rawptr);
#else
	free(usrptr);
#endif
}

kint kmem_rpt(kint *alloc_cnt, kint *free_cnt, kint *memusage, kint *mempeak)
{
#ifdef MEM_STAT
	if (alloc_cnt)
		*alloc_cnt = __g_alloc_cnt;
	if (free_cnt)
		*free_cnt = __g_free_cnt;
	if (memusage)
		*memusage = __g_memusage;
	if (mempeak)
		*mempeak = __g_mempeak;
	return 0;
#else
	if (alloc_cnt)
		*alloc_cnt = -1;
	if (free_cnt)
		*free_cnt = -1;
	if (memusage)
		*memusage = -1;
	if (mempeak)
		*mempeak = -1;
	return -1;
#endif
}

kvoid* kmem_move(kvoid *to, kvoid *fr, kuint num)
{
	kchar *s1, *s2;

	s1 = to;
	s2 = fr;
	if ((s2 < s1) && (s2 + num > s1))
		goto back;

	while (num > 0) {
		*s1++ = *s2++;
		num--;
	}
	return to;

back:
	s1 += num;
	s2 += num;
	while (num > 0) {
		*--s1 = *--s2;
		num--;
	}
	return to;
}

void kmem_dump(const char *banner, char *dat, int len, int width)
{
	int i, line, offset = 0, blen;
	char cache[2048], *pbuf, *p;

	if (width <= 0)
		width = 16;

	/* 6 => "%04x  "; 3 => "%02x ", 1 => "%c", 2 => " |", 2 => "|\n" */
	blen = 6 + (3 + 1) * width + 2 + 2;
	if (blen > sizeof(cache))
		pbuf = (char*)kmem_alloc(blen, char);
	else
		pbuf = cache;

	klogs("\n%s\n", banner);
	klogs("Data:%p, Length:%d\n", dat, len);

	while (offset < len) {
		p = pbuf;

		p += sprintf(p, "%04x  ", offset);
		line = len - offset;

		if (line > width)
			line = width;

		for (i = 0; i < line; i++)
			p += sprintf(p, "%02x ", (kuchar)dat[i]);
		for (; i < width; i++)
			p += sprintf(p, "   ");

		p += sprintf(p, " |");
		for (i = 0; i < line; i++)
			if ((kuchar)dat[i] >= 0x20 && (kuchar)dat[i] < 0x7f)
				p += sprintf(p, "%c",  (kuchar)dat[i]);
			else
				p += sprintf(p, ".");
		p += sprintf(p, "|\n");
		klogs("%s", pbuf);

		offset += line;
		dat += width;
	}

	if (pbuf != cache)
		kmem_free(pbuf);
}

