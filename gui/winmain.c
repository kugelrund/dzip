#include <stdio.h>
#include <windows.h>
#include "common.h"
#include "listview.h"
#include "extract.h"
#include "file.h"
#include "menu.h"
#include "misc.h"
#include "options.h"
#include "recent.h"
#include "resource.h"
#include "sbar.h"
#include "thread.h"
#include <shlobj.h>

void AddFilesFromCommandLine(char *, int);
long CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
int ScanForDLLs(void);
HACCEL hacl;
void crc_init();

const ACCEL accel[] = {
	{FVIRTKEY|FNOINVERT, VK_F2, ID_ACTIONS_RENAME},
	{FVIRTKEY|FNOINVERT|FCONTROL, 'N', ID_FILE_NEW},
	{FVIRTKEY|FNOINVERT|FCONTROL, 'O', ID_FILE_OPEN},
	{FVIRTKEY|FNOINVERT|FCONTROL, VK_F4, ID_FILE_CLOSE},
	{FVIRTKEY|FNOINVERT|FCONTROL, 'A', ID_ACTIONS_SELECTALL},
	{FVIRTKEY|FNOINVERT|FCONTROL, 'I', ID_ACTIONS_INVERT},
	{FVIRTKEY|FNOINVERT|FSHIFT, 'A', ID_ACTIONS_ADD},
	{FVIRTKEY|FNOINVERT|FSHIFT, 'D', ID_ACTIONS_DELETE},
	{FVIRTKEY|FNOINVERT, VK_DELETE, ID_ACTIONS_DELETE},
	{FVIRTKEY|FNOINVERT|FSHIFT, 'E', ID_ACTIONS_EXTRACT},
	{FVIRTKEY|FNOINVERT|FSHIFT, 'T', ID_ACTIONS_TEST},
	{FVIRTKEY|FNOINVERT|FSHIFT, 'V', ID_ACTIONS_VIEW},
};

void FileExit(void)
{
	int i;
	if (ThreadType)
	{
		if (ThreadType == THREAD_DELETE)
		{
			error("Sorry, you're going to have to wait...\n"
				"Unless you want to Ctrl+Alt+Delete me and end up with a corrupt file :)");
			return;
		}
		if (!YesNo("Abort current operation?", "Dzip", MainWnd, 1))
			return;
		if (ThreadType)	// make sure thread is still running
		{
			AbortOp = 2;
			return;
		}
	}

	SaveWindowPosition(MainWnd, "window");
	ShowWindow(MainWnd, SW_HIDE);	// hide window to make it seem we've already exited

	WriteRegValue(HKEY_CURRENT_USER, dzipreg, "options", REG_BINARY, (char *)&Options, 0, sizeof(Options));
	LVSaveState();
	
// write out recent lists
	i = Recent.NumFiles + (Recent.NumExtractPaths << 4) + (Recent.NumMovePaths << 8);
	WriteRegValue(HKEY_CURRENT_USER, dzipreg, "numrecent", REG_DWORD, (char *)&i, 0, 4);
	RecentWrite(Recent.FileList, "file", Recent.NumFiles);
	
	if (lvNumEntries)
		FileClose();

	DestroyWindow(MainWnd);
	DestroyAcceleratorTable(hacl);
	exit(0);
}

void ReadRegistry (int *CmdShow)
{
	int i;
	// read in stuff saved on exit
	if (RestoreWindowPosition(MainWnd, "window") && *CmdShow == SW_SHOWNORMAL)
		*CmdShow = SW_SHOWMAXIMIZED;
	if (!ReadRegValue(HKEY_CURRENT_USER, dzipreg, "numrecent", (char *)&i, 4))
		i = 0;
	Recent.NumFiles = RecentRead(&Recent.FileList, "file", i & 15);
	if (!ReadRegValue(HKEY_CURRENT_USER, dzipreg, "options", (char *)&Options, sizeof(Options)))
		SetDefaultOptions();
}

