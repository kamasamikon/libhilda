/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __OPT_RPC_SERVER_H__
#define __OPT_RPC_SERVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <hilda/sysdeps.h>

int kopt_rpc_server_init(unsigned short port, int argc, char *argv[]);
int kopt_rpc_server_final();

#ifdef __cplusplus
}
#endif

#endif /* __OPT_RPC_SERVER_H__ */

