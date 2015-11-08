/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_OPT_H__
#define __K_OPT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <hilda/sysdeps.h>
#include <hilda/sdlist.h>
#include <hilda/kstr.h>
#include <hilda/kflg.h>

static kinline char *kopt_path(void *oe);
static kinline char *kopt_desc(void *oe);

static kinline void *kopt_ua(void *oe);
static kinline void *kopt_ub(void *oe);

static kinline int kopt_get_setpa(void *oe, void **pa);
static kinline int kopt_get_setpb(void *oe, void **pb);

static kinline char **kopt_get_cur_arr(void *oe);
static kinline int kopt_get_cur_arr_len(void *oe);
static kinline void *kopt_get_cur_dat(void *oe);
static kinline int kopt_get_cur_dat_len(void *oe);
static kinline int kopt_get_cur_int(void *oe);
static kinline char *kopt_get_cur_str(void *oe);
static kinline void *kopt_get_cur_ptr(void *oe);

static kinline int kopt_set_cur_arr(void *oe, char **v_arr, int len);
static kinline int kopt_set_cur_dat(void *oe, char *v_dat, int len);
static kinline int kopt_set_cur_int(void *oe, int v_int);
static kinline int kopt_set_cur_str(void *oe, char *v_str);
static kinline int kopt_set_cur_ptr(void *oe, void *v_ptr);

static kinline char **kopt_get_new_arr(void *oe);
static kinline int kopt_get_new_arr_len(void *oe);
static kinline void *kopt_get_new_dat(void *oe);
static kinline int kopt_get_new_dat_len(void *oe);
static kinline int kopt_get_new_int(void *oe);
static kinline char *kopt_get_new_str(void *oe);
static kinline void *kopt_get_new_ptr(void *oe);

/* bool KOPT_IS_X(const char *path); */
#define KOPT_IS_A(_PATH_) (('a' == (_PATH_)[0]) && (':' == (_PATH_)[1]))
#define KOPT_IS_B(_PATH_) (('b' == (_PATH_)[0]) && (':' == (_PATH_)[1]))
#define KOPT_IS_D(_PATH_) (('d' == (_PATH_)[0]) && (':' == (_PATH_)[1]))
#define KOPT_IS_E(_PATH_) (('e' == (_PATH_)[0]) && (':' == (_PATH_)[1]))
#define KOPT_IS_I(_PATH_) (('i' == (_PATH_)[0]) && (':' == (_PATH_)[1]))
#define KOPT_IS_S(_PATH_) (('s' == (_PATH_)[0]) && (':' == (_PATH_)[1]))
#define KOPT_IS_P(_PATH_) (('p' == (_PATH_)[0]) && (':' == (_PATH_)[1]))

#define OA "a:"		/* arr: */
#define OB "b:"		/* bool: */
#define OD "d:"		/* dat: dat+len */
#define OE "e:"		/* evt: value is no need */
#define OI "i:"		/* int: */
#define OS "s:"		/* str: */
#define OP "p:"		/* ptr: */

static kinline void *kopt_wch_ua(void *ow);
static kinline void *kopt_wch_ub(void *ow);
static kinline void *kopt_wch_path(void *ow);

#define KOPT_CHK_TYPE(p) ((p) && \
		((p)[1] == ':') && \
		((p)[2] == '/') && \
		(((p)[0] == 'a') || \
		 ((p)[0] == 'b') || \
		 ((p)[0] == 'i') || \
		 ((p)[0] == 'e') || \
		 ((p)[0] == 'd') || \
		 ((p)[0] == 's') || \
		 ((p)[0] == 'p')))

#define OA_SET		0x00000001	/* can set */
#define OA_GET		0x00000002	/* can get */
#define OA_WCH		0x00000004	/* can wch */
#define OA_ONCE		0x00000008	/* Only add called */

#define OA_IN_SET	0x00010000	/* In Set call */
#define OA_IN_GET	0x00020000	/* In Get call */
#define OA_IN_DEL	0x00040000	/* In Del call */
#define OA_IN_AWCH	0x00080000	/* In After Watch */
#define OA_IN_BWCH	0x00100000	/* In Before Watch */

/* remove unused bits */
#define OA_MSK	(OA_SET | OA_GET | OA_WCH | OA_ONCE)
/* default value */
#define OA_DFT	(OA_SET | OA_GET | OA_WCH)

