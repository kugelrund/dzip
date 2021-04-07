#include <stdio.h>
#include <windows.h>
#include "common.h"
#include "listview.h"
#include "extract.h"
#include "lvhelper.h"
#include "misc.h"
#include "resource.h"
#include "thread.h"
#include <richedit.h>

static struct {
	HWND hEdit;
	WNDPROC EditProc;
	HFONT font;
	HICON hicon;
	UINT filesize;
	int type;
	char temppath[256];
} View;

long CALLBACK ViewEditSubclassProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC proc = View.EditProc;
	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE || wParam == VK_RETURN)
			return SendMessage(GetParent(hWnd), WM_COMMAND, IDOK, 0);
		break;
	case WM_CHAR:
		if (wParam == 1)	// ctrl+A
			return CallWindowProc(proc, hWnd, EM_SETSEL, 0, -1);
		else if (wParam == 3)	// ctrl+C
			return CallWindowProc(proc, hWnd, WM_COPY, 0, 0);
		return 0;	// prevent beeping on other chars
	case WM_RBUTTONDOWN:
	case WM_CONTEXTMENU:
	{	// wm_contextmenu isn't sent by right mouse click for some reason, only by menu key
		int i;
		HMENU em;
		RECT r;
		CHARRANGE cr;

		SetFocus(hWnd);
		if (lParam == -1)
		{	// hit menu key
			POINT pt;
			GetCaretPos(&pt);
			lParam = pt.x + (pt.y << 16);
		}
		GetWindowRect(hWnd, &r);
		lParam += r.left + (r.top << 16);

		CallWindowProc(proc, hWnd, EM_EXGETSEL, 0, (int)&cr);
		i = ((cr.cpMin == cr.cpMax) ? MF_GRAYED : 0);
		em = CreatePopupMenu();
		AppendMenu(em, i, 1, "&Copy");
		AppendMenu(em, 0, 2, "Select &All");
		i = TrackPopupMenuEx(em, TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RIGHTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD,
			LOWORD(lParam), HIWORD(lParam), hWnd, NULL);
		if (i == 1)
			CallWindowProc(proc, hWnd, WM_COPY, 0, 0);
		else if (i == 2)
			CallWindowProc(proc, hWnd, EM_SETSEL, 0, -1);
		DestroyMenu(em);
		return 0;
	}
	}
	return CallWindowProc(proc, hWnd, msg, wParam, lParam);
}

int CALLBACK ViewerDlgFunc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		char *buf, *filename;
		UINT i;
		tempfiles_t *oldtf;
		SHFILEINFO shfi;
		HWND hEdit;
		HANDLE f;

		filename = tempfiles->name;
		oldtf = tempfiles;
		tempfiles = tempfiles->next;
		free(oldtf);

		strcat(View.temppath, filename);
		f = CreateFile(View.temppath, GENERIC_READ, FILE_SHARE_READ,
			NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN|FILE_FLAG_DELETE_ON_CLOSE, NULL);
		buf = Dzip_malloc(View.filesize + 1);
		ReadFile(f, buf, View.filesize, &i, NULL);
		if (i != View.filesize)
			error("File read error");
		buf[i] = 0;
		CloseHandle(f);

		hEdit = View.hEdit = GetDlgItem(hDlg, IDC_EDIT1);
		SetFocus(hEdit);
		SendMessage(hEdit, WM_SETTEXT, 0, (int)buf);
		free(buf);
		View.font = CreateFont(15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Courier New");
		SendMessage(hEdit, WM_SETFONT, (int)View.font, 0);

		SetWindowTextf(hDlg, "View - %s", filename);
		SHGetFileInfo(filename, 0, &shfi, sizeof(shfi), SHGFI_ICON|SHGFI_SMALLICON|SHGFI_USEFILEATTRIBUTES);
		SendMessage(hDlg, WM_SETICON, ICON_SMALL, (int)shfi.hIcon);
		View.hicon = shfi.hIcon;
		free(filename);

		if (RestoreWindowPosition(hDlg, "vwindow"))
			ShowWindow(hDlg, SW_SHOWMAXIMIZED);
		View.EditProc = (WNDPROC)SetWindowLong(hEdit, GWL_WNDPROC, (long)ViewEditSubclassProc);
	}	// fallthrough!
	case WM_SIZE:
	{
		RECT r1, r2;
		GetClientRect(hDlg, &r1);
		MoveWindow(View.hEdit, 3, 4, r1.right - 6, r1.bottom - 40, 1);
		GetClientRect(GetDlgItem(hDlg, IDOK), &r2);
		MoveWindow(GetDlgItem(hDlg, IDOK), (r1.right - r2.right) / 2, r1.bottom - r2.bottom - 5, r2.right, r2.bottom, 1);
		return 0;
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			SaveWindowPosition(hDlg, "vwindow");
			DeleteObject(View.font);
			EndDialog(hDlg, 0);
			DestroyIcon(View.hicon);
		}
		return 1;
	}
	return 0;
}

