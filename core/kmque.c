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
	init_dlist_head(&mque->dpc_qhdr);
	mque->qhdr_lck = spl_lck_new();

	mque->msg_new_sem = spl_sema_new(0);
	mque->msg_snt_sem = spl_sema_new(0);

	return mque;
}

int kmque_del(kmque_t *mque)
{
	spl_sema_del(mque->msg_new_sem);
	spl_sema_del(mque->msg_snt_sem);
	spl_lck_del(mque->qhdr_lck);
	kmem_free(mque);

	return 0;
}

void kmque_set_quit(kmque_t *mque)
{
	mque->quit = 1;
	spl_sema_rel(mque->msg_new_sem);
	/* FIXME: What if when a send waiting? rel(msg_snt_sem)? */
}

static int kmque_cleanup(kmque_t *mque)
{
	mentry_t *me;
	K_dlist_entry *entry;

	spl_lck_get(mque->qhdr_lck);
	entry = mque->msg_qhdr.next;
	while (entry != &mque->msg_qhdr) {
		me = FIELD_TO_STRUCTURE(entry, mentry_t, entry);
		entry = entry->next;
		mentry_done(me);
	}
	entry = mque->dpc_qhdr.next;
	while (entry != &mque->dpc_qhdr) {
		me = FIELD_TO_STRUCTURE(entry, mentry_t, entry);
		entry = entry->next;
		mentry_done(me);
	}
	spl_lck_rel(mque->qhdr_lck);

	return 0;
}

static mentry_t *get_ready_me(kmque_t *mque)
{
	mentry_t *me = NULL;
	K_dlist_entry *entry;
	unsigned int now = spl_get_ticks();

	spl_lck_get(mque->qhdr_lck);
	if (!is_dlist_empty(&mque->msg_qhdr)) {
		entry = remove_dlist_head_entry(&mque->msg_qhdr);
		me = FIELD_TO_STRUCTURE(entry, mentry_t, entry);
	} else if (!is_dlist_empty(&mque->dpc_qhdr)) {
		me = FIELD_TO_STRUCTURE(mque->dpc_qhdr.next, mentry_t, entry);
		if (me->dpc.due_time < now)
			remove_dlist_head_entry(&mque->dpc_qhdr);
		else
			me = NULL;
	}
	spl_lck_rel(mque->qhdr_lck);

	return me;
}

static int calc_wait_timeout(kmque_t *mque, int timeout)
{
	mentry_t *me;
	unsigned int new_timeout, first_timeout, now = spl_get_ticks();

	if (timeout < 0)
		new_timeout = (unsigned int)-1;
	else
		new_timeout = timeout;

	spl_lck_get(mque->qhdr_lck);
	if (!is_dlist_empty(&mque->dpc_qhdr)) {
		me = FIELD_TO_STRUCTURE(mque->dpc_qhdr.next, mentry_t, entry);
		if (me->dpc.due_time > now) {
			first_timeout = me->dpc.due_time - now;
			if (new_timeout > first_timeout)
				new_timeout = first_timeout;
		} else
			new_timeout = 0;
	}
	spl_lck_rel(mque->qhdr_lck);

	return (int)new_timeout;
}

/**
 * \brief Return if exist pending message, or else wait till timeout
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
	int new_timeout;

	if (!mque)
		return -2;
	if (mque->quit)
		return -3;

	me = get_ready_me(mque);
	if (me) {
		*retme = me;
		return 0;
	}

	new_timeout = calc_wait_timeout(mque, timeout);
	if (new_timeout)
		spl_sema_get(mque->msg_new_sem, new_timeout);
	if (mque->quit)
		return -3;

	me = get_ready_me(mque);
	if (!me)
		return -1;

	*retme = me;
	return 0;
}

int mentry_send(kmque_t *mque, ME_WORKER worker, void *ua, void *ub)
{
	mentry_t *me;
	SPL_HANDLE caller_thread;

	if (!mque || mque->quit) {
		kerror("!mque || mque->quit");
		return -1;
	}

	if (mque->main_task <= 0) {
		kerror("mentry_send not work when mque not run.\n");
		return -1;
	}

	caller_thread = spl_thread_current();
	if (mque->main_task == caller_thread) {
		if (worker)
			worker(ua, ub);
	} else {
		me = kmem_alloz(1, mentry_t);
		me->worker = worker;
		me->ua = ua;
		me->ub = ub;
		me->is_send = ktrue;
		me->mque = mque;

		spl_lck_get(mque->qhdr_lck);
		insert_dlist_tail_entry(&mque->msg_qhdr, &me->entry);
		spl_lck_rel(mque->qhdr_lck);

		spl_sema_rel(mque->msg_new_sem);

		spl_sema_get(mque->msg_snt_sem, -1);
		if (mque->quit)
			return -1;
	}
	return 0;
}

int mentry_post(kmque_t *mque, ME_WORKER worker, ME_DESTORYER destoryer,
		void *ua, void *ub)
{
	mentry_t *me;

	if (!mque || mque->quit) {
		kerror("!mque || mque->quit");
		return -1;
	}

	me = kmem_alloz(1, mentry_t);
	me->worker = worker;
	me->destoryer = destoryer;
	me->ua = ua;
	me->ub = ub;
	me->mque = mque;

	spl_lck_get(mque->qhdr_lck);
	insert_dlist_tail_entry(&mque->msg_qhdr, &me->entry);
	spl_lck_rel(mque->qhdr_lck);

	spl_sema_rel(mque->msg_new_sem);
	return 0;
}

/*
 * Sort and queue.
 */
