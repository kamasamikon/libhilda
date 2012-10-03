/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include <ktypes.h>

#include <kflg.h>
#include <klog.h>
#include <kmem.h>
#include <kstr.h>
#include <sdlist.h>

#include <xtcool.h>

#include <opt.h>

/**
 * Type:
 *	arr, dat, evt, int, str, ptr
 *
 * Defination:
 *	RES: =
 *
 * Operation:
 *	arr: SET|GET|WCH
 *	bool: SET|GET|WCH
 *	dat: SET|GET|WCH
 *	evt: SET|WCH
 *	int: SET|GET|WCH
 *	str: SET|GET|WCH
 *	ptr: SET|GET|WCH
 *
 * Format (normal):
 *	arr: v.xxx.a.v = (char[][]), v.xxx.a.l = (int)
 *	dat: v.xxx.a.v = (char[]), v.xxx.a.l = (int)
 *	evt: (Empty)
 *	bool: v.xxx.i.v = (int)
 *	int: v.xxx.i.v = (int)
 *	str: v.xxx.s.v = (char*)
 *	ptr: v.xxx.p.v = (void*)
 *
 * Format (ini): No space after "="
 *	arr: =AAA;BBB;CCC
 *	dat: =123456789abcde
 *	evt: =<ignored>
 *	int: =OCT
 *	bool: =OCT
 *	str: =any_till_end_of_line
 *	ptr: =OCT
 *
 * Write:
 *	Everything copy int opt.
 *
 * Read:
 *	Return reference, call should copy them when needed.
 */

/*-----------------------------------------------------------------------
 * option interface
 */
/**
 * \defgroup FaintOpt
 * \brief Option as a database of module's options.
 * Opt is organized as a dictionary.  Every module has some
 * configuration, which can be accessed by a id, and all the ids
 * currently are public to all the module.
 */

/* XXX */
/* / : = */
/* - */
typedef struct _optcc_t optcc_t;
typedef struct _opt_watch_t opt_watch_t;
typedef struct _opt_entry_t opt_entry_t;

struct _opt_watch_t {
	/** queue to bwchhdr/awchhdr */
	K_dlist_entry entry;

	/** callback when watch hit */
	OPT_WATCH wch;
	/** have chance to free resource */
	OPT_WCH_DELTER delter;

	void *ua, *ub;

	/** watch what event */
	char *path;

	/** diag part */
	unsigned int wch_cnt;
};

struct _opt_entry_t {
	/** _optcc_t::oehdr */
	K_dlist_entry entry;

	char *path;
	char *desc;

	OPT_GETTER getter;
	OPT_SETTER setter;
	/* have chance to free resource */
	OPT_DELTER delter;

	void *ua, *ub;
	void *set_pa, *set_pb;
	void *get_pa, *get_pb;

	int ses;

	unsigned int attr;

	/* quick access of path[0] */
	char type;

	struct {
		/* cur is copy */
		union {
			struct {
				/* maintain other _opt_entry_t */
				/* kvfmt: [path1,path2,path3] */
				char **v;
				int l;
			} a;
			struct {
				/* kvfmt: "aa,bb,cc,34,78" */
				void *v;
				int l;
			} d;
			struct {
				/* kvfmt: "232432" or "0x34234" */
				int v;
			} i;
			struct {
				/* kvfmt: "Anything you like\n\rOrz" */
				char *v;
			} s;
			struct {
				/* kvfmt: "232432" or "0x34234" */
				void *v;
			} p;
		} cur;

		/* new is ref */
		union {
			struct {
				/* maintain other _opt_entry_t */
				/* kvfmt: [path1,path2,path3] */
				char **v;
				int l;
			} a;
			struct {
				/* kvfmt: "aa,bb,cc,34,78" */
				void *v;
				int l;
			} d;
			struct {
				/* kvfmt: "232432" or "0x34234" */
				int v;
			} i;
			struct {
				/* kvfmt: "Anything you like" */
				char *v;
			} s;
			struct {
				/* kvfmt: "232432" or "0x34234" */
				void *v;
			} p;
		} new;
	} v;

	/* watch list */
	K_dlist_entry bwchhdr;
	K_dlist_entry awchhdr;

	/** diag part */
	unsigned int set_called, get_called, awch_called, bwch_called;
	unsigned int awch_cnt, bwch_cnt;
};

/* Control Center for OPT */
struct _optcc_t {
	/** Opt Entry Header */
	K_dlist_entry oehdr;

	/** watch that Not Yet connect */
	struct {
		K_dlist_entry bhdr;
		/** before watch header */
		K_dlist_entry ahdr;
		/** after watch header */
	} nywch;

	int sesid_last;     /**< the last used session Id, can not be zero */
	kbean lck;	    /**< lck to protect oehdr, ahdr, bhdr */

	struct {
		struct {
			/* thread id */
			SPL_HANDLE task;
			int no;
			char msg[2048];
		} arr[4];
		int curpos;
	} err;
};

#define OPT_TYPE(oe) ((opt_entry_t*)(oe))->type

#define RET_IF_NOT_INT() do { \
	if ('i' != oe->type && 'b' != oe->type && 'e' != oe->type) { \
		wlogf("bad type, should be 'ibe'. >> %s\n", oe->path); \
		return EC_BAD_TYPE; \
	} \
} while (0)

#define RET_IF_NOT_PTR() do { \
	if ('p' != oe->type) { \
		wlogf("bad type, should be 'p'. >> %s\n", oe->path); \
		return EC_BAD_TYPE; \
	} \
} while (0)

#define RET_IF_NOT_STR() do { \
	if ('s' != oe->type) { \
		wlogf("bad type, should be 's'. >> %s\n", oe->path); \
		return EC_BAD_TYPE; \
	} \
} while (0)

#define RET_IF_NOT_ARR() do { \
	if ('a' != oe->type) { \
		wlogf("bad type, should be 'a'. >> %s\n", oe->path); \
		return EC_BAD_TYPE; \
	} \
} while (0)

#define RET_IF_NOT_DAT() do { \
	if ('d' != oe->type) { \
		wlogf("bad type, should be 'd'. >> %s\n", oe->path); \
		return EC_BAD_TYPE; \
	} \
} while (0)

#define RET_IF_RECURSIVE() do { \
	if (kflg_chk_bit(oe->attr, OA_IN_SET)) { \
		wlogf("Recursive: %s\n", oe->path); \
		kassert(!"Recursive set is not allowed"); \
		return EC_RECUR; \
	} \
} while (0)

#define RET_IF_FORBIDEN() do { \
	if (!kflg_chk_bit(oe->attr, OA_SET) && oe->set_called > 0) \
		return EC_FORBIDEN; \
} while (0)

#define SET_SET_PARA() do { \
	oe->set_pa = pa; \
	oe->set_pb = pb; \
	oe->ses = ses; \
	oe->set_called++; \
} while (0)

#define CALL_BWCH() do { \
	kflg_set(oe->attr, OA_IN_BWCH); \
	call_watch(ses, oe, &oe->bwchhdr, 0); \
	kflg_clr(oe->attr, OA_IN_BWCH); \
} while (0)

#define CALL_AWCH() do { \
	kflg_set(oe->attr, OA_IN_AWCH); \
	call_watch(ses, oe, &oe->awchhdr, 1); \
	kflg_clr(oe->attr, OA_IN_AWCH); \
} while (0)


static optcc_t *__g_optcc = NULL;

