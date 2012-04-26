/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <string.h>

#include <klog.h>
#include <kflg.h>
#include <karg.h>
#include <kmem.h>

#ifndef CFG_KLOG_DO_NOTHING

#define MAX_LOGGER 4

/* Control Center for klog */
typedef struct _klogcc_t klogcc_t;
struct _klogcc_t {
	/** Global flags, set by klog_init, default is no message */
	kuint flg;

	/** command line for application, seperate by '\0' */
	kchar ff[2048];

	/** touches is a ref count user change klog arg */
	kint touches;

	KLOGGER loggers[MAX_LOGGER];
};

static klogcc_t *__g_klogcc = NULL;

static void set_fa_arg(int argc, char **argv);
static void set_ff_arg(int argc, char **argv);

int klog_add_logger(KLOGGER logger)
{
	klogcc_t *cc = __g_klogcc;
	int i;

	if (!logger)
		return -1;

	for (i = 0; i < MAX_LOGGER; i++)
		if (cc->loggers[i] == logger)
			return 0;

	for (i = 0; i < MAX_LOGGER; i++)
		if (!cc->loggers[i]) {
			cc->loggers[i] = logger;
			return 0;
		}

	klogf("klog_add_logger: Only up to %d logger supported.\n", MAX_LOGGER);
	return -1;
}

int klog_del_logger(KLOGGER logger)
{
	klogcc_t *cc = __g_klogcc;
	int i;

	if (!logger)
		return -1;

	for (i = 0; i < MAX_LOGGER; i++)
		if (cc->loggers[i] == logger) {
			cc->loggers[i] = NULL;
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
	__g_klogcc = (klogcc_t*)logcc;
	return (void*)__g_klogcc;
}

void *klog_cc(void)
{
	return (void*)__g_klogcc;
}

kinline void klog_touch(void)
{
	__g_klogcc->touches++;
}
kinline int klog_touches(void)
{
	return __g_klogcc->touches;
}

/* opt setting should has the same format as argv */
/* just like a raw command line */
/* --klog-la=left --klog-lf=left=:werw:wer: */
/* --klog-fa=left --klog-ff=left=:werw:wer: */
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
	klogcc_t *cc = __g_klogcc;
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
				kflg_clr(cc->flg, LOG_TM_REL);
			else if (c == 'T')
				kflg_clr(cc->flg, LOG_TM_ABS);
		} else if (c == 'l')
			kflg_set(cc->flg, LOG_LOG);
		else if (c == 'e')
			kflg_set(cc->flg, LOG_ERR);
		else if (c == 'f')
			kflg_set(cc->flg, LOG_FAT);
		else if (c == 't')
			kflg_set(cc->flg, LOG_TM_REL);
		else if (c == 'T')
			kflg_set(cc->flg, LOG_TM_ABS);
	}
}

/* Flag for All files */
static void set_fa_arg(int argc, char **argv)
{
	int i;

	for (i = 0; i < argc && argv[i]; i++)
		if ((!strncmp(argv[i], "--klog-la=", 10)) ||
				(!strncmp(argv[i], "--klog-fa=", 10)))
			set_fa_str(argv[i] + 10);
}

/* Flag for Files */
static void set_ff_arg(int argc, char **argv)
{
	klogcc_t *cc = __g_klogcc;
	int i;
	char *p = cc->ff;

	/* XXX: tricky */
	strcat(p, " = ");

	for (i = 0; i < argc && argv[i]; i++)
		if ((!strncmp(argv[i], "--klog-lf=", 10)) ||
				(!strncmp(argv[i], "--klog-ff=", 10))) {
			p = strcat(p, argv[i] + 10);
			p = strcat(p, " ");
		}
}

/**
 * \brief Set parameters for debug message, should be called once in main
 * --klog-la=lef-t --klog-lf=<left>=:file1:file2:
 * --klog-fa=lef-t --klog-ff=<left>=:file1:file2:
 *
 * \param flg ored of LOG_LOG, LOG_ERR and LOG_FAT
 * \return Debug message flag after set.
 */
void *klog_init(kuint flg, int argc, char **argv)
{
	klogcc_t *cc;

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
kuint klog_getflg(const kchar *fn)
{
	klogcc_t *cc = __g_klogcc;
	kchar pattern[256], *start;
	kbool set;
	kuint flg = cc->flg;

	sprintf(pattern, ":%s:", fn);

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
					kflg_set(flg, LOG_TM_REL);
				else
					kflg_clr(flg, LOG_TM_REL);
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

/* flag => [ 'l', 'e', 'f', 't' ] */
/* int klogf(const char *prefix, kuint flg, const char *fmt, ...) */
int klogf(const char *fmt, ...)
{
	klogcc_t *cc = __g_klogcc;
	va_list ap;
	char buffer[2048], *bufptr = buffer;
	int i, j, used_logger = 0, ret, bufsize = sizeof(buffer);

	KLOGGER loggers[MAX_LOGGER];

	for (i = 0, j = 0; i < MAX_LOGGER; i++)
		if (cc->loggers[i]) {
			loggers[j] = cc->loggers[i];
			j++;
			used_logger++;
		}

	if (used_logger == 0)
		return 0;

	va_start(ap, fmt);
	ret = vsnprintf(bufptr, bufsize, fmt, ap);
	while (ret < 0) {
		bufsize <<= 1;
		if (bufptr != buffer)
			kmem_free(bufptr);
		bufptr = kmem_get(bufsize);
		ret = vsnprintf(bufptr, bufsize, fmt, ap);
	}
	va_end(ap);

	for (i = 0; i < used_logger; i++)
		loggers[i](bufptr, ret);

	if (bufptr != buffer)
		kmem_free(bufptr);
	return ret;
}

#endif /* CFG_KLOG_DO_NOTHING */
