#include <stdio.h>
#include <windows.h>
#include "common.h"
#include "listview.h"
#include "lvhelper.h"
#include "menu.h"
#include "misc.h"
#include "options.h"
#include "recent.h"
#include "resource.h"
#include "sbar.h"
#include "thread.h"
#include <shlobj.h>

options_t Options;
void PopulateFileTypes (HWND);
void ChangeExtensionRegistrations (HWND);

void OptionsToolbar(void)
{
	Options.Toolbar ^= 1;
	if (Options.Toolbar)
		CreateToolbar();
	else
	{
		DestroyToolbar();
		TBsize = 0;
	}
	LVResize();
}

void OptionsChangeToolbarSize(void)
{
	if (Options.Toolbar)
	{
		DestroyToolbar();
		Options.BigToolbar ^= 1;
		CreateToolbar();
		LVResize();
		return;
	}
	Options.BigToolbar ^= 1;
}

void OptionsMenuIcons(void)
{
	Options.MenuIcons ^= 1;
	MenuReset(0);
}

void OptionsStatusBar(void)
{
	Options.StatusBar ^= 1;
	if (Options.StatusBar)
		CreateStatusBar();
	else
	{
		DestroyWindow(SBar);
		SBsize = 0;
	}
	LVResize();
}

void OptionsSmoothPBar()
{
	Options.SmoothPBar ^= 1;
	if (ThreadType)
	{	// cant just set the window style, the dumb thing
		// won't notice that it has changed
		DestroyWindow(PBar);
		CreatePBar();
	}
}

void OptionsShowSelected(void)
{
	Options.ShowSelected ^= 1;
	if (!ThreadType)
		UpdateSBar();
}

void OptionsTypeCol(void)
{
	Options.TypeCol ^= 1;
	if (!Options.TypeCol)
	{
		int i = LVColumnWidth(5);
		WriteRegValue(HKEY_CURRENT_USER, dzipreg, "lvtype", REG_DWORD, (char *)&i, 1, 4);
		LVDeleteColumn(5);
	}
	else
	{
		LVCOLUMN column;
		column.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;
		column.fmt = LVCFMT_LEFT;
		if (!ReadRegValue(HKEY_CURRENT_USER, dzipreg, "lvtype", (char *)&column.cx, 4))
			column.cx = 86;
		column.pszText = "Type";
		LVInsertColumn(&column, 5);
		if ((SortType & 0x7f) == 5)
			LVMoveSortArrow(SortType, SortType);
	}
	LVSaveState();
}

void OptionsPathCol(void)
{
	int i;
	Options.PathCol ^= 1;
	if (!Options.PathCol)
	{
		i = LVColumnWidth(5 + Options.TypeCol);
		WriteRegValue(HKEY_CURRENT_USER, dzipreg, "lvpath", REG_DWORD, (char *)&i, 1, 4);
		LVDeleteColumn(5 + Options.TypeCol);
	}
	else
	{
		LVCOLUMN column;
		column.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;
		column.fmt = LVCFMT_LEFT;
		if (!ReadRegValue(HKEY_CURRENT_USER, dzipreg, "lvpath", (char *)&column.cx, 4))
			column.cx = 70;
		column.pszText = "Path";
		LVInsertColumn(&column, 6);
	}
//	if sort is path+filename, we need to move the arrow
	if (!(SortType & 0x7f))
		LVMoveSortArrow(6, SortType);
	LVSaveState();
}