static int entry_del(opt_entry_t *oe);
static kinline opt_entry_t *entry_find(const char *path);

static int setint(int ses, opt_entry_t *oe, void *pa, void *pb, int val);
static int setptr(int ses, opt_entry_t *oe, void *pa, void *pb, void *val);
static int setstr(int ses, opt_entry_t *oe, void *pa, void *pb, char *val);
static int setdat(int ses, opt_entry_t *oe, void *pa, void *pb,
		const char *val, int len);
static int setarr(int ses, opt_entry_t *oe, void *pa, void *pb,
		const char **val, int len);

static int getint(opt_entry_t *oe, void *pa, void *pb, int *val);
static int getstr(opt_entry_t *oe, void *pa, void *pb, char **val);
static int getptr(opt_entry_t *oe, void *pa, void *pb, void **val);
static int getdat(opt_entry_t *oe, void *pa, void *pb, char **val, int *len);

/*-----------------------------------------------------------------------
 * kinline functions
 */

/* normal access */
kinline char *opt_path(void *oe)
{
	return ((opt_entry_t*)(oe))->path;
}
kinline char *opt_desc(void *oe)
{
	return ((opt_entry_t*)(oe))->desc;
}

kinline void *opt_ua(void *oe)
{
	return ((opt_entry_t*)(oe))->ua;
}
kinline void *opt_ub(void *oe)
{
	return ((opt_entry_t*)(oe))->ub;
}

kinline int opt_get_setpa(void *oe, void **pa)
{
	if (kflg_chk_any(((opt_entry_t*)oe)->attr, OA_IN_AWCH | OA_IN_BWCH | OA_IN_SET)) {
		*pa = ((opt_entry_t*)oe)->set_pa;
		return 0;
	}
	return -1;
}
kinline int opt_get_setpb(void *oe, void **pb)
{
	if (kflg_chk_any(((opt_entry_t*)oe)->attr, OA_IN_AWCH | OA_IN_BWCH | OA_IN_SET)) {
		*pb = ((opt_entry_t*)oe)->set_pb;
		return 0;
	}
	return -1;
}

kinline char **opt_get_cur_arr(void *oe)
{
	return ((opt_entry_t*)(oe))->v.cur.a.v;
}
kinline int opt_get_cur_arr_len(void *oe)
{
	return ((opt_entry_t*)(oe))->v.cur.a.l;
}
kinline void *opt_get_cur_dat(void *oe)
{
	return ((opt_entry_t*)(oe))->v.cur.d.v;
}
kinline int opt_get_cur_dat_len(void *oe)
{
	return ((opt_entry_t*)(oe))->v.cur.d.l;
}
kinline int opt_get_cur_int(void *oe)
{
	return ((opt_entry_t*)(oe))->v.cur.i.v;
}
kinline char *opt_get_cur_str(void *oe)
{
	return ((opt_entry_t*)(oe))->v.cur.s.v;
}
kinline void *opt_get_cur_ptr(void *oe)
{
	return ((opt_entry_t*)(oe))->v.cur.p.v;
}

kinline char **opt_get_new_arr(void *oe)
{
	return ((opt_entry_t*)(oe))->v.new.a.v;
}
kinline int opt_get_new_arr_len(void *oe)
{
	return ((opt_entry_t*)(oe))->v.new.a.l;
}
kinline void *opt_get_new_dat(void *oe)
{
	return ((opt_entry_t*)(oe))->v.new.d.v;
}
kinline int opt_get_new_dat_len(void *oe)
{
	return ((opt_entry_t*)(oe))->v.new.d.l;
}
kinline int opt_get_new_int(void *oe)
{
	return ((opt_entry_t*)(oe))->v.new.i.v;
}
kinline char *opt_get_new_str(void *oe)
{
	return ((opt_entry_t*)(oe))->v.new.s.v;
}
kinline void *opt_get_new_ptr(void *oe)
{
	return ((opt_entry_t*)(oe))->v.new.p.v;
}

kinline void *wch_path(void *ow)
{
	return ((opt_watch_t*)(ow))->path;
}

kinline void *wch_ua(void *ow)
{
	return ((opt_watch_t*)(ow))->ua;
}

kinline void *wch_ub(void *ow)
{
	return ((opt_watch_t*)(ow))->ub;
}

kinline int opt_set_cur_arr(void *oe, char **val, int len)
{
	opt_entry_t *e = (opt_entry_t*)oe;

	if (e->attr & (OA_IN_SET | OA_IN_GET)) {
		kmem_free_sz(e->v.cur.a.v);
		if (len < 0)
			len = 0;
		if (len > 0) {
			kmem_free_sz(e->v.cur.a.v);
			e->v.cur.a.v = (char**)kmem_alloz(len, char*);
			memcpy(e->v.cur.a.v, val, len * sizeof(char*));
		}
		e->v.cur.a.l = len;
		return EC_OK;
	}
	return EC_FORBIDEN;
}

kinline int opt_set_cur_dat(void *oe, char *val, int len)
{
	opt_entry_t *e = (opt_entry_t*)oe;

	if (e->attr & (OA_IN_SET | OA_IN_GET)) {
		kmem_free_sz(e->v.cur.d.v);
		if (len < 0)
			len = 0;
		if (len > 0) {
			kmem_free_sz(e->v.cur.d.v);
			e->v.cur.d.v = (char*)kmem_alloz(len, char);
			memcpy(e->v.cur.d.v, val, len * sizeof(char));
		}
		e->v.cur.d.l = len;
		return EC_OK;
	}
	return EC_FORBIDEN;
}

kinline int opt_set_cur_int(void *oe, int val)
{
	opt_entry_t *e = (opt_entry_t*)oe;

	if (e->attr & (OA_IN_SET | OA_IN_GET)) {
		e->v.cur.i.v = val;
		return EC_OK;
	}
	return EC_FORBIDEN;
}

kinline int opt_set_cur_str(void *oe, char *val)
{
	opt_entry_t *e = (opt_entry_t*)oe;

	if (e->attr & (OA_IN_SET | OA_IN_GET)) {
		kmem_free_sz(e->v.cur.s.v);
		e->v.cur.s.v = kstr_dup(val);
		return EC_OK;
	}
	return EC_FORBIDEN;
}

kinline int opt_set_cur_ptr(void *oe, void *val)
{
	opt_entry_t *e = (opt_entry_t*)oe;

	if (e->attr & (OA_IN_SET | OA_IN_GET)) {
		e->v.cur.p.v = val;
		return EC_OK;
	}
	return EC_FORBIDEN;
}

void opt_set_err(int no, const char *msg)
{
	optcc_t *cc = __g_optcc;
	SPL_HANDLE curtsk = spl_thread_current();
	int i, arrlen = sizeof(cc->err.arr) / sizeof(cc->err.arr[0]);

	/* spl_lck_get(__g_optcc->lck); */

	if (!msg)
		msg = "";

	for (i = 0; i < arrlen; i++)
		if (cc->err.arr[i].task == curtsk) {
			cc->err.arr[i].no = no;
			strncpy(cc->err.arr[i].msg, msg,
					sizeof(cc->err.arr[i].msg));

			/* spl_lck_rel(__g_optcc->lck); */
			return;
		}

	i = cc->err.curpos = (++cc->err.curpos) % arrlen;

	cc->err.arr[i].task = curtsk;
	cc->err.arr[i].no = no;
	strncpy(cc->err.arr[i].msg, msg, sizeof(cc->err.arr[i].msg));

	/* spl_lck_rel(__g_optcc->lck); */
}

