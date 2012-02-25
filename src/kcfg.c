/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

/**
 * \file config.c
 * \brief Config act as a non-session INI file which save the
 * preference of the system.
 *
 * Configure module should be initialized by before any other
 * helper module and \c kcfg_load() MUST called after all the
 * other modules initialized. Because the \c kcfg_load() function
 * will write back the opt, the opt must be exists when them be set
 * by call \c opt_setxxx().
 *
 * The key function \c kcfg_save() theoretically can be called any
 * time any where. But continually call should be cached for reduce
 * access of flash etc.
 *
 * Only apts added by \c kcfg_target_opt_add() can be processed,
 * all the other opts will be
 * droped. So other module MUST call \c kcfg_target_opt_add() to
 * ensure opts can be saved.
 */
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include <ktypes.h>

#include <klog.h>
#include <kmem.h>
#include <kstr.h>
#include <md5sum.h>
#include <sdlist.h>

#include <helper.h>
#include <opt.h>
#include <kmque.h>
#include <kcfg.h>

/**
 * \brief INI file
 * Some config update, only `some' should be saved.
 * That, only which `dirty' should write back.
 */

static kcfg_t *__g_cfg = NULL;
static kmque_t *__g_mque_main = NULL;
static void *__g_tm_cfg_save = NULL;
static int __g_cfg_loaded = 0;

/* All the skipped targets */
static char *__g_skips[20];

static kinline int target_find(const char *name);
static void cfg_save_dpc(void *ua, void *ub);
static int target_opt_add(kcfg_target_t *ct, const char *opt);

static int is_skipped_target(const char *name);

/* argv and file are builtin */
static int target_argv();
static int target_file();

static int is_skipped_target(const char *name)
{
	int i, arrlen = sizeof(__g_skips) / sizeof(__g_skips[0]);

	for (i = 0; i < arrlen; i++)
		if (__g_skips[i] && !strcmp(__g_skips[i], name))
			return 1;

	return 0;
}

static int setup_skipped_target()
{
	int i, j, arrlen = sizeof(__g_skips) / sizeof(__g_skips[0]);

	int argc;
	char **argv;

	opt_getint("i:/prog/argc", &argc);
	opt_getptr("p:/prog/argv", (void**)&argv);

	for (i = 0; i < argc; i++) {
		if (!argv[i])
			continue;
		if (strncmp(argv[i], "--config-skip=", 14))
			continue;
		if (!argv[i][14])
			continue;

		for (j = 0; j < arrlen; j++)
			if (!__g_skips[j])
				__g_skips[j] = kstr_dup(argv[i] + 14);
	}

	return 0;
}

static void free_skipped_target()
{
	int i, arrlen = sizeof(__g_skips) / sizeof(__g_skips[0]);

	for (i = 0; i < arrlen; i++)
		kmem_free_s(__g_skips[i]);
}

static void queue_target(kcfg_target_t *ct)
{
	int i;
	kcfg_target_t *tct;
	kcfg_t *c = __g_cfg;

	for (i = 0; i < c->target.cnt; i++) {
		tct = c->target.arr[i];
		if (!tct) {
			c->target.arr[i] = ct;
			return;
		}
	}

	ARR_INC(1, c->target.arr, c->target.cnt, kcfg_target_t*);
	c->target.arr[i] = ct;
}


kcfg_target_t *kcfg_target_add(const char *name, KCFG_SAVE dosave,
		KCFG_LOAD doload, KCFG_DELETE dodelete, void *ua, void *ub)
{
	int i;
	kcfg_target_t *tct, *nct;
	kcfg_t *c = __g_cfg;

	if (is_skipped_target(name))
		return NULL;

	i = target_find(name);
	if (-1 != i) {
		tct = c->target.arr[i];
		if ((tct->save == dosave) && (tct->load == doload) &&
				(tct->del == dodelete) &&
				(tct->ua == ua) && (tct->ub == ub))
			return tct;
		else {
			kerror(("Target named '%s' already exists!\n", name));
			return NULL;
		}
	}

	nct = (kcfg_target_t*)kmem_alloz(1, kcfg_target_t);
	nct->name = kstr_dup(name);
	nct->save = dosave;
	nct->load = doload;
	nct->del = dodelete;
	nct->ua = ua;
	nct->ub = ub;

	queue_target(nct);
	return nct;
}

