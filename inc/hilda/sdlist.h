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

kinline void kdlist_init_head(K_dlist_entry *hdr);
kinline int kdlist_is_empty(K_dlist_entry *hdr);

kinline K_dlist_entry *kdlist_remove_head_entry(K_dlist_entry *hdr);
kinline K_dlist_entry *kdlist_remove_tail_entry(K_dlist_entry *hdr);

kinline void kdlist_remove_entry(K_dlist_entry *entry);
kinline void kdlist_insert_tail_entry(K_dlist_entry *hdr, K_dlist_entry *entry);
kinline void kdlist_insert_head_entry(K_dlist_entry *hdr, K_dlist_entry *entry);
kinline int kdlist_length(K_dlist_entry *hdr);

kinline void kdlist_join(K_dlist_entry *obj_hdr, K_dlist_entry *old_hdr);

#ifdef __cplusplus
}
#endif

#endif /* __SDLIST_H__ */

