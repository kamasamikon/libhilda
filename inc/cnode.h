/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __CNODE_H__
#define __CNODE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sysdeps.h>

/* Chain Node, as a module */
typedef struct _cnode_t cnode_t;

/* Downstream node */
typedef struct _dstr_node_t dstr_node_t;

/** Downstream node */
struct _dstr_node_t {
	cnode_t *node;
	void *link_dat;
};

typedef int (*CNODE_FOREACH)(cnode_t *node, void *ua, void *ub);

/* Core Node FLags */
#define CNFL_STOP       0x00000001
#define CNFL_PAUSE      0x00000002

/* Core Node ATtribute */
#define CNAT_INPUT      0x00000001
#define CNAT_OUTPUT     0x00000002

struct _cnode_t {
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
	int (*start)(cnode_t *self);
	int (*pause)(cnode_t *self);
	int (*resume)(cnode_t *self);
	int (*stop)(cnode_t *self);

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
	int (*link_query)(cnode_t *usnode, cnode_t *self, void **link_dat);
	int (*unlink)(cnode_t *usnode, cnode_t *self, void *link_dat);

	/* XXX: return correct data type according to cap. 0 for OK, otherwise for NG */
	int (*setting_query)(cnode_t *self, int setting, void**ret);

	/* upper layer call this when data is ready, for chained down stream node */
	/* XXX Only write, read is not needed */
	int (*write)(cnode_t *self, cnode_t *usnode, void *dat, int len, void *link_dat);

	/** Downstream node */
	struct {
		dstr_node_t *arr;
		int cnt;
	} dstr;
};

void cnode_data_ready(cnode_t *node, void *dat, int len);

int cnode_link_add(cnode_t *unode, cnode_t *dnode, void *link_dat);
int cnode_link_del(cnode_t *unode, cnode_t *dnode);
int cnode_link_clr(cnode_t *node);

int cnode_reg(cnode_t *node);
int cnode_unreg(cnode_t *node);

int cnode_link(cnode_t *node);

cnode_t *cnode_find(const char *name);

void *cnode_init(int argc, char *argv[]);
void *cnode_cc();
void *cnode_attach(void *cnodecc);
int cnode_final();

int cnode_call_start(cnode_t *node);
int cnode_call_stop(cnode_t *node);
int cnode_call_pause(cnode_t *node);
int cnode_call_resume(cnode_t *node);

int cnode_foreach_start();
int cnode_foreach_stop();
int cnode_foreach_pause();
int cnode_foreach_resume();

#ifdef __cplusplus
};
#endif

#endif /* __CNODE_H__ */

