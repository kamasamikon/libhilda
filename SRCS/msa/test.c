/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <time.h>

#include <hilda/ktypes.h>
#include <hilda/hget.h>

#include <hilda/klog.h>
#include <hilda/kopt.h>
#include <hilda/kopt-rpc-server.h>

static void logger_stdout(char *content, int len)
{
	printf("%s", content);
}

static int og_hostname(void *opt, void *pa, void *pb)
{
	static char v[1024] = {0};
	static char *cmd = "cmd";

	if (v[0] == '\0')
		if (!popen_read(cmd, v, sizeof(v)))
			return EC_NG;

	kopt_set_cur_str(opt, v);
	return EC_OK;
}

static int og_ipaddress(void *opt, void *pa, void *pb)
{
	static char v[1024] = {0};
	static char *cmd = "ifconfig eth0 | grep 'inet addr' | tr ':' ' ' | awk '{print $3}'";

	if (v[0] == '\0')
		if (!popen_read(cmd, v, sizeof(v)))
			return EC_NG;

	kopt_set_cur_str(opt, v);
	return EC_OK;
}

static int og_createAt(void *opt, void *pa, void *pb)
{
	static char v[1024] = {0};

	if (v[0] == '\0') {
		time_t t = time(0);
		strftime(v, sizeof(v), "%Y/%m/%d %H:%M:%S",localtime(&t) );
	}

	kopt_set_cur_str(opt, v);
	return EC_OK;
}


int main(int argc, char *argv[])
{
	char *s = "";
	int i = 0;

	klog_init(argc, argv);
	klog_add_logger(logger_stdout);

	kopt_init(argc, argv);

	kopt_setfile("./msa.cfg");

	kopt_add_s("b:/xx/sss", OA_GET, og_hostname, NULL);

	// # bottle
	// ("WS_PORT", "i:/msa/ws/port", 9200),
	// ("WS_DEBUG", "b:/msa/ws/debug", True),

	// # MSB Info
	// ("MSB_URL", "s:/msa/msb/url", "http://219.142.69.234:9080"),

	// # Project
	// ("PROJ_NAME", "s:/msa/build/dirname", "FIXME"),
	// ("BUILD_TIME", "s:/msa/build/time", "FIXME"),
	// ("BUILD_VERSION", "s:/msa/build/version", "FIXME"),
	// ("BUILD_UPDATED", "b:/msa/build/updated", 1),

	// # MS Info
	// ("MS_DIR", "s:/msa/ms/dir", "/root/ms/"),
	// ("MS_NAME", "s:/msa/ms/name", "demo"),
	// ("MS_VER", "s:/msa/ms/version", "v1"),
	// ("MS_PORT", "i:/msa/ms/port", 8020),
	// ("MS_DESC", "s:/msa/ms/desc", "N/A"),


	// start MSA
	//

	spl_process_create();

	for (;;) {
		klog("xxxxxxxxx\n");
		kerror("Error ...... \n");

		kopt_getstr("s:/a/b/c", &s);
		klog("SSSS: %s\n", s);

		kopt_getint("i:/a/b/c", &i);
		klog("IIII: %d\n", i);

		/* requests.post */
		hget(const char *a_url, const char *a_proxy, kbool a_get,
		const char *a_cmd, char **a_datbuf, int *a_datlen,
		char **a_hdrbuf, int *a_hdrlen, SOCKET *a_socket);


		sleep(1);
	}
}

