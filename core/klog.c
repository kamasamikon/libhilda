/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <time.h>
#include <sys/inotify.h>

#include <hilda/helper.h>
#include <hilda/kmem.h>
#include <hilda/kflg.h>
#include <hilda/kstr.h>
#include <hilda/xtcool.h>
#include <hilda/karg.h>
#include <hilda/klog.h>
#include <hilda/kbuf.h>

extern ssize_t getline(char **lineptr, size_t *n, FILE *stream);

static void *klog_cc(void);
static void klog_init_default(void);

static char *get_execpath(int *size);
static char *get_basename(char *name);
static char *get_progname();

static unsigned int __dft_mask = KLOG_ALL;

/*-----------------------------------------------------------------------
 * Local definition:
 */

/* STRing ARRay to save programe name, module name, file name etc */
typedef struct _strarr_s strarr_s;
struct _strarr_s {
	int size;
	int cnt;
	char **arr;
};

typedef struct _rule_s rule_s;
struct _rule_s {
	/* XXX: Don't filter ThreadID, it need the getflg called every log */

	/* 0 is know care */
	/* -1 is all */

	/* XXX: the prog modu file func is return from strarr_add */
	char *prog;	/* Program command line */
	char *modu;	/* Module name */
	char *file;	/* File name */
	char *func;	/* Function name */

	int line;	/* Line number */

	int pid;	/* Process ID */

	/* Which flag to be set or clear */
	unsigned int set, clr;
};

typedef struct _rulearr_s rulearr_s;
struct _rulearr_s {
	unsigned int size;
	unsigned int cnt;
	rule_s *arr;
};

/* How many logger slot */
#define MAX_NLOGGER 8
#define MAX_RLOGGER 8

/* Control Center for klog */
typedef struct _klogcc_s klogcc_s;
struct _klogcc_s {
	/** version is a ref count user change klog arg */
	int touches;

	pid_t pid;

	SPL_HANDLE mutex;

	strarr_s arr_file_name;
	strarr_s arr_modu_name;
	strarr_s arr_prog_name;
	strarr_s arr_func_name;

	rulearr_s arr_rule;

	unsigned char nlogger_cnt, rlogger_cnt;
	KNLOGGER nloggers[MAX_NLOGGER];
	KRLOGGER rloggers[MAX_RLOGGER];
};

static klogcc_s *__g_klogcc = NULL;

/*-----------------------------------------------------------------------
 * Control Center
 */
static void *klog_cc(void)
{
	if (unlikely(!__g_klogcc))
		klog_init_default();

	return (void*)__g_klogcc;
}

/*-----------------------------------------------------------------------
 * klog-logger
 */
int klog_add_logger(KNLOGGER logger)
{
	klogcc_s *cc = (klogcc_s*)klog_cc();
	int i;

	for (i = 0; i < MAX_NLOGGER; i++)
		if (cc->nloggers[i] == logger)
			return 0;

	if (cc->nlogger_cnt >= MAX_NLOGGER) {
		wlogf("klog_add_logger: Only up to %d logger supported.\n", MAX_NLOGGER);
		return -1;
	}

	cc->nloggers[cc->nlogger_cnt++] = logger;
	return 0;
}

int klog_del_logger(KNLOGGER logger)
{
	klogcc_s *cc = (klogcc_s*)klog_cc();
	int i;

	for (i = 0; i < cc->nlogger_cnt; i++)
		if (cc->nloggers[i] == logger) {
			cc->nlogger_cnt--;
			cc->nloggers[i] = cc->nloggers[cc->nlogger_cnt];
			cc->nloggers[cc->nlogger_cnt] = NULL;
			return 0;
		}

	return -1;
}

int klog_add_rlogger(KRLOGGER logger)
{
	klogcc_s *cc = (klogcc_s*)klog_cc();
	int i;

	for (i = 0; i < MAX_RLOGGER; i++)
		if (cc->rloggers[i] == logger)
			return 0;

	if (cc->rlogger_cnt >= MAX_RLOGGER) {
		wlogf("klog_add_logger: Only up to %d logger supported.\n", MAX_RLOGGER);
		return -1;
	}

	cc->rloggers[cc->rlogger_cnt++] = logger;
	return 0;
}