/* FIXME: think about timer overflow */
static int insert_dpc_entry(kmque_t *mque, mentry_t *me, unsigned int wait)
{
	mentry_t *tmp;
	K_dlist_entry *entry;
	unsigned int dpcid, me_due_time = spl_get_ticks() + wait;

	spl_lck_get(mque->qhdr_lck);

	dpcid = ++mque->dpc_ref;
	if (dpcid == 0)
		dpcid = ++mque->dpc_ref;
	me->dpc.id = dpcid;

	entry = mque->dpc_qhdr.next;
	while (entry != &mque->dpc_qhdr) {
		tmp = FIELD_TO_STRUCTURE(entry, mentry_t, entry);

		if (tmp->dpc.due_time > me_due_time)
			break;

		entry = entry->next;
	}

	me->dpc.due_time = me_due_time;
	insert_dlist_tail_entry(entry, &me->entry);

	spl_lck_rel(mque->qhdr_lck);

	return 0;
}

int mentry_dpc_add(kmque_t *mque, void (*worker)(void *ua, void *ub),
		void (*destoryer)(void *ua, void *ub), void *ua, void *ub,
		unsigned int wait)
{
	mentry_t *me;

	if (!mque || mque->quit) {
		kerror("!mque || mque->quit");
		return 0;
	}

	me = kmem_alloz(1, mentry_t);
	me->worker = worker;
	me->destoryer = destoryer;
	me->ua = ua;
	me->ub = ub;
	me->mque = mque;

	insert_dpc_entry(mque, me, wait);
	spl_sema_rel(mque->msg_new_sem);

	return me->dpc.id;
}

int mentry_dpc_kill(kmque_t *mque, unsigned int dpcid)
{
	mentry_t *me;
	K_dlist_entry *entry;
	int found = 0;

	if (!mque || mque->quit)
		return -1;

	/* XXX: Only check dpc queue. */
	spl_lck_get(mque->qhdr_lck);
	entry = mque->dpc_qhdr.next;
	while (entry != &mque->dpc_qhdr) {
		me = FIELD_TO_STRUCTURE(entry, mentry_t, entry);
		entry = entry->next;
		if (me->dpc.id != dpcid)
			continue;

		remove_dlist_entry(&me->entry);
		mentry_done(me);
		break;
	}
	spl_lck_rel(mque->qhdr_lck);

	return found;
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
			kerror("kmque_peek: Kuiting detected\n");
			return 0;
		case 0:
			mentry_do_all(me);
			break;
		case -1:
			kfatal("kmque_peek: Null return\n");
			break;
		case -2:
			kerror("kmque_peek: Null mque para\n");
			break;
		default:
			kerror("kmque_peek: Other error\n");
			break;
		}
	}

	mque->main_task = NULL;
	return 0;
}

#ifdef TEST_KMQUE
kmque_t *__g_mque;

static void worker(void *ua, void *ub)
{
	printf("ua:%s %s: %u\n", (char*)ua, (char*)ub, spl_get_ticks());
}
static void xworker(void *ua, void *ub)
{
	printf("ua:%s %s: %u\n", (char*)ua, (char*)ub, spl_get_ticks());
	mentry_dpc_add(__g_mque, xworker, NULL, "dpc:loop", "call worker", 1500);
}
static void call_sender(void *ua, void *ub)
{
	int dpc;

	printf("ua:%s %s: %u\n", (char*)ua, (char*)ub, spl_get_ticks());
	mentry_send(__g_mque, worker, "send:380", "call worker");
	mentry_dpc_add(__g_mque, worker, NULL, "dpc:2", "call worker", 4000);
	mentry_dpc_add(__g_mque, worker, NULL, "dpc:3", "call worker", 4000);
	mentry_dpc_add(__g_mque, worker, NULL, "dpc:6", "call worker", 5000);
	dpc = mentry_dpc_add(__g_mque, worker, NULL, "dpc:4", "call worker", 4000);
	mentry_dpc_add(__g_mque, worker, NULL, "dpc:5", "call worker", 4000);
	mentry_dpc_add(__g_mque, worker, NULL, "dpc:8", "call worker", 7000);
	mentry_dpc_add(__g_mque, worker, NULL, "dpc:7", "call worker", 5000);
	mentry_dpc_add(__g_mque, xworker, NULL, "dpc:callloop", "call worker", 5000);
	mentry_dpc_kill(__g_mque, dpc);
}

int main(int argc, char *argv[])
{
	klog_init(0, argc, argv);
	__g_mque = kmque_new();

	/* mentry_post(__g_mque, worker, NULL, "0", "0"); */
	/* mentry_post(__g_mque, worker, NULL, "1", "1"); */
	/* mentry_post(__g_mque, worker, NULL, "2", "2"); */
	mentry_post(__g_mque, worker, NULL, "post:388", "call worker");
	mentry_post(__g_mque, call_sender, NULL, "post:389", "call call_sender");
	mentry_post(__g_mque, worker, NULL, "post:390", "call worker");
	mentry_dpc_add(__g_mque, worker, NULL, "dpc:1", "call worker", 4000);

	kmque_run(__g_mque);
	kmque_del(__g_mque);

	return 0;
}
#endif

