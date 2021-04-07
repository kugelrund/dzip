#include <windows.h>
#include <stdio.h>
#include "listview.h"
#include "lvhelper.h"
#include "options.h"
#include "recent.h"
#include "thread.h"
#include <commctrl.h>

extern HWND MainWnd;
extern HINSTANCE hInstance;
HWND SBar, PBar;

const char defsbartext[] = "Dzip by Nolan Pflug";
char sbartext[2][280];
short sbarBuf;
short SBsize;

struct {
	ULONGLONG TotalSize, Progress, SkipVal, ThreshHold;
	short pos;
} PB;

char *GetFileFromPath (char *);

void CALLBACK UpdateSBar_timer(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	SendMessage(SBar, SB_SETTEXT, 0, (int)sbartext[sbarBuf]);
	KillTimer(hwnd, idEvent);
}

// write into the sbarBuf we're not using and then change
// sbarBuf to the other one, that way UpdateSBar_timer always sees
// a complete string in the buffer it looks at, not a partial one.
void UpdateSBar(void)
{
	if (!lvNumEntries)
		strcpy(sbartext[sbarBuf ^ 1], defsbartext);
	else
	{
		char *ptr = sbartext[sbarBuf ^ 1];
		int i = sprintf(ptr, "%s: %i files", GetFileFromPath(Recent.FileList[0]), lvNumEntries);
		if (lvNumEntries == 1)
			ptr[--i] = 0;	// fix "1 files" :)
		if (Options.ShowSelected)
			sprintf(ptr + i, " (selected %i)", LVNumSelected());
	}
	sbarBuf ^= 1;
	SetTimer(MainWnd, 1, 0, UpdateSBar_timer);
}

void SBarResize(void)
{
	int SBarSize[2];
	RECT r;

	if (!ThreadType)
	{
		SBarSize[0] = -1;
		SendMessage(SBar, SB_SETPARTS, 1, (int)SBarSize);
		SendMessage(PBar, PBM_SETPOS, 0, 0);
		ShowWindow(PBar, SW_HIDE);
		return;
	}

	GetClientRect(MainWnd, &r);
	SBarSize[0] = r.right / 2;
	SBarSize[1] = -1;
	SendMessage(SBar, SB_SETPARTS, 2, (int)SBarSize);
	SendMessage(SBar, SB_GETRECT, 1, (int)&r);
	SBarSize[1] = r.right;
	SendMessage(SBar, SB_SETPARTS, 2, (int)SBarSize);
	MoveWindow(PBar, r.left, r.top, r.right - r.left, r.bottom - r.top, 1);
	ShowWindow(PBar, SW_SHOWNORMAL);
}

void GuiProgressMsg (const char *msg, ...)
{
	va_list	the_args;
	va_start(the_args, msg);
	vsprintf(sbartext[sbarBuf ^ 1], msg, the_args);
	sbarBuf ^= 1;
	va_end(the_args);
	SetTimer(MainWnd, 1, 0, UpdateSBar_timer);
}

void CALLBACK UpdatePBar_timer(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	SendMessage(PBar, PBM_SETPOS, PB.pos, 0);
	KillTimer(hwnd, idEvent);
}

void PBarReset(void)
{
	memset(&PB, 0, sizeof(PB));
	PB.pos = -1;
	SendMessage(PBar, PBM_SETPOS, 0, 0);
}

void PBarAddTotal (UINT more)
{
	PB.TotalSize += more;
}

void PBarAdvance (UINT more)
{
	ULONGLONG testval;

	if (!PB.TotalSize)
		return;

	PB.Progress += more;

	/* shift PB.Progress left by 10 and put in testval
	   I could just do: testval = PB.Progress << 10;
	   but then the compiler uses the generic allshl routine */
	__asm {
		mov eax, dword ptr[PB.Progress]
		mov edx, dword ptr[PB.Progress + 4]
		shld edx, eax, 10
		mov dword ptr[testval + 4], edx
		shl eax, 10
		mov dword ptr[testval], eax
	}

	if (testval < PB.ThreshHold)
		return;

	do
	{
		PB.pos++;
		PB.ThreshHold += PB.TotalSize;
	} while (testval >= PB.ThreshHold);

	SetTimer(MainWnd, 2, 0, UpdatePBar_timer);
}

void PBarSetSkip (UINT more)
{
	PB.SkipVal = PB.Progress + more;
}

void PBarSkip(void)
{
	PB.Progress = PB.SkipVal;
	PBarAdvance(0);
}

void PBarGenericProgress (UINT total, UINT completed)
{
	PB.TotalSize = total;
	PB.Progress = PB.ThreshHold = 0;
	PB.pos = -1;
	PBarAdvance(completed);
}

void CreatePBar(void)
{
	PBar = CreateWindow(PROGRESS_CLASS, NULL,
		WS_CHILD|WS_VISIBLE| (Options.SmoothPBar ? PBS_SMOOTH : 0),
		0, 0, 0, 0, SBar, 0, hInstance, NULL);
	SendMessage(PBar, PBM_SETRANGE32, 0, 1024);
	SBarResize();
}

void CreateStatusBar(void)
{
	RECT r;
	SBar = CreateStatusWindow(WS_CHILD|WS_VISIBLE, "", MainWnd, 0);
	GetWindowRect(SBar, &r);
	SBsize = (short)(r.bottom - r.top);
	CreatePBar();
}