int CALLBACK OptionsDlgFunc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INITDIALOG)
	{
		char s[2];
		s[1] = 0;
		for (s[0] = '0'; s[0] <= '9'; s[0]++)
			SendDlgItemMessage(hDlg, IDC_RFL, CB_ADDSTRING, 0, (int)s);
		SendDlgItemMessage(hDlg, IDC_RFL, CB_SETCURSEL, Options.RFLsize, 0);
		CheckDlgButton(hDlg, IDC_TOOLBAR, Options.Toolbar);
		CheckDlgButton(hDlg, IDC_TOOLBARSMALL + Options.BigToolbar, 1);
		CheckDlgButton(hDlg, IDC_MENUICONS, Options.MenuIcons);
		CheckDlgButton(hDlg, IDC_TYPECOL, Options.TypeCol);
		CheckDlgButton(hDlg, IDC_PATHCOL, Options.PathCol);
		CheckDlgButton(hDlg, IDC_SBAR, Options.StatusBar);
		CheckDlgButton(hDlg, IDC_SELECTED, Options.ShowSelected);
		CheckDlgButton(hDlg, IDC_AUTOCLOSE, Options.AutoClose);
		CheckDlgButton(hDlg, IDC_LCFILENAMES, Options.LCFilenames);
		CheckDlgButton(hDlg, IDC_EXPCMPWDZ, Options.ExpCmpWDz);
		CheckDlgButton(hDlg, IDC_SMOOTHPBAR, Options.SmoothPBar);
		CheckDlgButton(hDlg, IDC_ALLFILES, Options.AllFilesInAdd);
		return 1;
	}
	if (msg == WM_COMMAND)
	{
		if (LOWORD(wParam) == IDOK)
		{
			int x;

			if (Options.Toolbar != IsDlgButtonChecked(hDlg, IDC_TOOLBAR))
				OptionsToolbar();
			if (Options.BigToolbar != IsDlgButtonChecked(hDlg, IDC_TOOLBARLARGE))
				OptionsChangeToolbarSize();
			if (Options.MenuIcons != IsDlgButtonChecked(hDlg, IDC_MENUICONS))
				OptionsMenuIcons();
			if (Options.TypeCol != IsDlgButtonChecked(hDlg, IDC_TYPECOL))
				OptionsTypeCol();
			if (Options.PathCol != IsDlgButtonChecked(hDlg, IDC_PATHCOL))
				OptionsPathCol();
			if (Options.StatusBar != IsDlgButtonChecked(hDlg, IDC_SBAR))
				OptionsStatusBar();
			if (Options.ShowSelected != IsDlgButtonChecked(hDlg, IDC_SELECTED))
				OptionsShowSelected();
			if (Options.SmoothPBar != IsDlgButtonChecked(hDlg, IDC_SMOOTHPBAR))
				OptionsSmoothPBar();
			if (Options.AutoClose != IsDlgButtonChecked(hDlg, IDC_AUTOCLOSE))
				Options.AutoClose ^= 1;
			if (Options.LCFilenames != IsDlgButtonChecked(hDlg, IDC_LCFILENAMES))
				Options.LCFilenames ^= 1;
			if (Options.ExpCmpWDz != IsDlgButtonChecked(hDlg, IDC_EXPCMPWDZ))
				Options.ExpCmpWDz ^= 1;
			if (Options.AllFilesInAdd != IsDlgButtonChecked(hDlg, IDC_ALLFILES))
				Options.AllFilesInAdd ^= 1;

			x = SendDlgItemMessage(hDlg, IDC_RFL, CB_GETCURSEL, 0, 0);
			if (Options.RFLsize != x)
			{
				Options.RFLsize = x;
				while (DeleteMenu(Menu.file, FILEMENULENGTH, MF_BYPOSITION));
				MenuReset(1);
				MenuUpdateRFL(NULL);
			}
			// write Options back to registry immediately
			// in case ExpCmpWDz changed or program is killed/crashes
			WriteRegValue(HKEY_CURRENT_USER, dzipreg, "options", REG_BINARY, (char *)&Options, 0, sizeof(Options));

			EndDialog(hDlg, 1);
		}
		else if (LOWORD(wParam) == IDCANCEL)
			EndDialog(hDlg, 0);
	}
	return 0;
}

void OptionsOptions(void)
{
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_OPTIONS), MainWnd, OptionsDlgFunc);
}

int CALLBACK OptionsFileTypesDlgFunc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		LVCOLUMN lvc;
		HWND lv = GetDlgItem(hDlg, IDC_LVFILETYPES);

		ListView_SetExtendedListViewStyle(lv, LVS_EX_CHECKBOXES);
		lvc.mask = LVCF_WIDTH;
		lvc.cx = 1000;
		ListView_InsertColumn(lv, 0, &lvc);
		PopulateFileTypes(lv);
		ListView_SetColumnWidth(lv, 0, -1);
		return 1;
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
			ChangeExtensionRegistrations(GetDlgItem(hDlg, IDC_LVFILETYPES));
		return EndDialog(hDlg, 0);
	case WM_NOTIFY:
		switch (((NMHDR *)lParam)->code)
		{	// prevent an item from getting the focus or a selection box
		case LVN_ITEMCHANGING:
		{
			NMLISTVIEW *nmlv = (NMLISTVIEW *)lParam;
			SetWindowLong(hDlg, DWL_MSGRESULT, nmlv->uNewState & 3);
			break;
		}	// no drag a rectangle box
		case LVN_MARQUEEBEGIN:
			SetWindowLong(hDlg, DWL_MSGRESULT, 1);
		}
		return 1;
	}
	return 0;
}