int klog_del_rlogger(KRLOGGER logger)
{
	klogcc_s *cc = (klogcc_s*)klog_cc();
	int i;

	for (i = 0; i < cc->rlogger_cnt; i++)
		if (cc->rloggers[i] == logger) {
			cc->rlogger_cnt--;
			cc->rloggers[i] = cc->rloggers[cc->rlogger_cnt];
			cc->rloggers[cc->rlogger_cnt] = NULL;
			return 0;
		}

	return -1;
}

/*-----------------------------------------------------------------------
 * implementation
 */
static void klog_parse_mask(char *mask, unsigned int *set, unsigned int *clr);

/**
 * \brief Other module call this to use already inited CC
 *
 * It should be called by other so or dll.
 */
void *klog_attach(void *logcc)
{
	__g_klogcc = (klogcc_s*)logcc;
	return (void*)__g_klogcc;
}

static char *get_execpath(int *size)
{
	kbuf_s kb;
	FILE *fp;
	char buff[256];
	int err = 0;
	char *path = NULL;

	sprintf(buff, "/proc/%d/cmdline", getpid());
	fp = fopen(buff, "rt");
	if (fp) {
		kbuf_init(&kb, 0);

		while (kbuf_fread(&kb, 1024, fp, &err) > 0 && !err)
			;
		fclose(fp);

		if (size)
			*size = kb.len;
		kb.buf[kb.len] = '\0';

		path = kbuf_detach(&kb, NULL);
		kbuf_release(&kb);
	}

	return path;
}

static void klog_init_default()
{
	int argc, cl_size = 0;
	char **argv, *cl_buf;

	if (__g_klogcc)
		return;

	argc = 0;
	argv = NULL;

	cl_buf = spl_get_cmdline(&cl_size);
	karg_build_nul(cl_buf, cl_size, &argc, &argv);
	kmem_free(cl_buf);

	klog_init(argc, argv);
	karg_free(argc, argv);
}

void klog_touch(void)
{
	klogcc_s *cc = (klogcc_s*)klog_cc();

	cc->touches++;
}
int klog_touches(void)
{
	klogcc_s *cc = (klogcc_s*)klog_cc();

	return cc->touches;
}

static char *get_basename(char *name)
{
	char *dupname, *bn;

	if (!name)
		return strdup("");

	dupname = strdup(name);
	bn = basename(dupname);

	bn = bn ? strdup(bn) : strdup("");

	kmem_free_s(dupname);

	return bn;
}

static char *get_progname()
{
	static char prog_name_buff[64];
	static char *prog_name = NULL;

	char *execpath, *execname;

	if (likely(prog_name))
		return prog_name;

	execpath = get_execpath(NULL);
	if (execpath) {
		execname = get_basename(execpath);
		strncpy(prog_name_buff, execname, sizeof(prog_name_buff));
		kmem_free(execpath);
		kmem_free(execname);
	} else
		snprintf(prog_name_buff, sizeof(prog_name_buff) - 1,
				"PROG-%d", (int)getpid());

	prog_name = prog_name_buff;
	return prog_name;
}

static void load_cfg_file(char *path)
{
	char *line = NULL;
	size_t len = 0;
	ssize_t bytes;

	FILE *fp;

	fp = fopen(path, "rt");
	if (!fp)
		return;

	while ((bytes = getline(&line, &len, fp)) != -1)
		klog_rule_add(line);

	fclose(fp);
	kmem_free_s(line);
}

static void apply_rtcfg(char *path)
{
	char *line = NULL;
	size_t len = 0;
	ssize_t bytes;

	static int line_applied = 0;
	int line_idx = 0;
	FILE *fp;

	fp = fopen(path, "rt");
	if (!fp)
		return;

	while ((bytes = getline(&line, &len, fp)) != -1) {
		if (line_idx++ < line_applied)
			continue;
		klog_rule_add(line);
		line_applied++;
	}

	fclose(fp);
	kmem_free_s(line);
}

