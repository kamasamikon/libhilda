/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <hilda/klog.h>
#include <hilda/kopt.h>
#include <hilda/kopt-rpc-server.h>

static void logger_stdout(char *content, int len)
{
	printf("%s", content);
}

static int og_uptime(void *opt, void *pa, void *pb)
{
	char buffer[4096];

	kopt_set_cur_str(opt, buffer);

	return 0;
}
static int os_login(int ses, void *opt, void *pa, void *pb)
{
	return 0;
}

int main(int argc, char *argv[])
{
	char *s = "";
	int i = 0;

	klog_init(argc, argv);
	klog_add_logger(logger_stdout);

	kopt_init(argc, argv);
	kopt_rpc_server_init(9000, argc, argv);

	kopt_setfile("/tmp/myproj.opt");

	kopt_add_s("b:/sys/admin/xxx/enable", OA_DFT, NULL, NULL);
	kopt_setint("b:/sys/admin/xxx/enable", 1);
	kopt_add_s("s:/sys/usr/xxx/passwd", OA_DFT, NULL, NULL);
	kopt_setstr("s:/sys/usr/xxx/passwd", "xxx");

	kopt_add_s("p:/env/log/cc", OA_GET, NULL, NULL);
	kopt_setptr("p:/env/log/cc", 0);

	kopt_add_s("s:/sys/proc/uptime", OA_GET, NULL, og_uptime);

	kopt_add_s("b:/wushan/cmd/login", OA_DFT, os_login, NULL);

	for (;;) {
		klog("xxxxxxxxx\n");
		kerror("Error ...... \n");

		kopt_getstr("s:/a/b/c", &s);
		klog("SSSS: %s\n", s);

		kopt_getint("i:/a/b/c", &i);
		klog("IIII: %d\n", i);

		sleep(1);
	}
}