void OptionsFileTypes(void)
{
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_FILETYPES), MainWnd, OptionsFileTypesDlgFunc);
}

void OptionsUninstall(void)
{
	IMalloc *m;
	ITEMIDLIST *itlist;
	HKEY hk;
	char *file, *msg;
	short dlldeleted = 0, exedeleted = 0;
	ULONG stupidpointer;	// for those functions that just won't take a null

	if (!YesNo("Are you sure you want\nto uninstall Dzip?", "Dzip", MainWnd, 1))
		return;
	if (ThreadType)	// if a thread is running, just kill it
		TerminateThread(hThread, 0);
	GetModuleFileName(hInstance, temp2, 256);
	SetDir(temp2);
	UnloadDLL();
// change all extensions back to what they were before
// and then delete everything else in the registry
	ChangeExtensionRegistrations(NULL);
	RegDeleteKey(HKEY_CURRENT_USER, dzipreg);
	RegDeleteKey(HKEY_CLASSES_ROOT, "*\\shellex\\ContextMenuHandlers\\Dzip");
	sprintf(temp1, "CLSID\\%s", dzGUID);
	RegDeleteKey(HKEY_CLASSES_ROOT, temp1);
	RegDeleteKey(HKEY_CLASSES_ROOT, "Drive\\shellex\\DragDropHandlers\\Dzip");
	RegDeleteKey(HKEY_CLASSES_ROOT, "Folder\\shellex\\ContextMenuHandlers\\Dzip");
	RegDeleteKey(HKEY_CLASSES_ROOT, "Folder\\shellex\\DragDropHandlers\\Dzip");
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", 0,
		KEY_ALL_ACCESS, &hk) == ERROR_SUCCESS)
	{	// damn them for not having a function to delete a value from a subkey
		RegDeleteValue(hk, dzGUID);
		RegCloseKey(hk);
	}
	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\SDA", 0,
		KEY_ALL_ACCESS, &hk) == ERROR_SUCCESS)
	{	// just in case I ever make more SDA software :)
		stupidpointer = 256;
		if (RegEnumKeyEx(hk, 0, temp1, &stupidpointer, NULL, NULL, NULL, NULL))
			RegDeleteKey(hk, "");
		RegCloseKey(hk);
	}
	GetShortPathName(temp2, temp1, 256);
	file = GetFileFromPath(temp1);

// delete DzipGui.txt
	strcpy(file, "DzipGui.txt");
	DeleteFile(temp1);

// try to delete DzipShlx.dll
	strcpy(file, "DzipShlx.dll");
	dlldeleted = 2 * DeleteFile(temp1);

// delete desktop shortcut
	SHGetMalloc(&m);
	SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOPDIRECTORY, &itlist);
	SHGetPathFromIDList(itlist, temp2);
	m->lpVtbl->Free(m, itlist);
	strcat(temp2, "\\Dzip.lnk");
	DeleteFile(temp2);

