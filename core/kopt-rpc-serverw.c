/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

/**
 * \file opt-rpc.c
 * \brief Program other then decoder or encoder use this to
 * access D/E opt database.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

#include <windows.h>
#include <winsock.h>

#include <stdarg.h>
#include <assert.h>

#include <hilda/ktypes.h>

#include <hilda/karg.h>
#include <hilda/klog.h>
#include <hilda/kmem.h>
#include <hilda/kstr.h>
#include <hilda/sdlist.h>

#include <hilda/helper.h>
#include <hilda/kopt.h>

#include <hilda/xtcool.h>

#include <hilda/kopt-rpc-common.h>
#include <hilda/kopt-rpc-server.h>
#include <hilda/kmque.h>

typedef struct _rpc_client_t rpc_client_t;
typedef struct _rpc_wch_t rpc_wch_t;

struct _rpc_wch_t {
	char *path;
	void *wch;
};

struct _rpc_client_t {
	char prompt[128];
	char connhash[33];

	int opt_socket;
	int wch_socket;

	struct {
		rpc_wch_t *arr;
		int cnt;
	} opts;			/* only these opt can be accessed */
};

#define BACKLOG 50

#define CRLF "\r\n"
#define PROMPT "$ "

static int send_watch_message(rpc_client_t *c, const char *buf);
static void config_socket(int s);
static void ignore_pipe();
static int process_connect(int new_fd);
static void close_client(rpc_client_t *c);

static rpc_client_t __g_clients[BACKLOG];

#define KOPT_PORT 9000

#define close CloseHandle

#define FD_MAX 64
static int __fds[FD_MAX];

void select_init()
{
	int i;
	for (i = 0; i < FD_MAX; i++)
		__fds[i] = -1;
}

void select_add_fd(int fd)
{
	int i;
	for (i = 0; i < FD_MAX; i++) {
		if (__fds[i] == -1) {
			__fds[i] = fd;
			break;
		}
	}
}

void select_del_fd(int fd)
{
	int i;
	for (i = 0; i < FD_MAX; i++) {
		if (__fds[i] == fd) {
			__fds[i] = -1;
			break;
		}
	}
}

int select_build(fd_set *set)
{
	int i, max = -1;

	FD_ZERO(set);
	for (i = 0; i < FD_MAX; i++) {
		if (__fds[i] == -1)
			continue;
		if (__fds[i] > max)
			max = __fds[i];
		FD_SET(__fds[i], set);
	}

	return max;
}

static char *mk_errline(int ret, char ebuf[])
{
	if (EC_OK == ret)
		sprintf(ebuf, "0 OK%s", CRLF);
	else if (ret == EC_NG)
		sprintf(ebuf, "1 NG%s", CRLF);
	else if (ret == EC_NOIMPL)
		sprintf(ebuf, "1 NOT IMPL%s", CRLF);
	else if (ret == EC_BAD_TYPE)
		sprintf(ebuf, "1 BAD TYPE%s", CRLF);
	else if (ret == EC_RECUR)
		sprintf(ebuf, "1 RECUR%s", CRLF);
	else if (ret == EC_NOINIT)
		sprintf(ebuf, "1 NOT INIT%s", CRLF);
	else if (ret == EC_NOTFOUND)
		sprintf(ebuf, "1 NOT FOUND%s", CRLF);
	else if (ret == EC_FORBIDEN)
		sprintf(ebuf, "1 FORBIDEN%s", CRLF);
	else if (ret == EC_COMMAND)
		sprintf(ebuf, "1 BAD COMMAND%s", CRLF);
	else if (ret == EC_NOTHING)
		sprintf(ebuf, "1 NOTHING BE DONE%s", CRLF);
	else if (ret == EC_EXIST)
		sprintf(ebuf, "1 ENTRY ALREADY EXIST%s", CRLF);
	else
		sprintf(ebuf, "1 UNKOWN ERROR%s", CRLF);

	return ebuf;
}

