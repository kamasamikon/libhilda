/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

/**
 * \file kmodu.c
 */

#include <string.h>

#include <hilda/klog.h>
#include <hilda/kflg.h>
#include <hilda/kopt.h>
#include <hilda/kstr.h>
#include <hilda/knode.h>
#include <hilda/sdlist.h>
#include <hilda/xtcool.h>
#include <hilda/strbuf.h>

#include <hilda/kmodu.h>

/* Control Center of K MODUles */
typedef struct _kmoducc_t kmoducc_t;
struct _kmoducc_t {
	K_dlist_entry modhdr;

	struct {
		char *root;
		char *dll;
		char *manifest;
		char *opt;
	} path;

	int ref;
};

static kmoducc_t *__g_kmoducc;

static void setup_opt(kmoducc_t *cc);
static void add_config_file(kmoducc_t *cc);
static int module_load(kmoducc_t *cc, const char *path);
static void module_unload();
static kmodu_t *lib_load(const char *path);
static int lib_unload(kmodu_t *mod);
static int manual_layout();

static int og_kmodu_diag_dump(void *opt, void *pa, void *pb)
{
	kmoducc_t *cc = (kmoducc_t*)kopt_ua(opt);
	kmodu_t *mod;
	K_dlist_entry *entry;
	struct strbuf sb;

	strbuf_init(&sb, 4096);

	entry = cc->modhdr.next;
	while (entry != &cc->modhdr) {
		mod = FIELD_TO_STRUCTURE(entry, kmodu_t, entry);
		entry = entry->next;

		strbuf_addf(&sb, "\r\n---->\r\n  path:%s\r\n  name:%s\r\n  "
				"version:%08x\r\n  type:%d\r\n  handle:%p\r\n  "
				"hey:%p\r\n  bye:%p\r\n<----\r\n\r\n",
				mod->path, mod->name, mod->version, mod->type,
				mod->handle, mod->hey, mod->bye);
	}
	kopt_set_cur_str(opt, sb.buf);
	strbuf_release(&sb);

	return EC_OK;
}

static void setup_opt(kmoducc_t *cc)
{
	kopt_add("s:/k/modu/diag/dump", NULL, OA_GET, NULL,
			og_kmodu_diag_dump, NULL, (void*)cc, NULL);
	kopt_add("b:/k/modu/layout/manual", NULL, OA_DFT, NULL,
			NULL, NULL, (void*)cc, NULL);
	kopt_add("s:/k/modu/layout/path", NULL, OA_DFT, NULL,
			NULL, NULL, (void*)cc, NULL);

	/* pa => module_scan_dir */
	kopt_add_s("e:/k/modu/load/start", OA_DFT, NULL, NULL);
	/* pa => module_scan_dir, pb => err */
	kopt_add_s("e:/k/modu/load/done", OA_DFT, NULL, NULL);

	/* pa => mod_path */
	kopt_add_s("e:/k/modu/load/mod/start", OA_DFT, NULL, NULL);
	/* pa => mod_path, pb => err */
	kopt_add_s("e:/k/modu/load/mod/done", OA_DFT, NULL, NULL);
}

static void add_config_file(kmoducc_t *cc)
{
	char path[1024];

	if (!cc->path.opt)
		return;

	sprintf(path, "%s%c%s", cc->path.root, kvfs_path_sep(), cc->path.opt);
	kopt_setstr("s:/k/cfg/target/file/add", path);
}

int kmodu_init(const char *rootdir, const char *dllname,
		const char *manifestname, const char *optname)
{
	kmoducc_t *cc;

	if (__g_kmoducc) {
		__g_kmoducc->ref++;
		return 0;
	}
	cc = __g_kmoducc = (kmoducc_t*)kmem_alloz(1, kmoducc_t);
	init_dlist_head(&cc->modhdr);

	cc->path.root = kstr_dup(rootdir);
	cc->path.dll = kstr_dup(dllname);
	cc->path.manifest = kstr_dup(manifestname);
	cc->path.opt = kstr_dup(optname);

	setup_opt(cc);
	add_config_file(cc);
	return 0;
}