// remove start menu group
	SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAMS, &itlist);
	SHGetPathFromIDList(itlist, temp2);
	m->lpVtbl->Free(m, itlist);
	msg = temp2 + strlen(temp2);
	strcpy(msg, "\\Dzip\\Dzip.lnk");
	DeleteFile(temp2);
	strcpy(msg + 6, "Readme.lnk");
	DeleteFile(temp2);
	strcpy(msg + 6, "Uninstall Dzip.lnk");
	DeleteFile(temp2);
	msg[5] = 0;
	RemoveDirectory(temp2);
	m->lpVtbl->Release(m);

	if (GetVersion() & 0x80000000)
	{	// non windows NT; have to put entries in wininit.ini
		HANDLE h;
		char *buf, *buf2, *ptr;
		int i = GetWindowsDirectory(temp2, 256);
		if (temp2[i - 1] != '\\')
			temp2[i++] = '\\';
		strcpy(temp2 + i, "wininit.ini");
	// buf2 is what i'll insert into wininit.ini
		buf2 = Dzip_malloc(30 + 2 * strlen(temp1));
		i = sprintf(buf2, "\r\n[rename]\r\n");
		if (!dlldeleted)	// temp1 still has short path to dll
			i += sprintf(buf2 + i, "NUL=%s\r\n", temp1);
		strcpy(file, "DzipGui.exe");
		sprintf(buf2 + i, "NUL=%s\r\n", temp1);
	// see if it already exists
		h = CreateFile(temp2, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL,
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (h == INVALID_HANDLE_VALUE)
			goto done;	// can't open/create it for some reason
		i = GetFileSize(h, NULL);
		if (i)	// i'm hoping its 0
		{
			buf = Dzip_malloc(i + 1);
			ReadFile(h, buf, i, &stupidpointer, NULL);
			buf[i] = 0;
			for (ptr = buf;; ptr++)
			{
				ptr = strchr(ptr, '[');
				if (!ptr || !strnicmp(ptr + 1, "rename]\r\n", 9))
					break;
			}

			if (ptr) SetFilePointer(h, ptr - buf, NULL, FILE_BEGIN);
			else WriteFile(h, buf2, 2, &stupidpointer, NULL);
			WriteFile(h, buf2 + 2, strlen(buf2 + 2), &stupidpointer, NULL);
			if (ptr)	// write anything after any existing [rename] section
				WriteFile(h, ptr + 10, i - (ptr + 10 - buf), &stupidpointer, NULL);
			free(buf);
		}
		else
			WriteFile(h, buf2 + 2, strlen(buf2 + 2), &stupidpointer, NULL);
		CloseHandle(h);
		free(buf2);
		exedeleted = 1;
		if (!dlldeleted) dlldeleted = 1;
	}
	else	// its alot easier on NT
	{
		if (!dlldeleted)
			dlldeleted = MoveFileEx(temp1, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
		strcpy(file, "DzipGui.exe");
		exedeleted = MoveFileEx(temp1, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
		*file = 0;	// set the directory to be deleted too
		MoveFileEx(temp1, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
	}

done:
	if (!dlldeleted && !exedeleted)
		msg = "DzipGui.exe and DzipShlx.dll\ncould not be deleted.";
	else if (!exedeleted && dlldeleted == 2)
		msg = "DzipGui.exe could not be deleted";
	else if (exedeleted && dlldeleted == 2)
		msg = "DzipGui.exe will be deleted after a reboot";
	else
		msg = "DzipGui.exe and DzipShlx.dll\nwill be deleted after a reboot.";
	MessageBox(MainWnd, msg, "Dzip uninstall", MB_OK);
	exit(0);
}

// i ought to just remove this
void OptionsResetShellIntegration(HWND hDlg)
{
	int i, SuccessCount = 0;

	i = GetModuleFileName(hInstance, temp1, 256);	// get full path and exe name
/*	SuccessCount += WriteRegValue(HKEY_CLASSES_ROOT, ".dz", "", REG_SZ, "dzfile", 1, 0);
	SuccessCount += WriteRegValue(HKEY_CLASSES_ROOT, ".dz", "Content Type", REG_SZ, "application/x-dzip", 1, 0);
	SuccessCount += WriteRegValue(HKEY_CLASSES_ROOT, "dzfile", "", REG_SZ, "Dzip file", 1, 0);
	strcpy(temp1 + i, ",0");
	SuccessCount += WriteRegValue(HKEY_CLASSES_ROOT, "dzfile\\DefaultIcon", "",
		REG_SZ, temp1, 1, 0);
	strcpy(temp1 + i, " %1");
	SuccessCount += WriteRegValue(HKEY_CLASSES_ROOT, "dzfile\\shell\\open\\command", "",
		REG_SZ, temp1, 1, 0);*/
// all the stuff for the dll
	strcpy(strrchr(temp1, '\\') + 1, "DzipShlx.dll");
	SuccessCount += WriteRegValue(HKEY_CLASSES_ROOT, "*\\shellex\\ContextMenuHandlers\\Dzip", "", REG_SZ, dzGUID, 1, 0);
	sprintf(temp2, "CLSID\\%s", dzGUID);
	SuccessCount += WriteRegValue(HKEY_CLASSES_ROOT, temp2, "", REG_SZ, "Dzip Shell Extension", 1, 0);
	strcat(temp2, "\\InProcServer32");
	SuccessCount += WriteRegValue(HKEY_CLASSES_ROOT, temp2, "", REG_SZ, temp1, 1, 0);
	SuccessCount += WriteRegValue(HKEY_CLASSES_ROOT, temp2, "ThreadingModel", REG_SZ, "Apartment", 1, 0);
	SuccessCount += WriteRegValue(HKEY_CLASSES_ROOT, "Drive\\shellex\\DragDropHandlers\\Dzip", "", REG_SZ, dzGUID, 1, 0);
	SuccessCount += WriteRegValue(HKEY_CLASSES_ROOT, "Folder\\shellex\\ContextMenuHandlers\\Dzip", "", REG_SZ, dzGUID, 1, 0);
	SuccessCount += WriteRegValue(HKEY_CLASSES_ROOT, "Folder\\shellex\\DragDropHandlers\\Dzip", "", REG_SZ, dzGUID, 1, 0);
//	SuccessCount += WriteRegValue(HKEY_CLASSES_ROOT, "dzfile\\shellex\\DropHandler", "", REG_SZ, dzGUID, 1, 0);
	SuccessCount += WriteRegValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", dzGUID, REG_SZ, "Dzip Shell Extension", 1, 0);
// see if it worked
	if (SuccessCount == 8)
		MessageBox(hDlg, "Shell integration reset succesfully", "Dzip", MB_OK|MB_ICONINFORMATION);
	else
		error("%i registry writes failed since you probably don't\n"
			"have access to that part of the registry.\n"
			"Not all of Dzip's shell interaction will function correctly.",
			8 - SuccessCount);
}

int CALLBACK OptionsResetDlgFunc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INITDIALOG)
	{
		CenterDialog(hDlg);
		return 1;
	}
	if (msg == WM_COMMAND)
		if (LOWORD(wParam) == IDCANCEL)
			EndDialog (hDlg, 0);
		else if (LOWORD(wParam) == IDC_RESETSHLX)
			OptionsResetShellIntegration(hDlg);
		else
		{
			char *amp;
			GetWindowText((HWND)lParam, temp2, 256);
			amp = strchr(temp2, '&');
			strcpy(amp, amp + 1);
			if (YesNo("Are you sure?", temp2, hDlg, 0))
				switch (LOWORD(wParam))
				{
				case IDC_CLEARRMENU:
					RecentClear("file");
					lParam = FILEMENULENGTH;
					if (lvNumEntries) lParam += 2;
					while (DeleteMenu(Menu.file, lParam, MF_BYPOSITION));
					MenuReset(1);
					Recent.NumFiles = !!lvNumEntries;
					break;
				case IDC_CLEARREXTRACT:
					RecentClear("path");
					Recent.NumExtractPaths = 0;
					break;
				case IDC_CLEARRMOVE:
					RecentClear("move");
					Recent.NumMovePaths = 0;
					break;
				case IDC_RESETCOLUMNS:
				{
					int cols[7] = {0, 1, 2, 3, 4, 5, 6};
					LVSetColumnOrder(7 - !Options.TypeCol - !Options.PathCol, cols);
					LVSetColumnWidth(0, 106);
					LVSetColumnWidth(1, 108);
					LVSetColumnWidth(2, 72);
					LVSetColumnWidth(3, 38);
					LVSetColumnWidth(4, 72);
					if (Options.TypeCol)
						LVSetColumnWidth(5, 86);
					else
					{
						int i = 86;
						WriteRegValue(HKEY_CURRENT_USER, dzipreg, "lvtype", REG_DWORD, (char *)&i, 1, 4);
					}
					if (Options.PathCol)
						LVSetColumnWidth(5 + Options.TypeCol, 90);
					else
					{
						int i = 90;
						WriteRegValue(HKEY_CURRENT_USER, dzipreg, "lvpath", REG_DWORD, (char *)&i, 1, 4);
					}
					LVSaveState();
					InvalidateRgn(LView, NULL, 0);
				}
				}
		}
	return 0;
}

void OptionsReset(void)
{
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_RESET), MainWnd, OptionsResetDlgFunc);
}