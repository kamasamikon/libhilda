/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <string.h>

#include <ktypes.h>
#include <kflg.h>
#include <klog.h>
#include <kstr.h>
#include <kmem.h>
#include <kopt.h>

#include <xtcool.h>
#include <strbuf.h>

#include <knode.h>
#include <helper.h>

/* Control Center for knode */
typedef struct _knodecc_t knodecc_t;
struct _knodecc_t {
	knode_t **arr;
	int cnt;
};

static knodecc_t *__g_knodecc = NULL;

/* foreach of down stream nodes */
void knode_data_ready(knode_t *node, void *dat, int len)
{
	int i;
	dstr_node_t *tmp;
	knode_t *tnode;

	for (i = 0; i < node->dstr.cnt; i++) {
		tmp = &node->dstr.arr[i];
		tnode = tmp->node;

		if (!tnode || !tnode->write)
			continue;

		if (kflg_chk_any(tnode->flg, CNFL_PAUSE | CNFL_STOP))
			continue;

		tnode->write(tnode, node, dat, len, tmp->link_dat);
	}
}

/* caller can be NULL or a dummy root for head of chain */
int knode_link_add(knode_t *unode, knode_t *dnode, void *link_dat)
{
	int i;
	dstr_node_t *tmp;

	if (!unode || !dnode)
		return -1;

	for (i = 0; i < unode->dstr.cnt; i++) {
		tmp = &unode->dstr.arr[i];
		if (tmp->node == dnode) {
			kerror("knode_link_add: %s already there\n",
						dnode->name);
			return 0;
		}
	}
	for (i = 0; i < unode->dstr.cnt; i++)
		if (!unode->dstr.arr[i].node) {
			unode->dstr.arr[i].node = dnode;
			unode->dstr.arr[i].link_dat = link_dat;
			return 0;
		}

	ARR_INC(1, unode->dstr.arr, unode->dstr.cnt, dstr_node_t);
	unode->dstr.arr[i].node = dnode;
	unode->dstr.arr[i].link_dat = link_dat;

	return 0;
}

int knode_link_del(knode_t *unode, knode_t *dnode)
{
	int i;
	dstr_node_t *tmp;

	if (!unode || !dnode)
		return -1;

	for (i = 0; i < unode->dstr.cnt; i++) {
		tmp = &unode->dstr.arr[i];
		if (tmp->node == dnode) {
			unode->dstr.arr[i].node = NULL;
			unode->dstr.arr[i].link_dat = NULL;
			return 0;
		}
	}

	return -1;
}

int knode_link_clr(knode_t *node)
{
	int i;

	if (!node)
		return -1;

	for (i = 0; i < node->dstr.cnt; i++) {
		node->dstr.arr[i].node = NULL;
		node->dstr.arr[i].link_dat = NULL;
	}

	return 0;
}

int knode_reg(knode_t *node)
{
	int i;
	knode_t *tmp;
	knodecc_t *cc = __g_knodecc;

	node->dstr.arr = 0;
	node->dstr.cnt = 0;
	node->flg = CNFL_STOP;
	node->attr &= 0xffff;

	for (i = 0; i < cc->cnt; i++) {
		tmp = cc->arr[i];
		if (tmp == node) {
			kerror("%s already\n", node->name);
			return 0;
		}
	}
	for (i = 0; i < cc->cnt; i++)
		if (!cc->arr[i]) {
			cc->arr[i] = node;
			klog("%s success.\n", node->name);
			return 0;
		}

	ARR_INC(1, cc->arr, cc->cnt, knode_t*);
	cc->arr[i] = node;
	klog("%s success.\n", node->name);
	return 0;
}

int knode_unreg(knode_t *node)
{
	int i;
	knode_t *tmp;
	knodecc_t *cc = __g_knodecc;

	for (i = 0; i < cc->cnt; i++) {
		tmp = cc->arr[i];
		if (tmp && tmp == node) {
			klog("%s OK.\n", node->name);
			return 0;
		}
	}

	kerror("%s not there.\n", node->name);
	return -1;
}

int knode_link(knode_t *node)
{
	int i;
	knode_t *dstr;
	knodecc_t *cc = __g_knodecc;
	void *link_dat;

	/* setup the chain */
	for (i = 0; i < cc->cnt; i++) {
		dstr = cc->arr[i];
		if (dstr && node && (dstr != node) && dstr->link_query)
			if (kflg_chk_bit(dstr->attr, CNAT_INPUT))
				if (!dstr->link_query(node, dstr, &link_dat))
					knode_link_add(node, dstr, link_dat);

		if (!node)
			knode_link(dstr);
	}

	return 0;
}

/**
 * \brief Find knode by given name.
 * Name can be reguler name for index name #ddd,
 * e.g #1 or #-2 etc.
 */
knode_t *knode_find(const char *name)
{
	int i, index;
	knode_t *tmp;
	knodecc_t *cc = __g_knodecc;

	if (!name || !cc->cnt)
		return NULL;

	if (name[0] == '#') {
		if (kstr_toint(name + 1, &index))
			return NULL;
		return cc->arr[index % cc->cnt];
	} else
		for (i = 0; i < cc->cnt; i++) {
			tmp = cc->arr[i];
			if (tmp && !strcmp(tmp->name, name))
				return tmp;
		}
	return NULL;
}

static int knode_foreach(knode_FOREACH foreach, void *ua, void *ub)
{
	int i;
	knode_t *tmp;
	knodecc_t *cc = __g_knodecc;

	for (i = 0; i < cc->cnt; i++) {
		tmp = cc->arr[i];
		if (tmp)
			foreach(tmp, ua, ub);
	}

	return 0;
}

