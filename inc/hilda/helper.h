/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_HELPER_H__
#define __K_HELPER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ARR_INC(STEP, ARR, LEN, TYPE) \
	do { \
		void *arr; \
		(LEN) += (STEP); \
		\
		arr = kmem_get((LEN) * sizeof(TYPE)); \
		if (ARR) { \
			memcpy(arr, (ARR), ((LEN) - (STEP)) * sizeof(TYPE)); \
			kmem_rel((void*)ARR); \
			memset(((char*)arr) + (((LEN) - (STEP)) * sizeof(TYPE)), 0, (STEP) * sizeof(TYPE)); \
		} \
		ARR = (TYPE*)arr; \
	} while (0)

#ifdef __cplusplus
}
#endif

#endif /* __K_HELPER_H__ */