int opt_get_err(int *no, char **msg)
{
	optcc_t *cc = __g_optcc;
	SPL_HANDLE curtsk = spl_thread_current();
	int i, arrlen = sizeof(cc->err.arr) / sizeof(cc->err.arr[0]);

	/* spl_lck_get(__g_optcc->lck); */

	for (i = 0; i < arrlen; i++)
		if (cc->err.arr[i].task == curtsk) {
			*no = cc->err.arr[i].no;
			*msg = cc->err.arr[i].msg;

			/* spl_lck_rel(__g_optcc->lck); */
			return 0;
		}

	/* spl_lck_rel(__g_optcc->lck); */
	return -1;
}


/**
 * \brief Convert the config buffer come from config file to KV pair
 *
 * \param buffer Read from config file etc
 * \param blen len of the buffer
 * \param okv k=okv[2n], v=okv[2n+1]
 * \param ocnt
 *
 * \warning Caller should call \c kmem_free() to release okv
 *
 * \return 0 for success, -1 for error
 */
int opt_make_kv(const char *buffer, int blen, char ***okv, int *ocnt)
{
	char kbuf[2048], vbuf[2048], *tmpkv[2 * 2000];
	int i, ki = 0, vi = 0, km, vm, kvcnt = 0; /* index and mode */
	char **kvarr = 0, c;

	if (!buffer)
		return EC_BAD_PARAM;

	km = 1;
	vm = 0;
	for (i = 0; (i < blen) && (c = buffer[i]); i++) {
		if (km) {
			if (c == '=') {
				kbuf[ki] = '\0';
				km = 0;
				vm = 1;
			} else if (c == '\n' || c == '\r') {
				/* error! no = when hit end */
			} else if (c != ' ') {
				kbuf[ki++] = c;
			}
		} else if (vm) {
			if (c == '\n' || c == '\r') {
				/* end of kv, push them to queue */
				vbuf[vi] = '\0';
				if (kbuf[0] != '\0') {
					tmpkv[(kvcnt << 1) + 0] = kbuf[0] ? kstr_dup(kbuf) : NULL;
					tmpkv[(kvcnt << 1) + 1] = vbuf[0] ? kstr_dup(vbuf) : kstr_dup("");
					kvcnt++;
					/* printf("'%s'='%s'\n", kbuf, vbuf); */
				}

				km = 1;
				vm = 0;
				ki = vi = 0;
			} else {
				vbuf[vi++] = c;
			}
		}
	}

	if (vm && (kbuf[0] != '\0')) {
		vbuf[vi] = '\0';
		tmpkv[(kvcnt << 1) + 0] = kbuf[0] ? kstr_dup(kbuf) : NULL;
		tmpkv[(kvcnt << 1) + 1] = vbuf[0] ? kstr_dup(vbuf) : kstr_dup("");
		kvcnt++;
		/* printf("'%s'='%s'\n", kbuf, vbuf); */
	}

	if (kvcnt > 0) {
		kvarr = kmem_alloz(2 * kvcnt, char*);
		memcpy(kvarr, tmpkv, (2 * kvcnt * sizeof(char*)));
	}

	*okv = kvarr;
	*ocnt = kvcnt;

	return EC_OK;
}

void opt_free_kv(char **kv, int cnt)
{
	int i, all = cnt * 2;

	for (i = 0; i < all; i++)
		kmem_free_s(kv[i]);

	kmem_free_s(kv);
}

/**
 * \brief undo \c opt_make_kv(), convert kv dict to a ini liked buffer.
 *
 * \param k Path of the opt.
 * \param v Value of the opt, as string format.
 *
 * \return New allocate buffer. Caller should call \c kmem_free() to free it.
 */
char *opt_pack_kv(const char **k, const char **v)
{
	/* FIXME: should calcuate the length but allocate one big enough */
	char *buffer = (char*)kmem_alloz(1024 * 1024, char), *curpos = buffer;

	while (*k) {
		curpos = strcat(curpos, *k);
		curpos = strcat(curpos, "=");
		curpos = strcat(curpos, *v);
		curpos = strcat(curpos, "\n");
		k++;
		v++;
	}
	return buffer;
}

/* user data for setxx */

static kinline opt_entry_t *entry_find(const char *path)
{
	K_dlist_entry *entry;
	opt_entry_t *oe;

	if (!path || !__g_optcc)
		return NULL;

	entry = __g_optcc->oehdr.next;
	while (entry != &__g_optcc->oehdr) {
		oe = FIELD_TO_STRUCTURE(entry, opt_entry_t, entry);
		entry = entry->next;

		if (0 == strcmp(path, oe->path))
			return oe;
	}

	return NULL;
}

int opt_setfile(const char *path)
{
	FILE *fp;
	int len, bufsize = 32 * 1024;
	char *buf = NULL;

	fp = fopen(path, "rt");
	if (!fp)
		return -1;

	buf = (char*)kmem_alloz(bufsize, char);
	len = fread(buf, sizeof(char), bufsize, fp);
	fclose(fp);

	if (len > 0)
		opt_setbat(buf, 0);

	kmem_free_s(buf);
	return 0;
}

int opt_setbat(const char *inibuf, int errbrk)
{
	int i, cnt = 0, opt_ret = EC_NG, sid, ses_ret, reterr = 0;
	char **kv = NULL, *k, *v;

	/* spl_lck_get(__g_optcc->lck); */
	opt_set_err(0, NULL);

	opt_make_kv(inibuf, 0x7fffffff, &kv, &cnt);
	sid = opt_session_start();
	for (i = 0; i < cnt; i++) {
		k = kv[(i << 1) + 0];
		v = kv[(i << 1) + 1];

		opt_ret = opt_setkv(sid, k, v);
		if (opt_ret == EC_SKIP)
			opt_ret = EC_OK;
		if (opt_ret) {
			kerror("ret:%d, k:<%s>, v:<%s>", opt_ret, k, v);
			if (errbrk)
				break;
		}
	}
	ses_ret = opt_session_commit(sid, opt_ret, &reterr);
	/* spl_lck_rel(__g_optcc->lck); */

	opt_free_kv(kv, cnt);
	klog("opt:%d, ses:%d, ret:%d\n", opt_ret, ses_ret, reterr);
	return opt_ret | ses_ret | reterr;
}

static char *dat_to_str(char *dat, int len)
{
	static char map[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8',
		'9', 'a', 'b', 'c', 'd', 'e', 'f' };
	char *ret, *p;
	int i;
	unsigned char c;

	p = ret = (char*)kmem_alloc(len * 2 + 1, char);
	for (i = 0; i < len; i++) {
		c = (unsigned char)dat[i];
		*p++ = map[c >> 4];
		*p++ = map[c & 0x0f];
	}
	*p = 0;
	return ret;
}