static int do_dump(knode_t *node, void *ua, void *ub)
{
	int i;
	struct strbuf *sb = (struct strbuf*)ua;

	strbuf_addf(sb, "\r\nT:%08x A:%08x F:%08x write:%p name:%s\r\n",
			node->type, node->attr, node->flg, node->write,
			node->name);

	for (i = 0; i < node->dstr.cnt; i++)
		strbuf_addf(sb, "\t%02d: %s\r\n", i,
				node->dstr.arr[i].node->name);

	return 0;
}

static int og_knode_diag_dump(void *opt, void *pa, void *pb)
{
	struct strbuf sb;

	strbuf_init(&sb, 4096);
	knode_foreach(do_dump, (void*)&sb, NULL);
	kopt_set_cur_str(opt, sb.buf);
	strbuf_release(&sb);

	return EC_OK;
}

static int os_knode_cmd(int ses, void *opt, void *pa, void *pb)
{
	char *name = kopt_get_new_str(opt);
	char type = (char)(int)kopt_ua(opt);
	knode_t *node;
	int ret;

	if (name && !strcmp(name, "*")) {
		if (type == 'S')
			ret = knode_foreach_start();
		else if (type == 's')
			ret = knode_foreach_stop();
		else if (type == 'p')
			ret = knode_foreach_pause();
		else if (type == 'r')
			ret = knode_foreach_resume();
		else if (type == 'l')
			ret = knode_link(NULL);
		else
			return EC_OK;
		return ret;
	}

	node = knode_find(name);
	if (!node)
		return EC_NG;

	if (type == 'S')
		ret = knode_call_start(node);
	else if (type == 's')
		ret = knode_call_stop(node);
	else if (type == 'p')
		ret = knode_call_pause(node);
	else if (type == 'r')
		ret = knode_call_resume(node);
	else if (type == 'l')
		ret = knode_link(node);
	else
		ret = EC_NG;

	if (ret)
		return EC_NG;
	return EC_OK;
}

void *knode_attach(void *knodecc)
{
	__g_knodecc = (knodecc_t*)knodecc;

	return (void*)__g_knodecc;
}

void *knode_cc()
{
	return (void*)__g_knodecc;
}

static void setup_opt()
{
	kopt_add_s("s:/k/knode/diag/dump", OA_GET, NULL,
			og_knode_diag_dump);

	kopt_add("s:/k/knode/cmd/start", NULL, OA_SET,
			os_knode_cmd, NULL, NULL, 'S', NULL);
	kopt_add("s:/k/knode/cmd/stop", NULL, OA_SET,
			os_knode_cmd, NULL, NULL, 's', NULL);
	kopt_add("s:/k/knode/cmd/pause", NULL, OA_SET,
			os_knode_cmd, NULL, NULL, 'p', NULL);
	kopt_add("s:/k/knode/cmd/resume", NULL, OA_SET,
			os_knode_cmd, NULL, NULL, 'r', NULL);
	kopt_add("s:/k/knode/cmd/link", NULL, OA_SET,
			os_knode_cmd, NULL, NULL, 'l', NULL);
}

void *knode_init(int argc, char *argv[])
{
	if (__g_knodecc)
		return (void*)__g_knodecc;

	__g_knodecc = (knodecc_t*)kmem_alloz(1, knodecc_t);
	setup_opt();
	return (void*)__g_knodecc;
}

int knode_final()
{
	if (!__g_knodecc)
		return 0;
	kmem_free_s(__g_knodecc->arr);
	kmem_free_sz(__g_knodecc);
	return 0;
}

int knode_call_start(knode_t *node)
{
	int ret = 0;

	if (node->start)
		ret = node->start(node);

	if (!ret) {
		kflg_clr(node->flg, CNFL_STOP);
		kflg_clr(node->flg, CNFL_PAUSE);
	}
	return ret;
}
static int do_start(knode_t *node, void *ua, void *ub)
{
	return knode_call_start(node);
}
int knode_foreach_start()
{
	knode_foreach(do_start, NULL, NULL);
	return 0;
}

int knode_call_stop(knode_t *node)
{
	int ret = 0;

	if (node->stop)
		ret = node->stop(node);

	if (!ret) {
		kflg_set(node->flg, CNFL_STOP);
		kflg_clr(node->flg, CNFL_PAUSE);
	}
	return ret;
}
static int do_stop(knode_t *node, void *ua, void *ub)
{
	return knode_call_stop(node);
}
int knode_foreach_stop()
{
	knode_foreach(do_stop, NULL, NULL);
	return 0;
}

int knode_call_pause(knode_t *node)
{
	int ret = 0;

	if (node->pause)
		ret = node->pause(node);

	if (!ret) {
		kflg_clr(node->flg, CNFL_STOP);
		kflg_set(node->flg, CNFL_PAUSE);
	}
	return ret;
}
static int do_pause(knode_t *node, void *ua, void *ub)
{
	return knode_call_pause(node);
}
int knode_foreach_pause()
{
	knode_foreach(do_pause, NULL, NULL);
	return 0;
}

int knode_call_resume(knode_t *node)
{
	int ret = 0;

	if (node->resume)
		ret = node->resume(node);

	if (!ret) {
		kflg_clr(node->flg, CNFL_STOP);
		kflg_clr(node->flg, CNFL_PAUSE);
	}
	return ret;
}
static int do_resume(knode_t *node, void *ua, void *ub)
{
	return knode_call_resume(node);
}
int knode_foreach_resume()
{
	knode_foreach(do_resume, NULL, NULL);
	return 0;
}

