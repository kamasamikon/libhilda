/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

/**
 * \file opt-rpc-client.c
 * \brief Program other then decoder or encoder use this to
 * access D/E opt database.
 *
 * Because \c pa and \c pb may contain some pointer, and
 * pointer can not be delived between program, so only NONE
 * parameter mode, call with \c opt_setbat() supported.
 *
 * Watch also diffent from what it is in opt.c, because the
 * inter-process watch can cause program slow down. User,
 * if to use watch, will treat it as a event, but a hook.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/select.h>

#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include <ktypes.h>

#include <karg.h>
#include <klog.h>
#include <kmem.h>
#include <kstr.h>
#include <sdlist.h>

#include <md5sum.h>

#include <xtcool.h>
#include <helper.h>
#include <opt.h>

#include <opt-rpc-common.h>
#include <opt-rpc-client.h>

typedef struct _opt_rpc_t opt_rpc_t;
struct _opt_rpc_t {
	/* connection id */
	char connhash[33];

	SPL_HANDLE wch_thread;
	SPL_HANDLE wch_running;
	volatile kbool quit;

	char client_name[128];
	char user_name[128];
	char user_pass[128];

	char server[1024];
	unsigned short port;
	int opt_socket, wch_socket;

	char errmsg[2048];
	int prompt_len;

	void (*wch_func)(void *conn, const char *path);
	void *ua, *ub;
};

#define __g_epoll_max 32
static int __g_epoll_fd = -1;
static struct epoll_event __g_epoll_events[__g_epoll_max];

static int connect_and_hey(opt_rpc_t *or, unsigned short port,
		int isopt, int *retsock);
static void ignore_pipe();
static void config_socket(int s);

/*
 * XXX Readme XXX
 *
 * There are two ways to do the remote opt, 'share file' and 'socket'.
 * For 'share file', the watch function can be done easily, meanwhile
 * it can not work across the network.
 * Socket way can work either local or remote, but the watch function
 * should be do 'right'.
 *
 * Currently, \c opt and \c wch use two different socket, they will be
 * merge into one next step.
 */

static void *watch_thread_or_client(void *userdata)
{
	opt_rpc_t *or = (opt_rpc_t*)userdata;
	char *buf;

	struct epoll_event ev, *e;

	int ready, i, n;
	const int buf_size = 64 * 1024;

	buf = (char*)kmem_alloc(buf_size, char);

	__g_epoll_fd = epoll_create(__g_epoll_max);

	ev.data.fd = or->wch_socket;
	ev.events = EPOLLIN;
	epoll_ctl(__g_epoll_fd, EPOLL_CTL_ADD, or->wch_socket, &ev);

	spl_sema_rel(or->wch_running);

	while (!or->quit) {
		do
			ready = epoll_wait(__g_epoll_fd,
					__g_epoll_events, __g_epoll_max, -1);
		while ((ready == -1) && (errno == EINTR));

		for (i = 0; i < ready; i++) {
			e = __g_epoll_events + i;

			if (!(e->events & EPOLLIN))
				continue;

			n = recv(e->data.fd, buf, buf_size, 0);
			if (n < 0) {
				kerror("error! f:%s, l:%d, c:%s, e:%s\n",
							__func__, __LINE__,
							"recv",
							strerror(errno));
				break;
			}
			if (n == 0) {
				kerror("watch_thread: recv: remote close.\n");
				or->quit = 1;
				break;
			}

			if (!strncmp("wchnotify ", buf, 10)) {
				or->wch_func(or, buf + 10);
				sprintf(buf, "0 OK\r\n");
			} else if (!strncmp("bye", buf, 3)) {
				kerror("watch_thread: remote say bye.\n");
				or->quit = 1;
				break;
			} else
				sprintf(buf, "1 NG\r\n");

			n = send(e->data.fd, "ACK", 4, 0);
			if (n < 0) {
				kerror("error! f:%s, l:%d, c:%s, e:%s\n",
							__func__, __LINE__,
							"send",
							strerror(errno));
				break;
			}
			if (n == 0) {
				kerror("watch_thread: send: remote close.\n");
				or->quit = 1;
				break;
			}
		}
	}

	or->wch_func(or, "");

	close(or->wch_socket);
	or->wch_socket = -1;

	close(__g_epoll_fd);
	__g_epoll_fd = -1;
	kmem_free_s(buf);

	/* FIXME: uncomment it will ban the socket after some connection */
	/* spl_thread_exit(); */
	return NULL;
}