int opt_getini_by_opt(void *opt, char **ret)
{
	opt_entry_t *oe = (opt_entry_t*)opt;
	int err = EC_NG, ival, dlen;
	char *sval, *dval;
	void *pval;

	switch (OPT_TYPE(oe)) {
	case 'a':
		/* TODO */
		break;
	case 'i':
	case 'b':
		err = getint(oe, NULL, NULL, &ival);
		if (err != EC_OK)
			break;
		*ret = (char*)kmem_alloz(20, char);
		sprintf(*ret, "%d", ival);
		break;
	case 'd':
		/* TODO */
		err = getdat(oe, NULL, NULL, &dval, &dlen);
		if (err != EC_OK)
			break;
		*ret = dat_to_str(dval, dlen);
		break;
	case 'e':
		/* XXX, can not get a event */
		return EC_FORBIDEN;
	case 's':
		err = getstr(oe, NULL, NULL, &sval);
		if (err != EC_OK)
			break;
		*ret = kstr_dup(sval);
		break;
	case 'p':
		err = getptr(oe, NULL, NULL, &pval);
		if (err != EC_OK)
			break;
		*ret = (char*)kmem_alloz(10, char);
		sprintf(*ret, "%p", pval);
		break;
	default:
		kassert(!"should not be here");
		break;
	}

	return err;
}

/**
 * \brief Get opt's in ini format.
 *
 * \param path
 * \param ret
 *
 * \return
 */
int opt_getini(const char *path, char **ret)
{
	opt_entry_t *oe = entry_find(path);
	int err = EC_NG;

	*ret = NULL;

	/* spl_lck_get(__g_optcc->lck); */
	if (!oe) {
		/* spl_lck_rel(__g_optcc->lck); */
		kerror("Opt not found: <%s>\n", path);
		return EC_NOTFOUND;
	}

	opt_set_err(0, NULL);
	err = opt_getini_by_opt(oe, ret);

	/* spl_lck_rel(__g_optcc->lck); */
	return err;
}

static void sync_from_nylist(opt_entry_t *oe)
{
	K_dlist_entry *entry;
	opt_watch_t *wch;
	char *path;

	if (!(oe->attr & OA_WCH))
		return;

	path = oe->path;
	entry = __g_optcc->nywch.ahdr.next;
	while (entry != &__g_optcc->nywch.ahdr) {
		wch = FIELD_TO_STRUCTURE(entry, opt_watch_t, entry);
		entry = entry->next;
		if (!strcmp(wch->path, path)) {
			remove_dlist_entry(&wch->entry);
			insert_dlist_tail_entry(&oe->awchhdr, &wch->entry);
			oe->awch_cnt++;
		}
	}
	entry = __g_optcc->nywch.bhdr.next;
	while (entry != &__g_optcc->nywch.bhdr) {
		wch = FIELD_TO_STRUCTURE(entry, opt_watch_t, entry);
		entry = entry->next;
		if (!strcmp(wch->path, path)) {
			remove_dlist_entry(&wch->entry);
			insert_dlist_tail_entry(&oe->bwchhdr, &wch->entry);
			oe->bwch_cnt++;
		}
	}
}

/*
 * XXX
 * opt = the whole entry
 * path = id of entry
 */
int opt_new(const char *path, const char *desc, unsigned int attr,
		OPT_SETTER setter, OPT_GETTER getter, OPT_DELTER delter,
		void *ua, void *ub)
{
	opt_entry_t *oe;

	/* spl_lck_get(__g_optcc->lck); */

	if (!OPT_CHK_TYPE(path)) {
		/* spl_lck_rel(__g_optcc->lck); */
		kerror("BadType: %s\n", path);
		kassert(0 && "opt path should be '[a|i|d|s|b|e|p]:/Xxx'");
		return EC_BAD_TYPE;
	}

	oe = entry_find(path);
	if (oe) {
		kerror("!!!Duplicate opt added (%s), abort adding\n", path);
		/* spl_lck_rel(__g_optcc->lck); */
		return EC_NOTFOUND;
	}

	oe = (opt_entry_t*)kmem_alloz(1, opt_entry_t);

	oe->ua = ua;
	oe->ub = ub;
	oe->path = kstr_dup(path);
	oe->desc = kstr_dup(desc);
	oe->type = path[0];
	oe->attr = attr & OA_MSK;
	oe->setter = setter;
	oe->getter = getter;
	oe->delter = delter;

	init_dlist_head(&oe->entry);
	insert_dlist_tail_entry(&__g_optcc->oehdr, &oe->entry);

	init_dlist_head(&oe->awchhdr);
	init_dlist_head(&oe->bwchhdr);

	/* process the NY watches */
	sync_from_nylist(oe);
	/* spl_lck_rel(__g_optcc->lck); */

	return EC_OK;
}

static void pushback_nylist(opt_entry_t *oe)
{
	K_dlist_entry *entry;
	opt_watch_t *wch;

	entry = oe->awchhdr.next;
	while (entry != &oe->awchhdr) {
		wch = FIELD_TO_STRUCTURE(entry, opt_watch_t, entry);
		entry = entry->next;

		remove_dlist_entry(&wch->entry);
		insert_dlist_tail_entry(&__g_optcc->nywch.ahdr, &wch->entry);
	}

	entry = oe->bwchhdr.next;
	while (entry != &oe->bwchhdr) {
		wch = FIELD_TO_STRUCTURE(entry, opt_watch_t, entry);
		entry = entry->next;

		remove_dlist_entry(&wch->entry);
		insert_dlist_tail_entry(&__g_optcc->nywch.bhdr, &wch->entry);
	}
}

static void free_opt_val(opt_entry_t *oe)
{
	switch (OPT_TYPE(oe)) {
	case 'a':
		kmem_free_sz(oe->v.cur.a.v);
		kmem_free_sz(oe->v.new.a.v);
		break;
	case 'd':
		kmem_free_sz(oe->v.cur.d.v);
		kmem_free_sz(oe->v.new.d.v);
		break;
	case 's':
		kmem_free_sz(oe->v.cur.s.v);
		kmem_free_sz(oe->v.new.s.v);
		break;
	case 'i':
	case 'b':
	case 'e':
	case 'p':
	default:
		break;
	}
}

static int entry_del(opt_entry_t *oe)
{
	kflg_set(oe->attr, OA_IN_DEL);

	pushback_nylist(oe);
	if (oe->delter)
		oe->delter(oe);
	remove_dlist_entry(&oe->entry);

	kflg_clr(oe->attr, OA_IN_DEL);

	free_opt_val(oe);

	kmem_free_sz(oe->desc);
	kmem_free_sz(oe->path);
	kmem_free(oe);

	return EC_OK;
}

int opt_del(const char *path)
{
	opt_entry_t *oe;

	/* spl_lck_get(__g_optcc->lck); */

	oe = entry_find(path);
	if (oe) {
		entry_del(oe);
		/* spl_lck_rel(__g_optcc->lck); */
		return EC_OK;
	} else {
		kerror("Opt not found: <%s>\n", path);
		opt_set_err(EC_NG, "Opt not found");
	}

	/* spl_lck_rel(__g_optcc->lck); */
	return EC_NOTFOUND;
}

/**
 * \brief Return a type of a existed opt,
 *
 * \param path Path to a opt.
 *
 * \return type 'i', 's', 'p', 'a', 'd', -1 error
 */
int opt_type(const char *path)
{
	opt_entry_t *oe = entry_find(path);

	if (oe)
		return OPT_TYPE(oe);
	opt_set_err(EC_NG, "Opt not found");
	kerror("Opt not found: <%s>\n", path);
	return -1;
}

static void call_watch(int ses, opt_entry_t *oe,
		K_dlist_entry *wchhdr, int awch)
{
	K_dlist_entry *entry;
	opt_watch_t *ow = NULL;

	entry = wchhdr->next;
	while (entry != wchhdr) {
		ow = FIELD_TO_STRUCTURE(entry, opt_watch_t, entry);
		entry = entry->next;

		if (ow->wch) {
			ow->wch(ses, (void*)oe, (void*)ow);
			oe->awch_called += !!awch;
			oe->awch_called += !awch;
		}
	}
}

