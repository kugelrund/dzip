#include <windows.h>
#include <stdio.h>
#include "misc.h"
#include "recent.h"
#include "resource.h"
#include "thread.h"
#include <shlobj.h>

char *GetFileFromPath (char *);

static NUMBERFMT NumberFmt;
const char dzipreg[] = "Software\\SDA\\Dzip";
static char errmsg[512];
static HWND errorParentWnd;

char *WinErrMsg (DWORD err)
{
	static char *msg = NULL;
	if (msg)
		LocalFree(msg);
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char *)&msg, 0, NULL);
	return msg;
}

// go through all 'top level' windows and see if there
// are any dialog boxes that belong to Dzip
int CALLBACK EnumChildProc (HWND hwnd, LPARAM lParam)
{
	if (GetParent(hwnd) == MainWnd &&
		GetClassWord(hwnd, GCW_ATOM) == (WORD)WC_DIALOG)
	{
		errorParentWnd = hwnd;
		return 0;
	}
	return 1;
}

void errorBox(void)
{
	errorParentWnd = MainWnd;
	EnumWindows(EnumChildProc, 0);
	MessageBox(errorParentWnd, errmsg, "Dzip Error", MB_OK|MB_ICONSTOP);
}

void error (const char *msg, ...)
{
	va_list	args;
	va_start(args, msg);
	vsprintf(errmsg, msg, args);
	errorBox();
}

void errorWithSysError (int er, const char *msg, ...)
{
	int i;
	va_list	the_args;

	if (!er) er = GetLastError();
	va_start(the_args, msg);
	i = vsprintf(errmsg, msg, the_args);
	sprintf(errmsg + i, "\n%s", WinErrMsg(er));
	errorBox();
}

void *Dzip_malloc (UINT size)
{
	void *m;
	while (!(m = malloc(size)))
		error("Out of memory!\nFree some memory and hit OK");
	return m;		
}

void *Dzip_realloc (void *ptr, UINT size)
{
	void *m;
	while (!(m = realloc(ptr, size)))
		error("Out of memory!\nFree some memory and hit OK");
	return m;		
}

char *Dzip_strdup(const char *str)
{
	char *m;
	while (!(m = strdup(str)))
		error("Out of memory!\nFree some memory and hit OK");
	return m;		
}

void *Dzip_calloc (UINT size)
{
	return memset(Dzip_malloc(size), 0, size);
}

void CenterDialog (HWND hDlg)
{
	RECT r1, r2;
	GetWindowRect(hDlg, &r1);
	GetWindowRect(MainWnd, &r2);

	MoveWindow(hDlg, (r2.left + r2.right - r1.right + r1.left) / 2,
		(r2.top + r2.bottom - r1.bottom + r1.top) / 2, 
		r1.right - r1.left, r1.bottom - r1.top, 0);
}

int CALLBACK YesNoDlgFunc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INITDIALOG)
	{
		int i;
		struct {
			char *text;
			char *title;
		} *param = (void *)lParam;

		CenterDialog(hDlg);
	// figure out if it's a one or two line msg		
		i = !!strchr(param->text, '\n');
		SetWindowText(hDlg, param->title);
		SendDlgItemMessage(hDlg, IDS_YESNO1 + i, WM_SETTEXT, 0, (int)param->text);
		ShowWindow(GetDlgItem(hDlg, IDS_YESNO1 + !i), SW_HIDE);
		SendDlgItemMessage(hDlg, IDC_QICON, STM_SETICON, (int)LoadIcon(NULL, IDI_QUESTION), 0);
		return 1;
	}
	else if (msg == WM_COMMAND)
		EndDialog (hDlg, LOWORD(wParam));
	
	return 0;
}

int YesNo (const char *text, const char *title, HWND hWnd, int beep)
{	// i dont use a MessageBox because it will always beep
	// and i can't center it over the window
	if (beep) MessageBeep(MB_ICONQUESTION);
	return (IDYES == DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_YESNO), hWnd, YesNoDlgFunc, (int)&text));
}

void SetWindowTextf (HWND hWnd, const char *msg, ...)
{
	va_list	the_args;
	va_start(the_args, msg);
	vsprintf(temp2, msg, the_args);
	va_end(the_args);
	SetWindowText(hWnd, temp2);
}

