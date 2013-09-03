/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <time.h>
#include <string.h>

#include <klog.h>
#include <kflg.h>
#include <karg.h>
#include <kmem.h>

#ifndef CFG_KLOG_DO_NOTHING

#define MAX_NLOGGER 8
#define MAX_RLOGGER 8

/* Control Center for klog */
typedef struct _klogcc_t klogcc_t;
struct _klogcc_t {
	/** Global flags, set by klog_init, default is no message */
	kuint flg;

	/** command line for application, seperate by '\0' */
	kchar ff[4096];

	/** touches is a ref count user change klog arg */
	kint touches;

	kuchar nlogger_cnt, rlogger_cnt;
	KNLOGGER nloggers[MAX_NLOGGER];
	KRLOGGER rloggers[MAX_RLOGGER];
};

static klogcc_t *__g_klogcc = NULL;

static void set_fa_arg(int argc, char **argv);
static void set_ff_arg(int argc, char **argv);

int klog_add_logger(KNLOGGER logger)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();
	int i;

	if (!logger)
		return -1;

	for (i = 0; i < MAX_NLOGGER; i++)
		if (cc->nloggers[i] == logger)
			return 0;

	for (i = 0; i < MAX_NLOGGER; i++)
		if (!cc->nloggers[i]) {
			cc->nloggers[i] = logger;
			cc->nlogger_cnt++;
			return 0;
		}

	wlogf("klog_add_logger: Only up to %d logger supported.\n", MAX_NLOGGER);
	return -1;
}

int klog_del_logger(KNLOGGER logger)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();
	int i;

	if (!logger)
		return -1;

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

	if (!logger)
		return -1;

	for (i = 0; i < MAX_RLOGGER; i++)
		if (cc->rloggers[i] == logger)
			return 0;

	for (i = 0; i < MAX_RLOGGER; i++)
		if (!cc->rloggers[i]) {
			cc->rloggers[i] = logger;
			cc->rlogger_cnt++;
			return 0;
		}

	wlogf("klog_add_rlogger: Only up to %d logger supported.\n", MAX_RLOGGER);
	return -1;
}

int klog_del_rlogger(KRLOGGER logger)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();
	int i;

	if (!logger)
		return -1;

	for (i = 0; i < cc->rlogger_cnt; i++)
		if (cc->rloggers[i] == logger) {
			cc->rlogger_cnt--;
			cc->rloggers[i] = cc->rloggers[cc->rlogger_cnt];
			cc->rloggers[cc->rlogger_cnt] = NULL;
			return 0;
		}

	return -1;
}


/**
 * \brief Other module call this to use already inited CC
 *
 * It should be called by other so or dll.
 */
void *klog_attach(void *logcc)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();

	return (void*)cc;
}

static int load_boot_args(int *argc, char ***argv)
{
	FILE *fp = fopen("/proc/cmdline", "rt");
	char buffer[4096];
	int bytes;

	if (fp) {
		bytes = fread(buffer, sizeof(char), sizeof(buffer), fp);
		fclose(fp);

		if (bytes <= 0)
			return -1;

		buffer[bytes] = '\0';
		kstr_trim(buffer);

		build_argv(buffer, argc, argv);
		return 0;
	}
	return -1;
}

kinline void *klog_cc(void)
{
	int argc;
	char **argv, *cmdline;
	void *cc;

	if (__g_klogcc)
		return (void*)__g_klogcc;

	/*
	 * klog_init not called, load args by myself
	 */
	argc = 0;
	argv = NULL;

	cmdline = spl_get_cmdline();
	if (cmdline)
		build_argv(cmdline, argc, argv);

	cc = klog_init(LOG_ALL, argc, argv);

	kmem_free(cmdline);
	free_argv(argv);

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

/* opt setting should has the same format as argv */
/* just like a raw command line */
void klog_setflg(const char *cmd)
{
	int argc;
	char **argv;

	if ((argv = build_argv(cmd, &argc, &argv)) == NULL)
		return;

	set_fa_arg(argc, argv);
	set_ff_arg(argc, argv);
	klog_touch();

	free_argv(argv);
}

static void set_fa_str(const char *arg)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();
	char c;
	const char *p = arg;

	while (c = *p++) {
		if (c == '-') {
			c = *p++;
			if (c == 'l')
				kflg_clr(cc->flg, LOG_LOG);
			else if (c == 'e')
				kflg_clr(cc->flg, LOG_ERR);
			else if (c == 'f')
				kflg_clr(cc->flg, LOG_FAT);
			else if (c == 't')
				kflg_clr(cc->flg, LOG_RTM);
			else if (c == 'T')
				kflg_clr(cc->flg, LOG_ATM);
			else if (c == 'L')
				kflg_clr(cc->flg, LOG_LINE);
			else if (c == 'F')
				kflg_clr(cc->flg, LOG_FILE);
			else if (c == 'M')
				kflg_clr(cc->flg, LOG_MODU);
			else if (c == 'i')
				kflg_clr(cc->flg, LOG_PID);
			else if (c == 'I')
				kflg_clr(cc->flg, LOG_TID);
		} else if (c == 'l')
			kflg_set(cc->flg, LOG_LOG);
		else if (c == 'e')
			kflg_set(cc->flg, LOG_ERR);
		else if (c == 'f')
			kflg_set(cc->flg, LOG_FAT);
		else if (c == 't')
			kflg_set(cc->flg, LOG_RTM);
		else if (c == 'T')
			kflg_set(cc->flg, LOG_ATM);
		else if (c == 'L')
			kflg_set(cc->flg, LOG_LINE);
		else if (c == 'F')
			kflg_set(cc->flg, LOG_FILE);
		else if (c == 'M')
			kflg_set(cc->flg, LOG_MODU);
		else if (c == 'i')
			kflg_set(cc->flg, LOG_PID);
		else if (c == 'I')
			kflg_set(cc->flg, LOG_TID);
	}
}

