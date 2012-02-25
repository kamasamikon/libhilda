/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __OPT_H__
#define __OPT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sysdeps.h>

kinline char *opt_desc(void *oe);

kinline void *opt_ua(void *oe);
kinline void *opt_ub(void *oe);

kinline int opt_set_pa(void *oe, void **pa);
kinline int opt_set_pb(void *oe, void **pb);

kinline char **opt_get_cur_arr(void *oe);
kinline int opt_get_cur_arr_len(void *oe);
kinline void *opt_get_cur_dat(void *oe);
kinline int opt_get_cur_dat_len(void *oe);
kinline int opt_get_cur_int(void *oe);
kinline char *opt_get_cur_str(void *oe);
kinline void *opt_get_cur_ptr(void *oe);

kinline int opt_set_cur_arr(void *oe, char **val, int len);
kinline int opt_set_cur_dat(void *oe, char *val, int len);
kinline int opt_set_cur_int(void *oe, int val);
kinline int opt_set_cur_str(void *oe, char *val);
kinline int opt_set_cur_ptr(void *oe, void *val);

kinline char **opt_get_new_arr(void *oe);
kinline int opt_get_new_arr_len(void *oe);
kinline void *opt_get_new_dat(void *oe);
kinline int opt_get_new_dat_len(void *oe);
kinline int opt_get_new_int(void *oe);
kinline char *opt_get_new_str(void *oe);
kinline void *opt_get_new_ptr(void *oe);

/* bool OPT_IS_X(const char *path); */
#define OPT_IS_A(_PATH_) (('a' == (_PATH_)[0]) && (':' == (_PATH_)[1]))
#define OPT_IS_B(_PATH_) (('b' == (_PATH_)[0]) && (':' == (_PATH_)[1]))
#define OPT_IS_D(_PATH_) (('d' == (_PATH_)[0]) && (':' == (_PATH_)[1]))
#define OPT_IS_E(_PATH_) (('e' == (_PATH_)[0]) && (':' == (_PATH_)[1]))
#define OPT_IS_I(_PATH_) (('i' == (_PATH_)[0]) && (':' == (_PATH_)[1]))
#define OPT_IS_S(_PATH_) (('s' == (_PATH_)[0]) && (':' == (_PATH_)[1]))
#define OPT_IS_P(_PATH_) (('p' == (_PATH_)[0]) && (':' == (_PATH_)[1]))

#define OA "a:"		/* arr: */
#define OB "b:"		/* bool: */
#define OD "d:"		/* dat: dat+len */
#define OE "e:"		/* evt: value is no need */
#define OI "i:"		/* int: */
#define OS "s:"		/* str: */
#define OP "p:"		/* ptr: */

kinline void *wch_ua(void *ow);
kinline void *wch_ub(void *ow);

#define OPT_CHK_TYPE(p) ((p) && \
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
typedef void (*OPT_FOREACH)(void *opt, const char *path, void *userdata);
typedef int (*OPT_SETTER)(int ses, void *opt, const char *path,
		void **pa, void **pb);
typedef int (*OPT_GETTER)(void *opt, const char *path, void **pa, void **pb);
typedef int (*OPT_DELTER)(void *opt, const char *path);

typedef int (*OPT_WATCH)(int ses, void *opt, const char *path, void *wch);
typedef int (*OPT_WCH_DELTER)(void *wch, const char *path);

int opt_make_kv(const char *buffer, int blen, char ***okv, int *ocnt);
void opt_free_kv(char **kv, int cnt);
char *opt_pack_kv(const char **k, const char **v);

void opt_set_err(int no, const char *msg);
int opt_get_err(int *no, char **msg);

/* _s => short */
#define opt_add_s(p, a, s, g) \
		opt_add((p), NULL, (a), (s), (g), NULL, NULL, NULL)

int opt_add(const char *path, const char *desc, unsigned int attr,
		OPT_SETTER setter, OPT_GETTER getter, OPT_DELTER delter,
		void *ua, void *ub);
int opt_del(const char *path);

int opt_type(const char *path);

int opt_load_argv(int argc, char *argv[]);

int opt_setkv(int ses, const char *k, const char *v);

int opt_setfile(const char *path);
int opt_setbat(const char *inibuf, int errbrk);

