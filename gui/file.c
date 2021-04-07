#include <stdio.h>
#include <windows.h>
#include "gui_import.h"
#include "common.h"
#include "lvhelper.h"
#include "menu.h"
#include "misc.h"
#include "sbar.h"
#include "recent.h"
#include "resource.h"
#include <shlobj.h>

extern int lvNumEntries;
int AddGetFilenames(void);
void LVFinishedAdding(void);

static HANDLE CurrentFile;

int ArchiveFile_Read (void *dest, UINT num)
{
	ULONG read;
	if (!ReadFile(CurrentFile, dest, num, &read, NULL))
	{
		errorWithSysError(0, "Error reading from %s", 
			GetFileFromPath(Recent.FileList[0]));
		AbortOp = 1;
	}
	return read;
}

void ArchiveFile_Write (void *dest, UINT num)
{
	ULONG wrote;
	WriteFile(CurrentFile, dest, num, &wrote, NULL);
	if (num != wrote)
	{
		errorWithSysError(0, "Error writing to %s",
			GetFileFromPath(Recent.FileList[0]));
		AbortOp = 1;
	}
}

int ArchiveFile_Size(void)
{
	return GetFileSize(CurrentFile, NULL);
}

void ArchiveFile_Seek (UINT dest)
{
	int hi = 0;	// so dest isn't treated as signed
	SetFilePointer(CurrentFile, dest, &hi, FILE_BEGIN);
}

void ArchiveFile_Truncate(void)
{
	SetEndOfFile(CurrentFile);
}

void ReopenFile (int AlreadyClosed)
{
	if (!AlreadyClosed)	// exception under win2k debugger w/o this
		CloseHandle(CurrentFile);
	SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, Recent.FileList[0], 0);
	CurrentFile = CreateFile(Recent.FileList[0], GENERIC_READ | (!ReadOnly * GENERIC_WRITE),
		FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
}

int OpenArchive (char *fname, int NoReadOnly)
{
	char *fonly;

	ReadOnly = 0;
	CurrentFile = CreateFile(fname, GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (CurrentFile == INVALID_HANDLE_VALUE)
	{
		DWORD err = GetLastError();
		if (err == ERROR_ACCESS_DENIED || err == ERROR_SHARING_VIOLATION)
		{
			CurrentFile = CreateFile(fname, GENERIC_READ,
				FILE_SHARE_READ|FILE_SHARE_WRITE,
				NULL, OPEN_EXISTING, 0, NULL);
			ReadOnly = 1;
		}
		if (CurrentFile == INVALID_HANDLE_VALUE)
		{
			errorWithSysError(0, "Could not open %s", fname);
			return 0;
		}
		if (NoReadOnly && ReadOnly)
		{
			error("Can't add files to %s; it is read-only", GetFileFromPath(fname));
			CloseHandle(CurrentFile);
			return 0;
		}
	}

	fonly = GetFileFromPath(fname);
	if (!LoadDLL(fonly))
	{
		CloseHandle(CurrentFile);
		return 0;
	}
	gi.OpenFile(fonly);
	if (!lvNumEntries)
	{
		CloseHandle(CurrentFile);
		return 0;
	}

	LVFinishedAdding();
	MenuUpdateRFL(fname);
	SetWindowTextf(MainWnd, "Dzip - %s", fonly);
	UpdateSBar();

	MenuActivateCommands();
	LVSetItemState(0, LVIS_FOCUSED);
	return 1;
}

void CloseArchive(void)
{
	ReadOnly = 0;

	LVDeleteAllItems();
	gi.CloseFile();
	CloseHandle(CurrentFile);
	MenuActivateCommands();	

	SetWindowText(MainWnd, "Dzip");
	UpdateSBar();

	// delete all temp files made while this archive was open
	if (tempfiles)
	{
		tempfiles_t *oldtf;
		int i = GetTempPath(256, temp2);
		do
		{
			strcpy(temp2 + i, tempfiles->name);
			free(tempfiles->name);
			DeleteFile(temp2);
			oldtf = tempfiles;
			tempfiles = tempfiles->next;
			free(oldtf);
		} while (tempfiles);
	}
}

// atempts to create 'fname'
// if successful will close the current file
int NewArchive (char *fname)
{
	HANDLE NewFile;
	NewFile = CreateNewFile(fname);	// prints error if doesn't work
	if (NewFile == INVALID_HANDLE_VALUE) return 0;
	if (lvNumEntries) CloseArchive();
	if (!LoadDLL(fname))
	{
		CloseHandle(NewFile);
		DeleteFile(fname);
		return 0;
	}
	CurrentFile = NewFile;
	SetWindowTextf(MainWnd, "Dzip - %s", GetFileFromPath(fname));
	return 1;
}

UINT CALLBACK FileNewOFNHookProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NOTIFY && ((NMHDR *)lParam)->code == CDN_INITDONE)
	{
		HWND p = GetParent(hDlg);
		CommDlg_OpenSave_SetControlText(p, stc4, "Create &in:");
		CommDlg_OpenSave_SetControlText(p, stc2, "Files of &type:");
		CommDlg_OpenSave_SetControlText(p, IDOK, "OK");
	}
	return 0;
}

