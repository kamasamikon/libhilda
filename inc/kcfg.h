/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_CFG_H__
#define __K_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _kcfg_target_t kcfg_target_t;
typedef struct _kcfg_t kcfg_t;

typedef int (*KCFG_SAVE)(kcfg_target_t *ct, char *dat, int len, void *ua, void *ub);
typedef int (*KCFG_LOAD)(kcfg_target_t *ct, char **dat, int *len, void *ua, void *ub);
typedef int (*KCFG_DELETE)(kcfg_target_t *ct, void *ua, void *ub);

struct _kcfg_target_t {
	char *name;

	struct {
		char **arr;
		int cnt;
	} opts;

	KCFG_SAVE save;
	KCFG_LOAD load;
	KCFG_DELETE del;

	void *ua, *ub;

	unsigned int pass_through;

	/* XXX: Hash the saved data, skip if same data to be save */
	char data_hash[32];
};

struct _kcfg_t {
	struct {
		kcfg_target_t **arr;
		int cnt;
	} target;
};

int kcfg_init();
int kcfg_final();

kcfg_target_t *kcfg_target_add(const char *name, KCFG_SAVE dosave, KCFG_LOAD doload, KCFG_DELETE dodelete, void *ua, void *ub);
int kcfg_target_del(const char *name);

int kcfg_target_opt_add(const char *name, const char *opt);
int kcfg_target_opt_del(const char *name, const char *opt);

int kcfg_save();
int kcfg_save_delay(int msec);
int kcfg_load();

#ifdef __cplusplus
}
#endif

#endif /* __K_CFG_H__ */