static int shake_hand(int socket, const char *connhash,
		int isopt, const char *myname, const char *user_name,
		const char *user_pass, char *respbuf, int rblen)
{
	int ret;
	char iodat[4096];

	sprintf(iodat, "hey %c %s %s %s %s\r\n", isopt ? 'o' : 'w',
			myname, connhash, user_name, user_pass);

	/* send hey */
	if (-1 == send(socket, iodat, strlen(iodat) + 1, 0)) {
		kerror("error! f:%s, l:%d, c:%s, e:%s\n", __func__,
					__LINE__, "send", strerror(errno));
		return -1;
	}

	/* first datagram in do_xxx_real */
	if ((ret = recv(socket, respbuf, rblen, 0)) <= 0) {
		kerror("error! f:%s, l:%d, c:%s, e:%s\n", __func__,
					__LINE__, "recv", strerror(errno));
		return -1;
	}

	return 0;
}

/**
 * \brief Connect to encoder/decoder, all the setting is default.
 *
 * Retry will be done automatically if connect failed.
 */
static int rpc_connect(const char *server, unsigned short port, int *retfd)
{
	int sockfd;
	struct hostent *he;
	struct sockaddr_in their_addr;

	if ((he = gethostbyname(server)) == NULL)
		return -1;
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		return -1;

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(port);
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);
	if (connect(sockfd, (struct sockaddr *)&their_addr,
				sizeof their_addr) == -1) {
		kerror("error! f:%s, l:%d, c:%s, e:%s\n", __func__,
					__LINE__, "connect", strerror(errno));
		close(sockfd);
		return -1;
	}

	config_socket(sockfd);

	*retfd = sockfd;
	return 0;
}

static int rpc_disconnect(int sockfd, kbool opt)
{
	int err;
	char buf[80];

	klog("rpc_disconnect: %d\n", sockfd);
	sprintf(buf, "bye\r\n");
	if (-1 == send(sockfd, buf, strlen(buf) + 1, 0)) {
		kerror("error! f:%s, l:%d, c:%s, e:%s\n", __func__,
					__LINE__, "send", strerror(errno));
	}

	err = recv(sockfd, buf, sizeof(buf), 0);
	assert(err == 0 || err == -1);
	err = close(sockfd);
	if (err)
		kerror("error! f:%s, l:%d, c:%s, e:%s\n", __func__,
					__LINE__, "close", strerror(errno));

	return err;
}

static void set_errmsg(opt_rpc_t *or, const char *err)
{
	strcpy(or->errmsg, err ? : "");
}

static int process_resp(opt_rpc_t *or, char *resp, char *retbuf, int rblen)
{
	char *st_start, *st_end;
	int ret;

	/* strip prompt part */
	resp[strlen(resp) - or->prompt_len] = '\0';

	ret = strtol(resp, 0, 10);
	st_start = strstr(resp, " ") + 1;
	st_end = strstr(resp, "\r\n");
	st_end[0] = '\0';
	set_errmsg(or, st_start);

	if (retbuf)
		strncpy(retbuf, st_end + 2, rblen);

	return ret;
}

/* >>: CMD_WCH_ADD(unsigned char)+OPT_PATH(char*)
 * <<: ERR(char)+WCH_ID
 */
int opt_rpc_watch(void *conn, const char *path, char *ebuf, int eblen)
{
	opt_rpc_t *or = (opt_rpc_t*)conn;
	char iodat[2048];
	int ret;

	set_errmsg(or, NULL);

	sprintf(iodat, "wa %s\r\n", path);
	send(or->opt_socket, iodat, strlen(iodat) + 1, 0);

	if (recv(or->opt_socket, iodat, sizeof(iodat), 0) <= 0) {
		kerror("error! f:%s, l:%d, c:%s, e:%s\n", __func__,
					__LINE__, "recv", strerror(errno));
		return EC_CONNECT;
	}

	ret = process_resp(or, iodat, NULL, 0);
	if (ebuf)
		strncpy(ebuf, or->errmsg, eblen);

	return ret;
}

