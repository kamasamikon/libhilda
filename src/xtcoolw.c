/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <io.h>
#include <direct.h>
#include <time.h>

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
	BOOL bSignaled = signaled?TRUE:FALSE;
	SPL_HANDLE h = NULL;
	h = (SPL_HANDLE)CreateEvent(NULL,FALSE,bSignaled,NULL);
	return h;
}

/**
 * @brief Signal the event
 *
 * @param h Handle of event instance
 *
 * @return SPL_EC_OK: Success
 * SPL_EC_HANDLE: Illegal event handle
 */
int spl_event_set(SPL_HANDLE h)
{
	int ret = SPL_EC_NG;

	if (SetEvent((HANDLE)h))
	{
		ret = SPL_EC_OK;
	}

	return ret;
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
	int ret = SPL_EC_NG;

	if (ResetEvent((HANDLE)h))
	{
		ret = SPL_EC_OK;
	}

	return ret;
}
int spl_event_wait(SPL_HANDLE h, unsigned int ms)
{
	int ret = SPL_EC_NG;
	ret = WaitForSingleObject((HANDLE)h,ms);
	return ret;
}
int spl_event_destroy(SPL_HANDLE h)
{
	CloseHandle((HANDLE)h);
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
	sync_lock *lck = (sync_lock*)calloc(1, sizeof(sync_lock));
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
	free(lck);
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

#if 10

/**
 * \brief Mutex
 */
SPL_HANDLE spl_mutex_create(void)
{
	CRITICAL_SECTION *mutex = (CRITICAL_SECTION *)calloc(1, sizeof(CRITICAL_SECTION));
	/* default = PTHREAD_MUTEX_FAST_NP */
	InitializeCriticalSection(mutex);

	return (SPL_HANDLE)mutex;
}
int spl_mutex_lock(SPL_HANDLE h)
{
	EnterCriticalSection((CRITICAL_SECTION *)h);
	return SPL_EC_OK;
}
int spl_mutex_unlock(SPL_HANDLE h)
{
	LeaveCriticalSection((CRITICAL_SECTION *)h);
	return SPL_EC_OK;
}
int spl_mutex_destroy(SPL_HANDLE h)
{
	DeleteCriticalSection((CRITICAL_SECTION *)h);
	free(h);
	return SPL_EC_OK;
}
#endif

/**
 * \brief semaphone
 */
#define SEM_VALUE_MAX 1024*512

SPL_HANDLE spl_sema_new(int init_val)
{
	return (SPL_HANDLE) CreateSemaphore(NULL, init_val,SEM_VALUE_MAX, NULL);

}
int spl_sema_del(SPL_HANDLE h)
{
	CloseHandle(h);
	return SPL_EC_OK;
}
int spl_sema_val(SPL_HANDLE h, int *val)
{
	return SPL_EC_NG;
}
int spl_sema_get(SPL_HANDLE h, int timeout)
{
	return WaitForSingleObject(h, timeout);
}
int spl_sema_rel(SPL_HANDLE h)
{
	int ret;
	ret = ReleaseSemaphore(h, 1, NULL) ? SPL_EC_OK : SPL_EC_NG;
	return ret;
}





typedef struct _win_tsk_p win_tsk_p;
struct _win_tsk_p {
	void * pThreadHandle;
	DWORD ThreadId;
};

/**
 * \brief Thread
 */
SPL_HANDLE spl_thread_create(void *(*entry)(void *), void *param, int prio)
{

	win_tsk_p *pTsk = (win_tsk_p*)kmem_alloz(1,win_tsk_p);
	pTsk->pThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) entry, param, 0 , &pTsk->ThreadId);
	spl_thread_set_priority(pTsk,prio);
	return (SPL_HANDLE)pTsk;
}

SPL_HANDLE spl_thread_current()
{
	return (SPL_HANDLE)GetCurrentThreadId();
}

int spl_thread_suspend(SPL_HANDLE h)
{
	int ret = SPL_EC_NG;
	win_tsk_p * pTsk = (win_tsk_p *)h;

	if (pTsk->pThreadHandle != NULL && SuspendThread((HANDLE)pTsk->pThreadHandle) != 0xFFFFFFFF)
	{
		ret = SPL_EC_OK;
	}

	return ret;
}
int spl_thread_resume(SPL_HANDLE h)
{
	int ret = SPL_EC_NG;
	win_tsk_p * pTsk = (win_tsk_p *)h;

	if (pTsk->pThreadHandle != NULL && ResumeThread((HANDLE)pTsk->pThreadHandle) != 0xFFFFFFFF)
	{
		ret = SPL_EC_OK;
	}

	return ret;
}

