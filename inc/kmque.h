/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_MQUE_H__
#define __K_MQUE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sysdeps.h>

#include <sdlist.h>
#include <xtcool.h>

typedef struct _mentry_t mentry_t;
typedef struct _kmque_t kmque_t;

struct _mentry_t {
	K_dlist_entry entry;

	void (*worker)(void *ua, void *ub);
	void (*destoryer)(void *ua, void *ub);

	void *ua, *ub;

	kbool is_send;
	kmque_t *mque;
};

struct _kmque_t {
	K_dlist_entry msg_qhdr;
	SPL_HANDLE msg_que_lck;

	SPL_HANDLE msg_new_sem;
	SPL_HANDLE msg_snt_sem;

	SPL_HANDLE main_task;

	int quit;
};

kmque_t *kmque_new();
int kmque_del(kmque_t *mque);

void kmque_set_quit(kmque_t *mque);
int kmque_peek(kmque_t *mque, mentry_t **retme, int timeout);
int mentry_send(kmque_t *mque, void (*worker)(void *ua, void *ub),
		void (*destoryer)(void *ua, void *ub), void *ua, void *ub);
int mentry_post(kmque_t *mque, void (*worker)(void *ua, void *ub),
		void (*destoryer)(void *ua, void *ub), void *ua, void *ub);
int kmque_run(kmque_t *mque);

#ifdef __cplusplus
}
#endif

#endif /* __K_MQUE_H__ */

