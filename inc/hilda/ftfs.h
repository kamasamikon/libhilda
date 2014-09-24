/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

/**
 * \file FaiT FS
 * A FS like data header used to wrap raw data apon RAW media
 * such as FLASH, EEPROM etc.
 */

#ifndef __K_FTFS_H__
#define __K_FTFS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Pack the raw data bo FTFS format.
 * Giving data, wrap the data with FTFS header. Caller should
 * call \c kmem_free to free the memeory.
 *
 * \param dat Raw data.
 * \param len Raw data length.
 * \param packlen Return the length of whole ftfs_s struct.
 *
 * \return A new allocated struct of ftfs_s.
 */
void *ftfs_pack(const char *dat, int len, int *packlen);

/**
 * \brief Peek the raw data form ftfs_s struct.
 *
 * \param fs
 * \param md5sum
 * \param dat
 * \param len
 *
 * \return 0 for success, -1 for bad magic, -2 for bad md5sum, -3 for otherwise
 */
int ftfs_unpack(void *pack, char **md5sum, char **dat, int *len);

/**
 * \brief Get the rest data length of the whole ftfs package.
 *
 * \param pack FTFS package.
 * \param length Current raw length, MUST more than 40 bytes.
 *
 * \return rest length;
 */
int ftfs_rest_length(void *pack, int length);

/**
 * \brief Return the header part length of FTFS.
 *
 * \return
 */
int ftfs_header_length();

#ifdef __cplusplus
}
#endif
#endif /* __K_FTFS_H__ */

