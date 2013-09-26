/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_NODE_H__
#define __K_NODE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <hilda/sysdeps.h>

/* Chain Node, as a module */
typedef struct _knode_t knode_t;

/* Downstream node */
typedef struct _dstr_node_t dstr_node_t;

/** Downstream node */
struct _dstr_node_t {
	knode_t *node;
	void *link_dat;
};

typedef int (*knode_FOREACH)(knode_t *node, void *ua, void *ub);

/* Core Node FLags */
#define CNFL_STOP       0x00000001
#define CNFL_PAUSE      0x00000002

/* Core Node ATtribute */
#define CNAT_INPUT      0x00000001
#define CNAT_OUTPUT     0x00000002

struct _knode_t {
	char *name;

	/* type and ofmt not defined here */
	unsigned short type;
	unsigned short ofmt;

	unsigned int attr;
	unsigned int flg;

	void *ua, *ub;

	/*
	 * Everything about 'Chain'.
	 *
	 * Any node can choice it input, either by private
	 * process or by \c link_query().
	 */

	/* To work state, it can wait/read data and process */
	int (*start)(knode_t *self);
	int (*pause)(knode_t *self);
	int (*resume)(knode_t *self);
	int (*stop)(knode_t *self);

	/**
	 * XXX: before \c start(), \c link_query should be
	 * called to build the data flow tree.
	 *
	 * \param usnode Upstream node.
	 * \param self This node
	 * \param link_dat Parameter will be used for \c write().
	 *
	 * \warning \c usnode can be NULL.
	 * \retval 0 the Downstream node agree to chain.
	 */
	int (*link_query)(knode_t *usnode, knode_t *self, void **link_dat);
	int (*unlink)(knode_t *usnode, knode_t *self, void *link_dat);

	/* XXX: return correct data type according to cap. 0 for OK, otherwise for NG */
	int (*setting_query)(knode_t *self, int setting, void**ret);

	/* upper layer call this when data is ready, for chained down stream node */
	/* XXX Only write, read is not needed */
	int (*write)(knode_t *self, knode_t *usnode, void *dat, int len, void *link_dat);

	/** Downstream node */
	struct {
		dstr_node_t *arr;
		int cnt;
	} dstr;
};

void knode_data_ready(knode_t *node, void *dat, int len);

int knode_link_add(knode_t *unode, knode_t *dnode, void *link_dat);
int knode_link_del(knode_t *unode, knode_t *dnode);
int knode_link_clr(knode_t *node);

int knode_reg(knode_t *node);
int knode_unreg(knode_t *node);

int knode_link(knode_t *node);

knode_t *knode_find(const char *name);

void *knode_init(int argc, char *argv[]);
void *knode_cc();
void *knode_attach(void *knodecc);
int knode_final();

int knode_call_start(knode_t *node);
int knode_call_stop(knode_t *node);
int knode_call_pause(knode_t *node);
int knode_call_resume(knode_t *node);

int knode_foreach_start();
int knode_foreach_stop();
int knode_foreach_pause();
int knode_foreach_resume();

#ifdef __cplusplus
};
#endif

#endif /* __K_NODE_H__ */

