/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __HELPER_H__
#define __HELPER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ARR_INC(STEP, ARR, LEN, TYPE) \
	do { \
		void *arr; \
		(LEN) += STEP; \
		\
		arr = kmem_get_z((LEN) * sizeof(TYPE)); \
		if (ARR) { \
			memcpy(arr, (ARR), ((LEN) - STEP) * sizeof(TYPE)); \
			kmem_rel((void*)ARR); \
		} \
		ARR = (TYPE*)arr; \
	} while (0)

#ifdef __cplusplus
}
#endif

#endif /* __HELPER_H__ */