static kinline int target_find(const char *name)
{
	int i;
	kcfg_target_t *ct;
	kcfg_t *c = __g_cfg;

	if (!name)
		return -1;

	for (i = 0; i < c->target.cnt; i++) {
		ct = c->target.arr[i];
		if (ct && (0 == strcmp(ct->name, name)))
			return i;
	}
	return -1;
}

int kcfg_target_del(const char *name)
{
	int i;
	kcfg_target_t *ct;
	kcfg_t *c = __g_cfg;

	i = target_find(name);
	if (-1 != i) {
		ct = c->target.arr[i];
		c->target.arr[i] = 0;
		if (ct->del)
			ct->del(ct, ct->ua, ct->ub);
		kmem_free_s(ct->name);
		kmem_free(ct);
		return 0;
	}

	return -1;
}

static kinline int target_opt_find(kcfg_target_t *ct, const char *opt)
{
	int i;
	const char *path;

	if (ct->pass_through)
		return -2;

	for (i = 0; i < ct->opts.cnt; i++) {
		path = ct->opts.arr[i];
		if (path && (0 == strcmp(path, opt))) {
			klog(("target_opt_find: %s, pos: %d\n", path, i));
			return i;
		}
	}

	return -1;
}

static int target_opt_add(kcfg_target_t *ct, const char *opt)
{
	int i;
	const char *path;

	i = target_opt_find(ct, opt);
	if (i >= 0) {
		klog(("target_opt_add: %s already there\n", opt));
		return 0;
	}
	if (i == -2)
		return 0;

	for (i = 0; i < ct->opts.cnt; i++) {
		path = ct->opts.arr[i];
		if (!path) {
			ct->opts.arr[i] = kstr_dup(opt);
			return 0;
		}
	}

	ARR_INC(10, ct->opts.arr, ct->opts.cnt, char*);
	ct->opts.arr[i] = kstr_dup(opt);

	return 0;
}

static void target_opt_clr(kcfg_target_t *ct)
{
	int i;
	for (i = 0; i < ct->opts.cnt; i++)
		kmem_free_sz(ct->opts.arr[i]);
}

static int ow_opt_dirty(int ses, void *opt, const char *path, void *wch)
{
	if (__g_cfg_loaded)
		kcfg_save_delay(500);
	return 0;
}

int kcfg_target_opt_add(const char *name, const char *opt)
{
	int err = -1, i = target_find(name);
	if (-1 != i)
		err = target_opt_add(__g_cfg->target.arr[i], opt);
	if (!err)
		opt_awch(opt, ow_opt_dirty);
	return err;
}

int kcfg_target_opt_del(const char *name, const char *opt)
{
	int i, j;
	kcfg_t *c = __g_cfg;

	i = target_find(name);
	if (-1 != i) {
		j = target_opt_find(c->target.arr[i], opt);
		if (j >= 0) {
			kmem_free_z(c->target.arr[i]->opts.arr[j]);
			return 0;
		}
	}
	return -1;
}

static void make_save_buffer(kcfg_target_t *ct, char **dat, int *len)
{
	int i, klen, vlen, dlen = 0;
	char *buf = 0, *v, *opt;

	for (i = 0; i < ct->opts.cnt; i++) {
		opt = ct->opts.arr[i];
		if (!opt)
			continue;

		v = NULL;
		if (EC_OK == opt_getini(opt, &v)) {
			if (!v)
				continue;

			klen = strlen(opt);
			vlen = strlen(v);
			buf = (char*)kmem_realloc(buf, dlen + klen + vlen + 2);
			memcpy(buf + dlen, opt, klen);
			memcpy(buf + dlen + klen, "=", 1);
			memcpy(buf + dlen + klen + 1, v, vlen);
			memcpy(buf + dlen + klen + 1 + vlen, "\n", 1);
			dlen += klen + 1 + vlen + 1;

			kmem_free(v);
		}
	}

	if (buf) {
		dlen--;
		buf[dlen] = '\0';
	}

	*dat = buf;
	*len = dlen;
}