/* Flag for All files */
static void set_fa_arg(int argc, char **argv)
{
	int i;

	for (i = 0; i < argc && argv[i]; i++)
		if (!strncmp(argv[i], "--klog-fa=", 10))
			set_fa_str(argv[i] + 10);
}

/* Flag for Files */
static void set_ff_arg(int argc, char **argv)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();
	int i;
	char *p = cc->ff;

	/* XXX: tricky */
	strcat(p, " = ");

	for (i = 0; i < argc && argv[i]; i++)
		if (!strncmp(argv[i], "--klog-ff=", 10)) {
			p = strcat(p, argv[i] + 10);
			p = strcat(p, " ");
		}
}

/**
 * \brief Set parameters for debug message, should be called once in main
 * --klog-fa=lef-t --klog-ff=<left>=:file1:file2:
 *
 * \param flg ored of LOG_LOG, LOG_ERR and LOG_FAT
 * \return Debug message flag after set.
 */
void *klog_init(kuint flg, int argc, char **argv)
{
	klogcc_t *cc;

	if (arg_find(argc, argv, "--klog-help", 1) > 0) {
		wlogf("klog-help:\n");
		wlogf("\t--klog-fa=<switch> --klog-ff=<switch>=:file1.c:fileX.c:\n");
		wlogf("\n");
		wlogf("\t--klog-fa: fa = Flags for All\n");
		wlogf("\t--klog-ff: fa = Flags for File\n");
		wlogf("\n");
		wlogf("\tswitches:\n");
		wlogf("\tl: Log\n");
		wlogf("\te: Error\n");
		wlogf("\tf: Fatal Error\n");
		wlogf("\tt: Relative Time\n");
		wlogf("\tT: ABS Time, in MS\n");
		wlogf("\ti: Process ID\n");
		wlogf("\tI: Thread ID\n");
		wlogf("\tN: Line Number\n");
		wlogf("\tF: File Name\n");
		wlogf("\tM: Module Name\n");
		wlogf("\n");

		exit(0);
	}

	if (__g_klogcc)
		return (void*)__g_klogcc;

	cc = (klogcc_t*)kmem_alloz(1, klogcc_t);
	__g_klogcc = cc;

	cc->flg = flg;

	set_fa_arg(argc, argv);
	set_ff_arg(argc, argv);
	klog_touch();

	return (void*)__g_klogcc;
}

/**
 * \brief Current debug message flag.
 *
 * This will check the command line to do more on each file.
 *
 * The command line format is --klog-ff=<left> :file1.ext:file2.ext:file:
 *      file.ext is the file name. If exist dup name file, the flag set to all.
 */