int opt_getini_by_opt(void *opt, char **ret);
int opt_getini(const char *path, char **ret);

/*
 * s => ses, p => pa,pb
 */
#define opt_setint(p, v) opt_setint_sp(0, (p), NULL, NULL, (v))
#define opt_setint_s(s, p, v) opt_setint_sp((s), (p), NULL, NULL, (v))
#define opt_setint_p(p, pa, pb, v) opt_setint_sp(0, (p), (void**)(pa), (void**)(pb), (v))
int opt_setint_sp(int ses, const char *path, void **pa, void **pb, int val);
#define opt_getint(p, v) opt_getint_p((p), NULL, NULL, (v))
int opt_getint_p(const char *path, void **pa, void **pb, int *val);

#define opt_setptr(p, v) opt_setptr_sp(0, (p), NULL, NULL, (v))
#define opt_setptr_s(s, p, v) opt_setptr_sp((s), (p), NULL, NULL, (v))
#define opt_setptr_p(p, pa, pb, v) opt_setptr_sp(0, (p), (void**)(pa), (void**)(pb), (v))
int opt_setptr_sp(int ses, const char *path, void **pa, void **pb, void *val);
#define opt_getptr(p, v) opt_getptr_p((p), NULL, NULL, (v))
int opt_getptr_p(const char *path, void **pa, void **pb, void **val);

#define opt_setstr(p, v) opt_setstr_sp(0, (p), NULL, NULL, (v))
#define opt_setstr_s(s, p, v) opt_setstr_sp((s), (p), NULL, NULL, (v))
#define opt_setstr_p(p, pa, pb, v) opt_setstr_sp(0, (p), (void**)(pa), (void**)(pb), (v))
int opt_setstr_sp(int ses, const char *path, void **pa, void **pb, char *val);
#define opt_getstr(p, v) opt_getstr_p((p), NULL, NULL, (v))
int opt_getstr_p(const char *path, void **pa, void **pb, char **val);

#define opt_setarr(p, v, l) opt_setarr_sp(0, (p), NULL, NULL, (v), (l))
#define opt_setarr_s(s, p, v, l) opt_setarr_sp((s), (p), NULL, NULL, (v), (l))
#define opt_setarr_p(p, pa, pb, v, l) opt_setarr_sp(0, (p), (void**)(pa), (void**)(pb), (v), (l))
int opt_setarr_sp(int ses, const char *path, void **pa, void **pb, const char **val, int len);
#define opt_getarr(p, v, l) opt_getarr_p((p), (pa), NULL, NULL, (v))
int opt_getarr_p(const char *path, void **pa, void **pb, void **arr, int *len);

#define opt_setdat(p, v, l) opt_setdat_sp(0, (p), NULL, NULL, (v), (l))
#define opt_setdat_s(s, p, v, l) opt_setdat_sp((s), (p), NULL, NULL, (v), (l))
#define opt_setdat_p(p, pa, pb, v, l) opt_setdat_sp(0, (p), (void**)(pa), (void**)(pb), (v), (l))
int opt_setdat_sp(int ses, const char *path, void **pa, void **pb, const char *val, int len);
#define opt_getdat(p, v, l) opt_getdat_p((p), (pa), NULL, NULL, (v))
int opt_getdat_p(const char *path, void **pa, void **pb, char **arr, int *len);

int opt_foreach(const char *pattern, OPT_FOREACH foreach, void *userdata);

/* _u => ua,ub */
#define opt_awch(p, w) opt_awch_u((p), (w), NULL, NULL)
void *opt_awch_u(const char *path, OPT_WATCH wch, void *ua, void *ub);
#define opt_bwch(p, w) opt_bwch_u((p), (w), NULL, NULL)
void *opt_bwch_u(const char *path, OPT_WATCH wch, void *ua, void *ub);
int opt_wch_del(void *wch);

void *opt_init(int argc, char *argv[]);
void *opt_cc();
void *opt_attach(void *optcc);
int opt_final();

int opt_session_start();
int opt_session_commit(int ses, int cancel, int *reterr);
void opt_session_set_err(void *opt, int error);

#ifdef __cplusplus
}
#endif

#endif /* __OPT_H__ */

