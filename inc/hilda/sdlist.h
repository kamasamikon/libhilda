/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_SDLIST_H__
#define __K_SDLIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <hilda/sysdeps.h>

typedef struct _K_dlist_entry {
	struct _K_dlist_entry *next;
	struct _K_dlist_entry *prev;
} K_dlist_entry;

#define FIELD_TO_STRUCTURE(address, type, field) \
	((type *)((char *)(address) - (char *)(&((type *)0)->field)))

static kinline void kdlist_init_head(K_dlist_entry *hdr)
{
	hdr->next = hdr->prev = hdr;
}

static kinline int kdlist_is_empty(K_dlist_entry *hdr)
{
	return (hdr->next == hdr);
}

static kinline void kdlist_remove_entry(K_dlist_entry *entry)
{
	K_dlist_entry *_ex_prev;
	K_dlist_entry *_ex_next;

	_ex_next = entry->next;
	_ex_prev = entry->prev;
	_ex_prev->next = _ex_next;
	_ex_next->prev = _ex_prev;
}

static kinline K_dlist_entry *kdlist_remove_head_entry(K_dlist_entry *hdr)
{
	K_dlist_entry *entry;

	entry = hdr->next;
	kdlist_remove_entry(hdr->next);

	return entry;
}

static kinline K_dlist_entry *kdlist_remove_tail_entry(K_dlist_entry *hdr)
{
	K_dlist_entry *entry;

	entry = hdr->prev;
	kdlist_remove_entry(hdr->prev);

	return entry;
}

static kinline void kdlist_insert_tail_entry(K_dlist_entry *hdr,
		K_dlist_entry *entry)
{
	K_dlist_entry *_ex_prev;
	K_dlist_entry *_ex_list_head;

	_ex_list_head = hdr;
	_ex_prev = _ex_list_head->prev;
	entry->next = _ex_list_head;
	entry->prev = _ex_prev;
	_ex_prev->next = entry;
	_ex_list_head->prev = entry;
}

static kinline void kdlist_insert_head_entry(K_dlist_entry *hdr,
		K_dlist_entry *entry)
{
	K_dlist_entry *_ex_next;
	K_dlist_entry *_ex_list_head;

	_ex_list_head = hdr;
	_ex_next = _ex_list_head->next;
	entry->next = _ex_next;
	entry->prev = _ex_list_head;
	_ex_next->prev = entry;
	_ex_list_head->next = entry;
}

static kinline void kdlist_concat(K_dlist_entry *obj_hdr,
		K_dlist_entry *old_hdr)
{
	K_dlist_entry *new_head;
	K_dlist_entry *new_tail;

	K_dlist_entry *old_tail;
	K_dlist_entry *old_head;

	new_head = obj_hdr;
	new_tail = old_hdr->prev;

	old_tail = obj_hdr->prev;
	old_head = old_hdr;

	old_tail->next = old_head;
	old_head->prev = old_tail;

	new_head->prev = new_tail;
	new_tail->next = new_head;
}

static kinline int kdlist_length(K_dlist_entry *hdr)
{
	int number = 0;
	K_dlist_entry *entry;

	entry = hdr->next;
	while (entry != hdr) {
		number++;
		entry = entry->next;
	}
	return number;
}

#ifdef __cplusplus
}
#endif

#endif /* __K_SDLIST_H__ */