/* >>: CMD_WCH_DEL(unsigned char)+OPT_PATH(char*)
 * <<: ERR(char)+WCH_ID
 */
int opt_rpc_unwatch(void *conn, const char *path, char *ebuf, int eblen)
{
	opt_rpc_t *or = (opt_rpc_t*)conn;
	char iodat[2048];
	int ret;

	set_errmsg(or, NULL);

	sprintf(iodat, "wd %s\r\n", path);
	send(or->opt_socket, iodat, strlen(iodat) + 1, 0);

	if (recv(or->opt_socket, iodat, sizeof(iodat), 0) <= 0) {
		kerror("error! f:%s, l:%d, c:%s, e:%s\n", __func__,
					__LINE__, "recv", strerror(errno));
		return EC_CONNECT;
	}

	ret = process_resp(or, iodat, NULL, 0);
	if (ebuf)
		strncpy(ebuf, or->errmsg, eblen);

	return ret;
}

/* >>: CMD_OPT_SET(unsigned char)+INI(char*)
 * <<: ERR(char)
 */
int opt_rpc_setini(void *conn, const char *inibuf, char *ebuf, int eblen)
{
	opt_rpc_t *or = (opt_rpc_t*)conn;
	char iodat[8192];
	int ret = -1;

	set_errmsg(or, NULL);

	sprintf(iodat, "os %s\r\n", inibuf);
	send(or->opt_socket, iodat, strlen(iodat) + 1, 0);

	if (recv(or->opt_socket, iodat, eblen + 1024, 0) <= 0) {
		kerror("error! f:%s, l:%d, c:%s, e:%s\n", __func__,
					__LINE__, "recv", strerror(errno));
		return EC_CONNECT;
	}

	ret = process_resp(or, iodat, NULL, 0);
	if (ebuf)
		strncpy(ebuf, or->errmsg, eblen);

	return ret;
}

/* >>: CMD_OPT_SET(unsigned char)+INI(char*)
 * <<: ERR(char)
 */
int opt_rpc_setkv(void *conn, const char *k, const char *v,
		char *ebuf, int eblen)
{
	opt_rpc_t *or = (opt_rpc_t*)conn;
	char buffer[8192];
	sprintf(buffer, "%s=%s", k, v);
	return opt_rpc_setini(or, buffer, ebuf, eblen);
}

char *opt_rpc_geterr(void *conn)
{
	opt_rpc_t *or = (opt_rpc_t*)conn;
	return or->errmsg;
}

/* >>: CMD_OPT_GET(unsigned char)+INI(char*)
 * <<: ERR(char)+len(unsigned int)+dat(void*)
 */
int opt_rpc_getini(void *conn, const char *path,
		char *retbuf, int rblen, char *ebuf, int eblen)
{
	opt_rpc_t *or = (opt_rpc_t*)conn;
	char *iodat;
	int ret = -1;

	iodat = (char*)kmem_alloc(rblen + 8192, char);

	set_errmsg(or, NULL);

	sprintf(iodat, "og %s\r\n", path);
	send(or->opt_socket, iodat, strlen(iodat) + 1, 0);

	if (recv(or->opt_socket, iodat, rblen + 8192, 0) <= 0) {
		kerror("error! f:%s, l:%d, c:%s, e:%s\n", __func__,
					__LINE__, "recv", strerror(errno));
		kmem_free(iodat);
		return EC_CONNECT;
	}

	ret = process_resp(or, iodat, retbuf, rblen);
	if (ebuf)
		strncpy(ebuf, or->errmsg, eblen);

	kmem_free(iodat);
	return ret;
}

int opt_rpc_getint(void *conn, const char *path, int *val)
{
	int err;
	char dat[8192], ebuf[1024];
	opt_rpc_t *or = (opt_rpc_t*)conn;

	err = opt_rpc_getini(or, path, dat, sizeof(dat), ebuf, sizeof(ebuf));
	if (err != EC_OK)
		return -1;

	*val = strtol(dat, 0, 10);
	return 0;
}