static void* thread_monitor_cfgfile(void *user_data)
{
	char *path = (char*)user_data;
	int fd, wd, len, tmp_len;
	char buffer[2048], *offset = NULL;
	struct inotify_event *event;

	fd = inotify_init();
	if (fd < 0)
		goto quit;

	FILE *fp = fopen(path, "a+");
	if (fp)
		fclose(fp);

	wd = inotify_add_watch(fd, path, IN_MODIFY);
	if (wd < 0)
		goto quit;

	while ((len = read(fd, buffer, sizeof(buffer)))) {
		offset = buffer;
		event = (struct inotify_event*)(void*)buffer;

		while (((char*)event - buffer) < len) {
			if (event->wd == wd) {
				if (IN_MODIFY & event->mask)
					apply_rtcfg(path);
				break;
			}

			tmp_len = sizeof(struct inotify_event) + event->len;
			event = (struct inotify_event*)(void*)(offset + tmp_len);
			offset += tmp_len;
		}
	}

quit:
	free((void*)path);
	return NULL;
}

static void process_cfg(int argc, char *argv[])
{
	char *cfg;
	int i;

	/*
	 * 1. Default configure file
	 */
	/* Load configure from env, DeFault ConFiGure */
	cfg = getenv("KLOG_DFCFG");
	if (cfg)
		load_cfg_file(cfg);

	/* Load configure from command line */
	i = karg_find(argc, argv, "--klog-dfcfg", 1);
	if (i > 0)
		load_cfg_file(argv[i + 1]);

	/*
	 * 2. Runtime configure file
	 */
	cfg = NULL;
	i = karg_find(argc, argv, "--klog-rtcfg", 1);
	if (i > 0)
		cfg = argv[i + 1];

	if (!cfg)
		cfg = getenv("KLOG_RTCFG");
	if (cfg)
		spl_thread_create(thread_monitor_cfgfile, strdup(cfg), 0);
}

static void rule_add_from_mask(unsigned int mask)
{
	char rule[256];
	int i;

	strcpy(rule, "mask=");
	i = 5;

	if (mask & KLOG_FATAL)
		rule[i++] = 'f';
	if (mask & KLOG_ALERT)
		rule[i++] = 'a';
	if (mask & KLOG_CRIT)
		rule[i++] = 'c';
	if (mask & KLOG_ERR)
		rule[i++] = 'e';
	if (mask & KLOG_WARNING)
		rule[i++] = 'w';
	if (mask & KLOG_NOTICE)
		rule[i++] = 'n';
	if (mask & KLOG_INFO)
		rule[i++] = 'i';
	if (mask & KLOG_DEBUG)
		rule[i++] = 'd';

	if (mask & KLOG_RTM)
		rule[i++] = 's';
	if (mask & KLOG_ATM)
		rule[i++] = 'S';

	if (mask & KLOG_PID)
		rule[i++] = 'j';
	if (mask & KLOG_TID)
		rule[i++] = 'x';

	if (mask & KLOG_LINE)
		rule[i++] = 'N';
	if (mask & KLOG_FILE)
		rule[i++] = 'F';
	if (mask & KLOG_MODU)
		rule[i++] = 'M';
	if (mask & KLOG_FUNC)
		rule[i++] = 'H';
	if (mask & KLOG_PROG)
		rule[i++] = 'P';

	rule[i++] = '\0';
	klog_rule_add(rule);
}

void klog_set_default_mask(unsigned int mask)
{
	__dft_mask = mask;
}

static void klog_cleanup()
{
	klogcc_s *cc = (klogcc_s*)klog_cc();
	int i;

	for (i = 0; i < cc->arr_file_name.cnt; i++)
		kmem_free_s((void*)cc->arr_file_name.arr[i]);
	kmem_free_s((void*)cc->arr_file_name.arr);

	for (i = 0; i < cc->arr_modu_name.cnt; i++)
		kmem_free_s((void*)cc->arr_modu_name.arr[i]);
	kmem_free_s((void*)cc->arr_modu_name.arr);

	for (i = 0; i < cc->arr_prog_name.cnt; i++)
		kmem_free_s((void*)cc->arr_prog_name.arr[i]);
	kmem_free_s((void*)cc->arr_prog_name.arr);

	for (i = 0; i < cc->arr_func_name.cnt; i++)
		kmem_free_s((void*)cc->arr_func_name.arr[i]);
	kmem_free_s((void*)cc->arr_func_name.arr);

	kmem_free_s((void*)cc->arr_rule.arr);
}

