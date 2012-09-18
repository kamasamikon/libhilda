/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <string.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <dlfcn.h>
#include <semaphore.h>
#include <time.h>
#include <pthread.h>
#include <dirent.h>
#include <unistd.h>

#include <klog.h>
#include <kmem.h>

#include <xtcool.h>

typedef struct _spl_event_t spl_event_t;
struct _spl_event_t {
	SPL_HANDLE sem;
	SPL_HANDLE lock;
};

/**
 * @brief Create an event instance
 *
 * @param signaled 1 for signal the event by default, 0 for non-signel
 *
 * @return A handle for instance, 0 for error
 */
SPL_HANDLE spl_event_create(int signaled)
{
	spl_event_t *ev = (spl_event_t*)kmem_alloz(1, spl_event_t);
	ev->sem = spl_sema_new(signaled ? 1 : 0);
	ev->lock = spl_mutex_create();
	return (SPL_HANDLE)ev;
}

/**
 * @brief Signal the event
 *
 * @param h Handle of event instance
 *
 * @return SPL_EC_OK: Success
 *         SPL_EC_HANDLE: Illegal event handle
 */
int spl_event_set(SPL_HANDLE h)
{
	spl_event_t *ev = (spl_event_t*)h;
	spl_mutex_lock(ev->lock);
	spl_sema_rel(ev->sem);
	spl_mutex_unlock(ev->lock);
	return 0;
}
/**
 * @brief Set the event to be non-signaled
 *
 * @param h Handle of event instance
 *
 * @return SPL_EC_OK for success, SPL_EC_NG for error
 */
int spl_event_clear(SPL_HANDLE h)
{
	spl_event_t *ev = (spl_event_t*)h;
	int val;
	spl_mutex_lock(ev->lock);
	for (;;) {
		int spl_sema_val(SPL_HANDLE h, int *val);
		spl_sema_val(ev->sem, &val);
		if (val == 0)
			break;
		spl_sema_get(ev->sem, -1);
	}
	spl_mutex_unlock(ev->lock);
	return SPL_EC_OK;
}
int spl_event_wait(SPL_HANDLE h, unsigned int ms)
{
	spl_event_t *ev = (spl_event_t*)h;
	return spl_sema_get(ev->sem, ms);
}
int spl_event_destroy(SPL_HANDLE h)
{
	spl_event_t *ev = (spl_event_t*)h;
	spl_sema_del(ev->sem);
	spl_mutex_destroy(ev->lock);
	kmem_free(ev);
	return SPL_EC_OK;
}

/**
 * \brief locks
 */

/*
 * internal structure, should not public to caller
 */
typedef struct _sync_lock {
	SPL_HANDLE sema;

	struct {
		SPL_HANDLE tsk;
		int ref;
	} owner;
} sync_lock;

/*
 * initial state for a lock is unlocked
 */
SPL_HANDLE spl_lck_new(void)
{
	sync_lock *lck = (sync_lock*)kmem_alloz(1, sync_lock);
	if (lck) {
		lck->sema = spl_sema_new(1);
	}
	return (SPL_HANDLE)lck;
}
int spl_lck_del(SPL_HANDLE a_lck)
{
	sync_lock *lck = (sync_lock*)a_lck;
	if (lck && lck->sema) {
		spl_sema_del(lck->sema);
	}
	kmem_free(lck);
	return 0;
}
void spl_lck_get(SPL_HANDLE a_lck)
{
	sync_lock *lck = (sync_lock*)a_lck;
	SPL_HANDLE curtsk = spl_thread_current();

	if (lck && (lck->owner.tsk == curtsk)) {
		lck->owner.ref++;
		return;
	}
	spl_sema_get(lck->sema, -1);
	if (lck->owner.tsk == curtsk) {
		lck->owner.ref++;
	} else {
		lck->owner.tsk = curtsk;
		lck->owner.ref = 1;
	}
}
void spl_lck_rel(SPL_HANDLE a_lck)
{
	sync_lock *lck = (sync_lock*)a_lck;
	SPL_HANDLE curtsk = spl_thread_current();

	if (lck && (lck->owner.tsk == curtsk)) {
		lck->owner.ref--;
		if (0 == lck->owner.ref) {
			lck->owner.tsk = 0;
			spl_sema_rel(lck->sema);
		}
	} else {
		assert(!"SHIT!! unlock not ownered lock");
	}
}