/**
 * \brief Return all register targets, with names seperated by '\n';
 *
 * \param opt
 *
 * \return
 */
static int og_targets(void *opt, const char *path,
		void **pa, void **pb)
{
	int i;
	kcfg_t *c = __g_cfg;
	kcfg_target_t *ct;
	char *targets = (char*)kmem_alloc(1024, char);

	targets[0] = '\0';

	for (i = 0; i < c->target.cnt; i++) {
		ct = c->target.arr[i];
		if (!ct)
			continue;
		strcat(targets, ct->name);
		strcat(targets, ";");
	}

	opt_set_cur_str(opt, targets);
	return 0;
}

/**
 * \brief Return content of configuration.
 *
 * \param opt
 *
 * \return
 */
static int og_target(void *opt, const char *path,
		void **pa, void **pb)
{
	int i, len = 0;
	kcfg_target_t *ct;
	char *dat = NULL;

	if (!pa || !*pa)
		return -1;

	i = target_find((char*)*pa);
	if (-1 == i)
		return -1;

	ct = __g_cfg->target.arr[i];
	make_save_buffer(ct, &dat, &len);
	opt_set_cur_str(opt, dat);
	return 0;
}

static int ow_reset(int ses, void *opt, const char *path, void *wch)
{
	int i;
	kcfg_target_t *ct;

	for (i = 0; i < __g_cfg->target.cnt; i++) {
		ct = __g_cfg->target.arr[i];
		if (!strcmp(ct->name, "fakeee"))
			continue;
		if (ct)
			target_opt_clr(ct);
	}

	i = system("/stv/reset_net_set.sh");
	cfg_save_dpc(NULL, NULL);

	return 0;
}

static int ow_load(int ses, void *opt, const char *path, void *wch)
{
	kcfg_load();
	return 0;
}
static int ow_save(int ses, void *opt, const char *path, void *wch)
{
	kcfg_save();
	return 0;
}
static int ow_sync(int ses, void *opt, const char *path, void *wch)
{
	cfg_save_dpc(NULL, NULL);
	return 0;
}
static int ow_session_start(int ses, void *opt, const char *path, void *wch)
{
	return 0;
}
static int ow_session_done(int ses, void *opt, const char *path, void *wch)
{
	/* XXX: error when commit session */
	int error = 0, cancel;
	cancel = opt_get_new_int(opt);
	if (cancel) {
		klog(("ow_session_done: Cancelled.\n"));
		return EC_NG;
	}
	opt_session_set_err(opt, error);
	return 0;
}

static void setup_opt()
{
	opt_add_s("s:/k/cfg/targets", OA_GET, NULL, og_targets);
	opt_add_s("s:/k/cfg/target", OA_GET, NULL, og_target);

	opt_add_s("e:/k/cfg/reset", OA_SET, NULL, NULL);
	opt_awch("e:/k/cfg/reset", ow_reset);
	opt_add_s("e:/k/cfg/load", OA_SET, NULL, NULL);
	opt_awch("e:/k/cfg/load", ow_load);
	opt_add_s("e:/k/cfg/save", OA_SET, NULL, NULL);
	opt_awch("e:/k/cfg/save", ow_save);

	opt_awch("e:/prog/sync", ow_sync);

	opt_awch("i:/k/opt/session/start", ow_session_start);
	opt_awch("i:/k/opt/session/done", ow_session_done);
}

int kcfg_init()
{
	if (__g_cfg)
		return -1;

	setup_skipped_target();

	__g_cfg = (kcfg_t*)kmem_alloz(1, kcfg_t);

	opt_getptr("p:/prog/mque/main", (void**)&__g_mque_main);

	target_argv();
	target_file();

	setup_opt();

	return 0;
}