/**
 * \brief prototype of callbacks
 * \retval 0 success processed.
 * \retval -1 error occurs.
 * \retval 1 Please do default
 */
typedef void (*KOPT_FOREACH)(void *opt, const char *path, void *userdata);
typedef int (*KOPT_SETTER)(int ses, void *opt, void *pa, void *pb);
typedef int (*KOPT_GETTER)(void *opt, void *pa, void *pb);
typedef int (*KOPT_DELTER)(void *opt);

typedef void (*KOPT_WATCH)(int ses, void *opt, void *wch);
typedef int (*KOPT_WCH_DELTER)(void *wch);

/* XXX */
/* / : = */
/* - */
typedef struct _optcc_s optcc_s;
typedef struct _opt_watch_s kopt_watch_s;
typedef struct _opt_entry_s kopt_entry_s;

struct _opt_watch_s {
	/** queue to bwchhdr/awchhdr */
	K_dlist_entry entry;

	/** callback when watch hit */
	KOPT_WATCH wch;
	/** have chance to free resource */
	KOPT_WCH_DELTER delter;

	void *ua, *ub;

	/** watch what event */
	char *path;

	/** diag part */
	unsigned int wch_cnt;
};

struct _opt_entry_s {
	/** _optcc_s::oehdr */
	K_dlist_entry entry;

	char *path;
	char *desc;