/**
 * \brief Mutex
 */
SPL_HANDLE spl_mutex_create(void)
{
	pthread_mutex_t *mutex;

	mutex = (pthread_mutex_t*)kmem_alloz(1, pthread_mutex_t);
	/* default = PTHREAD_MUTEX_FAST_NP */
	if (pthread_mutex_init(mutex, 0)) {
		kmem_free(mutex);
		return 0;
	}
	return (SPL_HANDLE)mutex;
}
int spl_mutex_lock(SPL_HANDLE h)
{
	pthread_mutex_t *mutex = (pthread_mutex_t*)h;
	return pthread_mutex_lock(mutex) ? SPL_EC_NG : SPL_EC_OK;
}
int spl_mutex_unlock(SPL_HANDLE h)
{
	pthread_mutex_t *mutex = (pthread_mutex_t*)h;
	return pthread_mutex_unlock(mutex) ? SPL_EC_NG : SPL_EC_OK;
}
int spl_mutex_destroy(SPL_HANDLE h)
{
	pthread_mutex_t *mutex = (pthread_mutex_t*)h;
	pthread_mutex_destroy(mutex);
	kmem_free(mutex);
	return SPL_EC_OK;
}

/**
 * \brief semaphone
 */
SPL_HANDLE spl_sema_new(int init_val)
{
	sem_t *sem = (sem_t*)kmem_alloz(1, sem_t);
	sem_init(sem, 1, init_val);
	return (SPL_HANDLE)sem;
}
int spl_sema_del(SPL_HANDLE h)
{
	sem_t *sem = (sem_t*)h;
	sem_destroy(sem);
	kmem_free(sem);
	return SPL_EC_OK;
}
int spl_sema_val(SPL_HANDLE h, int *val)
{
	sem_t *sem = (sem_t*)h;
	return sem_getvalue(sem, val) ? SPL_EC_NG : SPL_EC_OK;
}
int spl_sema_get(SPL_HANDLE h, int timeout)
{
	sem_t *sem = (sem_t*)h;
	struct timespec ts;
	int ret;

	if (timeout < 0) {
		while ((ret = sem_wait(sem)) == -1 && errno == EINTR)
			continue;       /* Restart */
		return (0 == ret) ? SPL_EC_OK : SPL_EC_NG;
	} else {
		clock_gettime(CLOCK_REALTIME, &ts);

		ts.tv_nsec += (timeout % 1000) * 1000 * 1000;
		ts.tv_sec += (timeout / 1000) + (ts.tv_nsec / 1000000000);

		if (ts.tv_nsec >= 1000000000) {
			ts.tv_nsec -= 1000000000;
		}

		while ((ret = sem_timedwait(sem, &ts)) == -1 && errno == EINTR)
			continue;       /* Restart */

		return (0 == ret) ? 0 :
			(((-1 == ret) && (ETIMEDOUT == errno)) ? 1 : -1);
	}
}
int spl_sema_rel(SPL_HANDLE h)
{
	sem_t *sem = (sem_t*)h;
	sem_post(sem);
	return SPL_EC_OK;
}

typedef struct _spl_thread_t spl_thread_t;
struct _spl_thread_t {
	pthread_t id;
	pthread_t self;

	void *(*entry)(void *);
	void *param;
	int prio;
};

/**
 * \brief Thread
 */
SPL_HANDLE spl_thread_create(void *(*entry)(void *), void *param, int prio)
{
	pthread_t threadid;
	pthread_attr_t attr;
	struct sched_param sp;

	if (pthread_attr_init(&attr))
		return NULL;

#if 0
	if (pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM))
		return NULL;
#endif

	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	sp.sched_priority = prio;
	pthread_attr_setschedparam(&attr, &sp);

	if (pthread_create(&threadid, &attr, entry, (void*)param)) {
		perror("pthread_create");
		return NULL;
	}

	return (SPL_HANDLE)threadid;
}
SPL_HANDLE spl_thread_current()
{
	return (SPL_HANDLE)pthread_self();
}
int spl_thread_suspend(SPL_HANDLE h)
{
	return SPL_EC_NOT_SUPPRTED;
}
int spl_thread_resume(SPL_HANDLE h)
{
	return SPL_EC_NOT_SUPPRTED;
}
int spl_thread_set_priority(SPL_HANDLE h, int prio)
{
	return pthread_setschedprio((pthread_t)h, prio) ? SPL_EC_NG : SPL_EC_OK;
}
int spl_thread_wait(SPL_HANDLE h)
{
	void *status = 0;
	return pthread_join((pthread_t)h, &status) ? SPL_EC_NG : SPL_EC_OK;
}