int BrowseForFolder (HWND parent)
{
	IMalloc *m;
	ITEMIDLIST *itlist;
	BROWSEINFO bi;

	memset(&bi, 0, sizeof(bi));
	bi.hwndOwner = parent;
	bi.lpszTitle = "Select folder:";
	bi.ulFlags = BIF_RETURNONLYFSDIRS;

	itlist = SHBrowseForFolder(&bi);
	if (!itlist) return 0;

	SHGetPathFromIDList(itlist, temp1);
	SHGetMalloc(&m);
	m->lpVtbl->Free(m, itlist);
	m->lpVtbl->Release(m);	// stupid "shell allocator"
	return 1;
}

/* type values:
	0: filename check, allow drives,
	1: filename check, dont allow drives
	2: path check, allow drives
	3: path check, dont allow drives
	4: path or filename check, no drives, silence errors
*/
int CheckForValidFilename (char *name, int type)
{
	char *ptr;
	int len, allowdrives = (type == 0 || type == 2);

	len = strlen(name);
	if (!len)
		return 0;
	else if (len > 256)
	{
		if (type != 4)
			error("%s is too long", type < 2 ? "Filename" : "Path");
		return 0;
	}

	if (strpbrk(name, "\"*?|=<>")	// definite bad chars
		|| name[0] == ':' || (len > 2 && strchr(name + 2, ':'))
		|| name[1] == ':' && !allowdrives	// colon only in 2nd char
		)									// and only if allowing drives
	{
		if (type != 4)
			error("%s has invalid characters", type < 2 ? "Filename" : "Path");
		return 0;
	}

	// go change all forward slash to back slash
	for (ptr = name; *ptr; ptr++)
		if (*ptr == '/')
			*ptr = '\\';

	// make sure there's not two backslash in a row
	// except at the very start for a \\netdrive\path
	// also cant start with backslash if not allowing drives
	if (strstr(name + 1, "\\\\") || (name[0] == '\\' && !allowdrives))
	{
		error("Invalid path");
		return 0;
	}

	// a filename can't end in a backslash
	if (name[len - 1] == '\\' && type < 2)
	{
		error("Invalid filename");
		return 0;
	}

	return 1;
}

int FileExists (const char *fname)
{
	return (GetFileAttributes(fname) != -1);
}

void CreateDirectories (char *path)
{
	char *dir = path;

	while ((dir = strchr(dir, '\\')))
	{
		*dir = 0;
		CreateDirectory(path, NULL);
		*dir++ = '\\';
	}
}

HANDLE CreateNewFile (char *filename)
{
	HANDLE f = CreateFile(filename, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (f != INVALID_HANDLE_VALUE)
		return f;

	if (GetLastError() == ERROR_PATH_NOT_FOUND)
	{
		CreateDirectories(filename);
		f = CreateFile(filename, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,
			NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (f != INVALID_HANDLE_VALUE)
			return f;
	}

	errorWithSysError(0, "Could not create %s", filename);
	return INVALID_HANDLE_VALUE;
}

int CheckDirectory (HWND hDlg)
{
	DWORD er;
	int i = GetDlgItemText(hDlg, IDC_COMBO1, temp1, 258) - 1;
	if (temp1[i] != '\\')
		*(short *)&temp1[++i] = '\\';	// CreateDirectories() won't create the last part without a \ at end
	if (!CheckForValidFilename(temp1, 2))
		return 0;
	if (!SetCurrentDirectory(temp1))
	{
		er = GetLastError();
		// any error other than 'not found' dont ask to create
		if (er != ERROR_FILE_NOT_FOUND && er != ERROR_PATH_NOT_FOUND &&
			er != ERROR_BAD_NETPATH)
		{
			errorWithSysError(er, "Invalid directory");
			return 0;
		}

		if (!YesNo("Directory does not exist\nCreate it?", "Dzip Error", hDlg, 1))
			return 0;
		CreateDirectories(temp1);
		if (!SetCurrentDirectory(temp1))
		{
			er = GetLastError();
			if (er == ERROR_FILE_NOT_FOUND || er == ERROR_PATH_NOT_FOUND)
				er = ERROR_ACCESS_DENIED;
			errorWithSysError(er, "Could not create directory");
			return 0;
		}
	}
	// get rid of the end \ now unless there's a colon before it
	if (temp1[i - 1] != ':')
		temp1[i] = 0;
	return 1;
}

void SetDir (char *fp)	// has path+file so SetCurrentDirectory would fail
{
	char *p = GetFileFromPath(fp);
	char i = *p;
	*p = 0;
	SetCurrentDirectory(fp);
	*p = i;
}

int DateTimeToString (UINT date, int dateisftpointer, int numspaces, char *dest)
{
	FILETIME ft;
	SYSTEMTIME st;
	int written;
	int size = dest ? 256 : 0;

	if (dateisftpointer)
		FileTimeToLocalFileTime((FILETIME *)date, &ft);
	else
		DosDateTimeToFileTime(HIWORD(date + (1 << 21)), LOWORD(date), &ft);
	FileTimeToSystemTime(&ft, &st);
	written = GetDateFormat(LOCALE_USER_DEFAULT, 0, &st,
		NULL, dest, size) - 1;
	if (dest)
		memset(dest + written, ' ', numspaces);
	written += numspaces;
	written += GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st,
		NULL, dest + written, size);
	return written;
}

int NumberToString (UINT num, char *dest)
{
	char temp[12];

	sprintf(temp, "%u", num);
	return GetNumberFormat(0, 0, temp, &NumberFmt, dest, dest ? 32 : 0);
}

void SetNumberFormat(void)
{
	memset(&NumberFmt, 0, sizeof(NumberFmt));
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, temp1, 256);
	NumberFmt.Grouping = temp1[0] - '0';
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, temp1, 256);
	// DecimalSep has to be valid even though it's not used
	NumberFmt.lpThousandSep = NumberFmt.lpDecimalSep = Dzip_strdup(temp1);
}

