#include <windows.h>
#include "gui_import.h"
#include "common.h"
#include "listview.h"
#include "extract.h"
#include "misc.h"
#include "recent.h"
#include "resource.h"
#include "sbar.h"
#include "thread.h"
#include <shlobj.h>

UINT ArchiveFile_Size(void);

char exepath[260];

void MakeExeThread()
{
	gi.MakeExe(exepath);
}

int CALLBACK MakeExeDlgFunc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int s;

	if (msg == WM_INITDIALOG)
	{
		CheckDlgButton(hDlg, IDC_PROGRAMFILES, 1);
		goto disableSubdirButtons;	// this saves 61 bytes :-)
	}
	if (msg == WM_COMMAND)
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (IsDlgButtonChecked(hDlg, IDC_CURRENTONLY))
				*(short *)exepath = 4;
			else
			{
				int subdir = IsDlgButtonChecked(hDlg, IDC_SUBDIR);
				char *ptr = exepath + subdir;
			
				GetDlgItemText(hDlg, IDC_EDIT1, ptr, 258);
				if (*ptr && !CheckForValidFilename(ptr, 2 + subdir))
					return 0;	// zero length is ok
				if (subdir)
				{
					char c;
					for (c = 0; c < 3; c++)
						if (IsDlgButtonChecked(hDlg, IDC_PROGRAMFILES + c))
							break;
					exepath[0] = c + 1;
				}
			}
			EndDialog (hDlg, 1);
			break;
		case IDCANCEL:
			EndDialog (hDlg, 0);
			break;
		case IDC_CURRENTONLY:
			s = !IsDlgButtonChecked(hDlg, IDC_CURRENTONLY);
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), s);
			EnableWindow(GetDlgItem(hDlg, IDC_SUBDIR), s);
			if (s) // yeah this is tricky...
		case IDC_SUBDIR:
		disableSubdirButtons:
			s = IsDlgButtonChecked(hDlg, IDC_SUBDIR);
			EnableWindow(GetDlgItem(hDlg, IDC_PROGRAMFILES), s);
			EnableWindow(GetDlgItem(hDlg, IDC_WINDOWS), s);
			EnableWindow(GetDlgItem(hDlg, IDC_TEMP), s);
			break;
		}
	return 0;
}

void MakeExe (int options)
{
	char exename[256];

	strcpy(exename, Recent.FileList[0]);
	strcpy(FileExtension(exename), ".exe");
	if (FileExists(exename))
		if (!YesNo("Exe file already exists,\nOverwrite?", "File exists", MainWnd, 1))
			return;

	Extract.handle = CreateNewFile(exename);
	if (Extract.handle == INVALID_HANDLE_VALUE)
		return;

	if (!options)
		exepath[0] = 0;
	else if (!DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAKESFX), MainWnd, MakeExeDlgFunc))
	{
		CloseHandle(Extract.handle);
		DeleteFile(exename);
		return;
	}
	PBarAddTotal(ArchiveFile_Size());
	PBarAddTotal(gi.SFXstart());
	GuiProgressMsg("creating %s", exename);
	RunThread(MakeExeThread, THREAD_MAKEEXE);
	CloseHandle(Extract.handle);
	if (AbortOp)
		DeleteFile(exename);
	else
		SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, exename, 0);
}

void ActionsMakeExe(void)
{
	MakeExe(1);
}