static rpc_client_t *rpc_client_get(const char *connhash, int fd, int is_opt)
{
	rpc_client_t *c = __g_clients;
	int i, arr_size = sizeof(__g_clients) / sizeof(__g_clients[0]);

	for (i = 0; i < arr_size; i++)
		if (!strcmp(c[i].connhash, connhash))
			break;

	if (i == arr_size)
		for (i = 0; i < arr_size; i++) {
			/* all the free client set connhash[0] to be zero */
			if (c[i].connhash[0] == '\0') {
				c[i].opt_socket = -1;
				c[i].wch_socket = -1;
				break;
			}
		}
	if (i == arr_size)
		return NULL;

	strcpy(c[i].connhash, connhash);
	if (is_opt) {
		assert(-1 == c[i].opt_socket);
		c[i].opt_socket = fd;
	} else {
		assert(-1 == c[i].wch_socket);
		c[i].wch_socket = fd;
	}

	return &c[i];
}

static rpc_client_t *rpc_client_by_socket(int s_opt)
{
	rpc_client_t *c = __g_clients;
	int i, arr_size = sizeof(__g_clients) / sizeof(__g_clients[0]);

	for (i = 0; i < arr_size; i++) {
		if (c[i].connhash[0] == '\0')
			continue;

		if (s_opt == c[i].opt_socket)
			return c + i;
	}

	return NULL;
}

static int rpc_client_wch_find(rpc_client_t *c, char *path)
{
	int i;
	const char *tmp;

	for (i = 0; i < c->opts.cnt; i++) {
		tmp = c->opts.arr[i].path;
		if (tmp && (0 == strcmp(tmp, path)))
			return i;
	}

	return -1;
}

static int rpc_client_wch_add(rpc_client_t *c, char *path, void *wch)
{
	int i;
	const char *tmp;

	for (i = 0; i < c->opts.cnt; i++) {
		tmp = c->opts.arr[i].path;
		if (tmp && (0 == strcmp(tmp, path))) {
			kerror("%s already\n", path);
			return -1;
		}
	}

	for (i = 0; i < c->opts.cnt; i++)
		if (!c->opts.arr[i].path) {
			c->opts.arr[i].path = kstr_dup(path);
			c->opts.arr[i].wch = wch;
			return 0;
		}

	ARR_INC(2, c->opts.arr, c->opts.cnt, rpc_wch_t);
	c->opts.arr[i].path = kstr_dup(path);
	c->opts.arr[i].wch = wch;

	return 0;
}

static int rpc_client_wch_del(rpc_client_t *c, const char *path)
{
	int i;
	const char *tmp;

	for (i = 0; i < c->opts.cnt; i++) {
		tmp = c->opts.arr[i].path;
		if (tmp && (0 == strcmp(tmp, path))) {
			kmem_free_sz(c->opts.arr[i].path);
			kopt_wch_del(c->opts.arr[i].wch);
			c->opts.arr[i].wch = NULL;
			klog("rpc_client_wch_del: %s deleted\n", path);
			return 0;
		}
	}

	kerror("%s not exists.\n", path);
	return -1;
}

static int rpc_client_wch_clr(rpc_client_t *c)
{
	int i;

	for (i = 0; i < c->opts.cnt; i++) {
		kmem_free_s(c->opts.arr[i].path);
		if (c->opts.arr[i].wch)
			kopt_wch_del(c->opts.arr[i].wch);
		c->opts.arr[i].wch = NULL;
	}
	kmem_free_sz(c->opts.arr);
	c->opts.cnt = 0;

	return 0;
}

static void rpc_watch(int ses, void *opt, void *wch)
{
	void *ua = wch_ua(wch);
	char msg[16384], *ini, *path = kopt_path(wch);
	int bytes;

	klog("~rpc_watch:path:%s\n", path);

	if (kopt_getini_by_opt(opt, &ini))
		return;

	bytes = sprintf(msg, "wchnotify %s\r\n%s", path, ini);
	if (send_watch_message((rpc_client_t*)ua, msg))
		close_client((rpc_client_t*)ua);

	kmem_free(ini);
}