/**
 * \brief Parse ini format arr "aaa;d;cccc"
 *
 * \param v Value of opt KV
 * \param a Returns a array of char**,
 *	caller should call \c kmem_free_s() for it
 * \param l Length of the returned arrary.
 *
 * \return 0 for OK, otherwise for NG.
 */
static int parse_arr(const char *v, char***a, int *l)
{
	const char *t = v;
	int cnt = 0, i, ai = 0;
	char *buf, c;

	cnt = 1;
	if (!t) {
		*a = (char**)kmem_alloz(cnt, char*);
		(*a)[ai++] = kstr_dup("");
		*l = cnt;
		return EC_OK;
	}

	/* aaa;bbb;ccc */
	while ((c = *t++)) {
		if (c == ';')
			cnt++;
	}

	buf = (char*)kmem_alloz(t - v + 1, char);
	*a = (char**)kmem_alloz(cnt, char*);
	*l = cnt;

	t = v;
	i = 0;
	while ((c = *t++)) {
		buf[i++] = c;
		if (c == ';') {
			buf[i - 1] = '\0';

			/* arr index inc */
			(*a)[ai++] = kstr_dup(buf);
			i = 0;
		}
	}

	/* XXX Last one */
	(*a)[ai++] = kstr_dup(buf);

	kmem_free_s(buf);
	return EC_OK;
}

static kinline int parse_dat(const char *v, char**d, int *l)
{
	return -1;
}

static kinline char *parse_str(const char *v)
{
	return (char*)v;
}

static kinline int parse_ptr(const char *v, void **ret)
{
	return kstr_toint(v, (int*)ret);
}

/**
 * \brief Batch set OPT by KV dictionary.
 *
 * \param ses
 * \param k Key
 * \param v Value
 *
 * \return  0 for success, -1 for bad path,  -2 for bad value
 */
int opt_setkv(int ses, const char *k, const char *v)
{
	int ret = 0, l, iv;
	opt_entry_t *oe;
	char **a;
	char *d;
	void *pv;

	/* spl_lck_get(__g_optcc->lck); */

	if (!k) {
		kerror("NULL key\n");
		opt_set_err(EC_NOTFOUND, "Opt not found.");

		/* spl_lck_rel(__g_optcc->lck); */
		return EC_NOTFOUND;
	}

	if (!OPT_CHK_TYPE(k)) {
		kerror("BadType: %s\n", k);
		kassert(0 && "opt path should be '[a|i|d|s|b|e|p]:/Xxx'");
		opt_set_err(EC_BAD_TYPE, "Bad OPT Type.");
		/* spl_lck_rel(__g_optcc->lck); */
		return EC_BAD_TYPE;
	}

	oe = entry_find(k);
	if (!oe) {
		/* spl_lck_rel(__g_optcc->lck); */
		kerror("Opt not found: <%s>\n", k);
		return EC_NOTFOUND;
	}

	klog("'%s' = '%s'\n", k, v);
	switch (OPT_TYPE(oe)) {
	case 'a':
		parse_arr(v, &a, &l);
		ret = setarr(ses, oe, NULL, NULL, (const char **)a, l);
		kmem_free_s(a);
		break;
	case 'b':
	case 'e':
	case 'i':
		ret = kstr_toint(v, &iv);
		if (EC_OK == ret)
			ret = setint(ses, oe, NULL, NULL, iv);
		else
			opt_set_err(EC_BAD_PARAM, "Bad number");
		break;
	case 'd':
		/* TODO: do like arr */
		parse_dat(v, &d, &l);
		kassert(!"opt_setkv for type 'd' is not implemented.\n");
		break;
	case 's':
		ret = setstr(ses, oe, NULL, NULL, parse_str(v));
		break;
	case 'p':
		ret = parse_ptr(v, &pv);
		if (EC_OK == ret)
			ret = setptr(ses, oe, NULL, NULL, pv);
		else
			opt_set_err(EC_BAD_PARAM, "Bad address");
		break;
	default:
		kassert(!"should not be here");
		ret = EC_NG;
	}

	/* spl_lck_rel(__g_optcc->lck); */
	return ret;
}

/**
 * \brief set int value of a opt
 *
 * \param ses Session this operation in
 *
 * \retval 0 success
 * \retval -1 bad type
 * \retval -2 not allowed
 * \retval -.
 */
static int setint(int ses, opt_entry_t *oe, void *pa, void *pb, int val)
{
	int ret = 0;

	RET_IF_NOT_INT();
	RET_IF_RECURSIVE();
	RET_IF_FORBIDEN();

	kflg_set(oe->attr, OA_IN_SET);
	oe->v.new.i.v = val;
	SET_SET_PARA();
	CALL_BWCH();

	if ('e' == OPT_TYPE(oe))
		oe->v.cur.i.v = oe->set_called;
	else {
		if (oe->setter) {
			ret = oe->setter(ses, oe, oe->set_pa, oe->set_pb);
			if (EC_DEFAULT == ret) {
				ret = 0;
				oe->v.cur.i.v = val;
			} else if (ret == EC_SKIP) {
				kflg_clr(oe->attr, OA_IN_SET);
				return ret;
			} else if (ret != EC_OK)
				kerror("set fail: %s, val:%d\n", oe->path, val);
		} else
			oe->v.cur.i.v = val;

		if ('b' == OPT_TYPE(oe))
			oe->v.cur.i.v = val;
	}

	CALL_AWCH();
	kflg_clr(oe->attr, OA_IN_SET);
	return ret;
}

int opt_setint_sp(int ses, const char *path, void *pa, void *pb, int val)
{
	int ret = -1;
	opt_entry_t *oe;

	/* spl_lck_get(__g_optcc->lck); */

	oe = entry_find(path);
	if (oe)
		ret = setint(ses, oe, pa, pb, val);
	else {
		kerror("Opt not found: <%s>\n", path);
		opt_set_err(EC_NG, "Opt not found");
	}

	/* spl_lck_rel(__g_optcc->lck); */
	return ret;
}

static int getint(opt_entry_t *oe, void *pa, void *pb, int *val)
{
	int ret = 0;

	RET_IF_NOT_INT();

	kflg_set(oe->attr, OA_IN_GET);

	oe->get_called++;

	/*
	 * XXX: How to deal with pa, pb when no getter set
	 */
	if (oe->getter) {
		oe->get_pa = pa;
		oe->get_pb = pb;
		ret = oe->getter(oe, oe->get_pa, oe->get_pb);
		if (EC_DEFAULT == ret)
			ret = 0;
	}
	*val = oe->v.cur.i.v;

	kflg_clr(oe->attr, OA_IN_GET);

	return ret;
}

int opt_getint_p(const char *path, void *pa, void *pb, int *val)
{
	int ret = -1;
	opt_entry_t *oe;

	/* spl_lck_get(__g_optcc->lck); */

	oe = entry_find(path);
	if (oe)
		ret = getint(oe, pa, pb, val);
	else {
		kerror("Opt not found: <%s>\n", path);
		opt_set_err(EC_NG, "Opt not found");
	}

	/* spl_lck_rel(__g_optcc->lck); */
	return ret;
}

/**
 * \brief set ptr value of a opt
 *
 * \param ses Session this operation in
 *
 * \retval 0 success
 * \retval -1 bad type
 * \retval -2 not allowed
 * \retval -.
 */
