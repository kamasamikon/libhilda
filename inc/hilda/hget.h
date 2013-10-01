/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_HGET_H__
#define __K_HGET_H__

#ifdef __cplusplus
extern "C" {
#endif

/* error code */
#define PGEC_SUCCESS            0
#define PGEC_HOST               -1
#define PGEC_SOCKET             -2
#define PGEC_CONNECT            -3
#define PGEC_SEND               -4
#define PGEC_HEADER             -5
#define PGEC_RESP_CODE          -6
#define PGEC_CONTENT_LEN        -7
#define PGEC_RECV               -8
#define PGEC_MEMORY             -9
#define PGEC_ERROR              -10
#define PGEC_TRUNC              -11

/* protocal http://, https://, file://, ftp:// etc */
#define PROT_HTTP   0
#define PROT_HTTPS  1

#if (defined(__WIN32__) || defined(__WINCE__))
#define SOCKET kint
#else
#define SOCKET kint
#endif

/**
 * @brief Get data by HTTP GET or HTTP POST
 *
 * @param[in] a_url URL from which get data.
 * @param[in] a_proxy Proxy URL
 * @param[in] a_get GET or POST method
 * @param[in] a_cmd A data as command.
 * @param[in] a_datbuf Buffer to save returned data, allocated in this function, caller should free it.
 * @param[in] a_datlen Buffer length of a_retbuf.
 * @param[in] a_hdrbuf IN:*a_hdrbuf: buffer provider by caller, if (*a_hdrbuf == zero) (*a_hdrbuf = kmem_alloc).
 * @param[in] a_hdrlen IN:*a_hdrlen: max hdr len should be return, if zero, do not return hdr data
 *                     OUT:*a_hdrlen: actually read header length.
 * @param[in] a_socket socket which can be close when hget last too long.
 *
 * @return 0 for success, -1 for error.
 */
kint hget(const kchar *a_url, const kchar *a_proxy, kbool a_get,
		const kchar *a_cmd, kchar **a_datbuf, kint *a_datlen,
		kchar **a_hdrbuf, kint *a_hdrlen, SOCKET *a_socket);

kint hget_parseurl(const kchar *a_url, kuint *a_prot, kchar a_user[],
		kchar a_pass[], kchar a_host[], kchar a_path[], kushort *a_port);

kint hget_connect(kint a_prot, const kchar *a_user,
		const kchar *a_pass, const kchar *a_host,
		const kchar *a_path, kushort a_port, SOCKET *a_socket);

kint hget_recv(SOCKET a_socket, const kchar *a_host,
		const kchar *a_path, const kchar *a_proxy, kbool a_get,
		const kchar *a_cmd, kchar **a_datbuf, kint *a_datlen, kchar **a_hdrbuf, kint *a_hdrlen);

/**
 * @brief close socket
 */
kvoid hget_free_socket(SOCKET a_socket);

#ifdef __cplusplus
}
#endif
#endif /* __K_HGET_H__ */