/*-----------------------------------------------------------------------
 * Server
 */
static int do_opt_command(int s, char *buf, int cmdlen)
{
	rpc_client_t *c;
	char *para, ebuf[256], *errmsg;
	int ret, errnum;

	/* XXX: some client won't append NUL to end of input */
	buf[cmdlen] = '\0';
	kstr_trim(buf);
	wlogf(">> opt-rpc >>%s\n", buf);

	c = rpc_client_by_socket(s);
	if (!c) {
		kerror("no client found for socket %d\n", s);
		return 1;
	}

	if (!strncmp("wa ", buf, 3)) {
		para = buf + 3;
		if (-1 != rpc_client_wch_find(c, para))
			sprintf(buf, "%s%s", mk_errline(EC_EXIST, ebuf), c->prompt);
		else {
			void *wch = NULL;
			if ((c->wch_socket != -1))
				wch = kopt_awch_u(para, rpc_watch, (void*)c, NULL);
			else
				kerror("wchadd while on wfunc set in c side\n");
			if (!wch)
				sprintf(buf, "%s%s", mk_errline(EC_NG, ebuf), c->prompt);
			else {
				ret = rpc_client_wch_add(c, para, wch);
				sprintf(buf, "%s%s", mk_errline(ret, ebuf), c->prompt);
			}
		}
	} else if (!strncmp("wd ", buf, 3)) {
		para = buf + 3;
		ret = rpc_client_wch_del(c, para);
		sprintf(buf, "%s%s", mk_errline(ret, ebuf), c->prompt);
	} else if (!strncmp("os ", buf, 3)) {
		para = buf + 3;
		ret = kopt_setbat(para, 1);
		if (ret && !kopt_get_err(&errnum, &errmsg))
			sprintf(buf, "%x %s%s%s", errnum, errmsg, CRLF, c->prompt);
		else
			sprintf(buf, "%s%s", mk_errline(ret, ebuf), c->prompt);
		klog("opt-rpc: optset: ret:%d, buf:%s\n", ret, buf);
	} else if (!strncmp("og ", buf, 3)) {
		char *iniret = NULL;
		para = buf + 3;
		ret = kopt_getini(para, &iniret);
		if (ret && !kopt_get_err(&errnum, &errmsg))
			sprintf(buf, "%x %s%s%s", errnum, errmsg, CRLF, c->prompt);
		else
			sprintf(buf, "%s%s%s", mk_errline(ret, ebuf), iniret ? iniret : "", c->prompt);
		kmem_free_s(iniret);
		klog("opt-rpc: optget: ret:%d, buf:%s\n", ret, buf);
	} else if (!strncmp("bye", buf, 3)) {
		return 1;
	} else if (!strncmp("help", buf, 4)) {
		sprintf(buf, "help(), hey(mode<o|w>, client, connhash, user, pass), bye(), wa(opt), wd(opt), os(ini), og(opt)%s",
				c->prompt);
	} else {
		sprintf(buf, "%s%s", mk_errline(EC_NOTHING, ebuf), c->prompt);
	}

	ret = send(c->opt_socket, buf, strlen(buf) + 1, 0);
	if (ret < 0) {
		klog("do_opt_command: send resp: s: %d, err %s\n", s, strerror(errno));
		return 1;
	} else if (ret == 0) {
		klog("do_opt_command: send resp: Remote close socket: %d\n", s);
		return 1;
	}

	return 0;
}

static void close_connect(int s)
{
	rpc_client_t *c = rpc_client_by_socket(s);
	if (c)
		close_client(c);
	else
		close((HANDLE)s);
}