void *klog_init(int argc, char **argv)
{
	klogcc_s *cc;

	if (__g_klogcc)
		return (void*)__g_klogcc;

	cc = (klogcc_s*)kmem_alloz(1, klogcc_s);
	__g_klogcc = cc;

	cc->mutex = spl_mutex_create();
	cc->pid = getpid();

	/* Set default before configure file */
	if (__dft_mask)
		rule_add_from_mask(__dft_mask);

	process_cfg(argc, argv);

	klog_touch();

	atexit(klog_cleanup);

	return (void*)__g_klogcc;
}

int klog_vf(unsigned char type, unsigned int mask, char *prog, char *modu,
		char *file, char *func, int ln, const char *fmt, va_list ap)
{
	klogcc_s *cc = (klogcc_s*)klog_cc();
	va_list ap_copy0, ap_copy1;

	char buffer[4096], *bufptr = buffer;
	int i, ret, ofs, bufsize = sizeof(buffer);

	char tmbuf[128];
	time_t t;
	struct tm *tmp;

	unsigned long tick = 0;

	for (i = 0; i < cc->rlogger_cnt; i++)
		if (cc->rloggers[i]) {
			va_copy(ap_copy1, ap);
			cc->rloggers[i](type, mask, prog, modu, file, func, ln, fmt, ap_copy1);
			va_end(ap_copy1);
		}

	if (unlikely(!cc->nlogger_cnt))
		return 0;

	if (mask & (KLOG_RTM | KLOG_ATM))
		tick = spl_get_ticks();

	ofs = 0;

	/* Type */
	if (likely(type))
		ofs += sprintf(bufptr, "|%c|", type);

	/* Time */
	if (mask & KLOG_RTM)
		ofs += sprintf(bufptr + ofs, "s:%lu|", tick);
	if (mask & KLOG_ATM) {
		t = time(NULL);
		tmp = localtime(&t);
		strftime(tmbuf, sizeof(tmbuf), "%Y/%m/%d %H:%M:%S", tmp);
		ofs += sprintf(bufptr + ofs, "S:%s.%03d|", tmbuf, (unsigned int)(tick % 1000));
	}

	/* ID */
	if (mask & KLOG_PID)
		ofs += sprintf(bufptr + ofs, "j:%d|", (int)cc->pid);
	if (mask & KLOG_TID)
		ofs += sprintf(bufptr + ofs, "x:%x|", (int)(long)spl_thread_current());

	/* Name and LINE */
	if ((mask & KLOG_PROG) && prog)
		ofs += sprintf(bufptr + ofs, "P:%s|", prog);

	if ((mask & KLOG_MODU) && modu)
		ofs += sprintf(bufptr + ofs, "M:%s|", modu);

	if ((mask & KLOG_FILE) && file)
		ofs += sprintf(bufptr + ofs, "F:%s|", file);

	if ((mask & KLOG_FUNC) && func)
		ofs += sprintf(bufptr + ofs, "H:%s|", func);

	if (mask & KLOG_LINE)
		ofs += sprintf(bufptr + ofs, "L:%d|", ln);

	if (likely(ofs))
		ofs += sprintf(bufptr + ofs, " ");

	va_copy(ap_copy0, ap);
	ret = vsnprintf(bufptr + ofs, bufsize - ofs, fmt, ap_copy0);
	va_end(ap_copy0);
	if (ret > bufsize - ofs - 1) {
		bufsize = ret + ofs + 1;
		bufptr = kmem_alloc(bufsize, char);

		memcpy(bufptr, buffer, ofs);
		ret = vsnprintf(bufptr + ofs, bufsize - ofs, fmt, ap);
	}

	ret += ofs;

	for (i = 0; i < cc->nlogger_cnt; i++)
		if (cc->nloggers[i])
			cc->nloggers[i](bufptr, ret);

	if (bufptr != buffer)
		kmem_free(bufptr);
	return ret;
}

int klog_f(unsigned char type, unsigned int mask, char *prog, char *modu,
		char *file, char *func, int ln, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = klog_vf(type, mask, prog, modu, file, func, ln, fmt, ap);
	va_end(ap);

	return ret;
}

