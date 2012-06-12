#include <stdlib.h>

#include <kmem.h>

#include <sdlist.h>
#include <dataqueue.h>

DQ_HANDLE data_queue_create(void)
{
    K_dlist_entry *hdr = (K_dlist_entry *) kmem_alloc(1, K_dlist_entry);
    init_dlist_head(hdr);
    return (DQ_HANDLE) hdr;
}

int data_queue_destroy(DQ_HANDLE h)
{
    kmem_free(h);
    return 0;
}

int data_queue_is_empty(DQ_HANDLE h)
{
    K_dlist_entry *hdr = (K_dlist_entry *) h;
    return is_dlist_empty(hdr);
}

void data_queue_push_head(DQ_HANDLE h, CLOUD_DATA_LIST * node)
{
    K_dlist_entry *hdr = (K_dlist_entry *) h;
    insert_dlist_head_entry(hdr, &node->list_entry);
}

void data_queue_push_back(DQ_HANDLE h, CLOUD_DATA_LIST * node)
{
    K_dlist_entry *hdr = (K_dlist_entry *) h;
    insert_dlist_tail_entry(hdr, &node->list_entry);
}

void data_queue_remove_entry(DQ_HANDLE h, CLOUD_DATA_LIST * node)
{
    remove_dlist_entry(&node->list_entry);
}

CLOUD_DATA_LIST *data_queue_pop_head(DQ_HANDLE h)
{
    K_dlist_entry *hdr = (K_dlist_entry *) h;
	K_dlist_entry *ent;
    if (is_dlist_empty(hdr))
        return 0;

    ent = remove_dlist_head_entry(hdr);
    return FIELD_TO_STRUCTURE(ent, CLOUD_DATA_LIST, list_entry);
}

CLOUD_DATA_LIST *data_queue_pop_back(DQ_HANDLE h)
{
    K_dlist_entry *hdr = (K_dlist_entry *) h;
	K_dlist_entry *ent;

    if (is_dlist_empty(hdr))
        return 0;

    ent = remove_dlist_tail_entry(hdr);
    return FIELD_TO_STRUCTURE(ent, CLOUD_DATA_LIST, list_entry);
}

CLOUD_DATA_LIST *data_queue_foreach(DQ_HANDLE h, DQ_FOREACH_CBK foreach_fun, void *args)
{
    K_dlist_entry *hdr = (K_dlist_entry *) h;
    K_dlist_entry *ent;
    CLOUD_DATA_LIST *cdl;

    ent = hdr->next;
    while (ent != hdr)
    {
        cdl = FIELD_TO_STRUCTURE(ent, CLOUD_DATA_LIST, list_entry);
        if (-1 == foreach_fun(cdl, args))
            return cdl;
        ent = ent->next;
    }
    return 0;
}

void data_queue_clear(DQ_HANDLE h, DQ_CLEAR_NODE_CBK clear_fun)
{
    CLOUD_DATA_LIST *data_node = NULL;

    while (!data_queue_is_empty(h))
    {
        data_node = data_queue_pop_head(h);
        clear_fun(&data_node);
    }
}

int data_queue_size(DQ_HANDLE h)
{
    K_dlist_entry *hdr = (K_dlist_entry *) h;
    return inquire_dlist_number(hdr);
}