void spl_thread_detach(SPL_HANDLE h)
{
	pthread_detach((pthread_t)h);
}

void spl_thread_exit(void)
{
	pthread_exit(0);
}
int spl_thread_kill(SPL_HANDLE h, int signo)
{
	int err;
	spl_thread_t *t = (spl_thread_t*)h;

	err = pthread_kill((pthread_t)h, signo) ? SPL_EC_NG : SPL_EC_OK;
	return err;

	err = pthread_kill(t->id, signo) ? SPL_EC_NG : SPL_EC_OK;
	return err;
}
int spl_thread_destroy(SPL_HANDLE h)
{
	int err;
	spl_thread_t *t = (spl_thread_t*)h;

	err = pthread_cancel((pthread_t)h) ? SPL_EC_NG : SPL_EC_OK;
	return err;

	err = pthread_cancel(t->id) ? SPL_EC_NG : SPL_EC_OK;
	return err;
}

/**
 * @brief Create a process and let it run freely
 *
 * @param prio priority of the process
 * @param argv[] same as argv in main, and ended with a NULL entry
 *
 * @return A handle for instance
 */
SPL_HANDLE spl_process_create(int prio, char *const argv[])
{
	pid_t pid;

	pid = fork();
	if (0 == pid) {
		/* child process, execute the command */
		execvp(argv[0], argv);
		exit(EXIT_FAILURE);
	}

	return (SPL_HANDLE)pid;
}
int spl_process_set_priority(SPL_HANDLE h, int prio)
{
	/* int pid = (int)h; */
	/* int setpriority(int which, int who, int prio); */
	return 0;
}
int spl_process_wait(SPL_HANDLE h)
{
	int pid = (int)h, retpid = -1;
	int status = 0;
	do {
		retpid = waitpid(pid, &status, WUNTRACED | WCONTINUED);
		if (retpid == -1) {
			perror("waitpid");
			break;
		}

		if (WIFEXITED(status))
			printf("exited, status=%d\n", WEXITSTATUS(status));
		else if (WIFSIGNALED(status))
			printf("killed by signal %d\n", WTERMSIG(status));
		else if (WIFSTOPPED(status))
			printf("stopped by signal %d\n", WSTOPSIG(status));
		else if (WIFCONTINUED(status))
			printf("continued\n");

	} while (!WIFEXITED(status) && !WIFSIGNALED(status));

	printf("spl_process_wait: pid:%x, retpid:%x, status:%x\n",
			pid, retpid, status);
	return pid == retpid ? SPL_EC_OK : SPL_EC_NG;
}
int spl_process_kill(SPL_HANDLE h, int signo)
{
	int pid = (int)h;
	return kill(pid, signo);
}
/**
 * \brief Other support
 */
/**
 * @brief Sleep the current thread
 *
 * @param ms Millisecond to sleep
 */
void spl_sleep(int ms)
{
	usleep(ms * 1000);
}

#ifdef WIN32
int gettimeofday (struct timeval *tv, void* tz)
{
	union {
		LONG_LONG ns100; /*time since 1 Jan 1601 in 100ns units */
		FILETIME ft;
	} now;

	GetSystemTimeAsFileTime (&now.ft);
	tv->tv_usec = (long) ((now.ns100 / 10LL) % 1000000LL);
	tv->tv_sec = (long) ((now.ns100 - 116444736000000000LL) / 10000000LL);
	return (0);
}
#endif

/**
 * @brief Get a tick timestamp
 *
 * @return tick in millisecond
 */
