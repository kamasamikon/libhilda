/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_NODE_H__
#define __K_NODE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <hilda/sysdeps.h>

/* Chain Node, as a module */
typedef struct _knode_s knode_s;

/* Downstream node */
typedef struct _dstr_node_s dstr_node_s;

/** Downstream node */
struct _dstr_node_s {
	knode_s *node;
	void *link_dat;
};

typedef int (*KNODE_FOREACH)(knode_s *node, void *ua, void *ub);

/* K Node FLags */
#define KNFL_STOP       0x00000001
#define KNFL_PAUSE      0x00000002

/* K Node ATtribute */
#define KNAT_INPUT      0x00000001
#define KNAT_OUTPUT     0x00000002

struct _knode_s {
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
	int (*start)(knode_s *self);
	int (*pause)(knode_s *self);
	int (*resume)(knode_s *self);
	int (*stop)(knode_s *self);

	/**
	 * XXX: before \c start(), \c link_query should be
	 * called to build the data flow tree.
	 *
	 * \param usnode Upstream node.
	 * \param self This node
	 * \param link_dat Parameter will be used for \c push().
	 *
	 * \warning \c usnode can be NULL.
	 * \retval 0 the Downstream node agree to chain.
	 */
	int (*link_query)(knode_s *usnode, knode_s *self, void **link_dat);
	int (*unlink)(knode_s *usnode, knode_s *self, void *link_dat);

	/* XXX: return correct data type according to cap. 0 for OK, otherwise for NG */
	int (*setting_query)(knode_s *self, int setting, void**ret);

	/* upper layer call this when data is ready, for chained down stream node */
	/* XXX Only push, read is not needed */
	int (*push)(knode_s *self, knode_s *usnode, void *dat, int len, void *link_dat);

	/** Downstream node */
	struct {
		dstr_node_s *arr;
		int cnt;
	} dstr;
};

void knode_push(knode_s *node, void *dat, int len);

int knode_link_add(knode_s *unode, knode_s *dnode, void *link_dat);
int knode_link_del(knode_s *unode, knode_s *dnode);
int knode_link_clr(knode_s *node);

int knode_reg(knode_s *node);
int knode_unreg(knode_s *node);

int knode_link(knode_s *node);

knode_s *knode_find(const char *name);

void *knode_init(int argc, char *argv[]);
void *knode_cc();
void *knode_attach(void *knodecc);
int knode_final();

int knode_call_start(knode_s *node);
int knode_call_stop(knode_s *node);
int knode_call_pause(knode_s *node);
int knode_call_resume(knode_s *node);

int knode_foreach(KNODE_FOREACH foreach, void *ua, void *ub);

int knode_foreach_start();
int knode_foreach_stop();
int knode_foreach_pause();
int knode_foreach_resume();

#ifdef __cplusplus
};
#endif

#endif /* __K_NODE_H__ */

