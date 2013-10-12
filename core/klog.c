/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <time.h>

#include <hilda/helper.h>
#include <hilda/kmem.h>
#include <hilda/kflg.h>
#include <hilda/kstr.h>
#include <hilda/xtcool.h>
#include <hilda/karg.h>
#include <hilda/klog.h>

/*-----------------------------------------------------------------------
 * Local definition:
 */

/* STRing ARRay to save programe name, module name, file name etc */
typedef struct _strarr_t strarr_t;
struct _strarr_t {
	int size;
	int cnt;
	char **arr;
};

typedef struct _rule_t rule_t;
struct _rule_t {
	/* XXX: Don't filter ThreadID, it need the getflg called every log */

	/* 0 is know care */
	/* -1 is all */

	int prog;	/* Program command line */
	int modu;	/* Module name */
	int file;	/* File name */
	int func;	/* Function name */

	int line;	/* Line number */

	int pid;	/* Process ID */

	/* Which flag to be set or clear */
	unsigned int set, clr;
};

typedef struct _rulearr_t rulearr_t;
struct _rulearr_t {
	int size;
	int cnt;
	rule_t *arr;
};

/* How many logger slot */
#define MAX_NLOGGER 8
#define MAX_RLOGGER 8

/* Control Center for klog */
typedef struct _klogcc_t klogcc_t;
struct _klogcc_t {
	/** version is a ref count user change klog arg */
	int touches;

	SPL_HANDLE mutex;

	strarr_t arr_file_name;
	strarr_t arr_modu_name;
	strarr_t arr_prog_name;
	strarr_t arr_func_name;

	rulearr_t arr_rule;

	unsigned char nlogger_cnt, rlogger_cnt;
	KNLOGGER nloggers[MAX_NLOGGER];
	KRLOGGER rloggers[MAX_RLOGGER];
};

static klogcc_t *__g_klogcc = NULL;

/*-----------------------------------------------------------------------
 * klog-logger
 */
int klog_add_logger(KNLOGGER logger)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();
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
	klogcc_t *cc = (klogcc_t*)klog_cc();
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
	klogcc_t *cc = (klogcc_t*)klog_cc();
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
	klogcc_t *cc = (klogcc_t*)klog_cc();
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
static void klog_parse_mask(const char *mask, unsigned int *set, unsigned int *clr);

/**
 * \brief Other module call this to use already inited CC
 *
 * It should be called by other so or dll.
 */
void *klog_attach(void *logcc)
{
	__g_klogcc = (klogcc_t*)logcc;
	return (void*)__g_klogcc;
}

kinline void *klog_cc(void)
{
	int argc, cl_size = 0;
	char **argv, *cl_buf;
	void *cc;

	if (__g_klogcc)
		return (void*)__g_klogcc;

	/* klog_init not called, load args by myself */
	argc = 0;
	argv = NULL;

	cl_buf = spl_get_cmdline(&cl_size);
	karg_build_nul(cl_buf, cl_size, &argc, &argv);
	kmem_free(cl_buf);

	cc = klog_init(KLOG_DFT, argc, argv);
	karg_free(argv);

	return cc;
}

kinline void klog_touch(void)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();

	cc->touches++;
}
kinline int klog_touches(void)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();

	return cc->touches;
}

static void load_cfg_file(const char *path)
{
	char buf[4096];
	FILE *fp;

	fp = fopen(path, "rt");
	if (!fp)
		return;

	while (fgets(buf, sizeof(buf), fp))
		klog_rule_add(buf);

	fclose(fp);
}

