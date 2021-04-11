#include <stdio.h>
#include <windows.h>
#include "..\external\zlib\zlib.h"
#include "gui_export.h"
#include "gui_import.h"
#include "common.h"
#include "misc.h"
#include "options.h"
#include "resource.h"
#include <commctrl.h>

int ArchiveFile_Read (void *, uInt);
void ArchiveFile_Seek (uInt);
uInt ArchiveFile_Size (void);
void ArchiveFile_Truncate (void);
void ArchiveFile_Write (void *, uInt);
void Infile_Read (void *, uInt);
void Infile_Seek (uInt);
void Infile_Store (uInt);
void LVAddFileToListView (char *, uInt, uInt, uInt, uInt, int);
void LVBeginAdding (uInt);
void PBarGenericProgress (uInt, uInt);
void Outfile_Write (void *, uInt);
extern uInt crcval;

gui_import_t gi;


/* deflateInit and inflateInit are macros in newer zlib versions to allow
   checking version. Therefore we cant use them directly in gui_export_t
   but have to use wrappers instead. */
int deflateInitWrapped(z_streamp strm, int level)
{
	return deflateInit(strm, level);
}

int inflateInitWrapped(z_streamp strm)
{
	return inflateInit(strm);
}

const gui_export_t ge = {
	&AbortOp,
	&crcval,
	ArchiveFile_Read,
	ArchiveFile_Seek,
	ArchiveFile_Size,
	ArchiveFile_Truncate,
	ArchiveFile_Write,
	Dzip_malloc,
	Dzip_realloc,
	Dzip_calloc,
	Dzip_strdup,
	GuiProgressMsg,
	Infile_Read,
	Infile_Seek,
	Infile_Store,
	LVAddFileToListView,
	LVBeginAdding,
	Outfile_Write,
	error,
	YesNo,
	deflate,
	deflateEnd,
	deflateInitWrapped,
	inflate,
	inflateEnd,
	inflateInitWrapped,
	make_crc
};

HINSTANCE hCurrentDLL;
const char *sCurrentDLL;

ext_dll_t *ext_dll_list;
char *filterList;
int ext_dll_size;

void ChangeExtensionRegistrations (HWND lv)
{
	int i, j, l;
	ext_dll_t *edt = ext_dll_list;
	char key[280], reg[280], ext[64];
	for (i = 0; i < ext_dll_size; i++, edt++)
	{
		j = sprintf(key, "Dzip_ext_%s", edt->ext);
		if (!lv)
		{	// called from Uninstall
			sprintf(ext, "dzip_%s.dll", edt->ext);
			DeleteFile(ext);
		}
		if (!lv && edt->isDzips ||
			(edt->isDzips != (ListView_GetItemState(lv, i, -1) >> 13)))
		{	// this item was changed
			sprintf(ext, ".%s", edt->ext);
			if (edt->isDzips)	// change from registered to Dzip to not anymore
			{
				if (!ReadRegValue(HKEY_CURRENT_USER, dzipreg, key + 5, reg, 120))
					reg[0] = 0;
				if (reg[0])
					WriteRegValue(HKEY_CLASSES_ROOT, ext, NULL, REG_SZ, reg, 0, 0);
				else
					RegDeleteKey(HKEY_CLASSES_ROOT, ext);
				RegDeleteKey(HKEY_CLASSES_ROOT, key);
			}
			else	// change from registered to something else to Dzip
			{
				if (ReadRegValue(HKEY_CLASSES_ROOT, ext, NULL, reg, 120)
					&& reg[0])	// save what it used to be registered as
					WriteRegValue(HKEY_CURRENT_USER, dzipreg, key + 5, REG_SZ, reg, 0, 0);
				WriteRegValue(HKEY_CLASSES_ROOT, ext, NULL, REG_SZ, key, 1, 0);
				WriteRegValue(HKEY_CLASSES_ROOT, key, NULL, REG_SZ, "Dzip file", 1, 0);
				strcpy(key + j, "\\DefaultIcon");
				l = GetModuleFileName(hInstance, reg, 260);
				strcpy(reg + l, ",0");
				WriteRegValue(HKEY_CLASSES_ROOT, key, NULL, REG_SZ, reg, 1, 0);
				strcpy(key + j, "\\shell\\open\\command");
				strcpy(reg + l, " %1");
				WriteRegValue(HKEY_CLASSES_ROOT, key, NULL, REG_SZ, reg, 1, 0);
				strcpy(key + j, "\\shellex\\DropHandler");
				WriteRegValue(HKEY_CLASSES_ROOT, key, NULL, REG_SZ, dzGUID, 1, 0);
			}
		}
	}
}