unsigned long spl_get_ticks(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

unsigned long long int spl_time_get_usec(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return (ts.tv_sec * 1000000) + (ts.tv_nsec / 1000);
}

/**
 * \brief seperator for directory, in *nix is '/' in win32s, is '\\'
 */
kchar kvfs_path_sep(kvoid)
{
	return '/';
}
kbool kvfs_exist(const kchar *a_path)
{
	if (-1 != access(a_path, 0))
		return ktrue;
	return kfalse;
}

/* Only fill d_type and d_name */
static int follow_link(const char *basedir, const char *name, struct dirent *ret_dirp)
{
	DIR *dir;
	struct stat buf;
	char fullpath[1024];
	int ret = 0;

	sprintf(fullpath, "%s/%s", basedir, name);
	if (stat(fullpath, &buf))
		return -1;

	if (S_ISREG(buf.st_mode))
		ret_dirp->d_type = KVFS_A_REG;
	else if (S_ISDIR(buf.st_mode))
		ret_dirp->d_type = KVFS_A_DIR;
	else if (S_ISCHR(buf.st_mode))
		ret_dirp->d_type = KVFS_A_DEV;
	else if (S_ISBLK(buf.st_mode))
		ret_dirp->d_type = KVFS_A_DEV;
	else if (S_ISFIFO(buf.st_mode))
		ret_dirp->d_type = KVFS_A_FIFO;
	else if (S_ISLNK(buf.st_mode))
		ret_dirp->d_type = KVFS_A_LNK;
	else
		ret_dirp->d_type = KVFS_A_UNK;

	strncpy(ret_dirp->d_name, name, sizeof(ret_dirp->d_name));
	return 0;
}

typedef struct _findbean_t findbean_t;
struct _findbean_t {
	char basedir[1024];
	DIR *dir;
};

static kuint get_file_attr(kuint d_type)
{
	switch (d_type) {
	case DT_DIR:
		return KVFS_A_DIR;
	case DT_LNK:
		return KVFS_A_LNK;
	case DT_FIFO:
		return KVFS_A_FIFO;
	case DT_CHR:
	case DT_BLK:
		return KVFS_A_DEV;
	case DT_REG:
		return KVFS_A_REG;
	case DT_SOCK:
		return KVFS_A_SKT;
	case DT_UNKNOWN:
	case DT_WHT:
	default:
		return KVFS_A_UNK;
	}
}

kbean kvfs_findfirst(const kchar *a_fspec, KVFS_FINDDATA *a_finfo)
{
	DIR *dir;
	struct dirent *dirp, lnk_dirp;
	findbean_t *fb;

	dir = opendir(a_fspec);
	if (!dir)
		return knil;

	dirp = readdir(dir);
	if (!dirp) {
		closedir(dir);
		return knil;
	}

	if (DT_LNK & dirp->d_type) {
		if (follow_link(a_fspec, dirp->d_name, &lnk_dirp)) {
			closedir(dir);
			return knil;
		}
		dirp = &lnk_dirp;
	}

	a_finfo->attrib = get_file_attr(dirp->d_type);
	strncpy(a_finfo->name, dirp->d_name, sizeof(a_finfo->name) - 1);
	a_finfo->name[sizeof(a_finfo->name) - 1] = '\0';

	fb = (findbean_t*)kmem_alloc(1, findbean_t);
	if (!fb) {
		closedir(dir);
		return knil;
	}

	strncpy(fb->basedir, a_fspec, sizeof(fb->basedir));
	fb->dir = dir;

	return (kbean)fb;
}

kint kvfs_findnext(kbean a_find, KVFS_FINDDATA *a_finfo)
{
	findbean_t *fb = (findbean_t*)a_find;
	DIR *dir;
	struct dirent *dirp, lnk_dirp;

	if (!fb)
		return -1;

	dir = fb->dir;
	dirp = readdir(dir);
	if (!dirp)
		return -1;

	if (DT_LNK & dirp->d_type) {
		if (follow_link(fb->basedir, dirp->d_name, &lnk_dirp)) {
			closedir(dir);
			return -1;
		}
		dirp = &lnk_dirp;
	}

	a_finfo->attrib = get_file_attr(dirp->d_type);
	strncpy(a_finfo->name, dirp->d_name, sizeof(a_finfo->name) - 1);
	a_finfo->name[sizeof(a_finfo->name) - 1] = '\0';

	return 0;
}

kint kvfs_findclose(kbean a_find)
{
	findbean_t *fb = (findbean_t*)a_find;
	if (fb) {
		if (fb->dir)
			closedir(fb->dir);
		kmem_free(fb);
		return 0;
	}
	return -1;
}

kint kvfs_chdir(const kchar *dir)
{
	return chdir(dir);
}

kchar *kvfs_getcwd(kchar *buf, kint size)
{
	return (kchar*)getcwd(buf, size);
}

typedef struct _spl_timer_t spl_timer_t;
struct _spl_timer_t {
	timer_t tid;

	char in_callback;
	char killed;

	TIMER_CBK callback;
	void *userdata;
};

static void sig_handle(sigval_t v)
{
	int conti = 0;
	spl_timer_t *t = (spl_timer_t*)v.sival_ptr;
	if (t) {
		t->in_callback = 1;
		conti = t->callback((void*)t, t->userdata);
		t->in_callback = 0;

		if (!conti || t->killed) {
			t->in_callback = 0;
			spl_timer_del((void*)t);
		}
	}
}

static void time_add_val(struct timespec *t1, struct timespec *t2)
{
	t1->tv_sec += t2->tv_sec;
	t1->tv_nsec += t2->tv_nsec;
	if (t1->tv_nsec < 0) {
		t1->tv_sec--;
		t1->tv_nsec += 1000000000;
	}
	if (t1->tv_nsec >= 1000000000) {
		t1->tv_sec++;
		t1->tv_nsec -= 1000000000;
	}
}

int spl_timer_add(void **retid, int interval, TIMER_CBK callback, void *userdata)
{
	struct sigevent se;
	struct itimerspec ts, ots;
	struct timespec tts;
	spl_timer_t *t = NULL;

	if (!callback)
		return -1;

	t = (spl_timer_t*)kmem_alloz(1, spl_timer_t);

	t->callback = callback;
	t->userdata = userdata;

	memset(&se, 0, sizeof(se));
	se.sigev_notify = SIGEV_THREAD;
	se.sigev_notify_function = sig_handle;
	se.sigev_value.sival_ptr = t;

	if (timer_create(CLOCK_REALTIME, &se, &t->tid) < 0) {
		perror("timer_creat");
		goto error;
	}

	clock_gettime(CLOCK_REALTIME, &ts.it_value);
	tts.tv_sec = interval / 1000;
	tts.tv_nsec = (interval % 1000) * 1000000;
	time_add_val(&ts.it_value, &tts);

	ts.it_interval.tv_sec = interval / 1000;
	ts.it_interval.tv_nsec = (interval % 1000) * 1000000;

	if (timer_settime(t->tid, TIMER_ABSTIME, &ts, &ots) < 0) {
		perror("timer_settime");
		goto error;
	}
	goto success;

error:
	if (t)
		kmem_free(t);
	return -1;

success:
	*retid = (void*)t;
	return 0;
}

int spl_timer_del(void *id)
{
	spl_timer_t *t = (spl_timer_t*)id;
	if (t) {
		if (t->in_callback)
			t->killed = 1;
		else {
			timer_delete(t->tid);
			kmem_free(t);
		}
	}
	return 0;
}

char* popen_read(const char *cmd, char *buffer, int buflen)
{
	int ret;
	FILE *fp = popen(cmd, "r");

	if (!fp)
		return NULL;

	ret = fread(buffer, sizeof(char), buflen, fp);
	fclose(fp);

	if (ret >= 0) {
		buffer[ret] = '\0';
		kstr_trim(buffer);
		return buffer;
	} else
		return NULL;
}

void *spl_lib_load(const char *path, int flags)
{
	void *handle = dlopen(path, RTLD_LAZY);
	if (!handle)
		printf("dlerror: %s\n", dlerror());
	return handle;
}

int spl_lib_unload(void *handle)
{
	if (handle)
		dlclose(handle);
	return 0;
}

void *spl_lib_getsym(void *lib, const char *name)
{
	return (void*)dlsym(lib, name);
}

void spl_exedir(char *argv[], kchar *exedir)
{
	char *p, buf[1024];
	int ret;

	ret = readlink("/proc/self/exe", buf, sizeof(buf));

	strcpy(exedir, buf);
	p = strrchr(exedir, '/');
	if (p)
		*p = '\0';
}

int spl_mkdir(const char *path, unsigned int mode)
{
	return mkdir(path, mode);
}

int wlogf(const char *fmt, ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = vfprintf(stdout, fmt, ap);
	va_end(ap);

	return ret;
}

int wlog(const char *msg)
{
	int ret;
	ret = fputs(msg, stdout);
	return ret;
}

int spl_sock_close(void *s)
{
	return close((int)s);
}
int spl_sock_err()
{
	return errno;
}