// my functions to read/write to registry
// return 1 on success... none of that ERROR_SUCCESS crap
int ReadRegValue(HKEY basekey, const char *path, const char *name,
					char *data, int dsize)
{
	HKEY hKey;
	int i;

	i = RegOpenKey(basekey, path, &hKey);

	if (i != ERROR_SUCCESS)
		return 0;

	i = RegQueryValueEx(hKey, name, 0, &i, data, &dsize);
	RegCloseKey(hKey);

	return (i == ERROR_SUCCESS);
}

int WriteRegValue(HKEY basekey, const char *path, const char *name,
					int type, const char *data, int create, int len)
{
	HKEY hKey;
	int i;

	i = RegOpenKey(basekey, path, &hKey);
	if ((i != ERROR_SUCCESS) && create)
		i = RegCreateKeyEx(basekey, path, 0, NULL,
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &i);

	if (i != ERROR_SUCCESS)
		return 0;

	if (!len)
		len = strlen(data) + 1;
	i = RegSetValueEx(hKey, name, 0, type, data, len);
	RegCloseKey(hKey);
	return (i == ERROR_SUCCESS);
}

// returns 1 if the window should be maximized
int RestoreWindowPosition (HWND wnd, const char *regname)
{
	if (ReadRegValue(HKEY_CURRENT_USER, dzipreg, regname, temp2, 256))
	{
		int i, x, y;
		RECT r;
		HDC hdc = GetDC(NULL);
		x = GetDeviceCaps(hdc, HORZRES);
		y = GetDeviceCaps(hdc, VERTRES);
		ReleaseDC(NULL, hdc);
		sscanf(temp2, "%i,%i,%i,%i,%i", &i, &r.left, &r.top, &r.right, &r.bottom);
		if (r.left >= x || r.right < 0) return 0;
		if (r.top >= y || r.bottom < 0) return 0;
		MoveWindow(wnd, r.left, r.top, r.right - r.left, r.bottom - r.top, 1);
		return i;
	}
	return 0;
}

void SaveWindowPosition (HWND wnd, const char *regname)
{
	char str[30];
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);

	GetWindowPlacement(wnd, &wp);

	sprintf(str, "%i,%i,%i,%i,%i", (wp.flags & WPF_RESTORETOMAXIMIZED || wp.showCmd == SW_SHOWMAXIMIZED),
		wp.rcNormalPosition.left, wp.rcNormalPosition.top, wp.rcNormalPosition.right, wp.rcNormalPosition.bottom);
	WriteRegValue(HKEY_CURRENT_USER, dzipreg, regname, REG_SZ, str, 1, 0);
}