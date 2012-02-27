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

typedef void (*ME_WORKER)(void *ua, void *ub);
typedef void (*ME_DESTORYER)(void *ua, void *ub);


struct _mentry_t {
	/*
	 * Queue to kmque_t::msg_qhdr or kmque_t::dpc_qhdr for
	 * mentry_delay().
	 */
	K_dlist_entry entry;

	/* Delayed Process Call */
	struct {
		unsigned int id;
		unsigned int due_time;
	} dpc;

	ME_WORKER worker;
	ME_DESTORYER destoryer;

	/* Userdata A and B */
	void *ua, *ub;

	/* emit by mentry_send */
	kbool is_send;

	/* pointer back to kmque_t */
	kmque_t *mque;
};

struct _kmque_t {
	/* Queue HeaDeR for queue message */
	K_dlist_entry msg_qhdr;

	/* Queue HeaDeR for Delayed Process Call message */
	K_dlist_entry dpc_qhdr;

	/* Lock for both msg_qhdr and dpc_qhdr */
	SPL_HANDLE qhdr_lck;

	/* event when message added */
	SPL_HANDLE msg_new_sem;
	/* event when mentry_send() done */
	SPL_HANDLE msg_snt_sem;

	SPL_HANDLE main_task;

	int quit;

	unsigned int dpc_ref;
};

kmque_t *kmque_new();
int kmque_del(kmque_t *mque);

void kmque_set_quit(kmque_t *mque);

int kmque_peek(kmque_t *mque, mentry_t **retme, int timeout);
int mentry_send(kmque_t *mque, ME_WORKER worker, void *ua, void *ub);
int mentry_post(kmque_t *mque, ME_WORKER worker, ME_DESTORYER destoryer,
		void *ua, void *ub);

int mentry_dpc_add(kmque_t *mque, void (*worker)(void *ua, void *ub),
		void (*destoryer)(void *ua, void *ub), void *ua, void *ub,
		unsigned int wait);
int mentry_dpc_kill(kmque_t *mque, unsigned int dpcid);

int kmque_run(kmque_t *mque);

#ifdef __cplusplus
}
#endif

#endif /* __K_MQUE_H__ */

