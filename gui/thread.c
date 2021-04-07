#include <windows.h>
#include "common.h"
#include "menu.h"
#include "misc.h"
#include "sbar.h"
#include "thread.h"
#include <commctrl.h>

char ThreadType;
HANDLE hThread;

void ThreadCleanUp(void)
{
	UpdateSBar();
	ThreadType = THREAD_NONE;
	MenuActivateCommands();

	SBarResize();
	if (AbortOp == 2)
		PostMessage(MainWnd, WM_CLOSE, 0, 0);
}

DWORD WINAPI ThreadFunc (void (*func)(void))
{
	func();
	return 0;
}

void RunThread (void (*func)(void), int ttype)
{
	int i;

	ThreadType = ttype;
	AbortOp = 0;

	SBarResize();
	MenuActivateCommands();

	if (func)
	{
		hThread = CreateThread(NULL, 0, ThreadFunc, func, 0, &i);
		for (;;)
		switch (MsgWaitForMultipleObjects(1, &hThread, 0, INFINITE, QS_ALLINPUT|QS_ALLPOSTMESSAGE))
		{
		case WAIT_OBJECT_0:	// the thread finished
			CloseHandle(hThread);
			hThread = NULL;
			ThreadCleanUp();
			return;
		case WAIT_OBJECT_0 + 1:	// some input is available
			{
				MSG msg;
				while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
					MsgLoop();
			}
		}
	}
}