static int setptr(int ses, opt_entry_t *oe, void *pa, void *pb, void *val)
{
	int ret = 0;

	RET_IF_NOT_PTR();
	RET_IF_RECURSIVE();
	RET_IF_FORBIDEN();

	kflg_set(oe->attr, OA_IN_SET);
	oe->v.new.p.v = val;
	SET_SET_PARA();
	CALL_BWCH();

	if (oe->setter) {
		ret = oe->setter(ses, oe, oe->set_pa, oe->set_pb);
		if (EC_DEFAULT == ret) {
			ret = 0;
			oe->v.cur.p.v = val;
		} else if (ret == EC_SKIP) {
			kflg_clr(oe->attr, OA_IN_SET);
			return ret;
		} else if (ret != EC_OK)
			kerror("set fail: %s, val:%p\n", oe->path, val);
	} else
		oe->v.cur.p.v = val;

	CALL_AWCH();
	kflg_clr(oe->attr, OA_IN_SET);
	return ret;
}

int opt_setptr_sp(int ses, const char *path, void *pa, void *pb, void *val)
{
	int ret = -1;
	opt_entry_t *oe;

	/* spl_lck_get(__g_optcc->lck); */

	oe = entry_find(path);
	if (oe)
		ret = setptr(ses, oe, pa, pb, val);
	else {
		kerror("Opt not found: <%s>\n", path);
		opt_set_err(EC_NG, "Opt not found");
	}

	/* spl_lck_rel(__g_optcc->lck); */
	return ret;
}

static int getptr(opt_entry_t *oe, void *pa, void *pb, void **val)
{
	int ret = 0;

	RET_IF_NOT_PTR();

	kflg_set(oe->attr, OA_IN_GET);

	oe->get_called++;

	/*
	 * XXX: How to deal with pa, pb when no getter set
	 */
	if (oe->getter) {
		oe->get_pa = pa;
		oe->get_pb = pb;
		ret = oe->getter(oe, oe->get_pa, oe->get_pb);
		if (EC_DEFAULT == ret)
			ret = 0;
	}
	*val = oe->v.cur.p.v;

	kflg_clr(oe->attr, OA_IN_GET);

	return ret;
}

int opt_getptr_p(const char *path, void *pa, void *pb, void **val)
{
	int ret = -1;
	opt_entry_t *oe;

	/* spl_lck_get(__g_optcc->lck); */

	oe = entry_find(path);
	if (oe)
		ret = getptr(oe, pa, pb, val);
	else {
		kerror("Opt not found: <%s>\n", path);
		opt_set_err(EC_NG, "Opt not found");
	}

	/* spl_lck_rel(__g_optcc->lck); */
	return ret;
}

/**
 * \brief set str value of a opt
 *
 * \param ses Session this operation in
 *
 * \retval 0 success
 * \retval -1 bad type
 * \retval -2 not allowed
 * \retval -.
 */
static int setstr(int ses, opt_entry_t *oe, void *pa, void *pb, char *val)
{
	int ret = 0;

	RET_IF_NOT_STR();
	RET_IF_RECURSIVE();
	RET_IF_FORBIDEN();

	kflg_set(oe->attr, OA_IN_SET);
	oe->v.new.s.v = val;
	SET_SET_PARA();
	CALL_BWCH();

	if (oe->setter) {
		ret = oe->setter(ses, oe, oe->set_pa, oe->set_pb);
		if (EC_DEFAULT == ret) {
			ret = 0;
			kmem_free_sz(oe->v.cur.s.v);
			oe->v.cur.s.v = kstr_dup(val);
		} else if (ret == EC_SKIP) {
			kflg_clr(oe->attr, OA_IN_SET);
			return ret;
		} else if (ret != EC_OK)
			kerror("set fail: %s, val:%s\n", oe->path, val);
	} else {
		kmem_free_sz(oe->v.cur.s.v);
		oe->v.cur.s.v = kstr_dup(val);
	}

	CALL_AWCH();
	kflg_clr(oe->attr, OA_IN_SET);
	return ret;
}

int opt_setstr_sp(int ses, const char *path, void *pa, void *pb, char *val)
{
	int ret = -1;
	opt_entry_t *oe;

	/* spl_lck_get(__g_optcc->lck); */

	oe = entry_find(path);
	if (oe)
		ret = setstr(ses, oe, pa, pb, val);
	else {
		kerror("Opt not found: <%s>\n", path);
		opt_set_err(EC_NG, "Opt not found");
	}

	/* spl_lck_rel(__g_optcc->lck); */
	return ret;
}

static int getstr(opt_entry_t *oe, void *pa, void *pb, char **val)
{
	int ret = 0;

	RET_IF_NOT_STR();

	kflg_set(oe->attr, OA_IN_GET);

	oe->get_called++;

	/*
	 * XXX: How to deal with pa, pb when no getter set
	 */
	if (oe->getter) {
		oe->get_pa = pa;
		oe->get_pb = pb;
		ret = oe->getter(oe, oe->get_pa, oe->get_pb);
		if (EC_DEFAULT == ret)
			ret = 0;
	}
	*val = oe->v.cur.s.v;

	kflg_clr(oe->attr, OA_IN_GET);

	return ret;
}

int opt_getstr_p(const char *path, void *pa, void *pb, char **val)
{
	int ret = -1;
	opt_entry_t *oe;

	/* spl_lck_get(__g_optcc->lck); */

	oe = entry_find(path);
	if (oe)
		ret = getstr(oe, pa, pb, val);
	else {
		kerror("Opt not found: <%s>\n", path);
		opt_set_err(EC_NG, "Opt not found");
	}

	/* spl_lck_rel(__g_optcc->lck); */
	return ret;
}

static int setarr(int ses, opt_entry_t *oe,
		void *pa, void *pb, const char **val, int len)
{
	int ret = 0;

	RET_IF_NOT_ARR();
	RET_IF_RECURSIVE();
	RET_IF_FORBIDEN();

	kflg_set(oe->attr, OA_IN_SET);
	oe->v.new.a.v = (char**)val;
	oe->v.new.a.l = len;
	SET_SET_PARA();
	CALL_BWCH();

	if (oe->setter) {
		ret = oe->setter(ses, oe, oe->set_pa, oe->set_pb);
		if (EC_DEFAULT == ret) {
			ret = 0;
			kmem_free_sz(oe->v.cur.a.v);
			oe->v.cur.a.v = (char**)kmem_alloz(len, char);
			memcpy(oe->v.cur.a.v, val, len);
			oe->v.cur.a.l = len;
		} else if (ret == EC_SKIP) {
			kflg_clr(oe->attr, OA_IN_SET);
			return ret;
		} else if (ret != EC_OK)
			kerror("set fail: %s.\n", oe->path);
	} else {
		kmem_free_sz(oe->v.cur.a.v);
		oe->v.cur.a.v = (char**)kmem_alloz(len, char);
		memcpy(oe->v.cur.a.v, val, len);
		oe->v.cur.a.l = len;
	}

	CALL_AWCH();
	kflg_clr(oe->attr, OA_IN_SET);
	return ret;
}

int opt_setarr_sp(int ses, const char *path,
		void *pa, void *pb, const char **val, int len)
{
	int ret = -1;
	opt_entry_t *oe;

	/* spl_lck_get(__g_optcc->lck); */

	oe = entry_find(path);
	if (oe)
		ret = setarr(ses, oe, pa, pb, val, len);
	else {
		kerror("Opt not found: <%s>\n", path);
		opt_set_err(EC_NG, "Opt not found");
	}

	/* spl_lck_rel(__g_optcc->lck); */
	return ret;
}