void PopulateFileTypes (HWND lv)
{
	int i;
	ext_dll_t *edt = ext_dll_list;
	LVITEM lvi;
	char text[200], key[200], reg[120];
	lvi.iItem = 0;
	lvi.iSubItem = 0;
	lvi.mask = LVIF_TEXT;
	lvi.pszText = text;

	for (i = 0; i < ext_dll_size; i++, edt++)
	{
		sprintf(text, ".%s", edt->ext);
		sprintf(key, "Dzip_ext_%s", edt->ext);
		lvi.state = 1 << 12;
		edt->isDzips = 0;
		if (!ReadRegValue(HKEY_CLASSES_ROOT, text, NULL, reg, 120))
			sprintf(text, "%s - Currently not registered", edt->ext);
		else if (!strcmp(reg, key))
		{	// currently registered to Dzip
			lvi.state <<= 1;
			edt->isDzips++;
			if (ReadRegValue(HKEY_CURRENT_USER, dzipreg, key + 5, reg, 120))
			{
				sprintf(key, "%s\\shell\\open\\command", reg);
				if (!ReadRegValue(HKEY_CLASSES_ROOT, key, NULL, reg, 120))
					goto notreg;
				*FileExtension(reg) = 0;
				sprintf(text, "%s - Previously registered to %s", edt->ext, GetFileFromPath(reg));
			}
			else notreg:
				sprintf(text, "%s - Previously not registered", edt->ext);
		}
		else
		{
			sprintf(key, "%s\\shell\\open\\command", reg);
			if (ReadRegValue(HKEY_CLASSES_ROOT, key, NULL, reg, 120))
			{
				*FileExtension(reg) = 0;
				sprintf(text, "%s - Currently registered to %s", edt->ext, GetFileFromPath(reg));
			}
			else
				sprintf(text, "%s - Currently not registered", edt->ext);
		}
		ListView_InsertItem(lv, &lvi);
		ListView_SetItemState(lv, i, lvi.state, -1);
	}
}

int CALLBACK ChooseExtDlgFunc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int i;
	HWND combo = GetDlgItem(hDlg, IDC_COMBO1);
	switch (msg)
	{
	case WM_INITDIALOG:
		for (i = 0; i < ext_dll_size; i++)
			SendMessage(combo, CB_ADDSTRING, 0, (int)ext_dll_list[i].ext);
		SendMessage(combo, CB_SETCURSEL, 0, 0);
		return 1;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
			EndDialog(hDlg, ext_dll_size);
		else if (LOWORD(wParam) == IDOK)
			EndDialog(hDlg, SendMessage(combo, CB_GETCURSEL, 0, 0));
		return 1;
	}
	return 0;
}

int IsAnArchive (char *filename)
{
	int i;
	filename = FileExtension(filename);
	if (*filename) filename++;
	for (i = 0; i < ext_dll_size; i++)
		if (!stricmp(filename, ext_dll_list[i].ext))
			return 1;
	return 0;
}
	
