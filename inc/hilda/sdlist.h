/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __SDLIST_H__
#define __SDLIST_H__

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

kinline void init_dlist_head(K_dlist_entry *hdr);
kinline int is_dlist_empty(K_dlist_entry *hdr);

kinline K_dlist_entry *remove_dlist_head_entry(K_dlist_entry *hdr);
kinline K_dlist_entry *remove_dlist_tail_entry(K_dlist_entry *hdr);

kinline void remove_dlist_entry(K_dlist_entry *entry);
kinline void insert_dlist_tail_entry(K_dlist_entry *hdr, K_dlist_entry *entry);
kinline void insert_dlist_head_entry(K_dlist_entry *hdr, K_dlist_entry *entry);
kinline int inquire_dlist_number(K_dlist_entry *hdr);

kinline void incorporate_dlist(K_dlist_entry *obj_hdr, K_dlist_entry *old_hdr);

#ifdef __cplusplus
}
#endif

#endif /* __SDLIST_H__ */