static void close_client(rpc_client_t *c)
{
	select_del_fd(c->opt_socket);

	if (c->opt_socket != -1) {
		close((HANDLE)(c->opt_socket));
		kopt_setstr("s:/k/opt/rpc/o/disconnect", c->connhash);
	}

	if (c->wch_socket != -1) {
		close((HANDLE)(c->wch_socket));
		kopt_setstr("s:/k/opt/rpc/w/disconnect", c->connhash);
	}

	rpc_client_wch_clr(c);

	c->wch_socket = c->opt_socket = -1;
	c->connhash[0] = '\0';
}

static void *worker_thread_or_server(void *userdata)
{
	unsigned int i;
	int ready, n;
	char buf[8192 * 8];

	fd_set fdset;

	unsigned short port = (unsigned short)(int)userdata;

	int s_listen, s_new;
	struct sockaddr_in their_addr;
	struct sockaddr_in my_addr;
	int sin_size;

	ignore_pipe();

	if ((s_listen = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		kerror("c:%s, e:%s\n", "socket", strerror(errno));
		return NULL;
	}

	config_socket(s_listen);

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));
	if (bind(s_listen, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1) {
		kerror("c:%s, e:%s\n", "bind", strerror(errno));
		return NULL;
	}

	if (listen(s_listen, BACKLOG) == -1) {
		kerror("c:%s, e:%s\n", "listen", strerror(errno));
		return NULL;
	}

	select_add_fd(s_listen);

	for (;;) {
		struct timeval tv;
		int max = select_build(&fdset);

		tv.tv_sec = 600;
		tv.tv_usec = 0;

		ready = select(max + 1, &fdset, 0, 0, &tv);
		if (ready <= 0)
			continue;

		for (i = 0; i < fdset.fd_count; i++) {
			int fd = fdset.fd_array[i];

			if (fd == s_listen) {

				sin_size = sizeof(their_addr);
				if ((s_new = accept(s_listen, (struct sockaddr*)&their_addr, &sin_size)) == -1) {
					kerror("c:%s, e:%s\n", "accept", strerror(errno));
				} else if (process_connect(s_new)) /* XXX: s_new can be o or w */
					close_connect(s_new);
				continue;
			}

			if ((n = recv(fd, buf, sizeof(buf), 0)) > 0) {
				if (do_opt_command(fd, buf, n)) {
					select_del_fd(fd);
					close_connect(fd);
				}
			} else {
				klog("Remote close socket: %d\n", fd);
				select_del_fd(fd);
				close_connect(fd);
			}
		}
	}
}

int kopt_rpc_server_init(unsigned short port, int argc, char *argv[])
{
	int i;

	i = arg_find(argc, argv, "--or-port", 1);
	if (i > 0 && (i + 1) < argc) {
		int tmp;
		if (!kstr_toint(argv[i + 1], &tmp))
			port = (unsigned short)tmp;
	}

	select_init();
	ignore_pipe();
	spl_thread_create(worker_thread_or_server, (void *)(int)port, 0);
	return 0;
}

int kopt_rpc_server_final()
{
	return 0;
}

static int send_watch_message(rpc_client_t *c, const char *buf)
{
	int ret;

	klog("send_watch_message: s:<%d>, buf:<%s>\n", c->wch_socket, buf);

	ret = send(c->wch_socket, buf, strlen(buf) + 1, 0);
	if (0 == ret || (-1 == c->wch_socket)) {
		/* socket disconnected */
		kerror("c:%s, e:%s\n", "send", strerror(errno));
		return -1;
	} else if (-1 == ret) {
		kerror("c:%s, e:%s\n", "send", strerror(errno));
		return -1;
	}

	ret = recv(c->wch_socket, (void*)buf, sizeof(buf), 0);
	if (0 == ret) {
		kerror("c:%s, e:%s\n", "recv", strerror(errno));
		return -1;
	}
	if (-1 == ret) {
		kerror("c:%s, e:%s\n", "recv", strerror(errno));
		return -1;
	}

	return 0;
}

