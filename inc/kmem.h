/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_MEM_H__
#define __K_MEM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <sysdeps.h>

#include <ktypes.h>

kint kmem_init(kuint a_flg);
kint kmem_final(kvoid);
kint kmem_rpt(kint *alloc_cnt, kint *free_cnt, kint *memusage, kint *mempeak);

kvoid *kmem_get(kuint size);
kvoid *kmem_get_z(kuint size);
kvoid *kmem_reget(kvoid *usrptr, kuint size);
kvoid kmem_rel(kvoid *usrptr);

#if 0
#define kmem_alloc(cnt, type) ({ void *__mem_temp_p = kmem_get((cnt) * sizeof(type)); printf("MEM.A:%d:%s\n", __LINE__, __func__); __mem_temp_p; })
#define kmem_alloz(cnt, type) ({ void *__mem_temp_p = kmem_get_z((cnt) * sizeof(type)); printf("MEM.A:%d:%s\n", __LINE__, __func__); __mem_temp_p; })
#define kmem_realloc(p, sz) ({ void *__mem_temp_np = kmem_reget((p), (sz)); printf("MEM.A:%d:%s\n", __LINE__, __func__); __mem_temp_np; })
#define kmem_free(p) do { kmem_rel(p); printf("MEM.F:%d:%s\n", __LINE__, __func__); } while (0)
#define kmem_free_s(p) do { if (p) kmem_rel(p); printf("MEM.F:%d:%s\n", __LINE__, __func__); } while (0)
#define kmem_free_z(p) do { kmem_rel(p); p = knil; printf("MEM.F:%d:%s\n", __LINE__, __func__); } while (0)
#define kmem_free_sz(p) do { if (p) { kmem_rel(p); p = knil; printf("MEM.F:%d:%s\n", __LINE__, __func__); } } while (0)
#else
#define kmem_alloc(cnt, type) kmem_get((cnt) * sizeof(type))
#define kmem_alloz(cnt, type) kmem_get_z((cnt) * sizeof(type))
#define kmem_realloc(p, sz) kmem_reget((p), (sz))
#define kmem_free(p) do { kmem_rel(p); } while (0)
#define kmem_free_s(p) do { if (p) kmem_rel(p); } while (0)
#define kmem_free_z(p) do { kmem_rel(p); p = knil; } while (0)
#define kmem_free_sz(p) do { if (p) { kmem_rel(p); p = knil; } } while (0)
#endif

kvoid* kmem_move(kvoid *to, kvoid *fr, kuint num);
void kmem_dump(const char *banner, char *dat, int len, int width);

#ifdef __cplusplus
}
#endif

#endif /* __K_MEM_H__ */

