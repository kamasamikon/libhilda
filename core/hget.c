/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <hilda/ktypes.h>
#include <hilda/klog.h>
#include <hilda/kmem.h>

#if (defined(__WIN32__) || defined(__WINCE__))
#include <winsock2.h>
/* #define EINTR (errno + 1) */
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <signal.h>
#include <errno.h>

#include <hilda/hget.h>

#if (defined(__WIN32__) || defined(__WINCE__))
#define CAN_CONTI() ((WSAETIMEDOUT == socket_last_error()) || \
		(WSAEINTR == socket_last_error()))
#else
#define CAN_CONTI() (EINTR == errno)
#endif

#define SOCKET int

#define PATH_LEN    1024
#define DATA_LEN    2048	/**< XXX Large enough to hold on header */

static int socket_last_error()
{
#if (defined(__WIN32__) || defined(__WINCE__))
	return WSAGetLastError();
#else
	return errno;
#endif
}

/**
 * @brief Close socket
 */
kvoid hget_free_socket(SOCKET a_socket)
{
	klog("%d\n", a_socket);
	if (-1 != a_socket) {
#if (defined(__WIN32__) || defined(__WINCE__))
		shutdown(a_socket, SD_BOTH);
		closesocket(a_socket);
#else
		shutdown(a_socket, SHUT_RDWR);
		close(a_socket);
#endif
	}
}

/**
 * @brief parse the url like,
 * https://auv:woshiauv@192.155.35.65:9088/index.php?l=en
 */
int hget_parseurl(const char *a_url, kuint *a_prot, char a_user[],
		char a_pass[], char a_host[], char a_path[], kushort *a_port)
{
	const char *url_cur = a_url;
	char *tmp, *end;

	/* get a_prot */
	*a_prot = PROT_HTTP;
	if (strstr(url_cur, "http://")) {
		url_cur += 7;
		*a_prot = PROT_HTTP;
	} else if (strstr(url_cur, "https://")) {
		url_cur += 8;
		*a_prot = PROT_HTTPS;
	}

	/* if not found ? assume ? in long long furtue */
	if (!(end = strchr(url_cur, '?')))
		end = (char*)url_cur + 80000;

	/* get user and pass */
	*a_user = '\0';
	*a_pass = '\0';
	if ((tmp = strchr(url_cur, '@')) && (tmp < end)) {
		strncpy(a_user, url_cur, tmp - url_cur);
		a_user[tmp - url_cur] = '\0';

		/* adjust url_cur to next word of @ */
		url_cur = tmp + 1;

		/* get pass */
		if ((tmp = strchr(a_user, ':')) && (tmp < end)) {
			/* user: auv:woshiauv to auv */
			*tmp = '\0';

			strcpy(a_pass, tmp + 1);
		}
	}

	/* get port */
	*a_port = 80;
	if ((tmp = strchr(url_cur, ':')) && (tmp < end))
		*a_port = (kushort) strtoul(tmp + 1, 0, 10);

	/* get host */
	strcpy(a_host, url_cur);
	for (tmp = a_host; *tmp != '/' && *tmp != ':' && *tmp != '\0'; ++tmp);
	*tmp = '\0';
	url_cur += tmp - a_host;

	/* get path */
	tmp = strchr(url_cur, '/');
	if (tmp)
		strcpy(a_path, tmp);
	else
		strcpy(a_path, "/");

	return 0;
}

/* create socket and connect */
/* a_socket for quick free while connecting */
int hget_connect(int a_prot, const char *a_user, const char *a_pass,
		const char *a_host, const char *a_path, kushort a_port,
		SOCKET *a_socket)
{
	int ret = -1;
	SOCKET sockfd = -1;
	struct sockaddr_in addr;
	struct hostent *host_ent;
	kuint ip_addr;
	struct timeval timeout;

#if (defined(__WIN32__) || defined(__WINCE__))
#else
	signal(SIGPIPE, SIG_IGN);
#endif

	if (a_socket)
		*a_socket = -1;

	/* Set socket parameters */
	klog("inet_addr\n");
	ip_addr = inet_addr(a_host);
	if (INADDR_NONE == ip_addr) {
		klog("gethostbyname\n");
		host_ent = gethostbyname(a_host);
		if (!host_ent) {
			kerror("gethostbyname : ng : url(%s%s:%d)\n", a_host, a_path, a_port);
			kerror("gethostbyname failed: %d\n", socket_last_error());
			return PGEC_HOST;
		}
		ip_addr = *((kulong *) host_ent->h_addr);
	}

	klog("socket\n");
	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (-1 == sockfd) {
		kerror("socket : ng : url(%s%s:%d)\n", a_host, a_path, a_port);
		kerror("create socket failed: %d\n", socket_last_error());
		return PGEC_SOCKET;
	}

	if (a_socket)
		*a_socket = sockfd;

	/* set timeout for recv and send */
	timeout.tv_sec = 15;
	timeout.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) < 0)
		kerror("setsockopt:SO_RCVTIMEO:%d\n", socket_last_error());
	if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(timeout)) < 0)
		kerror("setsockopt:SO_SNDTIMEO:%d\n", socket_last_error());

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ip_addr;
	addr.sin_port = htons(a_port);

	/* connect */
	klog("<%d>: connect\n", sockfd);
	while ((-1 == (ret = connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)))) && CAN_CONTI());
	if (-1 == ret) {
		if (a_socket)
			*a_socket = -1;

		kerror("<%d>: connect : ng : url(%s%s:%d)\n", sockfd, a_host, a_path, a_port);
		kerror("<%d>: connect failed: %d\n", sockfd, socket_last_error());
		hget_free_socket(sockfd);
		return PGEC_CONNECT;
	}

	klog("sockfd:%d\n", sockfd);
	return PGEC_SUCCESS;
}

