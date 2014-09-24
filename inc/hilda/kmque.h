/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_MQUE_H__
#define __K_MQUE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <hilda/sysdeps.h>

#include <hilda/sdlist.h>
#include <hilda/xtcool.h>

typedef struct _mentry_s mentry_s;
typedef struct _kmque_s kmque_s;

typedef void (*ME_WORKER)(void *ua, void *ub);
typedef void (*ME_DESTORYER)(void *ua, void *ub);


struct _mentry_s {
	/*
	 * Queue to kmque_s::msg_qhdr or kmque_s::dpc_qhdr for
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

	/* pointer back to kmque_s */
	kmque_s *mque;
};

struct _kmque_s {
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

kmque_s *kmque_new();
int kmque_del(kmque_s *mque);

void kmque_set_quit(kmque_s *mque);

int kmque_peek(kmque_s *mque, mentry_s **retme, int timeout);
int mentry_send(kmque_s *mque, ME_WORKER worker, void *ua, void *ub);
int mentry_post(kmque_s *mque, ME_WORKER worker, ME_DESTORYER destoryer,
		void *ua, void *ub);

int mentry_dpc_add(kmque_s *mque, void (*worker)(void *ua, void *ub),
		void (*destoryer)(void *ua, void *ub), void *ua, void *ub,
		unsigned int wait);
int mentry_dpc_kill(kmque_s *mque, unsigned int dpcid);

int kmque_run(kmque_s *mque);

#ifdef __cplusplus
}
#endif

#endif /* __K_MQUE_H__ */