void ProcessCmdLine (char *CmdLine)
{
	// /x dzfile"file1"file2"...fileX" will compress file1...fileX
	// into dzfile, make dzfile.exe and then quit
//	int makeexe = !strncmp (CmdLine, "/x ", 3);  strcmp sucks for small strings
	int CmdLine3 = 0xffffff & *(int *)CmdLine;
	int makeexe = (CmdLine3 == ' x/');

	if (*CmdLine != '/' || makeexe)
	{	// if we didn't get /something just try to open it
		// for /x open it and then makeexe
		WIN32_FIND_DATA f;
		HANDLE h;
		char *ext;

		if (makeexe)
		{
			CmdLine += 3;
			*strchr(CmdLine, '"') = 0;
		}

		if (*CmdLine == '"')	// stupid WinME
		{
			CmdLine++;
			CmdLine[strlen(CmdLine) - 1] = 0;
		}
		GetFullPathName(CmdLine, 256, temp1, NULL);
		ext = FileExtension(temp1);
		if (!*ext)
			strcpy(ext, ".dz");	// use 'default extension'
		// convert a *~? short name to the real thing
		h = FindFirstFile(temp1, &f);
		if (h == INVALID_HANDLE_VALUE)
		{
			if (!makeexe)
			{	// only if somebody started it manually with an arg
				error("%s does not exist", CmdLine);
				return;
			}
			if (!NewArchive(temp1))
				return;
		}
		else
		{
			FindClose(h);
			strcpy(GetFileFromPath(temp1), f.cFileName);
			OpenArchive(temp1, 0);
			if (!lvNumEntries)
				return;
		}
		if (makeexe)
		{
			AddFilesFromCommandLine(CmdLine + strlen(CmdLine) + 1, 2);
			MakeExe(0);
			FileExit();
		}
	}
//	else if (!strncmp (CmdLine, "/a ", 3) || !strncmp (CmdLine, "/d ", 3))  strcmp sucks for small strings
	else if (CmdLine3 == ' a/' || CmdLine3 == ' d/')
		// right click on files and chose compress with Dzip
		// /d is for files dropped on a file, then the first file of the string is the destination
		AddFilesFromCommandLine(CmdLine + 3, CmdLine[1] == 'd');
//	else if (!strncmp (CmdLine, "/e ", 3))  strcmp sucks for small strings
	else if (CmdLine3 == ' e/')
	{	// right click on file and chose extract [all] here
		// multiple files seperated by "
		char *curfile, *ptr;
		int esc = 0;

		// only the first file has the full path
		curfile = GetFileFromPath(CmdLine + 3);
		curfile[-1] = 0;

		while (*curfile && !esc)
		{
			if (lvNumEntries) CloseArchive();
			for (ptr = curfile + 1; *ptr != '"'; ptr++)
				if (!*ptr) {esc = 1; break;}	// work whether there is " at end or just a null
			*ptr = 0;
			sprintf(temp1, "%s\\%s", CmdLine + 3, curfile);
			OpenArchive(temp1, 0);
			curfile = ptr + 1;
			if (lvNumEntries)
			{
				ExtractSetup(0, 1, NULL);
				RunThread(ExtractThread, THREAD_EXTRACT);
			}
		}
		if (Options.AutoClose)
			FileExit();
	}
//	else if (!strcmp(CmdLine, "/u")) strcmp sucks for small strings
	else if (CmdLine3 == '\0u/')
		OptionsUninstall();
	else
	{
		strcpy(temp1, CmdLine);
		CmdLine = strchr(temp1, ' ');
		if (CmdLine) *CmdLine = 0;
		error("Unknown switch %s", temp1);
	}
}

int GuiInit (char *CmdLine, int CmdShow)
{
	WNDCLASS wc;

	memset(&wc, 0, 36);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DZ));
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszClassName = "DzipWndClass";

	RegisterClass(&wc);
	MainWnd = CreateWindowEx(WS_EX_WINDOWEDGE|WS_EX_ACCEPTFILES, 
		"DzipWndClass", "Dzip",	WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		514, 374, NULL, NULL, hInstance, NULL);

	ReadRegistry(&CmdShow);

	InitCommonControls();
	CreateMenuBar();
	if (Options.Toolbar) CreateToolbar();
	if (Options.StatusBar)
	{
		CreateStatusBar();
		UpdateSBar();
	}
	CreateListView();

	if (!MainWnd || !LView) return 0;

	SetErrorMode(SEM_FAILCRITICALERRORS);
	ShowWindow(MainWnd, CmdShow);

	crc_init();
	SetNumberFormat();
	if (!ScanForDLLs())
		return 0;

	if (*CmdLine)
		ProcessCmdLine(CmdLine);
	else if (Recent.NumFiles)
		SetDir(Recent.FileList[0]);

	hacl = CreateAcceleratorTable((ACCEL *)accel, sizeof accel/sizeof(ACCEL));
	return 1;
}

long WINAPI ExceptionHandler(EXCEPTION_POINTERS *ExceptionInfo)
{
	static char crashed = 0;
	if (crashed)
	{
		if (crashed == 2)
			return EXCEPTION_EXECUTE_HANDLER;
		crashed = 2;
		DestroyWindow(MainWnd);
		MessageBox(NULL, "Recursive crashed detected, hit OK to kill process", "Dzip fatal error", MB_TASKMODAL|MB_OK|MB_ICONSTOP);
		return EXCEPTION_EXECUTE_HANDLER;
	}
	crashed = 1;
	error("Exception %x occured at address %x\nPlease report what you did to radix@planetquake.com",
		ExceptionInfo->ExceptionRecord->ExceptionCode,
		ExceptionInfo->ExceptionRecord->ExceptionAddress);
	DestroyAcceleratorTable(hacl);
	return EXCEPTION_EXECUTE_HANDLER;
}

// pretty dull, huh?
int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hPrevInst, char *CmdLine, int CmdShow)
{
	hInstance = hInst;

	SetUnhandledExceptionFilter(ExceptionHandler);

	if (!GuiInit(CmdLine, CmdShow))
		return 1;

	for (;; MsgLoop());
}