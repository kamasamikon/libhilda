/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <string.h>

#include <hilda/ktypes.h>
#include <hilda/kflg.h>
#include <hilda/klog.h>
#include <hilda/kstr.h>
#include <hilda/kmem.h>
#include <hilda/kopt.h>

#include <hilda/xtcool.h>
#include <hilda/strbuf.h>

#include <hilda/knode.h>
#include <hilda/helper.h>

/* Control Center for knode */
typedef struct _knodecc_s knodecc_s;
struct _knodecc_s {
	knode_s **arr;
	int cnt;
};

static knodecc_s *__g_knodecc = NULL;

/* foreach of down stream nodes */
void knode_push(knode_s *node, void *dat, int len)
{
	int i;
	dstr_node_s *tmp;
	knode_s *tnode;

	for (i = 0; i < node->dstr.cnt; i++) {
		tmp = &node->dstr.arr[i];
		tnode = tmp->node;

		if (!tnode || !tnode->push)
			continue;

		if (kflg_chk_any(tnode->flg, KNFL_PAUSE | KNFL_STOP))
			continue;

		tnode->push(tnode, node, dat, len, tmp->link_dat);
	}
}

/* caller can be NULL or a dummy root for head of chain */
int knode_link_add(knode_s *unode, knode_s *dnode, void *link_dat)
{
	int i;
	dstr_node_s *tmp;

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

	ARR_INC(1, unode->dstr.arr, unode->dstr.cnt, dstr_node_s);
	unode->dstr.arr[i].node = dnode;
	unode->dstr.arr[i].link_dat = link_dat;

	return 0;
}