static void load_cfg(int argc, char *argv[])
{
	char *cfgpath;
	int i;

	/* Load configure from env, DeFault ConFiGure */
	cfgpath = getenv("KLOG_DFCFG");
	if (cfgpath)
		load_cfg_file(cfgpath);

	/* Load configure from command line */
	i = karg_find(argc, argv, "--klog-cfgfile", 1);
	if (i > 1)
		load_cfg_file(argv[i + 1]);
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

void *klog_init(unsigned int mask, int argc, char **argv)
{
	klogcc_t *cc;

	if (__g_klogcc)
		return (void*)__g_klogcc;

	cc = (klogcc_t*)kmem_alloz(1, klogcc_t);
	__g_klogcc = cc;

	cc->mutex = spl_mutex_create();

	/* Set default before configure file */
	if (mask)
		rule_add_from_mask(mask);

	load_cfg(argc, argv);

	klog_touch();

	return (void*)__g_klogcc;
}

int klog_vf(unsigned char type, unsigned int mask,
		const char *prog, const char *modu,
		const char *file, const char *func, int ln,
		const char *fmt, va_list ap)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();

	char buffer[2048], *bufptr = buffer;
	int i, ret, ofs, bufsize = sizeof(buffer);

	char tmbuf[128];
	time_t t;
	struct tm *tmp;

	unsigned long tick;

	for (i = 0; i < cc->rlogger_cnt; i++)
		if (cc->rloggers[i])
			cc->rloggers[i](type, mask, prog, modu, file, func, ln, fmt, ap);

	if (mask & (KLOG_RTM | KLOG_ATM))
		tick = spl_get_ticks();

	ofs = 0;

	/* Type */
	if (type)
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
		ofs += sprintf(bufptr + ofs, "j:%d|", (int)spl_process_current());
	if (mask & KLOG_TID)
		ofs += sprintf(bufptr + ofs, "x:%x|", (int)spl_thread_current());

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

	if (ofs)
		ofs += sprintf(bufptr + ofs, " ");

	ret = vsnprintf(bufptr + ofs, bufsize - ofs, fmt, ap);
	while (ret > bufsize - ofs - 1) {
		bufsize = ret + ofs + 1;
		if (bufptr != buffer)
			kmem_free(bufptr);
		bufptr = kmem_get(bufsize);

		ofs = 0;

		/* Type */
		if (type)
			ofs += sprintf(bufptr, "|%c|", type);

		/* Time */
		if (mask & KLOG_RTM)
			ofs += sprintf(bufptr + ofs, "s:%lu|", tick);
		if (mask & KLOG_ATM)
			ofs += sprintf(bufptr + ofs, "%s.%03d|", tmbuf, (unsigned int)(tick % 1000));

		/* ID */
		if (mask & KLOG_PID)
			ofs += sprintf(bufptr + ofs, "j:%d|", (int)spl_process_current());
		if (mask & KLOG_TID)
			ofs += sprintf(bufptr + ofs, "x:%x|", (int)spl_thread_current());

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

		if (ofs)
			ofs += sprintf(bufptr + ofs, " ");

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

int klog_f(unsigned char type, unsigned int mask,
		const char *prog, const char *modu,
		const char *file, const char *func, int ln,
		const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = klog_vf(type, mask, prog, modu, file, func, ln, fmt, ap);
	va_end(ap);

	return ret;

}

char *klog_get_name_part(char *name)
{
	char *dup = kstr_dup(name);
	char *bn = basename(dup);

	bn = bn ? kstr_dup(bn) : kstr_dup("");

	kmem_free_s(dup);

	return bn;
}

char *klog_get_prog_name()
{
	static char *pname = NULL;
	char *cl;

	if (pname)
		return pname;

	cl = spl_get_cmdline(NULL);
	if (cl) {
		pname = klog_get_name_part(cl);
		kmem_free(cl);
	}
	return pname;
}

static int strarr_find(strarr_t *sa, const char *str)
{
	int i;

	if (!str)
		return -1;

	for (i = 0; i < sa->cnt; i++)
		if (sa->arr[i] && !strcmp(sa->arr[i], str))
			return i;
	return -1;
}

static int strarr_add(strarr_t *sa, const char *str)
{
	int pos = strarr_find(sa, str);

	if (pos != -1)
		return pos;

	if (sa->cnt >= sa->size)
		ARR_INC(10, sa->arr, sa->size, char*);

	sa->arr[sa->cnt] = kstr_dup(str);
	sa->cnt++;

	/* Return the position been inserted */
	return sa->cnt - 1;
}

/*
 * XXX: About return strarr_add(xyz, name) + 1;
 *
 * The name index 0 means don't care, it conflict with
 * array index 0, so here add ONE to skip the value zero
 */
int klog_file_name_add(const char *name)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();
	int pos;

	spl_mutex_lock(cc->mutex);
	pos = strarr_add(&cc->arr_file_name, name ? name : "unknown") + 1;
	spl_mutex_unlock(cc->mutex);
	return pos;
}
int klog_modu_name_add(const char *name)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();
	int pos;

	spl_mutex_lock(cc->mutex);
	pos = strarr_add(&cc->arr_modu_name, name ? name : "unknown") + 1;
	spl_mutex_unlock(cc->mutex);
	return pos;
}
int klog_prog_name_add(const char *name)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();
	int pos;

	spl_mutex_lock(cc->mutex);
	pos = strarr_add(&cc->arr_prog_name, name ? name : "unknown") + 1;
	spl_mutex_unlock(cc->mutex);
	return pos;
}
int klog_func_name_add(const char *name)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();
	int pos;

	spl_mutex_lock(cc->mutex);
	pos = strarr_add(&cc->arr_func_name, name ? name : "unknown") + 1;
	spl_mutex_unlock(cc->mutex);
	return pos;
}