int spl_thread_set_priority(SPL_HANDLE h, int prio)
{
	int ret = SPL_EC_NG;
	win_tsk_p * pTsk = (win_tsk_p *)h;
	int iPriorty = THREAD_PRIORITY_NORMAL;

	if (pTsk->pThreadHandle != NULL)
	{
		if (prio<-10)
		{
			iPriorty = THREAD_PRIORITY_IDLE;
		}
		else if (prio<0)
		{
			iPriorty = THREAD_PRIORITY_BELOW_NORMAL;
		}
		else if (prio < 10)
		{
			iPriorty = THREAD_PRIORITY_NORMAL;
		}
		else if (prio < 20)
		{
			iPriorty = THREAD_PRIORITY_ABOVE_NORMAL;
		}
		else if (prio < 50)
		{
			iPriorty = THREAD_PRIORITY_HIGHEST;
		}
		else
		{
			iPriorty = THREAD_PRIORITY_TIME_CRITICAL;
		}

		if (!SetThreadPriority((HANDLE)pTsk->pThreadHandle,iPriorty))
		{
			ret = SPL_EC_OK;
		}
	}


	return ret ;
}
int spl_thread_wait(SPL_HANDLE h)
{
	int ret = SPL_EC_NG;
	win_tsk_p * pTsk = (win_tsk_p *)h;

	if (pTsk->pThreadHandle == NULL)
	{
		return SPL_EC_PARAM;
	}

	ret = WaitForSingleObject((HANDLE)pTsk->pThreadHandle,INFINITE);

	if (WAIT_OBJECT_0 == ret)
	{
		ret = SPL_EC_OK ; // sucess
	}

	return ret;
}

void spl_thread_exit(void)
{
	DWORD dwExticode = 0;

	if (GetExitCodeThread((HANDLE)GetCurrentThread(),&dwExticode))
	{
		ExitThread(dwExticode);
	}
}
int spl_thread_kill(SPL_HANDLE h, int signo)
{
	DWORD dwExticode = 0;
	win_tsk_p * pTsk = (win_tsk_p *)h;

	if (GetExitCodeThread((HANDLE)pTsk->pThreadHandle,&dwExticode))
	{
		TerminateThread((HANDLE)pTsk->pThreadHandle,dwExticode);
	}
	return dwExticode;
}
int spl_thread_destroy(SPL_HANDLE h)
{
	win_tsk_p * pTsk = (win_tsk_p *)h;
	CloseHandle((HANDLE)pTsk->pThreadHandle);
	free(h);
	return SPL_EC_OK;
}

typedef struct _win_process_p {
	DWORD ProcessId;
	HANDLE ProcessHandle;
	HANDLE ThreadHandle;
	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcessInfo;
} win_process_p;

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
	win_process_p * pProc = kmem_alloz(1,win_process_p);
	DWORD dwCreate = CREATE_NO_WINDOW;
	char cmd[2048] = {0}, *p = cmd;
	int i = 0;
	int ofs = 0, len;

	if (argv == NULL)
		return NULL;

	while (argv[i]) {
		len = strlen(argv[i]);
		if (len > 0)
			ofs += _snprintf(p + ofs, sizeof(cmd) - ofs, "%s ", argv[i]);
		i++;
	}

	printf(cmd);

	pProc->StartupInfo.cb = sizeof(STARTUPINFO);

	if (CreateProcess(NULL,//(LPCWSTR) (l_vpMoudleName), // No module name (use command line).
				(LPTSTR)cmd,//(LPCWSTR)l_vpCommandline, // Command line.
				NULL, // Process handle not inheritable.
				NULL, // Thread handle not inheritable.
				FALSE, // Set handle inheritance to FALSE.
				dwCreate,//CREATE_NO_WINDOW, // No creation flags.
				NULL, // Use parent's environment block.
				NULL, // Use parent's starting directory.
				&pProc->StartupInfo, // Pointer to STARTUPINFO structure.
				&pProc->ProcessInfo) // Pointer to PROCESS_INFORMATION structure.
	)
	{
		pProc->ProcessHandle = pProc->ProcessInfo.hProcess;
		pProc->ThreadHandle = pProc->ProcessInfo.hThread;
		pProc->ProcessId = pProc->ProcessInfo.dwProcessId;
	}
	else
	{
		free(pProc);
		pProc = NULL;
	}

	return (SPL_HANDLE)pProc;
}