int knode_link_del(knode_s *unode, knode_s *dnode)
{
	int i;
	dstr_node_s *tmp;

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

int knode_link_clr(knode_s *node)
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

int knode_reg(knode_s *node)
{
	int i;
	knode_s *tmp;
	knodecc_s *cc = __g_knodecc;

	node->dstr.arr = 0;
	node->dstr.cnt = 0;
	node->flg = KNFL_STOP;
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

	ARR_INC(1, cc->arr, cc->cnt, knode_s*);
	cc->arr[i] = node;
	klog("%s success.\n", node->name);
	return 0;
}

int knode_unreg(knode_s *node)
{
	int i;
	knode_s *tmp;
	knodecc_s *cc = __g_knodecc;

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

static int sort_node(knode_s **arr, int cnt)
{
	int i, j, k, swaps = 0;
	knode_s *tmp_i, *tmp_j;
	knode_s **bak = kmem_alloc(cnt, knode_s*);

	int my_pos, ds_pos;

	memcpy(bak, arr, sizeof(knode_s*) * cnt);

	for (i = 0; i < cnt; i++) {
		tmp_i = bak[i];

		ds_pos = 7777777;

		/* 1. Search the left-most dstr node pos */
		for (j = 0; j < cnt; j++) {
			tmp_j = arr[j];

			if (tmp_i == tmp_j) {
				my_pos = j;
				continue;
			}

			for (k = 0; k < tmp_i->dstr.cnt; k++)
				if (tmp_i->dstr.arr[k].node == tmp_j)
					if (ds_pos > j)
						ds_pos = j;
		}

		/* 2. Put me before all the DS */
		if (ds_pos != 7777777 && ds_pos < my_pos) {
			swaps = 1;

			tmp_j = arr[my_pos];
			memmove(&arr[ds_pos + 1], &arr[ds_pos],
					sizeof(knode_s*) * (my_pos - ds_pos));
			arr[ds_pos] = tmp_j;
		}
	}

	kmem_free(bak);

	if (swaps)
		return sort_node(arr, cnt);

	return 0;
}

static int knode_link_internal(knode_s *node)
{
	int i;
	knode_s *dstr;
	knodecc_s *cc = __g_knodecc;
	void *link_dat;

	/* setup the chain */
	for (i = 0; i < cc->cnt; i++) {
		dstr = cc->arr[i];
		if (dstr && node && (dstr != node) && dstr->link_query)
			if (kflg_chk_bit(dstr->attr, KNAT_INPUT))
				if (!dstr->link_query(node, dstr, &link_dat))
					knode_link_add(node, dstr, link_dat);

		if (!node)
			knode_link_internal(dstr);
	}

	return 0;
}


int knode_link(knode_s *node)
{
	knodecc_s *cc = __g_knodecc;

	knode_link_internal(node);
	sort_node(cc->arr, cc->cnt);

	return 0;
}

/**
 * \brief Find knode by given name.
 * Name can be reguler name for index name #ddd,
 * e.g #1 or #-2 etc.
 */
knode_s *knode_find(const char *name)
{
	int i, index;
	knode_s *tmp;
	knodecc_s *cc = __g_knodecc;

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

int knode_foreach(KNODE_FOREACH foreach, void *ua, void *ub)
{
	int i;
	knode_s *tmp;
	knodecc_s *cc = __g_knodecc;

	for (i = 0; i < cc->cnt; i++) {
		tmp = cc->arr[i];
		if (tmp)
			foreach(tmp, ua, ub);
	}

	return 0;
}

static int do_dump(knode_s *node, void *ua, void *ub)
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
	knode_s *node;
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
	__g_knodecc = (knodecc_s*)knodecc;

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

	__g_knodecc = (knodecc_s*)kmem_alloz(1, knodecc_s);
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

int knode_call_start(knode_s *node)
{
	int ret = 0;

	if (!kflg_chk_any(node->flg, KNFL_STOP | KNFL_PAUSE))
		return 0;

	if (node->start)
		ret = node->start(node);

	if (!ret)
		kflg_clr(node->flg, KNFL_STOP | KNFL_PAUSE);

	return ret;
}
int knode_foreach_start()
{
	int i;
	knode_s *tmp;
	knodecc_s *cc = __g_knodecc;

	for (i = cc->cnt; i >= 0; i--) {
		tmp = cc->arr[i];
		if (tmp)
			knode_call_start(tmp);
	}

	return 0;
}

int knode_call_stop(knode_s *node)
{
	int ret = 0;

	if (kflg_chk_any(node->flg, KNFL_STOP))
		return 0;

	if (node->stop)
		ret = node->stop(node);

	if (!ret)
		kflg_set(node->flg, KNFL_STOP);

	return ret;
}
int knode_foreach_stop()
{
	int i;
	knode_s *tmp;
	knodecc_s *cc = __g_knodecc;

	for (i = 0; i < cc->cnt; i++) {
		tmp = cc->arr[i];
		if (tmp)
			knode_call_stop(tmp);
	}

	return 0;
}

int knode_call_pause(knode_s *node)
{
	int ret = 0;

	if (kflg_chk_any(node->flg, KNFL_STOP | KNFL_PAUSE))
		return 0;

	if (node->pause)
		ret = node->pause(node);

	if (!ret)
		kflg_set(node->flg, KNFL_PAUSE);

	return ret;
}
int knode_foreach_pause()
{
	int i;
	knode_s *tmp;
	knodecc_s *cc = __g_knodecc;

	for (i = 0; i < cc->cnt; i++) {
		tmp = cc->arr[i];
		if (tmp)
			knode_call_pause(tmp);
	}

	return 0;
}

int knode_call_resume(knode_s *node)
{
	int ret = 0;

	if (kflg_chk_any(node->flg, KNFL_STOP))
		return 0;
	if (!kflg_chk_any(node->flg, KNFL_PAUSE))
		return 0;

	if (node->resume)
		ret = node->resume(node);

	if (!ret)
		kflg_clr(node->flg, KNFL_PAUSE);

	return ret;
}

int knode_foreach_resume()
{
	int i;
	knode_s *tmp;
	knodecc_s *cc = __g_knodecc;

	for (i = cc->cnt; i >= 0; i--) {
		tmp = cc->arr[i];
		if (tmp)
			knode_call_resume(tmp);
	}

	return 0;
}