int kmodu_final()
{
	kmoducc_t *cc = __g_kmoducc;

	kassert(cc);

	cc->ref--;
	if (cc->ref <= 0) {
		kmodu_unload();

		kmem_free_s(cc->path.root);
		kmem_free_s(cc->path.dll);
		kmem_free_s(cc->path.manifest);
		kmem_free_s(cc->path.opt);

		kmem_free(cc);
		cc = NULL;
	}
	return 0;
}

static unsigned int mk_version(char *verstr)
{
	unsigned int mj, mn, rev;
	sscanf(verstr, "%d.%d.%d", &mj, &mn, &rev);
	return ((mj & 0xff) << 24) + ((mn & 0xff) << 16) + (rev & 0xffff);
}

static int read_manifest(FILE *fp, char name[],
		unsigned int *enable, unsigned int *version)
{
	char line[4096];
	char has_name = 0, has_enable = 0, has_version = 0;

	while (fgets(line, sizeof(line), fp)) {
		if (!strncmp(line, "name: ", 6)) {
			has_name = 1;
			strcpy(name, line + 6);
		} else if (!strncmp(line, "enable: ", 8)) {
			has_enable = 1;
			*enable = !strncmp(line + 8, "true", 4);
		} else if (!strncmp(line, "version: ", 9)) {
			has_version = 1;
			*version = mk_version(line + 9);
		}
	}

	if (has_name && has_enable && has_version)
		return 0;
	kerror("read_manifest ng\n");
	return -1;
}

static int module_load(kmoducc_t *cc, const char *path)
{
	kmodu_t *mod = NULL;
	FILE *fp;
	char tmp[1024], cwd[1024], name[256], ps = kvfs_path_sep();
	unsigned int enable, version;

	klog("loading module: %s\n", path);

	sprintf(tmp, "%s%c%s", path, ps, cc->path.manifest);
	fp = fopen(tmp, "rt");
	if (!fp) {
		kerror("open error: %s\n", tmp);
		return -1;
	}

	if (read_manifest(fp, name, &enable, &version))
		return -1;
	if (!enable) {
		klog("module disabled: %s\n", path);
		return -1;
	}

	kvfs_getcwd(cwd, sizeof(cwd));
	kvfs_chdir(path);
	sprintf(tmp, "%s%c%s", path, ps, cc->path.dll);
	mod = lib_load(tmp);
	kvfs_chdir(cwd);
	if (!mod) {
		kerror("lib_load: NG: %s\n", tmp);
		return -1;
	}

	strcpy(mod->name, name);
	mod->version = version;

	if (mod->hey(kopt_cc())) {
		kmem_free(mod);
		kerror("Error when say hey: <%s>\n", path);
		return -1;
	}

	insert_dlist_tail_entry(&cc->modhdr, &mod->entry);

	sprintf(tmp, "%s%c%s", path, ps, cc->path.opt);
	if (kvfs_exist(tmp))
		kopt_setstr("s:/k/cfg/target/file/add", tmp);

	return 0;
}

int kmodu_load()
{
	kmoducc_t *cc = __g_kmoducc;
	kbean fd;
	char path[1024], ps = kvfs_path_sep();
	KVFS_FINDDATA fdat;
	int err;

	kopt_setint_p("e:/k/modu/load/start", cc->path.root, NULL, 1);

	fd = kvfs_findfirst(cc->path.root, &fdat);
	if (!fd) {
		kopt_setint_p("e:/k/modu/load/done", cc->path.root, 1, 1);
		kerror("Scan module dir failed. <%s>\n", cc->path.root);
		return -1;
	}
	do {
		if (!strcmp(fdat.name, ".") || !strcmp(fdat.name, ".."))
			continue;
		if (!kflg_chk_bit(fdat.attrib, KVFS_A_DIR))
			continue;
		sprintf(path, "%s%c%s", cc->path.root, ps, fdat.name);
		kopt_setint_p("e:/k/modu/load/mod/start", path, NULL, 1);
		err = module_load(cc, path);
		kopt_setint_p("e:/k/modu/load/mod/done", path, err, 1);
	} while (-1 != kvfs_findnext(fd, &fdat));
	kvfs_findclose(fd);

	kopt_setint_p("e:/k/modu/load/done", cc->path.root, 0, 1);
	return 0;
}