int opt_getarr_p(const char *path, void *pa, void *pb, void **arr, int *len)
{
	return EC_OK;
}

static int setdat(int ses, opt_entry_t *oe,
		void *pa, void *pb, const char *val, int len)
{
	int ret = 0;

	RET_IF_NOT_DAT();
	RET_IF_RECURSIVE();
	RET_IF_FORBIDEN();

	kflg_set(oe->attr, OA_IN_SET);
	oe->v.new.d.v = (void*)val;
	oe->v.new.d.l = len;
	SET_SET_PARA();
	CALL_BWCH();

	if (oe->setter) {
		ret = oe->setter(ses, oe, oe->set_pa, oe->set_pb);
		if (EC_DEFAULT == ret) {
			ret = 0;
			kmem_free_sz(oe->v.cur.d.v);
			oe->v.cur.d.v = (char**)kmem_alloz(len, char);
			memcpy(oe->v.cur.d.v, val, len);
			oe->v.cur.d.l = len;
		} else if (ret == EC_SKIP) {
			kflg_clr(oe->attr, OA_IN_SET);
			return ret;
		} else if (ret != EC_OK)
			kerror("set fail: %s\n", oe->path);
	} else {
		kmem_free_sz(oe->v.cur.d.v);
		oe->v.cur.d.v = (char*)kmem_alloc(len, char);
		memcpy(oe->v.cur.d.v, val, len);
		oe->v.cur.d.l = len;
	}

	CALL_AWCH();
	kflg_clr(oe->attr, OA_IN_SET);
	return ret;
}

int opt_setdat_sp(int ses, const char *path,
		void *pa, void *pb, const char *val, int len)
{
	int ret = -1;
	opt_entry_t *oe;

	/* spl_lck_get(__g_optcc->lck); */

	oe = entry_find(path);
	if (oe)
		ret = setdat(ses, oe, pa, pb, val, len);
	else {
		kerror("Opt not found: <%s>\n", path);
		opt_set_err(EC_NG, "Opt not found");
	}

	/* spl_lck_rel(__g_optcc->lck); */
	return ret;
}

static int getdat(opt_entry_t *oe, void *pa, void *pb, char **val, int *len)
{
	int ret = 0;

	RET_IF_NOT_DAT();

	kflg_set(oe->attr, OA_IN_GET);

	oe->get_called++;

	/*
	 * XXX: How to deal with pa, pb when no getter set
	 */
	if (oe->getter) {
		oe->get_pa = pa;
		oe->get_pb = pb;
		ret = oe->getter(oe, oe->get_pa, oe->get_pb);
		if (EC_DEFAULT == ret)
			ret = 0;
	}
	*val = oe->v.cur.d.v;
	*len = oe->v.cur.d.l;

	kflg_clr(oe->attr, OA_IN_GET);

	return ret;
}

int opt_getdat_p(const char *path, void *pa, void *pb, char **val, int *len)
{
	int ret = -1;
	opt_entry_t *oe;

	/* spl_lck_get(__g_optcc->lck); */

	oe = entry_find(path);
	if (oe)
		ret = getdat(oe, pa, pb, val, len);
	else {
		kerror("Opt not found: <%s>\n", path);
		opt_set_err(EC_NG, "Opt not found");
	}

	/* spl_lck_rel(__g_optcc->lck); */
	return ret;
}

/* find . -name xxx */
int opt_foreach(const char *pattern, OPT_FOREACH foreach, void *userdata)
{
	K_dlist_entry *entry;
	opt_entry_t *oe;

	/* spl_lck_get(__g_optcc->lck); */

	entry = __g_optcc->oehdr.next;
	while (entry != &__g_optcc->oehdr) {
		oe = FIELD_TO_STRUCTURE(entry, opt_entry_t, entry);
		entry = entry->next;

		/* FIXME: apply pattern */
		foreach((void*)oe, oe->path, userdata);
	}

	/* spl_lck_rel(__g_optcc->lck); */
	return EC_OK;
}

static void queue_watch(opt_entry_t *oe, opt_watch_t *ow, int awch)
{
	K_dlist_entry *hdr;

	if (oe) {
		if (awch)
			hdr = &oe->awchhdr;
		else
			hdr = &oe->bwchhdr;

		insert_dlist_tail_entry(hdr, &ow->entry);
		oe->bwch_cnt++;
	} else {
		if (awch)
			hdr = &__g_optcc->nywch.ahdr;
		else
			hdr = &__g_optcc->nywch.bhdr;
		insert_dlist_tail_entry(hdr, &ow->entry);
	}
}

/**
 * \brief Watch a option update.
 * \c opt_wch_prev() hook before update, \c opt_wch_post() hook after update
 *
 * \return watch number, 0 for error
 */
void *opt_wch_new(const char *path, OPT_WATCH wch,
		void *ua, void *ub, int awch)
{
	opt_entry_t *oe;
	opt_watch_t *ow;

	/* spl_lck_get(__g_optcc->lck); */

	if (!OPT_CHK_TYPE(path)) {
		/* spl_lck_rel(__g_optcc->lck); */
		kerror("BadType: %s\n", path);
		kassert(0 && "opt path should be '[a|i|d|s|b|e|p]:/Xxx'");
		return NULL;
	}

	oe = entry_find(path);
	ow = (opt_watch_t*)kmem_alloz(1, opt_watch_t);
	ow->path = kstr_dup(path);
	ow->wch = wch;
	ow->ua = ua;
	ow->ub = ub;

	queue_watch(oe, ow, awch);

	/* spl_lck_rel(__g_optcc->lck); */
	return (void*)ow;
}

int opt_wch_del(void *wch)
{
	opt_watch_t *ow = (opt_watch_t*)wch;

	/* spl_lck_get(__g_optcc->lck); */

	if (ow->delter)
		ow->delter((void*)ow);

	remove_dlist_entry(&ow->entry);

	/* spl_lck_rel(__g_optcc->lck); */

	kmem_free_s(ow->path);
	kmem_free(ow);

	return EC_OK;
}

static void diag_list_foreach(void *opt, const char *path, void *userdata)
{
	strcat((char*)userdata, path);
	strcat((char*)userdata, "\r\n");
}

static int og_diag_list(void *opt, void *pa, void *pb)
{
	char lsbuf[8192];

	lsbuf[0] = '\0';
	opt_foreach(NULL, diag_list_foreach, (void*)lsbuf);
	opt_set_cur_str(opt, lsbuf);
	return EC_OK;
}