int opt_rpc_getstr(void *conn, const char *path, char **val)
{
	int err;
	char dat[8192], ebuf[1024];
	opt_rpc_t *or = (opt_rpc_t*)conn;

	err = opt_rpc_getini(or, path, dat, sizeof(dat), ebuf, sizeof(ebuf));
	if (err != EC_OK)
		return err;

	*val = kstr_dup(dat);
	return EC_OK;
}

int opt_rpc_getarr(void *conn, const char *path, void **arr, int *len)
{
	int err;
	char dat[8192], ebuf[1024];
	opt_rpc_t *or = (opt_rpc_t*)conn;

	err = opt_rpc_getini(or, path, dat, sizeof(dat), ebuf, sizeof(ebuf));
	if (err != EC_OK)
		return err;

	/* FIXME */
	return EC_NG;
}

int opt_rpc_getbin(void *conn, const char *path, char **arr, int *len)
{
	int err;
	char dat[8192], ebuf[1024];
	opt_rpc_t *or = (opt_rpc_t*)conn;

	err = opt_rpc_getini(or, path, dat, sizeof(dat), ebuf, sizeof(ebuf));
	if (err != EC_OK)
		return err;

	/* FIXME */
	return EC_NG;
}

static int connect_and_hey(opt_rpc_t *or, unsigned short port,
		int isopt, int *retsock)
{
	char respbuf[4096];
	int s;

	*retsock = -1;
	if (!rpc_connect(or->server, port, &s)) {
		*retsock = s;
		if (!shake_hand(s, or->connhash, isopt, or->client_name,
					or->user_name, or->user_pass,
					respbuf, sizeof(respbuf))) {
			char *start = strstr(respbuf, "\r\n");
			or->prompt_len = strtol(start + 2, 0, 10);
			return 0;
		}
		close(s);
	}

	return -1;
}

static int connect_opt(opt_rpc_t *or)
{
	int err;
	err = connect_and_hey(or, or->port, 1, &or->opt_socket);
	return err;
}

static int connect_wch(opt_rpc_t *or)
{
	int err;
	if (!or->wch_func)
		return 0;

	err = connect_and_hey(or, or->port, 0, &or->wch_socket);
	if (!err) {
		or->wch_thread = spl_thread_create(
				watch_thread_or_client, (void*)or, 0);
		spl_sema_get(or->wch_running, -1);
	}

	return err;
}

static void ignore_pipe()
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, 0);
}

static void config_socket(int s)
{
	int yes = 1;
	struct linger lin;

	lin.l_onoff = 0;
	lin.l_linger = 0;
	setsockopt(s, SOL_SOCKET, SO_LINGER, (const char *) &lin, sizeof(lin));
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
}

void *opt_rpc_connect(const char *server, unsigned short port,
		void (*wfunc)(void *conn, const char *path),
		const char *client_name, const char *user_name,
		const char *user_pass)
{
	char connstr[256];
	static int callref = 0;
	pid_t pid = getpid();
	opt_rpc_t *or = (opt_rpc_t*)kmem_alloz(1, opt_rpc_t);

	or->wch_func = wfunc;
	or->opt_socket = -1;
	or->wch_socket = -1;
	strcpy(or->client_name, client_name);
	strcpy(or->user_name, user_name);
	strcpy(or->user_pass, user_pass);
	strcpy(or->server, server);
	or->port = port;

	ignore_pipe();

	sprintf(connstr, "%d-%d%d", pid, (unsigned int)time(NULL), callref++);
	md5_calculate(or->connhash, connstr, strlen(connstr));

	if (wfunc)
		or->wch_running = spl_sema_new(0);

	if (!connect_opt(or) && !connect_wch(or))
		return (void*)or;

	kerror("opt_rpc_connect: error\n");
	opt_rpc_disconnect(or);
	return NULL;
}

int opt_rpc_disconnect(void *conn)
{
	opt_rpc_t *or = (opt_rpc_t*)conn;

	if (!or)
		return -1;

	or->quit = ktrue;

	/* XXX: opt disconnect can cause wch_thread quit */
	if ((or->opt_socket != -1) && (!rpc_disconnect(or->opt_socket, ktrue)))
		or->opt_socket = -1;

	if (or->wch_thread)
		spl_thread_wait(or->wch_thread);

	assert(or->wch_socket == -1);

	if (or->wch_running)
		spl_sema_del(or->wch_running);

	kmem_free(or);
	return 0;
}

