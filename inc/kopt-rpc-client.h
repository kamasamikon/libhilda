/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __OPT_RPC_CLIENT_H__
#define __OPT_RPC_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sysdeps.h>

int kopt_rpc_watch(void *conn, const char *path, char *errbuf, int eblen);
int kopt_rpc_unwatch(void *conn, const char *path, char *errbuf, int eblen);

int kopt_rpc_setkv(void *conn, const char *k, const char *v, char *errbuf, int eblen);

char *kopt_rpc_geterr(void *conn);

int kopt_rpc_setini(void *conn, const char *inibuf, char *errbuf, int eblen);
int kopt_rpc_getini(void *conn, const char *path, char *retbuf, int rblen, char *errbuf, int eblen);

int kopt_rpc_getint(void *conn, const char *path, int *val);
int kopt_rpc_getstr(void *conn, const char *path, char **val);
int kopt_rpc_getarr(void *conn, const char *path, void **arr, int *len);
int kopt_rpc_getbin(void *conn, const char *path, char **arr, int *len);

void *kopt_rpc_connect(const char *server, unsigned short port,
		void (*wfunc)(void *conn, const char *path, void *ua, void *ub),
		void *wfunc_ua, void *wfunc_ub,
		const char *client_name, const char *user_name,
		const char *user_pass);
int kopt_rpc_disconnect(void *conn);

#ifdef __cplusplus
}
#endif

#endif /* __OPT_RPC_CLIENT_H__ */

