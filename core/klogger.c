/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <hilda/helper.h>
#include <hilda/xtcool.h>
#include <hilda/klog.h>
#include <hilda/klogger.h>

/*
 * Log to stdout
 */
static void builtin_logger_stdout(char *content, int len)
{
	fprintf(stdout, "%s", content);
}
void klog_add_stdout_logger()
{
	klog_add_logger(builtin_logger_stdout);
}
void klog_del_stdout_logger()
{
	klog_del_logger(builtin_logger_stdout);
}


/*
 * Log to stderr
 */
static void builtin_logger_stderr(char *content, int len)
{
	fprintf(stderr, "%s", content);
}
void klog_add_stderr_logger()
{
	klog_add_logger(builtin_logger_stderr);
}
void klog_del_stderr_logger()
{
	klog_add_logger(builtin_logger_stderr);
}



/*
 * Log to file
 */
#include <pwd.h>

struct _filelog_info_s {
	char user[256];

	/*
	 * /tmp/xxlog-%U-%P_%N%Y%R%S%F%M.log
	 *
	 * N Y R S F M = Year Month Day Hours Minute Second
	 * U = USER
	 * P = PID
	 * I = INDEX
	 */
	char pathfmt[256];

	/* Converted path form pathfmt */
	char path[256];

	/* current file size */
	size_t filesize;

	FILE *fp;

	/* New when exceed the size */
	int maxsize;

	/* New when exceed the maxtime seconds lifetime of current log */
	int maxtime;

	/* New when now.seconds % save_at == 0 */
	int save_at;

	/* Start time of current log */
	time_t savetime;

	/* File name index */
	unsigned int index;
};

static struct _filelog_info_s __fi;

static void get_user_name()
{
	struct passwd *pass;

	pass = getpwuid(geteuid());
	if (pass)
		strcpy(__fi.user, pass->pw_name);
	else
		strcpy(__fi.user, "(zbd)");
}

static void set_file_path()
{
#define APPEND(fmt, val) bpos += sprintf(buf + bpos, fmt, val);

	char c, *p, buf[sizeof(__fi.path)];
	int percent, bpos;

	int use_index = 0;
	unsigned int index = __fi.index;

	time_t now = time(NULL);
	struct tm tm;
	if (NULL == localtime_r(&now, &tm)) {
		fprintf("ERR: localtime_r: %d\n", errno);
		memset(tm, 0, sizeof(tm));
	}

try:
	buf[0] = '\0';
	percent = 0;
	bpos = 0;

	for (p = __fi.pathfmt; *p; p++) {
		c = *p;

		if (percent) {
			switch (c) {
			case 'N':
				APPEND("%04d", tm.tm_year + 1900);
				break;
			case 'Y':
				APPEND("%02d", tm.tm_mon + 1);
				break;
			case 'R':
				APPEND("%02d", tm.tm_mday);
				break;
			case 'S':
				APPEND("%02d", tm.tm_hour);
				break;
			case 'F':
				APPEND("%02d", tm.tm_min);
				break;
			case 'M':
				APPEND("%02d", tm.tm_sec);
				break;
			case 'P':
				APPEND("%d", getpid());
				break;
			case 'U':
				APPEND("%s", __fi.user);
				break;
			case 'I':
				APPEND("%04d", index);
				use_index = 1;
				break;
			case '%':
				APPEND("%s", "%");
				break;
			default:
				fprintf(stderr, "Bad fmt: '%s'\n", __fi.pathfmt);
				break;
			}
			percent = 0;
		} else {
			if (c == '%') {
				percent = 1;
				continue;
			}
			APPEND("%c", c);
		}
	}

	if (use_index) {
		if (kvfs_exist(buf)) {
			index++;
			goto try;
		} else
			__fi.index = index + 1;
	}

	strcpy(__fi.path, buf);
	printf("PATH: '%s'\n", buf);
}

static void next_file(int append)
{
	set_file_path();

	if (__fi.fp)
		fclose(__fi.fp);
	__fi.fp = fopen(__fi.path, append ? "a" : "w");
	if (!__fi.fp) {
		fprintf(stderr, "fopen error: path:'%s', errno:%d\n", __fi.path, errno);
		return;
	}

	__fi.savetime = time(NULL);

	fseek(__fi.fp, 0, SEEK_END);
	__fi.filesize = ftell(__fi.fp);
	fseek(__fi.fp, 0, SEEK_SET);
}

static void builtin_logger_file(char *content, int len)
{
	int next = 0;

	if (!__fi.fp)
		next_file(1);

	if (__fi.maxsize > 0) {
		if (__fi.filesize + len > __fi.maxsize) {
			next = 1;
		}
	}
	if (__fi.maxtime > 0) {
		if (__fi.savetime + __fi.maxtime < time(NULL)) {
			next = 1;
		}
	}
	if (__fi.save_at > 0) {
		time_t now = time(NULL);
		time_t beg = now - (now % __fi.save_at);

		if (beg > __fi.savetime) {
			next = 1;
		}
	}

	if (next || !__fi.fp)
		next_file(1);

	if (!__fi.fp)
		return;

	fwrite(content, sizeof(char), len, __fi.fp);
	fflush(__fi.fp);
}
void klog_add_file_logger(const char *pathfmt, unsigned int size,
		unsigned int time, unsigned int when)
{
	get_user_name();

	strncpy(__fi.pathfmt, pathfmt, sizeof(__fi.pathfmt));
	__fi.maxsize = size;
	__fi.maxtime = time;
	__fi.save_at = when;
}


/*
 * Log to file
 */
void klog_add_network_logger(const char *addr, unsigned short port);
void klog_del_network_logger(void);