static int strarr_find(strarr_s *sa, char *str)
{
	int i;

	if (unlikely(!str))
		return -1;

	for (i = 0; i < sa->cnt; i++)
		if (sa->arr[i] && !strcmp(sa->arr[i], str))
			return i;
	return -1;
}

static char *strarr_add(strarr_s *sa, char *str)
{
	int pos = strarr_find(sa, str);

	if (pos != -1)
		return sa->arr[pos];

	if (sa->cnt >= sa->size)
		ARR_INC(256, sa->arr, sa->size, char*);

	sa->arr[sa->cnt] = strdup(str);
	sa->cnt++;

	/* Return the position been inserted */
	return sa->arr[sa->cnt - 1];
}

char *klog_file_name_add(char *name)
{
	klogcc_s *cc = (klogcc_s*)klog_cc();
	char *newstr, *newname = get_basename(name);

	spl_mutex_lock(cc->mutex);
	newstr = strarr_add(&cc->arr_file_name, newname);
	spl_mutex_unlock(cc->mutex);

	free(newname);

	return newstr;
}
char *klog_modu_name_add(char *name)
{
	klogcc_s *cc = (klogcc_s*)klog_cc();
	char *newstr;

	spl_mutex_lock(cc->mutex);
	newstr = strarr_add(&cc->arr_modu_name, name);
	spl_mutex_unlock(cc->mutex);
	return newstr;
}
char *klog_prog_name_add(char *name)
{
	klogcc_s *cc = (klogcc_s*)klog_cc();
	char *newstr, *newname, *progname;

	if (name)
		newname = get_basename(name);
	else {
		progname = get_progname();
		newname = get_basename(progname);
	}

	spl_mutex_lock(cc->mutex);
	newstr = strarr_add(&cc->arr_prog_name, newname);
	spl_mutex_unlock(cc->mutex);

	free(newname);

	return newstr;
}
char *klog_func_name_add(char *name)
{
	klogcc_s *cc = (klogcc_s*)klog_cc();
	char *newstr;

	spl_mutex_lock(cc->mutex);
	newstr = strarr_add(&cc->arr_func_name, name);
	spl_mutex_unlock(cc->mutex);
	return newstr;
}

static int rulearr_add(rulearr_s *ra, char *prog, char *modu,
		char *file, char *func, int line, int pid,
		unsigned int fset, unsigned int fclr)
{
	if (unlikely(ra->cnt >= ra->size))
		ARR_INC(16, ra->arr, ra->size, rule_s);

	rule_s *rule = &ra->arr[ra->cnt];

	rule->prog = prog;
	rule->modu = modu;
	rule->file = file;
	rule->func = func;

	rule->line = line;
	rule->pid = pid;

	rule->set = fset;
	rule->clr = fclr;

	ra->cnt++;

	/* Return the position been inserted */
	return ra->cnt - 1;
}

/*
 * rule =
 * prog=xxx,modu=xxx,file=xxx,func=xxx,line=xxx,pid=xxx,mask=left
 */