kuint klog_getflg(const kchar *file)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();
	kchar pattern[256], *start;
	kbool set;
	kuint flg = cc->flg;

	sprintf(pattern, ":%s:", file);

	start = strstr(cc->ff, pattern);
	if (start) {
		while (*start-- != '=')
			;

		while (*start != ' ') {
			if (*(start - 1) == '-')
				set = kfalse;
			else
				set = ktrue;

			switch(*start) {
			case 'l':
				if (set)
					kflg_set(flg, LOG_LOG);
				else
					kflg_clr(flg, LOG_LOG);
				break;
			case 'e':
				if (set)
					kflg_set(flg, LOG_ERR);
				else
					kflg_clr(flg, LOG_ERR);
				break;
			case 'f':
				if (set)
					kflg_set(flg, LOG_FAT);
				else
					kflg_clr(flg, LOG_FAT);
				break;
			case 't':
				if (set)
					kflg_set(flg, LOG_RTM);
				else
					kflg_clr(flg, LOG_RTM);
				break;
			case 'T':
				if (set)
					kflg_set(flg, LOG_ATM);
				else
					kflg_clr(flg, LOG_ATM);
				break;
			case 'N':
				if (set)
					kflg_set(flg, LOG_LINE);
				else
					kflg_clr(flg, LOG_LINE);
				break;
			case 'F':
				if (set)
					kflg_set(flg, LOG_FILE);
				else
					kflg_clr(flg, LOG_FILE);
				break;
			case 'M':
				if (set)
					kflg_set(flg, LOG_MODU);
				else
					kflg_clr(flg, LOG_MODU);
				break;
			default:
				break;
			}

			if (set)
				start--;
			else
				start -= 2;
		}
	}

	return flg;
}

int klogf(unsigned char type, unsigned int flg, const char *modu, const char *file, int ln, const char *fmt, ...)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();
	va_list ap;
	char buffer[2048], *bufptr = buffer;
	int i, ret, ofs, bufsize = sizeof(buffer);

	char tmbuf[128];
	time_t t;
	struct tm *tmp;

	unsigned long tick = 0;

	va_start(ap, fmt);

	for (i = 0; i < cc->rlogger_cnt; i++)
		if (cc->rloggers[i])
			cc->rloggers[i](type, flg, modu, file, ln, fmt, ap);

	if (flg & (LOG_RTM | LOG_ATM))
		tick = spl_get_ticks();

	ofs = 0;
	if (type)
		ofs += sprintf(bufptr, "|%c|", type);
	if (flg & LOG_RTM)
		ofs += sprintf(bufptr + ofs, "%lu|", tick);
	if (flg & LOG_ATM) {
		t = time(NULL);
		tmp = localtime(&t);
		strftime(tmbuf, sizeof(tmbuf), "%Y/%m/%d %H:%M:%S", tmp);
		ofs += sprintf(bufptr + ofs, "%s.%03d|", tmbuf, (kuint)(tick % 1000));
	}
	if (flg & LOG_PID)
		ofs += sprintf(bufptr + ofs, "%d|", (int)spl_process_currrent());
	if (flg & LOG_TID)
		ofs += sprintf(bufptr + ofs, "%x|", (int)spl_thread_current());
	if ((flg & LOG_MODU) && modu)
		ofs += sprintf(bufptr + ofs, "%s|", modu);
	if ((flg & LOG_FILE) && file)
		ofs += sprintf(bufptr + ofs, "%s|", file);
	if (flg & LOG_LINE)
		ofs += sprintf(bufptr + ofs, "%d|", ln);
	if (ofs)
		ofs += sprintf(bufptr + ofs, " ");

	ret = vsnprintf(bufptr + ofs, bufsize - ofs, fmt, ap);
	while (ret > bufsize - ofs - 1) {
		bufsize = ret + ofs + 1;
		if (bufptr != buffer)
			kmem_free(bufptr);
		bufptr = kmem_get(bufsize);

		ofs = 0;
		if (type)
			ofs += sprintf(bufptr, "|%c|", type);
		if (flg & LOG_RTM)
			ofs += sprintf(bufptr + ofs, "%lu|", tick);
		if (flg & LOG_ATM)
			ofs += sprintf(bufptr + ofs, "%s.%03d|", tmbuf, (kuint)(tick % 1000));
		if (flg & LOG_PID)
			ofs += sprintf(bufptr + ofs, "%d|", (int)spl_process_currrent());
		if (flg & LOG_TID)
			ofs += sprintf(bufptr + ofs, "%x|", (int)spl_thread_current());
		if ((flg & LOG_MODU) && modu)
			ofs += sprintf(bufptr + ofs, "%s|", modu);
		if ((flg & LOG_FILE) && file)
			ofs += sprintf(bufptr + ofs, "%s|", file);
		if (flg & LOG_LINE)
			ofs += sprintf(bufptr + ofs, "%d|", ln);
		if (ofs)
			ofs += sprintf(bufptr + ofs, " ");

		ret = vsnprintf(bufptr + ofs, bufsize - ofs, fmt, ap);
	}
	va_end(ap);

	ret += ofs;
	for (i = 0; i < cc->nlogger_cnt; i++)
		if (cc->nloggers[i])
			cc->nloggers[i](bufptr, ret);

	if (bufptr != buffer)
		kmem_free(bufptr);
	return ret;
}

#endif /* CFG_KLOG_DO_NOTHING */
