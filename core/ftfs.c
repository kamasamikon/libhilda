/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

/**
 * \file FaiT FS
 * A FS like data header used to wrap raw data apon RAW media
 * such as FLASH, EEPROM etc.
 */

#include <string.h>

#include <ktypes.h>
#include <md5sum.h>
#include <klog.h>
#include <kmem.h>

#include <ftfs.h>

static char MAGIC[4] = { 'F', 'T', 'F', 'S' };

typedef struct _ftfs_t ftfs_t;
struct _ftfs_t {
	char magic[4];                  /** 'F' 'T' 'F' 'S' */
	char md5sum[32];                /** check sum of length and data */
	int len;
	char data[0];                   /** Some compiler dont know data[0] */
};

/**
 * \brief Pack the raw data bo FTFS format.
 * Giving data, wrap the data with FTFS header. Caller should
 * call \c kmem_free to free the memeory.
 *
 * \param dat Raw data.
 * \param len Raw data length.
 * \param packlen Return the length of whole ftfs_t struct.
 *
 * \return A new allocated struct of ftfs_t.
 */
void *ftfs_pack(const char *dat, int len, int *packlen)
{
	int packsize = sizeof(ftfs_t) + len;
	ftfs_t *fs = (ftfs_t*)kmem_alloz(packsize, char);

	fs->magic[0] = MAGIC[0];
	fs->magic[1] = MAGIC[1];
	fs->magic[2] = MAGIC[2];
	fs->magic[3] = MAGIC[3];

	fs->len = len;
	memcpy(fs->data, dat, len);

	md5_calculate(fs->md5sum, (char*)&fs->len, sizeof(fs->len) + len);

	if (packlen)
		*packlen = packsize;

	return (void*)fs;
}

/**
 * \brief Peek the raw data form ftfs_t struct.
 *
 * \param fs
 * \param md5sum
 * \param dat
 * \param len
 *
 * \return 0 for success, -1 for bad magic, -2 for bad md5sum, -3 for otherwise
 */
int ftfs_unpack(void *pack, char **md5sum, char **dat, int *len)
{
	ftfs_t *fs = (ftfs_t*)pack;
	char newhash[32];

	if (!fs)
		return -3;

	if (fs->magic[0] != MAGIC[0] || fs->magic[1] != MAGIC[1] ||
			fs->magic[2] != MAGIC[2] ||
			fs->magic[3] != MAGIC[3]) {
		kerror("ftfs_unpack: Bad magic number.\n");
		return -1;
	}

	md5_calculate(newhash, (char*)&fs->len, sizeof(fs->len) + fs->len);
	if (memcmp(newhash, fs->md5sum, 32)) {
		kerror("ftfs_unpack: Bad md5 checksum.\n");
		return -2;
	}

	if (md5sum)
		*md5sum = fs->md5sum;
	if (dat)
		*dat = fs->data;
	if (len)
		*len = fs->len;

	return 0;
}

/**
 * \brief Get the rest data length of the whole ftfs package.
 *
 * \param pack FTFS package.
 * \param len Current raw length, MUST more than 40 bytes.
 *
 * \return rest length;
 */
int ftfs_rest_length(void *pack, int len)
{
	ftfs_t *fs = (ftfs_t*)pack;

	if (fs->magic[0] != MAGIC[0] || fs->magic[1] != MAGIC[1] ||
			fs->magic[2] != MAGIC[2] ||
			fs->magic[3] != MAGIC[3]) {
		kerror("ftfs_unpack: Bad magic number.\n");
		return -1;
	}

	len -= sizeof(ftfs_t);
	return (len >= fs->len) ? 0 : (fs->len - len);
}

/**
 * \brief Return the header part length of FTFS.
 *
 * \return
 */
int ftfs_header_length()
{
	return sizeof(ftfs_t);
}