static void module_unload()
{
	kmoducc_t *cc = __g_kmoducc;
	kmodu_t *mod;
	K_dlist_entry *entry;

	entry = cc->modhdr.next;
	while (entry != &cc->modhdr) {
		mod = FIELD_TO_STRUCTURE(entry, kmodu_t, entry);
		entry = entry->next;

		remove_dlist_entry(&mod->entry);
		if (mod->bye())
			kerror("bye: NG: %p:%s\n", mod->handle, mod->path);
		lib_unload(mod);
		kmem_free(mod);
	}
}

int kmodu_unload()
{
	module_unload();
	return 0;
}

/*
 * UpStream >> DownStream
 * UpStream >> NULL
 * NULL >> DownStream
 */
static int manual_layout()
{
	FILE *fp;
	char *sv = NULL, buf[8192], *uname, *arrow, *dname;
	knode_t *unode, *dnode;
	void *link_dat;

	kopt_getstr("s:/k/modu/layout/path", &sv);
	if (!sv) {
		kerror("kmodu: Manual: Cannot get layout path.\n");
		return -1;
	}

	fp = fopen(sv, "r");
	if (!fp) {
		kerror("kmodu: Manual: Open failed: %s.\n", sv);
		return -1;
	}

	while (fgets(buf, sizeof(buf), fp)) {
		uname = strtok(buf," \t\r\n");
		arrow = strtok(NULL, " \t\r\n");
		dname = strtok(NULL, " \t\r\n");

		if (!uname || !dname || !arrow || strcmp(arrow, ">>"))
			continue;

		klog("LinkMap: {%s} >> {%s}\n", uname, dname);
		if (strcmp(uname, "NULL"))
			continue;
		unode = knode_find(uname);

		if (!strcmp(dname, "NULL"))
			continue;
		dnode = knode_find(dname);

		if (dnode && unode && (dnode != unode) && dnode->link_query)
			if (kflg_chk_bit(dnode->attr, CNAT_INPUT))
				if (!dnode->link_query(unode, dnode, &link_dat))
					knode_link_add(unode, dnode, link_dat);
	}
	fclose(fp);
	return 0;
}

static int auto_layout()
{
	return knode_link(NULL);
}

int kmodu_layout()
{
	int iv = 0, ret;

	kopt_getint("b:/k/modu/layout/manual", &iv);
	ret = iv ? manual_layout() : auto_layout();
	if (ret)
		kerror("layout failed\n");
	return ret;
}

static kmodu_t *lib_load(const char *path)
{
	void *handle;
	KMODU_HEY hey;
	KMODU_BYE bye;
	kmodu_t *mod;

	handle = spl_lib_load(path, 0);
	if (!handle) {
		kerror("spl_lib_load ng: %s\n", path);
		goto error;
	}

	hey = (KMODU_HEY)spl_lib_getsym(handle, "hey");
	if (!hey) {
		kerror("get hey ng: %s\n", path);
		goto error;
	}

	bye = (KMODU_BYE)spl_lib_getsym(handle, "bye");
	if (!bye) {
		kerror("get bye ng: %s\n", path);
		goto error;
	}

	mod = (kmodu_t*)kmem_alloz(1, kmodu_t);
	init_dlist_head(&mod->entry);
	strcpy(mod->path, path);
	mod->handle = handle;
	mod->hey = hey;
	mod->bye = bye;
	mod->cfg = (KMODU_CFG)spl_lib_getsym(handle, "cfg");

	return mod;
error:
	if (handle)
		spl_lib_unload(handle);
	return NULL;
}

static int lib_unload(kmodu_t *mod)
{
	int ret;

	if (!mod)
		return 0;

	ret = spl_lib_unload(mod->handle);
	if (ret)
		kerror("NG: %p:%s\n", mod->handle, mod->path);

	mod->path[0] = '\0';
	mod->handle = NULL;
	return ret;
}