int LoadDLL (char *filename)
{
	int i;
	gui_import_t *(*GetDllFuncs)(const gui_export_t *);

	filename = FileExtension(filename);
	if (*filename) filename++;

	for (i = 0; i < ext_dll_size; i++)
		if (!stricmp(filename, ext_dll_list[i].ext))
			break;

	if (i == ext_dll_size)
	{
		char err[400];
		sprintf(err, "There is no Plugin Dll that works with %s files\nDo you want to try to open this file as a certain extension?", filename);
		if (IDNO == MessageBox(MainWnd, err, "Can't open file", MB_YESNO|MB_ICONQUESTION))
			return 0;
		i = DialogBox(hInstance, MAKEINTRESOURCE(IDD_CHOOSEEXT), MainWnd, ChooseExtDlgFunc);
		if (i == ext_dll_size)	// hit cancel
			return 0;
	}

	if (hCurrentDLL)
		if (strcmp(ext_dll_list[i].dllname, sCurrentDLL))
			FreeLibrary(hCurrentDLL);
		else
			return 1;	// it's already loaded

	sCurrentDLL = ext_dll_list[i].dllname;
#ifdef _DEBUG
	{
	char dll[260];
	GetModuleFileName(hInstance, dll, 260);
	sprintf(GetFileFromPath(dll), "..\\%s\\debug\\%s.dll",
		sCurrentDLL, sCurrentDLL);
	hCurrentDLL = LoadLibrary(dll);
	}
#else
	hCurrentDLL = LoadLibrary(sCurrentDLL);
#endif

	if (!hCurrentDLL)
		goto failed;
	GetDllFuncs = (void *)GetProcAddress(hCurrentDLL, "GetDllFuncs");
	if (!GetDllFuncs)
		goto failed;
	gi = *GetDllFuncs(&ge);
	return 1;

failed:
	error("Unable to load the DLL used with %s files", filename);
	return 0;
}

void UnloadDLL (void)
{
	FreeLibrary(hCurrentDLL);
	hCurrentDLL = NULL;
}

int qsort_ext_dll (const ext_dll_t *e1, const ext_dll_t *e2)
{
	return strcmp(e1->ext, e2->ext);
}

int ScanForDLLs (void)
{
	int dirofs, extlength = 0;
	WIN32_FIND_DATA fd;
	HANDLE ffh;
	gui_import_t *(*GetDllFuncs)(const gui_export_t *);

	GetModuleFileName(hInstance, temp2, 260);
	dirofs = GetFileFromPath(temp2) - temp2;

#ifdef _DEBUG
	dirofs -= 6;
	strcpy(temp2 + dirofs, "dzip_*");
#else
	strcpy(temp2 + dirofs, "dzip_*.dll");
#endif

	ext_dll_size = 0;
	ffh = FindFirstFile(temp2, &fd);
	if (ffh != INVALID_HANDLE_VALUE)
	{
		do {
	#ifdef _DEBUG
			sprintf(temp2 + dirofs, "%s\\debug\\%s.dll", fd.cFileName, fd.cFileName);
	#else
			strcpy(temp2 + dirofs, fd.cFileName);
	#endif
			hCurrentDLL = LoadLibrary(temp2);
			if (!hCurrentDLL)
				continue;
			GetDllFuncs = (void *)GetProcAddress(hCurrentDLL, "GetDllFuncs");
			if (!GetDllFuncs)
				continue;
			gi = *GetDllFuncs(&ge);
			if (gi.version != 1)
				continue;
			ext_dll_list = Dzip_realloc(ext_dll_list, sizeof(ext_dll_t) * ++ext_dll_size);
			ext_dll_list[ext_dll_size - 1].dllname = Dzip_strdup(fd.cFileName);
			ext_dll_list[ext_dll_size - 1].ext = Dzip_strdup(gi.extlist);	// this only works while the dlls only support one extension each
			extlength += strlen(gi.extlist);
			FreeLibrary(hCurrentDLL);
		} while (FindNextFile(ffh, &fd));
		FindClose(ffh);
	}
	if (!ext_dll_size)
	{
		error("No plugin Dll files found!\nDzipGui is pretty worthless without any!");
		return 0;
	}
	hCurrentDLL = NULL;

	// sort the ext_dll_list by extension
	qsort(ext_dll_list, ext_dll_size, sizeof(ext_dll_t), qsort_ext_dll);
	filterList = Dzip_malloc(extlength * 3 + ext_dll_size * 13 + 35);
	strcpy(filterList, "All supported files\0");
	dirofs = 20;
	for (extlength = 0; extlength < ext_dll_size; extlength++)
		dirofs += sprintf(filterList + dirofs, "*.%s;", ext_dll_list[extlength].ext);

	filterList[dirofs - 1] = 0;
	for (extlength = 0; extlength < ext_dll_size; extlength++)
		dirofs += 1 + sprintf(filterList + dirofs, "%s Files%c*.%s", ext_dll_list[extlength].ext, 0, ext_dll_list[extlength].ext);

	dirofs += 1 + sprintf(filterList + dirofs, "All Files%c*.*", 0);
	filterList[dirofs] = 0;
	return 1;
}