enum {
	THREAD_NONE, THREAD_ADD, THREAD_DELETE, THREAD_EXTRACT,
	THREAD_VIEW, THREAD_DRAGOUT, THREAD_DRAGTOEXPLORER,
	THREAD_DRAGTOWIN2KEXPLORER, THREAD_TEST
};

extern char ThreadType;
extern HANDLE hThread;

void AddThread(void);
void ExtractThread(void);
void MsgLoop(void);
void RunThread(void (*)(void), int);
void ThreadCleanUp(void);