static void diag_dump_foreach(void *opt, const char *path, void *userdata)
{
	char *rets, buffer[4096 * 8], value[4096 * 8];
	opt_entry_t *oe = (opt_entry_t*)opt;
	int err, reti;
	void *retp;

	if (!strcmp("s:/k/opt/diag/dump", path))
		return;

	switch (OPT_TYPE(oe)) {
	case 'a':
		sprintf(value, "a:%p, l:%d", oe->v.cur.a.v, oe->v.cur.a.l);
		break;
	case 'b':
	case 'e':
	case 'i':
		reti = 0;
		err = getint(oe, NULL, NULL, &reti);
		sprintf(value, "%d", err ? -1 : reti);
		break;
	case 'd':
		sprintf(value, "a:%p, l:%d", oe->v.cur.a.v, oe->v.cur.a.l);
		break;
	case 's':
		rets = NULL;
		err = getstr(oe, NULL, NULL, &rets);
		if (err)
			sprintf(value, "(err)");
		else if (!rets)
			sprintf(value, "(null)");
		else
			sprintf(value, "%s", rets);
		break;
	case 'p':
		retp = 0;
		err = getptr(oe, NULL, NULL, &retp);
		sprintf(value, "%p", err ? NULL : retp);
		break;
	}

	sprintf(buffer, "%1d:%1d:%1d %4d:%4d:%4d:%4d %2d:%2d %-30s\t%s\r\n",
			!!oe->setter, !!oe->getter, !!oe->delter,
			oe->set_called, oe->get_called,
			oe->awch_called, oe->bwch_called,
			oe->awch_cnt, oe->bwch_cnt,
			path, value);

	strcat((char*)userdata, buffer);
}

static int og_diag_dump(void *opt, void *pa, void *pb)
{
	char dmpbuf[8192 * 80];

	sprintf(dmpbuf, "\r\nS:G:D   SC:  GC: AWC: BWC AC:BC PATH ...\r\n");

	opt_foreach(NULL, diag_dump_foreach, (void*)dmpbuf);
	opt_set_cur_str(opt, dmpbuf);
	return EC_OK;
}

static int og_diag_wch_notyet(void *opt, void *pa, void *pb)
{
	K_dlist_entry *entry;
	opt_watch_t *wch;
	char dmpbuf[8192 * 80], *p = dmpbuf;

	p += sprintf(p, "nywch.ahdr:\n");
	entry = __g_optcc->nywch.ahdr.next;
	while (entry != &__g_optcc->nywch.ahdr) {
		wch = FIELD_TO_STRUCTURE(entry, opt_watch_t, entry);
		entry = entry->next;

		p += sprintf(p, "%s\n", wch->path);
	}

	p += sprintf(p, "\nnywch.bhdr:\n");
	entry = __g_optcc->nywch.bhdr.next;
	while (entry != &__g_optcc->nywch.bhdr) {
		wch = FIELD_TO_STRUCTURE(entry, opt_watch_t, entry);
		entry = entry->next;

		p += sprintf(p, "%s\n", wch->path);
	}

	opt_set_cur_str(opt, dmpbuf);
	return EC_OK;
}

void *opt_attach(void *logcc)
{
	__g_optcc = (optcc_t*)logcc;

	return (void*)__g_optcc;
}
void *opt_cc()
{
	return (void*)__g_optcc;
}

void *opt_init(int argc, char *argv[])
{
	if (__g_optcc)
		return (void*)__g_optcc;

	__g_optcc = (optcc_t*)kmem_alloz(1, optcc_t);
	init_dlist_head(&__g_optcc->oehdr);
	init_dlist_head(&__g_optcc->nywch.bhdr);
	init_dlist_head(&__g_optcc->nywch.ahdr);

	__g_optcc->lck = spl_lck_new();

	/*
	 * XXX for every module that can support session,
	 * it should watch start and done opt and test the ses.
	 */
	opt_add_s("i:/k/opt/session/start", OA_DFT, NULL, NULL);
	opt_add_s("i:/k/opt/session/done", OA_DFT, NULL, NULL);

	/* opt-rpc */
	opt_add_s("s:/k/opt/rpc/o/connect", OA_DFT, NULL, NULL);
	opt_add_s("s:/k/opt/rpc/w/connect", OA_DFT, NULL, NULL);
	opt_add_s("s:/k/opt/rpc/o/disconnect", OA_DFT, NULL, NULL);
	opt_add_s("s:/k/opt/rpc/w/disconnect", OA_DFT, NULL, NULL);

	/* self diag */
	opt_add_s("s:/k/opt/diag/list", OA_GET, NULL, og_diag_list);
	opt_add_s("s:/k/opt/diag/dump", OA_GET, NULL, og_diag_dump);
	opt_add_s("s:/k/opt/wch/notyet", OA_GET, NULL, og_diag_wch_notyet);

	return (void*)__g_optcc;
}

/* XXX: should be called within lock */
static void delete_watch()
{
	K_dlist_entry *entry;
	opt_watch_t *ow;

	entry = __g_optcc->nywch.bhdr.next;
	while (entry != &__g_optcc->nywch.bhdr) {
		ow = FIELD_TO_STRUCTURE(entry, opt_watch_t, entry);
		entry = entry->next;

		opt_wch_del((void*)ow);
	}

	entry = __g_optcc->nywch.ahdr.next;
	while (entry != &__g_optcc->nywch.ahdr) {
		ow = FIELD_TO_STRUCTURE(entry, opt_watch_t, entry);
		entry = entry->next;

		opt_wch_del((void*)ow);
	}
}

/* XXX: should be called within lock */
static void delete_entries()
{
	K_dlist_entry *entry;
	opt_entry_t *oe;

	entry = __g_optcc->oehdr.next;
	while (entry != &__g_optcc->oehdr) {
		oe = FIELD_TO_STRUCTURE(entry, opt_entry_t, entry);
		entry = entry->next;

		entry_del(oe);
	}
}

int opt_final()
{
	if (!__g_optcc)
		return -1;

	/* spl_lck_get(__g_optcc->lck); */
	delete_entries();
	delete_watch();
	/* spl_lck_rel(__g_optcc->lck); */

	spl_lck_del(__g_optcc->lck);
	kmem_free_z(__g_optcc);

	return EC_OK;
}

/**
 * \brief Session is a group of pending operation, when done called
 * all the operation emit all at once.
 */
/**
 * \brief return session id, can not be zero
 */
int opt_session_start()
{
	int now = ++__g_optcc->sesid_last;

	/* XXX: skip the sesid 0 */
	__g_optcc->sesid_last += !now;
	opt_setint_s(__g_optcc->sesid_last, "i:/k/opt/session/start", 0);
	return __g_optcc->sesid_last;
}

/* end session with error number */
/* XXX reterr should be ORed by watcher */
int opt_session_commit(int ses, int cancel, int *reterr)
{
	int ret = -1;
	opt_entry_t *oe;

	/* spl_lck_get(__g_optcc->lck); */

	oe = entry_find("i:/k/opt/session/done");
	if (oe) {
		/* XXX: ub = error when commit session */
		oe->ub = (void*)0;
		ret = setint(ses, oe, NULL, NULL, cancel);
		if (reterr)
			*reterr = (int)oe->ub;
	} else {
		kerror("Opt not found: <%s>\n", "i:/k/opt/session/done");
		opt_set_err(EC_NG, "Opt not found");
	}

	/* spl_lck_rel(__g_optcc->lck); */
	return ret;
}

void opt_session_set_err(void *opt, int error)
{
	opt_entry_t *oe = (opt_entry_t*)opt;

	if (oe->ses && !strcmp("i:/k/opt/session/done", oe->path)) {
		int curerr = (int)oe->ub;
		curerr |= error;
		oe->ub = (void*)curerr;
	}
}

/*
 * Only use for debugging
 */
int os_opt_hook(int ses, void *opt, void *pa, void *pb)
{
	klog("ses:%d, path:%s\n", ses, opt_path(opt));
	return EC_DEFAULT;
}
int og_opt_hook(void *opt, void *pa, void *pb)
{
	klog("path:%s\n", opt_path(opt));
	return EC_DEFAULT;
}