int GetNewFileName (int addingfiles)
{
	char *ext, title[32];
	OPENFILENAME ofn;

	strcpy(title, "New file");

	memset(&ofn, 0, 76);
	ofn.lStructSize = 76;
	ofn.hwndOwner = MainWnd;
	ofn.lpstrFilter = filterList;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = temp1;
	temp1[0] = 0;
	ofn.nMaxFile = 256;
	ofn.lpstrInitialDir = temp2;
	GetCurrentDirectory(256, temp2);
	ofn.lpstrTitle = title;
	ofn.Flags = OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|OFN_HIDEREADONLY|OFN_EXPLORER|OFN_ENABLEHOOK;
	ofn.lpfnHook = FileNewOFNHookProc;

	if (addingfiles)
	{
		sprintf(title + 8, " - adding %i files", addingfiles);
		if (addingfiles == 1)
			title[24] = 0;
	}

	if (!GetSaveFileName (&ofn))
		return 0;

	ext = FileExtension(temp1);
	if (!*ext)
		if (ofn.nFilterIndex == 1)	// filter was 'All supported files'
			strcat(temp1, ".dz");	// change this to be 'default extension'
		else
			sprintf(temp1 + strlen(temp1), ".%s", ext_dll_list[ofn.nFilterIndex - 2].ext);

	if (FileExists(temp1))
	{
		sprintf(temp2, "%s already exists; open it?", temp1);
		// dont use YesNo() here because the size varies
		if (IDYES == MessageBox(MainWnd, temp2, "Dzip", MB_YESNO|MB_ICONQUESTION))
			OpenArchive(temp1, addingfiles);
		return lvNumEntries;
	}
	return NewArchive(temp1);
}

void FileNew(void)
{	// if they chose an already existing file in the New box
	// and then opened it, dont open the Add files box.
	if (GetNewFileName(0) && !lvNumEntries)
		if (!AddGetFilenames())
		{	// they cancelled the add box
			CloseHandle(CurrentFile);
			DeleteFile(temp1);	// bad to assume temp1 has not changed
			SetWindowText(MainWnd, "Dzip");
		}
}

void FileOpen(void)
{
	OPENFILENAME ofn;
	
	memset(&ofn, 0, 76);
	ofn.lStructSize = 76;
	ofn.hwndOwner = MainWnd;
	ofn.lpstrFilter = filterList;
	ofn.nFilterIndex++;
	ofn.lpstrFile = temp1;
	temp1[0] = 0;
	ofn.nMaxFile = 256;
	ofn.lpstrInitialDir = temp2;
	GetCurrentDirectory(256, temp2);
	ofn.Flags = OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;
//	ofn.lpstrDefExt = "dz";
	ofn.lpfnHook = FileNewOFNHookProc;

	if (GetOpenFileName(&ofn))
	{
		if (lvNumEntries)
			CloseArchive();

		OpenArchive(temp1, 0);
	}
}