int hget_recv(SOCKET a_socket, const char *a_host, const char *a_path,
		const char *a_proxy, kbool a_get, const char *a_cmd,
		char **a_datbuf, int *a_datlen,
		char **a_hdrbuf, int *a_hdrlen)
{
	char *pTemp, *pNext;
	char header[PATH_LEN] = "";

	char data[DATA_LEN];
	int rc = PGEC_ERROR, ret = -1, resp_code, pkg_len = 0, dat_len = 0, hdr_len, cur_len = 0, lft_len;

	/*
	 * Fill the HTTP header information
	 */
	/*
	 * An Example for Request
	 *
	 * "POST /SGHCServ/SGHCI.ashx HTTP/1.0\r\n"
	 * "Accept: text/xml\r\n"
	 * "Content-Length: 36\r\n"
	 * "Host: 192.168.1.19\r\n"
	 * "Content-Type: text/xml\r\n"
	 * "\r\n"
	 * "<Msg><Cmd>GetServiceList</Cmd></Msg>"
	 */
	if (a_get)
		sprintf(header,
				"GET %s HTTP/1.1\r\n"
				"Connection: Keep-Alive\r\n"
				"Host: %s\r\n"
				"Connection: Close\r\n\r\n", a_path, a_host);
	else
		sprintf(header,
				"POST %s HTTP/1.1\r\n"
				"Accept: text/xml\r\n"
				"Connection: Keep-Alive\r\n"
				"Content-Length: %zd\r\n"
				"Host: %s\r\n"
				"Content-Type: text/xml\r\n\r\n" "%s",
				a_path, a_cmd ? strlen(a_cmd) : 0,
				a_host, a_cmd ? (char*)a_cmd : "");

	klog("(%d): start send\n", a_socket);

	while ((-1 == (ret = send(a_socket, header, strlen(header), 0))) && CAN_CONTI());
	if (-1 == ret) {
		kerror("(%d): send failed: %d\n", a_socket, socket_last_error());
		rc = PGEC_SEND;
		goto done;
	}

	/*
	 * first, get the information
	 */
	/* try the first buffer as full as posible */
	ret = cur_len = 0;
	data[0] = '\0';
	do {
		cur_len += ret;
		if (strstr(data, "\r\n\r\n"))
			break;
		while ((-1 == (ret = recv(a_socket, data + ret, DATA_LEN - ret, 0))) && CAN_CONTI())
			ret++;
		if (-1 == ret)
			kerror("(%d): recv failed: %d\n", a_socket, socket_last_error());
		if (0 == ret)
			kerror("(%d): recv failed: socket closed.\n", a_socket);
		if (ret >= DATA_LEN)
			cur_len += ret;
	} while ((ret > 0) && (ret < DATA_LEN));

	ret = cur_len;

	if (ret > 0) {

		/*
		 * try to get header, only when (*a_hdrbuf != zero)
		 */
		if (*a_hdrlen < 0)
			*a_hdrlen = 0x7FFFFFFF;
		if (*a_hdrlen > 0) {
			cur_len = *a_hdrlen;
			pTemp = strstr(data, "\r\n\r\n");
			hdr_len = pTemp - data + 4;
			if (!*a_hdrbuf) {
				if (hdr_len > cur_len)
					*a_hdrbuf = (char *)kmem_alloz(cur_len + 1, char);
				else
					*a_hdrbuf = (char *)kmem_alloz(hdr_len + 1, char);
			}
			*a_hdrlen = 0;
			if (*a_hdrbuf) {
				if (hdr_len > cur_len) {
					memcpy(*a_hdrbuf, data, cur_len);
					*a_hdrlen = cur_len;
				} else {
					memcpy(*a_hdrbuf, data, hdr_len);
					*a_hdrlen = hdr_len;
				}
			}
		}

		if (*a_datlen < 0)
			*a_datlen = 0x7FFFFFFF;
		if (*a_datlen == 0) {
			/* not need content, return */
			rc = PGEC_SUCCESS;
			goto done;
		}

		/* Get Response Code */
		pTemp = strstr(data, "HTTP/1.1 ");
		if (!pTemp)
			pTemp = strstr(data, "HTTP/1.0 ");
		if (!pTemp) {
			kerror("(%d): Can not get Response code\n", a_socket);
			rc = PGEC_HEADER;
			goto done;
		}
		resp_code = atoi(pTemp + 9);
		if (resp_code < 200 || resp_code >= 300) {
			kerror("(%d): resp_code (%d) tell an error\n", a_socket, resp_code);
			rc = PGEC_RESP_CODE;
			goto done;
		}

		/* Get Buffer Length */
		pTemp = strstr(data, "Content-Length: ");
		if (!pTemp) {
			kerror("(%d): Can not get content length\n", a_socket);
			rc = PGEC_CONTENT_LEN;
			goto done;
		}

		pkg_len = dat_len = atoi(pTemp + 16);
		cur_len = *a_datlen;
		if (!*a_datbuf) {
			if (dat_len > cur_len) {
				*a_datbuf = (char *) kmem_alloz(cur_len + 1, char);
				dat_len = cur_len;
			} else {
				*a_datbuf = (char *) kmem_alloz(dat_len + 1, char);
				dat_len = dat_len;
			}
		}

		pNext = *a_datbuf;
		if (!pNext) {
			kerror("(%d): Can not alloc memory\n", a_socket);
			rc = PGEC_MEMORY;
			goto done;
		}

		cur_len = 0;

		/* Copy the first part of data */
		pTemp = strstr(data, "\r\n\r\n");

		lft_len = ret - (pTemp + 4 - data);

		if (lft_len < dat_len) {
			memcpy(pNext, pTemp + 4, lft_len);
			pNext += lft_len;
			cur_len += lft_len;
		} else {
			memcpy(pNext, pTemp + 4, dat_len);
			pNext += dat_len;
			cur_len += dat_len;
		}
		kassert(cur_len <= dat_len, "bad cur_len or all_en");

		/*
		 * Then got the rest data
		 */
		klog("(%d): cur_len:%d, dat_len:%d\n", a_socket, cur_len, dat_len);
		if (cur_len < dat_len) {
			resp_code = 1;
			while ((-1 == (ret = recv(a_socket, data, DATA_LEN, 0)))
					&& CAN_CONTI());
			if (-1 == ret)
				kerror("(%d): recv failed: %d\n", a_socket, socket_last_error());
			if (0 == ret)
				kerror("(%d): recv failed: socket closed.\n", a_socket);

			klog("(%d): dat_len:%d, cur_len:%d, recv_cnt:%d, ret:%d, err:%d\n",
						a_socket, dat_len, cur_len, resp_code++, ret, socket_last_error());

			while (ret > 0) {

				lft_len = ret;

				if (lft_len < (dat_len - cur_len)) {
					memcpy(pNext, data, lft_len);
					pNext += lft_len;
					cur_len += lft_len;
				} else {
					memcpy(pNext, data, dat_len - cur_len);
					pNext += dat_len - cur_len;
					cur_len += dat_len - cur_len;
				}
				kassert(cur_len <= dat_len, "bad cur_len or all_en");

				if (cur_len < dat_len)
					while ((-1 == (ret = recv(a_socket, data, DATA_LEN, 0))) && CAN_CONTI());
				else {
					klog("(%d): all received\n", a_socket);
					break;
				}

				klog("(%d): dat_len:%d, cur_len:%d, recv_cnt:%d, ret:%d, err:%d\n",
							a_socket, dat_len, cur_len, resp_code++, ret, socket_last_error());
			}
		}

		*a_datlen = cur_len;
		rc = PGEC_SUCCESS;
	}

done:
	if (PGEC_SUCCESS != rc)
		kerror("(%d): ng : skt(%d), host(%s), path(%s), get(%d), cmd(%s)\n", a_socket, a_socket, a_host,
					a_path, a_get, a_cmd);

	klog("(%d): pkg_len:%d, rcv_len:%d, return : %d\n", a_socket, rc, pkg_len, cur_len);
	return rc;
}

int hget(const char *a_url, const char *a_proxy, kbool a_get,
		const char *a_cmd, char **a_datbuf, int *a_datlen,
		char **a_hdrbuf, int *a_hdrlen, int *a_socket)
{
	char user[32] = { 0 }, pass[32] = { 0 }, host[1024] = { 0 }, path[1024] = { 0 };
	kuint prot;
	kushort port;
	int ret = PGEC_ERROR;
	SOCKET ls, *s = a_socket ? a_socket : &ls;

	hget_parseurl(a_url, &prot, user, pass, host, path, &port);

	if (PGEC_SUCCESS == (ret = hget_connect(prot, user, pass, host, path, port, s))) {
		ret = hget_recv(*s, host, path, a_proxy, a_get, a_cmd, a_datbuf, a_datlen, a_hdrbuf, a_hdrlen);
		hget_free_socket(*s);
	}
	return ret;
}

