/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __OPT_RPC_CLIENT_H__
#define __OPT_RPC_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sysdeps.h>

int opt_rpc_watch(void *conn, const char *path, char *errbuf, int eblen);
int opt_rpc_unwatch(void *conn, const char *path, char *errbuf, int eblen);

int opt_rpc_setkv(void *conn, const char *k, const char *v, char *errbuf, int eblen);

char *opt_rpc_geterr(void *conn);

int opt_rpc_setini(void *conn, const char *inibuf, char *errbuf, int eblen);
int opt_rpc_getini(void *conn, const char *path, char *retbuf, int rblen, char *errbuf, int eblen);

int opt_rpc_getint(void *conn, const char *path, int *val);
int opt_rpc_getstr(void *conn, const char *path, char **val);
int opt_rpc_getarr(void *conn, const char *path, void **arr, int *len);
int opt_rpc_getbin(void *conn, const char *path, char **arr, int *len);

void *opt_rpc_connect(const char *server, unsigned short port,
		void (*wfunc)(void *conn, const char *path, void *ua, void *ub),
		void *wfunc_ua, void *wfunc_ub,
		const char *client_name, const char *user_name,
		const char *user_pass);
int opt_rpc_disconnect(void *conn);

#ifdef __cplusplus
}
#endif

#endif /* __OPT_RPC_CLIENT_H__ */