void FileClose(void)
{
	CloseArchive();
	UnloadDLL();
}

int CALLBACK FileMoveDlgFunc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		int i;
		Recent.NumMovePaths = RecentRead(&Recent.MovePaths, "move", 10);
		for (i = 0; i < Recent.NumMovePaths; i++)
			SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (int)Recent.MovePaths[i]);
		if (Recent.NumMovePaths)
			SetDlgItemText(hDlg, IDC_COMBO1, Recent.MovePaths[0]);
		sprintf(temp1, "Move %s", FileExtension(Recent.FileList[0]) + 1);
		SetWindowText(hDlg, temp1);
		return 1;
	}
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case ID_BROWSE:
			if (BrowseForFolder(hDlg))
				SetDlgItemText(hDlg, IDC_COMBO1, temp1);
			break;
		case IDOK:
			if (!CheckDirectory(hDlg))
				break;
			RecentAddString(&Recent.MovePaths, &Recent.NumMovePaths, temp1);
			RecentWrite(Recent.MovePaths, "move", Recent.NumMovePaths);
			EndDialog (hDlg, 1);
			break;
		case IDCANCEL:
			EndDialog (hDlg, 0);
		}
		return 1;
	}
	return 0;
}

void FileMove(void)
{
	if (!DialogBox(hInstance, MAKEINTRESOURCE(IDD_MOVE), MainWnd, FileMoveDlgFunc))
		return;

	GetFullPathName(GetFileFromPath(Recent.FileList[0]), 256, temp2, NULL);

	CloseHandle(CurrentFile);
	if (MoveFile(Recent.FileList[0], temp2))
		strcpy(Recent.FileList[0], temp2);
	else
		errorWithSysError(0, "Move failed");
	
	ReopenFile(1);
	MenuUpdateRFL(NULL);
}

int CALLBACK FileRenameDlgFunc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hDlg, IDC_EDIT1, GetFileFromPath(Recent.FileList[0]));
		sprintf(temp1, "Rename %s", FileExtension(Recent.FileList[0]) + 1);
		SetWindowText(hDlg, temp1);
		return 1;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			GetDlgItemText(hDlg, IDC_EDIT1, temp1, 258);
			if (CheckForValidFilename(temp1, 1))
				EndDialog (hDlg, 1);
			break;
		case IDCANCEL:
			EndDialog (hDlg, 0);
		}
		return 1;
	}
	return 0;
}

void FileRename(void)
{
	int i;
	char *oldext, *newext;
	if (!DialogBox(hInstance, MAKEINTRESOURCE(IDD_RENAME), MainWnd, FileRenameDlgFunc))
		return;

	if (strchr(temp1, '\\'))
	{
		error("New name can not contain a path\nUse File->Move to move file");
		return;
	}
	i = GetFileFromPath(Recent.FileList[0]) - Recent.FileList[0];
	oldext = FileExtension(Recent.FileList[0]);
	newext = FileExtension(temp1);
	sprintf(temp2, "%.*s%s", i, Recent.FileList[0], temp1);
	if (stricmp(oldext, newext))
		strcat(temp2, oldext);

	CloseHandle(CurrentFile);
	if (MoveFile(Recent.FileList[0], temp2))
		strcpy(Recent.FileList[0], temp2);
	else
		errorWithSysError(0, "Rename failed");

	ReopenFile(1);
	MenuUpdateRFL(NULL);
	UpdateSBar();

	SetWindowTextf(MainWnd, "Dzip - %s", GetFileFromPath(Recent.FileList[0]));
}