int ViewGetExeName (char *name)
{
	char MSDev = 0;
	name = FileExtension(name);
	if (!ReadRegValue(HKEY_CLASSES_ROOT, name, "", temp1, 256))
		return 0;
	strcat(temp1, "\\shell\\open\\command");
	if (!ReadRegValue(HKEY_CLASSES_ROOT, temp1, "", temp2, 256))
	{	// hacky way to deal with msdev files
		strcpy(temp1 + strlen(temp1) - 12, "&Open with MSDev\\command");
		if (!ReadRegValue(HKEY_CLASSES_ROOT, temp1, "", temp2, 256))
			return 0;
		MSDev = 1;
	}
	name = temp2;
	if (*name == '"')
	{
		name = strchr(name + 1, '"');
		*name++ = 0;
	}
	name = strchr(name, ' ');
	if (name) *name = 0;

//	if (!strcmp(temp2, "\"%1"))
//		return 0;	strcmp sucks for small strings
	if (*(int *)temp2 == '1%"')
		return 0;
	return 1 + MSDev;
}

int CALLBACK ViewDlgFunc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		if (ViewGetExeName((char *)lParam))
		{
			sprintf(temp1, "&Associated program: %s", GetFileFromPath(temp2));
			*FileExtension(temp1) = 0;
			SetDlgItemText(hDlg, IDC_RADIO1, temp1);
		}
		else
		{
			SetDlgItemText(hDlg, IDC_RADIO1, "&Associated program:");
			EnableWindow(GetDlgItem(hDlg, IDC_RADIO1), 0);
		}
		CheckDlgButton(hDlg, IDC_RADIO2, 1);
		return 1;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
			EndDialog(hDlg, IsDlgButtonChecked(hDlg, IDC_RADIO1));
		else if (LOWORD(wParam) == IDCANCEL)
			EndDialog(hDlg, -1);
		return 1;
	}
	return 0;
}

void ViewFile (int type)
{
	lventry_t *lve = lventries + focus;
	if (lve->status.folder)
		return;

	if (!type)
	{	// called from view menu, let user pick, otherwise just use associated program
		type = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_VIEW), MainWnd, ViewDlgFunc, (int)lve->filename);
		if (type == -1) return;
	}

	if (type)
		if (!(type = ViewGetExeName(lve->filename)))
		{
			error("No associated program\n");
			return;
		}	// now type is 2 if it's an msdev file

	ExtractSetup(2, 0, NULL);
	Extract.NoPaths = 1;

	View.filesize = lve->realsize;
	View.type = type;

	GetTempPath(256, View.temppath);
	SetCurrentDirectory(View.temppath);

	RunThread(ExtractThread, THREAD_VIEW);

	if (Extract.numSuccessfull)
		if (!View.type)
		{
			HINSTANCE re = LoadLibrary("riched32.dll");
			DialogBox(hInstance, MAKEINTRESOURCE(IDD_VIEWER), MainWnd, ViewerDlgFunc);
			FreeLibrary(re);
		}
		else
			ShellExecute(MainWnd, View.type == 1 ? "open" : "&Open with MSDev",
				tempfiles->name, NULL, View.temppath, SW_SHOW);
}

void ActionsView(void)
{
	ViewFile(0);
}