int kcfg_final(kcfg_t *cfg)
{
	if (!__g_cfg)
		return 0;

	free_skipped_target();
	kmem_free_sz(__g_cfg);

	return 0;
}

/**
 * \brief Save `Valid' config to memory, all the config with error
 * will be skipped.
 *
 * \return
 */

static void cfg_save_dpc(void *ua, void *ub)
{
	int i, len;
	char *dat, hash[32];
	kcfg_t *c = __g_cfg;
	kcfg_target_t *ct;

	klog((">>> cfg_save_dpc\n"));
	for (i = 0; i < c->target.cnt; i++) {
		ct = c->target.arr[i];
		if ((!ct) || (!ct->save) || is_skipped_target(ct->name))
			continue;

		dat = NULL;
		len = 0;
		make_save_buffer(ct, &dat, &len);
		klog(("make_save_buffer, return:\n%s\n", dat));

		md5_calculate(hash, dat, len);
		if (memcmp(hash, ct->data_hash, sizeof(hash))) {
			if (!ct->save(ct, dat, len, ct->ua, ct->ub))
				memcpy(ct->data_hash, hash, sizeof(hash));
			else
				kerror(("cfg_save_dpc: fail: %s\n", ct->name));
		} else
			kerror(("cfg_save_dpc:(%s): not touched\n", ct->name));

		kmem_free_s(dat);
	}
	klog(("<<< cfg_save_dpc\n"));
}

static int tc_cfg_save(void *id, void *userdata)
{
	if (id == __g_tm_cfg_save) {
		mentry_post(__g_mque_main, cfg_save_dpc, NULL, NULL, NULL);
		__g_tm_cfg_save = NULL;
	}
	return 0;
}
int kcfg_save_delay(int msec)
{
	/* FIXME: no spl_timer_xxx in windows */
#ifdef __GNUC__
	spl_timer_del(__g_tm_cfg_save);
	__g_tm_cfg_save = NULL;
	spl_timer_add(&__g_tm_cfg_save, msec, tc_cfg_save, NULL);
#endif
	return 0;
}

int kcfg_save()
{
	mentry_post(__g_mque_main, cfg_save_dpc, NULL, NULL, NULL);
	return 0;
}

static kinline int del_bad_opt(kcfg_target_t *ct, char **kvarrarr, int pos)
{
	char *k = kvarrarr[(pos * 2) + 0];

	if (-1 != target_opt_find(ct, k))
		return 0;

	kmem_free_z(kvarrarr[(pos * 2) + 0]);
	kmem_free_z(kvarrarr[(pos * 2) + 1]);
	return 1;
}

static kinline void del_dup_opt(char ***kvarrarr,
		int *kvcntarr, int kvcnt, const char *k)
{
	int i, j;
	for (i = 0; i < kvcnt; i++)
		for (j = 0; j < kvcntarr[i]; j++) {
			if (!kvarrarr[i][(j * 2) + 0])
				continue;
			if (!strcmp(k, kvarrarr[i][(j * 2) + 0])) {
				kmem_free_z(kvarrarr[i][(j * 2) + 0]);
				kmem_free_z(kvarrarr[i][(j * 2) + 1]);
			}
		}
}

static void apply_opt(char ***kvarrarr, int *kvcntarr, int kvcnt)
{
	int i, j;
	for (i = 0; i < kvcnt; i++)
		for (j = 0; j < kvcntarr[i]; j++)
			opt_setkv(0, kvarrarr[i][(j * 2) + 0],
					kvarrarr[i][(j * 2) + 1]);
}

static void free_kvarrarr(char ***kvarrarr, int *kvcntarr, int kvcnt)
{
	int i;
	for (i = 0; i < kvcnt; i++)
		opt_free_kv(kvarrarr[i], kvcntarr[i]);
}