int spl_process_set_priority(SPL_HANDLE h, int prio)
{
	win_process_p * pProc = (win_process_p *)h;
	int ret = SPL_EC_NG;

	int iPriorty = NORMAL_PRIORITY_CLASS;

	if (prio<-10)
	{
		iPriorty = IDLE_PRIORITY_CLASS;
	}
	else if (prio<0)
	{
		iPriorty = BELOW_NORMAL_PRIORITY_CLASS;
	}
	else if (prio < 10)
	{
		iPriorty = NORMAL_PRIORITY_CLASS;
	}
	else if (prio < 20)
	{
		iPriorty = ABOVE_NORMAL_PRIORITY_CLASS;
	}
	else if (prio < 50)
	{
		iPriorty = HIGH_PRIORITY_CLASS;
	}
	else
	{
		iPriorty = REALTIME_PRIORITY_CLASS;
	}


	if (!SetPriorityClass((HANDLE)pProc->ProcessHandle,iPriorty))
	{
		ret = SPL_EC_OK;
	}


	return ret;
}
int spl_process_wait(SPL_HANDLE h)
{
	win_process_p * pProc = (win_process_p *)h;
	int status = 0;

	status = WaitForSingleObject(pProc->ProcessHandle, INFINITE);

	return status ? SPL_EC_OK : SPL_EC_NG;
}
int spl_process_kill(SPL_HANDLE h, int signo)
{
	int ret = SPL_EC_NG;
	STARTUPINFO l_Si;
	PROCESS_INFORMATION l_Pi;
	win_process_p * pProc = (win_process_p *)h;
	BOOL l_Bret = FALSE;
	char killCommand[256] = {0};

	memset(&l_Si,0,sizeof(STARTUPINFO));
	l_Si.cb = sizeof(l_Si);
	memset(&l_Pi,0,sizeof(PROCESS_INFORMATION));
	sprintf_s(killCommand, 256, "tskill %d",(DWORD)pProc->ProcessId);

	l_Bret = CreateProcess(NULL, // No module name (use command line).
			killCommand, // Command line.
			NULL, // Process handle not inheritable.
			NULL, // Thread handle not inheritable.
			FALSE, // Set handle inheritance to FALSE.
			CREATE_NO_WINDOW,//CREATE_NO_WINDOW, // No creation flags.
			NULL, // Use parent's environment block.
			NULL, // Use parent's starting directory.
			&l_Si, // Pointer to STARTUPINFO structure.
			&l_Pi); // Pointer to PROCESS_INFORMATION structure.

	if (l_Bret)
	{
		// Wait until child process exits.
		//WaitForSingleObject(l_Pi.hProcess, INFINITE);
		WaitForSingleObject(l_Pi.hProcess, 30*1000);

		// Close process and thread handles.
		CloseHandle(l_Pi.hProcess);
		CloseHandle(l_Pi.hThread);
		ret = SPL_EC_OK;
	}

	return ret;
}

int spl_process_destroy(SPL_HANDLE h)
{
	win_process_p * pProc = (win_process_p *)h;

	if (pProc == NULL)
	{
		return SPL_EC_NG;
	}

	CloseHandle(pProc->ThreadHandle);
	CloseHandle(pProc->ProcessHandle);
	free(pProc);

	return SPL_EC_OK;
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
	Sleep(ms);
}

/**
 * @brief Get a tick timestamp
 *
 * @return tick in millisecond
 */
unsigned long spl_get_ticks(void)
{
	static LARGE_INTEGER freq;
	static LARGE_INTEGER start;
	static int first = 1;

	LARGE_INTEGER counter;

	if (first) {
		first = 0;
		QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
		QueryPerformanceCounter((LARGE_INTEGER*)&start);
	}
	QueryPerformanceCounter((LARGE_INTEGER*)&counter);
	return (unsigned long)(1000 * (double) (counter.QuadPart - start.QuadPart) / freq.QuadPart);
}