static int check_authority(const char mode, const char *rpc_client,
		const char *connhash, const char *user, const char *pass)
{
	int enable;
	char buffer[1024], *sv;

	sprintf(buffer, "b:/sys/admin/%s/enable", rpc_client);
	if (kopt_getint(buffer, &enable) || !enable) {
		kerror("Bad client <%s> or client disabled\n", rpc_client);
		return -1;
	}

	sprintf(buffer, "s:/sys/usr/%s/passwd", user);
	klog("\n\nFor remote administrator, must check user name and passwd\n");
	if (!kopt_getstr(buffer, &sv)) {
		if (!strcmp(sv, pass))
			return 0;
		kerror("Get password for user '%s' failed\n", user);
		kerror("Should be '%s', but input '%s'\n", sv, pass);
	}

	return -1;
}

static int process_connect(int new_fd)
{
	char buf[1024], *cmd;
	int ret, try_cnt, ofsarr[80], argc;
	rpc_client_t *client;

	try_cnt = 10;
	while (try_cnt--) {

		buf[0] = '\0';
		ret = recv(new_fd, buf, sizeof(buf), 0);
		if (0 == ret) {
			kerror("c:%s, e:%s\n", "recv", strerror(errno));
			break;
		}
		if (-1 == ret) {
			kerror("c:%s, e:%s\n", "recv", strerror(errno));
			continue;
		}

		buf[ret] = '\0';
		argc = get_argv(buf, ofsarr);
		if (0 == argc) {
			sprintf(buf, PROMPT);
			send(new_fd, buf, strlen(buf) + 1, 0);
			continue;
		}

		cmd = buf + ofsarr[0];

		if ((argc > 5) && (0 == strcmp("hey", cmd))) {
			char buffer[1024];
			char mode = (buf + ofsarr[1])[0];
			char *rpc_client = buf + ofsarr[2];
			char *connhash = buf + ofsarr[3];
			char *user = buf + ofsarr[4];
			char *pass = buf + ofsarr[5];

			wlogf("---------------------------\n");
			wlogf("\tsocket: %d\n", new_fd);
			wlogf("\tclient_mode: %s\n", mode == 'o' ? "Opt" : "Wch");
			wlogf("\tclient_name: %s\n", rpc_client);
			wlogf("\tconn_hash: %s\n", connhash);
			wlogf("\tuser_name: %s\n", user);
			wlogf("\tuser_pass: %s\n", pass);
			wlogf("---------------------------\n");

			if (check_authority(mode, rpc_client, connhash, user, pass))
				return -1;

			client = rpc_client_get(connhash, new_fd, (mode == 'o'));
			if (client) {
				sprintf(client->prompt, "\r\n(%s)%s", rpc_client, PROMPT);

				if (mode == 'o')
					kopt_setstr("s:/k/opt/rpc/o/connect", connhash);
				else
					kopt_setstr("s:/k/opt/rpc/w/connect", connhash);

				/* XXX: w socket can be process insite */
				if (mode == 'o') {
					select_add_fd(new_fd);
				}

				/* send the ACK */
				sprintf(buf, "%s%d%s", mk_errline(0, buffer), strlen(client->prompt), client->prompt);
				send(new_fd, buf, strlen(buf) + 1, 0);

				return 0;
			} else
				kerror("rpc_client_get return NULL, increase the size of __g_clients\n");
		}

		if (!strncmp("help", cmd, 4))
			sprintf(buf, "help(), hey(mode<o|w>, client, connhash, user, pass), bye(), wa(opt), wd(opt), os(ini), og(opt)%s%s",
					CRLF, PROMPT);
		else
			sprintf(buf, "%s: bad command" CRLF PROMPT, cmd);
		send(new_fd, buf, strlen(buf) + 1, 0);
	}

	return -1;
}

static void config_socket(int s)
{
#if 0
	int yes = 1;
	struct linger lin;

	lin.l_onoff = 0;
	lin.l_linger = 0;
	setsockopt(s, SOL_SOCKET, SO_LINGER, (const char*)&lin, sizeof(lin));
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
#endif
}

static void ignore_pipe()
{
}