	KOPT_GETTER getter;
	KOPT_SETTER setter;
	/* have chance to free resource */
	KOPT_DELTER delter;

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
				/* maintain other _opt_entry_s */
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
				/* maintain other _opt_entry_s */
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
struct _optcc_s {
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

/*-----------------------------------------------------------------------
 * kinline functions
 */

/* normal access */
static kinline char *kopt_path(void *oe)
{
	return ((kopt_entry_s*)(oe))->path;
}
static kinline char *kopt_desc(void *oe)
{
	return ((kopt_entry_s*)(oe))->desc;
}

static kinline void *kopt_ua(void *oe)
{
	return ((kopt_entry_s*)(oe))->ua;
}
static kinline void *kopt_ub(void *oe)
{
	return ((kopt_entry_s*)(oe))->ub;
}

static kinline int kopt_get_setpa(void *oe, void **pa)
{
	if (kflg_chk_any(((kopt_entry_s*)oe)->attr, OA_IN_AWCH | OA_IN_BWCH | OA_IN_SET)) {
		*pa = ((kopt_entry_s*)oe)->set_pa;
		return 0;
	}
	return -1;
}
static kinline int kopt_get_setpb(void *oe, void **pb)
{
	if (kflg_chk_any(((kopt_entry_s*)oe)->attr, OA_IN_AWCH | OA_IN_BWCH | OA_IN_SET)) {
		*pb = ((kopt_entry_s*)oe)->set_pb;
		return 0;
	}
	return -1;
}

static kinline char **kopt_get_cur_arr(void *oe)
{
	return ((kopt_entry_s*)(oe))->v.cur.a.v;
}
static kinline int kopt_get_cur_arr_len(void *oe)
{
	return ((kopt_entry_s*)(oe))->v.cur.a.l;
}
static kinline void *kopt_get_cur_dat(void *oe)
{
	return ((kopt_entry_s*)(oe))->v.cur.d.v;
}
static kinline int kopt_get_cur_dat_len(void *oe)
{
	return ((kopt_entry_s*)(oe))->v.cur.d.l;
}
static kinline int kopt_get_cur_int(void *oe)
{
	return ((kopt_entry_s*)(oe))->v.cur.i.v;
}
static kinline char *kopt_get_cur_str(void *oe)
{
	return ((kopt_entry_s*)(oe))->v.cur.s.v;
}
static kinline void *kopt_get_cur_ptr(void *oe)
{
	return ((kopt_entry_s*)(oe))->v.cur.p.v;
}

static kinline char **kopt_get_new_arr(void *oe)
{
	return ((kopt_entry_s*)(oe))->v.new.a.v;
}
static kinline int kopt_get_new_arr_len(void *oe)
{
	return ((kopt_entry_s*)(oe))->v.new.a.l;
}
static kinline void *kopt_get_new_dat(void *oe)
{
	return ((kopt_entry_s*)(oe))->v.new.d.v;
}
static kinline int kopt_get_new_dat_len(void *oe)
{
	return ((kopt_entry_s*)(oe))->v.new.d.l;
}
static kinline int kopt_get_new_int(void *oe)
{
	return ((kopt_entry_s*)(oe))->v.new.i.v;
}
static kinline char *kopt_get_new_str(void *oe)
{
	return ((kopt_entry_s*)(oe))->v.new.s.v;
}
static kinline void *kopt_get_new_ptr(void *oe)
{
	return ((kopt_entry_s*)(oe))->v.new.p.v;
}

static kinline void *kopt_wch_path(void *ow)
{
	return ((kopt_watch_s*)(ow))->path;
}

static kinline void *kopt_wch_ua(void *ow)
{
	return ((kopt_watch_s*)(ow))->ua;
}

static kinline void *kopt_wch_ub(void *ow)
{
	return ((kopt_watch_s*)(ow))->ub;
}

static kinline int kopt_set_cur_arr(void *oe, char **v_arr, int len)
{
	kopt_entry_s *e = (kopt_entry_s*)oe;

	if (e->attr & (OA_IN_SET | OA_IN_GET)) {
		kmem_free_sz(e->v.cur.a.v);
		if (len < 0)
			len = 0;
		if (len > 0) {
			kmem_free_sz(e->v.cur.a.v);
			e->v.cur.a.v = (char**)kmem_alloz(len, char*);
			memcpy(e->v.cur.a.v, v_arr, len * sizeof(char*));
		}
		e->v.cur.a.l = len;
		return EC_OK;
	}
	return EC_FORBIDEN;
}

static kinline int kopt_set_cur_dat(void *oe, char *v_dat, int len)
{
	kopt_entry_s *e = (kopt_entry_s*)oe;

	if (e->attr & (OA_IN_SET | OA_IN_GET)) {
		kmem_free_sz(e->v.cur.d.v);
		if (len < 0)
			len = 0;
		if (len > 0) {
			kmem_free_sz(e->v.cur.d.v);
			e->v.cur.d.v = (char*)kmem_alloz(len, char);
			memcpy(e->v.cur.d.v, v_dat, len * sizeof(char));
		}
		e->v.cur.d.l = len;
		return EC_OK;
	}
	return EC_FORBIDEN;
}

static kinline int kopt_set_cur_int(void *oe, int v_int)
{
	kopt_entry_s *e = (kopt_entry_s*)oe;

	if (e->attr & (OA_IN_SET | OA_IN_GET)) {
		e->v.cur.i.v = v_int;
		return EC_OK;
	}
	return EC_FORBIDEN;
}

static kinline int kopt_set_cur_str(void *oe, char *v_str)
{
	kopt_entry_s *e = (kopt_entry_s*)oe;

	if (e->attr & (OA_IN_SET | OA_IN_GET)) {
		kmem_free_sz(e->v.cur.s.v);
		e->v.cur.s.v = kstr_dup(v_str);
		return EC_OK;
	}
	return EC_FORBIDEN;
}

static kinline int kopt_set_cur_ptr(void *oe, void *v_ptr)
{
	kopt_entry_s *e = (kopt_entry_s*)oe;

	if (e->attr & (OA_IN_SET | OA_IN_GET)) {
		e->v.cur.p.v = v_ptr;
		return EC_OK;
	}
	return EC_FORBIDEN;
}


int kopt_make_kv(const char *buffer, int blen, char ***okv, int *ocnt);
void kopt_free_kv(char **kv, int cnt);
char *kopt_pack_kv(const char **k, const char **v);

void kopt_set_err(int no, const char *msg);
int kopt_get_err(int *no, char **msg);

/* _s => short */
#define kopt_add_s(p, a, s, g) \
	kopt_new((p), NULL, (a), (s), (g), NULL, NULL, NULL)

#define kopt_add(p, d, a, set, get, del, ua, ub) \
	kopt_new((p), (d), (a), (set), (get), (del), (void*)(ua), (void*)(ub))

int kopt_new(const char *path, const char *desc, unsigned int attr,
		KOPT_SETTER setter, KOPT_GETTER getter, KOPT_DELTER delter,
		void *ua, void *ub);
int kopt_del(const char *path);

int kopt_type(const char *path);

int kopt_load_argv(int argc, char *argv[]);

int kopt_setkv(int ses, const char *k, const char *v);

int kopt_setfile(const char *path);
int kopt_setbat(const char *inibuf, int errbrk);

int kopt_getini_by_opt(void *opt, char **ret);
int kopt_getini(const char *path, char **ret);

/*
 * s => ses, p => pa,pb
 */
#define kopt_setint(p, v) kopt_setint_sp(0, (p), NULL, NULL, (v))
#define kopt_setint_s(s, p, v) kopt_setint_sp((s), (p), NULL, NULL, (v))
#define kopt_setint_p(p, pa, pb, v) kopt_setint_sp(0, (p), (void*)(pa), (void*)(pb), (v))
int kopt_setint_sp(int ses, const char *path, void *pa, void *pb, int v_int);
#define kopt_getint(p, v) kopt_getint_p((p), NULL, NULL, (v))
int kopt_getint_p(const char *path, void *pa, void *pb, int *v_int);

#define kopt_setptr(p, v) kopt_setptr_sp(0, (p), NULL, NULL, (v))
#define kopt_setptr_s(s, p, v) kopt_setptr_sp((s), (p), NULL, NULL, (v))
#define kopt_setptr_p(p, pa, pb, v) kopt_setptr_sp(0, (p), (void*)(pa), (void*)(pb), (v))
int kopt_setptr_sp(int ses, const char *path, void *pa, void *pb, void *v_ptr);
#define kopt_getptr(p, v) kopt_getptr_p((p), NULL, NULL, (v))
int kopt_getptr_p(const char *path, void *pa, void *pb, void **v_ptr);

#define kopt_setstr(p, v) kopt_setstr_sp(0, (p), NULL, NULL, (v))
#define kopt_setstr_s(s, p, v) kopt_setstr_sp((s), (p), NULL, NULL, (v))
#define kopt_setstr_p(p, pa, pb, v) kopt_setstr_sp(0, (p), (void*)(pa), (void*)(pb), (v))
int kopt_setstr_sp(int ses, const char *path, void *pa, void *pb, char *v_str);
#define kopt_getstr(p, v) kopt_getstr_p((p), NULL, NULL, (v))
int kopt_getstr_p(const char *path, void *pa, void *pb, char **v_str);

#define kopt_setarr(p, v, l) kopt_setarr_sp(0, (p), NULL, NULL, (v), (l))
#define kopt_setarr_s(s, p, v, l) kopt_setarr_sp((s), (p), NULL, NULL, (v), (l))
#define kopt_setarr_p(p, pa, pb, v, l) kopt_setarr_sp(0, (p), (void*)(pa), (void*)(pb), (v), (l))
int kopt_setarr_sp(int ses, const char *path, void *pa, void *pb, const char **v_arr, int len);
#define kopt_getarr(p, v, l) kopt_getarr_p((p), (pa), NULL, NULL, (v))
int kopt_getarr_p(const char *path, void *pa, void *pb, void **arr, int *len);

#define kopt_setdat(p, v, l) kopt_setdat_sp(0, (p), NULL, NULL, (v), (l))
#define kopt_setdat_s(s, p, v, l) kopt_setdat_sp((s), (p), NULL, NULL, (v), (l))
#define kopt_setdat_p(p, pa, pb, v, l) kopt_setdat_sp(0, (p), (void*)(pa), (void*)(pb), (v), (l))
int kopt_setdat_sp(int ses, const char *path, void *pa, void *pb, const char *v_dat, int len);
#define kopt_getdat(p, v, l) kopt_getdat_p((p), NULL, NULL, (v), (l))
int kopt_getdat_p(const char *path, void *pa, void *pb, char **arr, int *len);

int kopt_foreach(const char *pattern, KOPT_FOREACH foreach, void *userdata);

/* _u => ua,ub */
void *kopt_wch_new(const char *path, KOPT_WATCH wch, void *ua, void *ub, int awch);

#define kopt_awch(p, w) kopt_wch_new((p), (w), NULL, NULL, 1)
#define kopt_awch_u(p, w, ua, ub) kopt_wch_new((p), (w), (void*)(ua), (void*)(ub), 1)
#define kopt_bwch(p, w) kopt_wch_new((p), (w), NULL, NULL, 0)
#define kopt_bwch_u(p, w, ua, ub) kopt_wch_new((p), (w), (void*)(ua), (void*)(ub), 0)
int kopt_wch_del(void *wch);

void *kopt_init(int argc, char *argv[]);
void *kopt_cc();
void *kopt_attach(void *optcc);
int kopt_final();

int kopt_session_start();
int kopt_session_commit(int ses, int cancel, int *reterr);
void kopt_session_set_err(void *opt, int error);

#ifdef __cplusplus
}
#endif

#endif /* __K_OPT_H__ */

