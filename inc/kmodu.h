/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_MODU_H__
#define __K_MODU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <ktypes.h>
#include <sdlist.h>

#define kmodu_setup_env(cc) \
	do { \
		void *pv; \
		opt_attach(cc); \
		opt_getptr("p:/env/log/cc", &pv); \
		klog_attach(pv); \
		opt_getptr("p:/env/cnode/cc", &pv); \
		cnode_attach(pv); \
	} while (0)


typedef int (*KMODU_HEY)(void*);
typedef int (*KMODU_BYE)(void);
typedef int (*KMODU_CFG)(const char *kv, ...);

typedef struct _kmodu_t kmodu_t;
struct _kmodu_t {
	K_dlist_entry entry;

	char path[1024];
	char name[256];

	/* mj:8 + mn:8 + rev:16 */
	unsigned int version;

	/* lib_open returns */
	void *handle;

	/* D3D, VVID, VAUD, RDP */
	int type;

	/* Only say Hi, don't work */
	KMODU_HEY hey;
	/* Bye and die */
	KMODU_BYE bye;
	/* Set configure between hey and bye, NULL tells an end */
	KMODU_CFG cfg;

	int error;
};

int kmodu_init(const char *rootdir, const char *dllname,
		const char *manifestname, const char *optname);
int kmodu_final();

int kmodu_load();
int kmodu_unload();

int kmodu_layout();

#ifdef __cplusplus
};
#endif

#endif /* __K_MODU_H__ */

