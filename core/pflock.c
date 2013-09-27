/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

/**
 * \file pflock.c
 * pflock, Pid File Lock, is used as single instance detector.
 */

#include <hilda/pflock.h>

/**
 * \brief Try lock a file.
 * If the pid file is not locked, create it then lock it. When use it
 * as multi-run detector, return 0 means the PID file is locked by
 * another instance, caller should quit to avoid duplicated instance.
 * When return 1, means NO same instance running.
 *
 * \param[in] pidfile Path to the PID file.
 * \param[out] retlck File handle when lock the PID file.
 *
 * \retval 0 Error when lock.
 * \retval 1 Success lock the PID file.
 */
int pflck_get(const char *pidfile, int *retlck)
{
	int fd;
	struct flock fl;

	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 1;

	if ((fd = open(pidfile, O_WRONLY | O_CREAT, 0666)) == -1)
		return 0;

	if (fcntl(fd, F_SETLK, &fl) == -1)
		return 0;

	*retlck = fd;
	return 1;
}

/**
 * \brief Close the lock file.
 *
 * \param lck Handle of the pid file opened by \c pflck_get().
 * \see pflck_get.
 *
 * \return Return value by \c close().
 */
int pflck_rel(int lck)
{
	return close(lck);
}

#ifdef PFLOCK_TEST
int main(void)
{
	int lck;

	if (!pflck_get("sdfasdfsadfsad.pid", &lck)) {
		fputs("Process already running!\n", stderr);
		return 1;
	}

	getchar();

	pflck_rel(lck);
	return 0;
}
#endif /* PFLOCK_TEST */

