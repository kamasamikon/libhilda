/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#include <klog.h>
#include <kmem.h>

#include <xtcool.h>
#include <kmque.h>

static int kmque_cleanup(kmque_t *mque);
static void mentry_do(mentry_t *me);
static void mentry_done(mentry_t *me);

kmque_t *kmque_new()
{
	kmque_t *mque = (kmque_t*)kmem_alloz(1, kmque_t);

	init_dlist_head(&mque->msg_qhdr);
	mque->msg_que_lck = spl_lck_new();
	mque->msg_new_sem = spl_sema_new(0);
	mque->msg_snt_sem = spl_sema_new(0);

	return mque;
}

int kmque_del(kmque_t *mque)
{
	spl_sema_del(mque->msg_new_sem);
	spl_sema_del(mque->msg_snt_sem);
	spl_lck_del(mque->msg_que_lck);
	kmem_free(mque);

	return 0;
}

void kmque_set_quit(kmque_t *mque)
{
	mque->quit = 1;
	spl_sema_rel(mque->msg_new_sem);
}

static int kmque_cleanup(kmque_t *mque)
{
	mentry_t *me;
	K_dlist_entry *entry;

	spl_lck_get(mque->msg_que_lck);
	entry = mque->msg_qhdr.next;
	while (entry != &mque->msg_qhdr) {
		me = FIELD_TO_STRUCTURE(entry, mentry_t, entry);
		entry = entry->next;

		mentry_done(me);
	}
	spl_lck_rel(mque->msg_que_lck);

	return 0;
}


/**
 * \brief Return if have, or else wait till timeout
 *
 * \param mque
 * \param retme
 * \param timeout -1 = NOTIMEOUT
 *
 * \return 0:OK, -1:TIMEOUT, -2:NG, -3:Quit
 */
int kmque_peek(kmque_t *mque, mentry_t **retme, int timeout)
{
	mentry_t *me;
	K_dlist_entry *entry;

	if (!mque)
		return -2;

	if (mque->quit) {
		kmque_cleanup(mque);
		return -3;
	}

	/* If has, peek it and return */
	me = NULL;
	spl_lck_get(mque->msg_que_lck);
	if (!is_dlist_empty(&mque->msg_qhdr)) {
		entry = remove_dlist_head_entry(&mque->msg_qhdr);
		me = FIELD_TO_STRUCTURE(entry, mentry_t, entry);
	}
	spl_lck_rel(mque->msg_que_lck);

	/* wait till timeout */
	if (me) {
		*retme = me;
		return 0;
	} else {
		spl_sema_get(mque->msg_new_sem, timeout);
	}

	if (mque->quit) {
		kmque_cleanup(mque);
		return -3;
	}

	/* check again */
	me = NULL;
	spl_lck_get(mque->msg_que_lck);
	if (!is_dlist_empty(&mque->msg_qhdr)) {
		entry = remove_dlist_head_entry(&mque->msg_qhdr);
		me = FIELD_TO_STRUCTURE(entry, mentry_t, entry);
	}
	spl_lck_rel(mque->msg_que_lck);

	if (me)
		*retme = me;
	else
		return -1;
	return 0;
}

int mentry_send(kmque_t *mque, void (*worker)(void *ua, void *ub),
		void (*destoryer)(void *ua, void *ub), void *ua, void *ub)
{
	mentry_t *me;
	SPL_HANDLE caller_thread;

	if (!mque || mque->quit) {
		kerror(("!mque || mque->quit"));
		return -1;
	}

	if (mque->main_task <= 0) {
		kerror(("mentry_send not work when mque not run.\n"));
		return -1;
	}

	caller_thread = spl_thread_current();
	if (mque->main_task == caller_thread) {
		if (worker)
			worker(ua, ub);
		if (destoryer)
			destoryer(ua, ub);
	} else {
		me = kmem_alloz(1, mentry_t);
		init_dlist_head(&me->entry);
		me->worker = worker;
		me->destoryer = destoryer;
		me->ua = ua;
		me->ub = ub;
		me->is_send = ktrue;
		me->mque = mque;

		spl_lck_get(mque->msg_que_lck);
		insert_dlist_tail_entry(&mque->msg_qhdr, &me->entry);
		spl_lck_rel(mque->msg_que_lck);

		spl_sema_rel(mque->msg_new_sem);

		spl_sema_get(mque->msg_snt_sem, -1);
	}
	return 0;
}

int mentry_post(kmque_t *mque, void (*worker)(void *ua, void *ub),
		void (*destoryer)(void *ua, void *ub), void *ua, void *ub)
{
	mentry_t *me;

	if (!mque || mque->quit) {
		kerror(("!mque || mque->quit"));
		return -1;
	}

	me = kmem_alloz(1, mentry_t);
	init_dlist_head(&me->entry);
	me->worker = worker;
	me->destoryer = destoryer;
	me->ua = ua;
	me->ub = ub;
	me->mque = mque;

	spl_lck_get(mque->msg_que_lck);
	insert_dlist_tail_entry(&mque->msg_qhdr, &me->entry);
	spl_lck_rel(mque->msg_que_lck);

	spl_sema_rel(mque->msg_new_sem);
	return 0;
}

static void mentry_do(mentry_t *me)
{
	if (me->worker)
		me->worker(me->ua, me->ub);
}

static void mentry_done(mentry_t *me)
{
	if (me->destoryer)
		me->destoryer(me->ua, me->ub);
	kmem_free(me);
}

static void mentry_do_all(mentry_t *me)
{
	kbool is_send = me->is_send;

	mentry_do(me);
	mentry_done(me);
	if (is_send)
		spl_sema_rel(me->mque->msg_snt_sem);
}

int kmque_run(kmque_t *mque)
{
	int ret;
	mentry_t *me;
	mque->main_task = spl_thread_current();

	while (1) {
		ret = kmque_peek(mque, &me, -1);
		switch (ret) {
		case -3:
			kerror(("kmque_peek: Kuiting detected\n"));
			return 0;
		case 0:
			mentry_do_all(me);
			break;
		case -1:
			kfatal(("kmque_peek: Null return\n"));
			break;
		case -2:
			kerror(("kmque_peek: Null mque para\n"));
			break;
		default:
			kerror(("kmque_peek: Other error\n"));
			break;
		}
	}

	mque->main_task = NULL;
	return 0;
}

#ifdef TEST_kmque
static void worker(void *ua, void *ub)
{
	printf("ua:%s %s", (char*)ua, (char*)ub);
	fflush(stdout);
}

int main(int argc, char *argv[])
{
	kmque_t *mque;

	mque = kmque_new();
	/* mentry_post(mque, worker, NULL, "0", "0"); */
	/* mentry_post(mque, worker, NULL, "1", "1"); */
	/* mentry_post(mque, worker, NULL, "2", "2"); */
	mentry_post(mque, worker, NULL, "3", "3");

	kmque_run(mque);
	kmque_del(mque);

	return 0;
}
#endif

