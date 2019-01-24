/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <time.h>

#include <hilda/ktypes.h>
#include <hilda/hget.h>
#include <hilda/kbuf.h>

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
	static char *cmd = "hostname";

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
		strftime(v, sizeof(v), "%Y/%m/%d %H:%M:%S", localtime(&t));
	}

	kopt_set_cur_str(opt, v);
	return EC_OK;
}

static char* optstr(const char *path, const char *dft, int duptan)
{
	char *s;

	if (kopt_getstr(path, &s))
		s = (char*)dft;

	return duptan ? kstr_dup(s) : s;
}

static int optint(const char *path, int dft)
{
	int i;

	if (kopt_getint(path, &i))
		i = dft;

	return i;
}

static char *serviceTempl = " \
{ \
	\"ipAddr\": \"%s\", \
	\"port\": \"%d\", \
	\"hostName\": \"%s\", \
	\"projName\": \"%s\", \
	\"serviceName\": \"%s\", \
	\"version\": \"%s\", \
	\"desc\": \"%s\", \
	\"createdAt\": \"%s\" \
}";

int main(int argc, char *argv[])
{
	char *s = "";
	int i = 0;
	char *args[8];
	char *path;

	kchar _datbuf[4096] = { 0 }, *datbuf = _datbuf, *hdrbuf = NULL;
	kint datlen = sizeof(_datbuf), hdrlen = 0;
	kchar url[2048], *tmp;

	kbuf_s j;

	klog_init(argc, argv);
	klog_add_logger(logger_stdout);
	kopt_init(argc, argv);

	kbuf_init(&j, 2048);

	kopt_setfile("./msa.cfg");

	kopt_add_s("s:/msa/ipAddr", OA_DFT, NULL, og_ipaddress);
	kopt_add_s("s:/msa/hostName", OA_DFT, NULL, og_hostname);
	kopt_add_s("s:/msa/createdAt", OA_DFT, NULL, og_createAt);

	int wsPort = optint("i:/msa/ws/port", 9200);

	char *msbUrl = optstr("s:/msa/msb/url", "http://219.142.69.234:9080", 1);

	char *projName = optstr("s:/msa/build/dirname", "FIXME", 1);
	char *buildTime = optstr("s:/msa/build/time", "FIXME", 1);
	char *buildVersion = optstr("s:/msa/build/version", "FIXME", 1);

	int updated = optint("i:/msa/build/updated", 1);


	// # MS Info
	char *msDir = optstr("s:/msa/ms/dir", "/root/ms/", 1);
	char *msName = optstr("s:/msa/ms/name", "demo", 1);
	char *msVer = optstr("s:/msa/ms/version", "v1", 1);
	char *msDesc = optstr("s:/msa/ms/desc", "N/A", 1);

	int msPort = optint("i:/msa/ms/port", 8020);

	char *ipAddr = optstr("s:/msa/ipAddr", "", 1);
	char *hostName = optstr("s:/msa/hostName", "", 1);
	char *createdAt = optstr("s:/msa/createdAt", "", 1);

	kbuf_addf(&j, serviceTempl, ipAddr, msPort, hostName, projName, msName, msVer, msDesc, createdAt);

	klog(j.buf);

	sprintf(url, "http://127.0.0.1:9100/services");
	if (hget(url, knil, "POST", j.buf, &datbuf, &datlen,
				&hdrbuf, &hdrlen, NULL))
		return -1;


	// start MSA
	//

	args[0] = "ls";
	args[1] = NULL;
	spl_process_create(1, args);
	klog("\n");

	for (;;) {
		klog("xxxxxxxxx\n");
		kerror("Error ...... \n");

		if (!kopt_getstr("s:/a/b/c", &s))
			klog("SSSS: %s\n", s);

		if (!kopt_getint("i:/a/b/c", &i))
			klog("IIII: %d\n", i);

		/* requests.post */
		/*
		   hget(const char *a_url, const char *a_proxy, kbool a_get,
		   const char *a_cmd, char **a_datbuf, int *a_datlen,
		   char **a_hdrbuf, int *a_hdrlen, SOCKET *a_socket);
		   */

		sleep(1);
	}
}