static int load_targets()
{
	int i, j, len, *kvcntarr, kvcnt = 0;
	char ***kvarrarr, *dat;
	kcfg_t *c = __g_cfg;
	kcfg_target_t *ct;

	kvarrarr = kmem_alloz(c->target.cnt, char**);
	kvcntarr = kmem_alloz(c->target.cnt, int);

	for (i = 0; i < c->target.cnt; i++) {
		ct = c->target.arr[i];
		if ((!ct) || is_skipped_target(ct->name))
			continue;
		if (ct->load(ct, &dat, &len, ct->ua, ct->ub))
			continue;
		md5_calculate(ct->data_hash, dat, len);

		kvarrarr[kvcnt] = NULL;
		kvcntarr[kvcnt] = 0;
		if (opt_make_kv(dat, len, &kvarrarr[kvcnt], &kvcntarr[kvcnt]))
			continue;

		for (j = 0; j < kvcntarr[kvcnt]; j++)
			if (!del_bad_opt(ct, kvarrarr[kvcnt], j))
				del_dup_opt(kvarrarr, kvcntarr, kvcnt,
						(kvarrarr[kvcnt][j * 2]));

		kvcnt++;
		kmem_free(dat);
	}

	apply_opt(kvarrarr, kvcntarr, kvcnt);
	free_kvarrarr(kvarrarr, kvcntarr, kvcnt);

	kmem_free_s(kvarrarr);
	kmem_free_s(kvcntarr);

	return 0;
}

/**
 * \brief Load all the configure from disk etc to memory.
 *
 * \warning Must be called when all the other module loaded, some configure
 * item maybe failed because the opt must be added by other modules.
 *
 * \return 0 for success, otherwise for error.
 */
int kcfg_load()
{
	load_targets();
	__g_cfg_loaded++;
	return 0;
}

/*----------------------------------------------------
 * command line
 *
 * OPT from command line
 */
static int argv_load(kcfg_target_t *ct, char **dat,
		int *len, void *ua, void *ub)
{
	char *path = &ct->name[5], buf[64 * 1024];
	FILE *fp;

	fp = fopen(path, "rt");
	if (!fp)
		return -1;
	*len = fread(buf, sizeof(char), sizeof(buf), fp);
	fclose(fp);

	if (*len <= 0)
		return -1;

	*dat = (char*)kmem_alloz(*len, char);
	memcpy(*dat, buf, *len);
	return 0;
}

static int target_argv()
{
	int i, argc;
	char **argv, name[1024], *path;
	kcfg_target_t *ct;

	opt_getint("i:/prog/argc", &argc);
	opt_getptr("p:/prog/argv", (void**)&argv);

	for (i = 0; i < argc; i++)
		if (argv[i] && (!strncmp(argv[i], "--opt-file=", 11))) {
			path = argv[i] + 11;
			if (!path)
				continue;

			sprintf(name, "argv:%s", path);
			ct = kcfg_target_add(name, NULL, argv_load,
					NULL, (void*)path, NULL);
			if (ct)
				ct->pass_through = 1;
		}

	return 0;
}

/*----------------------------------------------------
 * Normal file opt
 *
 * OPT register by other module
 */
static int file_load(kcfg_target_t *ct, char **dat,
		int *len, void *ua, void *ub)
{
	char *path = &ct->name[5], buf[64 * 1024];
	FILE *fp;

	fp = fopen(path, "rt");
	if (!fp)
		return -1;
	*len = fread(buf, sizeof(char), sizeof(buf), fp);
	fclose(fp);

	if (*len <= 0)
		return -1;

	*dat = (char*)kmem_alloz(*len, char);
	memcpy(*dat, buf, *len);
	return 0;
}

static int os_cfg_target_file_add(int ses, void *opt,
		const char *path, void **pa, void **pb)
{
	char *file = opt_get_new_str(opt);
	char name[1024];
	kcfg_target_t *ct;

	sprintf(name, "file:%s", file);
	ct = kcfg_target_add(name, NULL, file_load, NULL, NULL, NULL);
	if (ct)
		ct->pass_through = 1;
	return 0;
}

static int target_file()
{
	opt_add_s("s:/k/cfg/target/file/add", OA_SET,
			os_cfg_target_file_add, NULL);
	return 0;
}

