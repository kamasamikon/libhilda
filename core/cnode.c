/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <string.h>

#include <ktypes.h>
#include <kflg.h>
#include <klog.h>
#include <kstr.h>
#include <kmem.h>
#include <opt.h>

#include <xtcool.h>

#include <cnode.h>
#include <helper.h>

/* Control Center for cnode */
typedef struct _cnodecc_t cnodecc_t;
struct _cnodecc_t {
	cnode_t **arr;
	int cnt;
};

static cnodecc_t *__g_cnodecc = NULL;

/* foreach of down stream nodes */
void cnode_data_ready(cnode_t *node, void *dat, int len)
{
	int i;
	dstr_node_t *tmp;
	cnode_t *tnode;

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
int cnode_link_add(cnode_t *unode, cnode_t *dnode, void *link_dat)
{
	int i;
	dstr_node_t *tmp;

	if (!unode || !dnode)
		return -1;

	for (i = 0; i < unode->dstr.cnt; i++) {
		tmp = &unode->dstr.arr[i];
		if (tmp->node == dnode) {
			kerror("cnode_link_add: %s already there\n",
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

int cnode_link_del(cnode_t *unode, cnode_t *dnode)
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

int cnode_link_clr(cnode_t *node)
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

int cnode_reg(cnode_t *node)
{
	int i;
	cnode_t *tmp;
	cnodecc_t *cc = __g_cnodecc;

	node->dstr.arr = 0;
	node->dstr.cnt = 0;
	node->flg = CNFL_STOP;
	node->attr &= 0xffff;

	for (i = 0; i < cc->cnt; i++) {
		tmp = cc->arr[i];
		if (tmp == node) {
			kerror("cnode_reg: %s already\n", node->name);
			return 0;
		}
	}
	for (i = 0; i < cc->cnt; i++)
		if (!cc->arr[i]) {
			cc->arr[i] = node;
			klog("cnode_reg: %s success.\n", node->name);
			return 0;
		}

	ARR_INC(1, cc->arr, cc->cnt, cnode_t*);
	cc->arr[i] = node;
	klog("cnode_reg: %s success.\n", node->name);
	return 0;
}

int cnode_unreg(cnode_t *node)
{
	int i;
	cnode_t *tmp;
	cnodecc_t *cc = __g_cnodecc;

	for (i = 0; i < cc->cnt; i++) {
		tmp = cc->arr[i];
		if (tmp && tmp == node) {
			klog("cnode_unreg: %s OK.\n", node->name);
			return 0;
		}
	}

	kerror("cnode_unreg: %s not there.\n", node->name);
	return -1;
}

int cnode_link(cnode_t *node)
{
	int i;
	cnode_t *dstr;
	cnodecc_t *cc = __g_cnodecc;
	void *link_dat;

	/* setup the chain */
	for (i = 0; i < cc->cnt; i++) {
		dstr = cc->arr[i];
		if (dstr && node && (dstr != node) && dstr->link_query)
			if (kflg_chk_bit(dstr->attr, CNAT_INPUT))
				if (!dstr->link_query(node, dstr, &link_dat))
					cnode_link_add(node, dstr, link_dat);

		if (!node)
			cnode_link(dstr);
	}

	return 0;
}

/**
 * \brief Find cnode by given name.
 * Name can be reguler name for index name #ddd,
 * e.g #1 or #-2 etc.
 */
cnode_t *cnode_find(const char *name)
{
	int i, index;
	cnode_t *tmp;
	cnodecc_t *cc = __g_cnodecc;

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

static int cnode_foreach(CNODE_FOREACH foreach, void *ua, void *ub)
{
	int i;
	cnode_t *tmp;
	cnodecc_t *cc = __g_cnodecc;

	for (i = 0; i < cc->cnt; i++) {
		tmp = cc->arr[i];
		if (tmp)
			foreach(tmp, ua, ub);
	}

	return 0;
}

static int do_dump(cnode_t *node, void *ua, void *ub)
{
	char *dmpbuf = (char*)ua, node_buf[1024], dstr_buf[2048], *p;
	int i, ret;

	sprintf(node_buf, "\r\nT:%08x A:%08x F:%08x write:%p name:%s\r\n",
			node->type, node->attr, node->flg, node->write,
			node->name);

	dstr_buf[0] = '\0';
	ret = 0;
	p = dstr_buf;
	for (i = 0; i < node->dstr.cnt; i++) {
		ret = sprintf(p, "\t%02d: %s\r\n", i,
				node->dstr.arr[i].node->name);
		p += ret;
	}

	strcat(dmpbuf, node_buf);
	strcat(dmpbuf, dstr_buf);

	return 0;
}

static int og_cnode_diag_dump(void *opt, const char *path,
		void **pa, void **pb)
{
	char dmpbuf[8192 * 8];

	dmpbuf[0] = '\0';
	cnode_foreach(do_dump, (void*)dmpbuf, NULL);
	opt_set_cur_str(opt, dmpbuf);
	return EC_OK;
}

static int os_cnode_cmd(int ses, void *opt, const char *path,
		void **pa, void **pb)
{
	char *name = opt_get_new_str(opt);
	char type = (char)(int)opt_ua(opt);
	cnode_t *node;
	int ret;

	if (name && !strcmp(name, "*")) {
		if (type == 'S')
			ret = cnode_foreach_start();
		else if (type == 's')
			ret = cnode_foreach_stop();
		else if (type == 'p')
			ret = cnode_foreach_pause();
		else if (type == 'r')
			ret = cnode_foreach_resume();
		else if (type == 'l')
			ret = cnode_link(NULL);
		else
			return EC_OK;
		return ret;
	}

	node = cnode_find(name);
	if (!node)
		return EC_NG;

	if (type == 'S')
		ret = cnode_call_start(node);
	else if (type == 's')
		ret = cnode_call_stop(node);
	else if (type == 'p')
		ret = cnode_call_pause(node);
	else if (type == 'r')
		ret = cnode_call_resume(node);
	else if (type == 'l')
		ret = cnode_link(node);
	else
		ret = EC_NG;

	if (ret)
		return EC_NG;
	return EC_OK;
}

void *cnode_attach(void *cnodecc)
{
	__g_cnodecc = (cnodecc_t*)cnodecc;

	return (void*)__g_cnodecc;
}

void *cnode_cc()
{
	return (void*)__g_cnodecc;
}

static void setup_opt()
{
	opt_add_s("s:/k/cnode/diag/dump", OA_GET, NULL,
			og_cnode_diag_dump);

	opt_add("s:/k/cnode/cmd/start", NULL, OA_SET,
			os_cnode_cmd, NULL, NULL, (void*)'S', NULL);
	opt_add("s:/k/cnode/cmd/stop", NULL, OA_SET,
			os_cnode_cmd, NULL, NULL, (void*)'s', NULL);
	opt_add("s:/k/cnode/cmd/pause", NULL, OA_SET,
			os_cnode_cmd, NULL, NULL, (void*)'p', NULL);
	opt_add("s:/k/cnode/cmd/resume", NULL, OA_SET,
			os_cnode_cmd, NULL, NULL, (void*)'r', NULL);
	opt_add("s:/k/cnode/cmd/link", NULL, OA_SET,
			os_cnode_cmd, NULL, NULL, (void*)'l', NULL);
}

void *cnode_init(int argc, char *argv[])
{
	if (__g_cnodecc)
		return (void*)__g_cnodecc;

	__g_cnodecc = (cnodecc_t*)kmem_alloz(1, cnodecc_t);
	setup_opt();
	return (void*)__g_cnodecc;
}

int cnode_final()
{
	if (!__g_cnodecc)
		return 0;
	kmem_free_s(__g_cnodecc->arr);
	kmem_free_sz(__g_cnodecc);
	return 0;
}

int cnode_call_start(cnode_t *node)
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
static int do_start(cnode_t *node, void *ua, void *ub)
{
	return cnode_call_start(node);
}
int cnode_foreach_start()
{
	cnode_foreach(do_start, NULL, NULL);
	return 0;
}

int cnode_call_stop(cnode_t *node)
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
static int do_stop(cnode_t *node, void *ua, void *ub)
{
	return cnode_call_stop(node);
}
int cnode_foreach_stop()
{
	cnode_foreach(do_stop, NULL, NULL);
	return 0;
}

int cnode_call_pause(cnode_t *node)
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
static int do_pause(cnode_t *node, void *ua, void *ub)
{
	return cnode_call_pause(node);
}
int cnode_foreach_pause()
{
	cnode_foreach(do_pause, NULL, NULL);
	return 0;
}

int cnode_call_resume(cnode_t *node)
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
static int do_resume(cnode_t *node, void *ua, void *ub)
{
	return cnode_call_resume(node);
}
int cnode_foreach_resume()
{
	cnode_foreach(do_resume, NULL, NULL);
	return 0;
}