void klog_rule_add(char *rule)
{
	klogcc_s *cc = (klogcc_s*)klog_cc();
	int i_line, i_pid;
	char *s_prog, *s_modu, *s_file, *s_func, *s_line, *s_pid, *s_mask;
	char buf[1024];
	int i, blen;

	unsigned int set = 0, clr = 0;

	if (!rule || rule[0] == '#')
		return;

	buf[0] = ',';
	strncpy(buf + 1, rule, sizeof(buf) - 2);

	s_mask = strstr(buf, ",mask=");
	if (!s_mask || !s_mask[6])
		return;
	s_mask += 6;

	s_prog = strstr(buf, ",prog=");
	s_modu = strstr(buf, ",modu=");
	s_file = strstr(buf, ",file=");
	s_func = strstr(buf, ",func=");
	s_line = strstr(buf, ",line=");
	s_pid = strstr(buf, ",pid=");

	blen = strlen(buf);
	for (i = 0; i < blen; i++)
		if (buf[i] == ',')
			buf[i] = '\0';

	if (!s_prog || !s_prog[6])
		s_prog = NULL;
	else
		s_prog = klog_prog_name_add(s_prog + 6);

	if (!s_modu || !s_modu[6])
		s_modu = NULL;
	else
		s_modu = klog_modu_name_add(s_modu + 6);

	if (!s_file || !s_file[6])
		s_file = NULL;
	else
		s_file = klog_file_name_add(s_file + 6);

	if (!s_func || !s_func[6])
		s_func = NULL;
	else
		s_func = klog_func_name_add(s_func + 6);

	if (!s_line || !s_line[6])
		i_line = -1;
	else
		i_line = atoi(s_line + 6);

	if (!s_pid || !s_pid[5])
		i_pid = -1;
	else
		i_pid = atoi(s_pid + 6);

	klog_parse_mask(s_mask, &set, &clr);

	if (set || clr) {
		spl_mutex_lock(cc->mutex);
		rulearr_add(&cc->arr_rule, s_prog, s_modu, s_file, s_func, i_line, i_pid, set, clr);
		spl_mutex_unlock(cc->mutex);

		klog_touch();
	}
}
void klog_rule_del(unsigned int idx)
{
	klogcc_s *cc = (klogcc_s*)klog_cc();

	if (idx >= cc->arr_rule.cnt)
		return;

	memcpy(&cc->arr_rule.arr[idx], &cc->arr_rule.arr[idx + 1],
			(cc->arr_rule.cnt - idx - 1) * sizeof(rule_s));
	klog_touch();
}
void klog_rule_clr()
{
	klogcc_s *cc = (klogcc_s*)klog_cc();

	cc->arr_rule.cnt = 0;
	klog_touch();
}

static unsigned int get_mask(char c)
{
	unsigned int i;

	static struct {
		char code;
		unsigned int bit;
	} flagmap[] = {
		{ '0', KLOG_FATAL },
		{ 'f', KLOG_FATAL },

		{ '1', KLOG_ALERT },
		{ 'a', KLOG_ALERT },

		{ '2', KLOG_CRIT },
		{ 'c', KLOG_CRIT },

		{ '3', KLOG_ERR },
		{ 'e', KLOG_ERR },

		{ '4', KLOG_WARNING },
		{ 'w', KLOG_WARNING },

		{ '5', KLOG_NOTICE },
		{ 'n', KLOG_NOTICE },

		{ '6', KLOG_INFO },
		{ 'i', KLOG_INFO },
		{ 'l', KLOG_INFO },

		{ '7', KLOG_DEBUG },
		{ 'd', KLOG_DEBUG },
		{ 't', KLOG_DEBUG },

		{ 's', KLOG_RTM },
		{ 'S', KLOG_ATM },

		{ 'j', KLOG_PID },
		{ 'x', KLOG_TID },

		{ 'N', KLOG_LINE },
		{ 'F', KLOG_FILE },
		{ 'M', KLOG_MODU },
		{ 'H', KLOG_FUNC },
		{ 'P', KLOG_PROG },
	};

	for (i = 0; i < sizeof(flagmap) / sizeof(flagmap[0]); i++)
		if (flagmap[i].code == c)
			return flagmap[i].bit;
	return 0;
}

static void klog_parse_mask(char *mask, unsigned int *set, unsigned int *clr)
{
	int i = 0, len = strlen(mask);
	char c;

	*set = 0;
	*clr = 0;

	for (i = 0; i < len; i++) {
		c = mask[i];
		if (c == '-') {
			c = mask[++i];
			*clr |= get_mask(c);
		} else
			*set |= get_mask(c);
	}
}

unsigned int klog_calc_mask(char *prog, char *modu, char *file, char *func, int line)
{
	klogcc_s *cc = (klogcc_s*)klog_cc();
	unsigned int i, all = 0;
	int pid = (int)cc->pid;

	for (i = 0; i < cc->arr_rule.cnt; i++) {
		rule_s *rule = &cc->arr_rule.arr[i];

		if (rule->prog != NULL && rule->prog != prog)
			continue;
		if (rule->modu != NULL && rule->modu != modu)
			continue;
		if (rule->file != NULL && rule->file != file)
			continue;
		if (rule->func != NULL && rule->func != func)
			continue;
		if (rule->line != -1 && rule->line != line)
			continue;
		if (rule->pid != -1 && rule->pid != pid)
			continue;

		kflg_clr(all, rule->clr);
		kflg_set(all, rule->set);
	}

	return all;
}

