/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <sdlist.h>

kinline void init_dlist_head(K_dlist_entry *hdr)
{
	hdr->next = hdr->prev = hdr;
}

kinline int is_dlist_empty(K_dlist_entry *hdr)
{
	return (hdr->next == hdr);
}

kinline K_dlist_entry *remove_dlist_head_entry(K_dlist_entry *hdr)
{
	K_dlist_entry *entry;

	entry = hdr->next;
	remove_dlist_entry(hdr->next);

	return entry;
}

kinline K_dlist_entry *remove_dlist_tail_entry(K_dlist_entry *hdr)
{
	K_dlist_entry *entry;

	entry = hdr->prev;
	remove_dlist_entry(hdr->prev);

	return entry;
}

kinline void remove_dlist_entry(K_dlist_entry *entry)
{
	K_dlist_entry *_ex_prev;
	K_dlist_entry *_ex_next;

	_ex_next = entry->next;
	_ex_prev = entry->prev;
	_ex_prev->next = _ex_next;
	_ex_next->prev = _ex_prev;
}

kinline void insert_dlist_tail_entry(K_dlist_entry *hdr,
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

kinline void insert_dlist_head_entry(K_dlist_entry *hdr,
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

kinline void incorporate_dlist(K_dlist_entry *obj_hdr,
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

kinline int inquire_dlist_number(K_dlist_entry *hdr)
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