/**
 * @brief seperator for directory, in *nix it should be '/' in win32s, should be '\\'
 */
kchar kvfs_path_sep(kvoid)
{
	return '\\';
}

/**
 * @brief is this path, include file or directory exist
 */
kbool kvfs_exist(const kchar *a_path)
{
	if (-1 != _access(a_path, 0)) {
		return ktrue;
	}
	return kfalse;
}

kbean kvfs_findfirst(const kchar *a_fspec, KVFS_FINDDATA *a_finfo)
{
	kchar fspec[300];
	struct _finddata_t fileinfo;
	kint hfile;

	sprintf(fspec, "%s/*.*", a_fspec);

	hfile = _findfirst(fspec, &fileinfo);
	if (hfile != -1) {
		a_finfo->attrib = (unsigned char)fileinfo.attrib;
		strncpy(a_finfo->name, fileinfo.name, sizeof(a_finfo->name) - 1);
		a_finfo->name[sizeof(a_finfo->name) - 1] = '\0';
		a_finfo->size = fileinfo.size;
		return (kbean) hfile;
	} else {
		return (kbean) 0;
	}
}

kint kvfs_findnext(kbean a_find, KVFS_FINDDATA *a_finfo)
{
	struct _finddata_t fileinfo;
	if (!_findnext((intptr_t)a_find, &fileinfo)) {
		a_finfo->attrib = (unsigned char)fileinfo.attrib;
		strncpy(a_finfo->name, fileinfo.name, sizeof(a_finfo->name) - 1);
		a_finfo->name[sizeof(a_finfo->name) - 1] = '\0';
		a_finfo->size = fileinfo.size;
		return 0;
	} else {
		return -1;
	}
}

kint kvfs_findclose(kbean a_find)
{
	return _findclose((intptr_t)a_find);
}

kint kvfs_chdir(const kchar *dir)
{
	return _chdir(dir);
}

kchar *kvfs_getcwd(kchar *buf, kint size)
{
	return _getcwd(buf, size);
}

int wlogx(const char *fmt, ...)
{
        int ret;
        va_list ap;
	char buffer[8192];

        va_start(ap, fmt);
	ret = vsnprintf(buffer, sizeof(buffer), fmt, ap);
        va_end(ap);

	OutputDebugString(buffer);
        return ret;
}

int wlogf(const char *fmt, ...)
{
	va_list ap;
	char buffer[4096], *bufptr = buffer;
	int ret, bufsize = sizeof(buffer);

	va_start(ap, fmt);
	ret = vsnprintf(bufptr, bufsize, fmt, ap);
	while (ret < 0) {
		bufsize <<= 1;
		if (bufptr != buffer)
			kmem_free(bufptr);
		bufptr = kmem_get(bufsize);
		ret = vsnprintf(bufptr, bufsize, fmt, ap);
	}
	va_end(ap);

	OutputDebugString(bufptr);

	if (bufptr != buffer)
		kmem_free(bufptr);
	return ret;
}

int wlog(const char *msg)
{
	OutputDebugString(msg);
	return 1;
}


void *spl_lib_load(const char *path, int flags)
{
	void *handle;
	wchar_t wpath[4096];
	mbstowcs(wpath, path, 4096);

	handle = (void*)LoadLibraryW(wpath);
	if (!handle)
		printf("LoadLibrary(%s) failure, errno:%d\n", path, GetLastError());

	return handle;
}


void *spl_lib_getsym(void *lib, const char *name)
{
	wchar_t wname[4096];
	mbstowcs(wname, name, 4096);

	return (void*)GetProcAddress(lib, name);
}

int spl_lib_unload(void *handle)
{
	if (handle)
		FreeLibrary(handle);
	return 0;
}

void spl_exedir(char *argv[], kchar *exedir)
{
	kbool fullpath;
	char *pspos, cwd[1024];
	kchar *ps = "\\";

	kvfs_getcwd(cwd, sizeof(cwd));
	fullpath = (kbool) strchr(argv[0], ':');

	if (fullpath)
		strcpy(exedir, argv[0]);
	else {
		strcpy(exedir, cwd);
		strcat(exedir, ps);
		strcat(exedir, argv[0]);
	}
	pspos = strrchr(exedir, ps[0]);
	if (pspos)
		*pspos = '\0';
}