static int rulearr_add(rulearr_t *ra, int prog, int modu,
		int file, int func, int line, int pid,
		unsigned int fset, unsigned int fclr)
{
	if (ra->cnt >= ra->size)
		ARR_INC(1, ra->arr, ra->size, rule_t);

	rule_t *rule = &ra->arr[ra->cnt];

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
void klog_rule_add(const char *rule)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();
	int i_prog, i_modu, i_file, i_func, i_line, i_pid;
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
		i_prog = 0;
	else
		i_prog = klog_prog_name_add(s_prog + 6);
	if (!s_modu || !s_modu[6])
		i_modu = 0;
	else
		i_modu = klog_modu_name_add(s_modu + 6);
	if (!s_file || !s_file[6])
		i_file = 0;
	else
		i_file = klog_file_name_add(s_file + 6);
	if (!s_func || !s_func[6])
		i_func = 0;
	else
		i_func = klog_func_name_add(s_func + 6);
	if (!s_line || !s_line[6])
		i_line = 0;
	else
		i_line = atoi(s_line + 6);
	if (!s_pid || !s_pid[5])
		i_pid = 0;
	else
		i_pid = atoi(s_pid + 6);

	klog_parse_mask(s_mask, &set, &clr);

	if (set || clr) {
		spl_mutex_lock(cc->mutex);
		rulearr_add(&cc->arr_rule, i_prog, i_modu, i_file, i_func, i_line, i_pid, set, clr);
		spl_mutex_unlock(cc->mutex);

		klog_touch();
	}
}
void klog_rule_del(int index)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();

	if (index < 0 || index >= cc->arr_rule.cnt)
		return;

	memcpy(&cc->arr_rule.arr[index], &cc->arr_rule.arr[index + 1],
			(cc->arr_rule.cnt - index - 1) * sizeof(rule_t));
	klog_touch();
}
void klog_rule_clr()
{
	klogcc_t *cc = (klogcc_t*)klog_cc();

	cc->arr_rule.cnt = 0;
	klog_touch();
}

char *klog_rule_all()
{
	klogcc_t *cc = (klogcc_t*)klog_cc();

	/* TODO: return all the rule */
	return NULL;
}

static unsigned int get_mask(char c)
{
	int i;

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

static void klog_parse_mask(const char *mask, unsigned int *set, unsigned int *clr)
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

unsigned int klog_calc_mask(int prog, int modu, int file, int func, int line, int pid)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();
	unsigned int i, all = 0;

	for (i = 0; i < cc->arr_rule.cnt; i++) {
		rule_t *rule = &cc->arr_rule.arr[i];

		if (rule->prog && rule->prog != prog)
			continue;
		if (rule->modu && rule->modu != modu)
			continue;
		if (rule->file && rule->file != file)
			continue;
		if (rule->func && rule->func != func)
			continue;
		if (rule->line && rule->line != line)
			continue;
		if (rule->pid && rule->pid != pid)
			continue;

		kflg_clr(all, rule->clr);
		kflg_set(all, rule->set);
	}

	return all;
}

