/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __OPT_RPC_SERVER_H__
#define __OPT_RPC_SERVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sysdeps.h>

int opt_rpc_server_init(int argc, char *argv[]);
int opt_rpc_server_final();

#ifdef __cplusplus
}
#endif

#endif /* __OPT_RPC_SERVER_H__ */

