#include <stdio.h>
#include <windows.h>
#include "gui_import.h"
#include "common.h"
#include "file.h"
#include "lvhelper.h"
#include "listview.h"
#include "menu.h"
#include "misc.h"
#include "options.h"
#include "recent.h"
#include "sbar.h"
#include "thread.h"
#include <commctrl.h>

static struct {
	OPENFILENAME *ofn;
	WNDPROC dproc;
	HWND hDlg;
	HANDLE handle;
	UINT NumNewFiles;
	struct AddFile {
		char *name;
		UINT size;
		FILETIME ft;
	} *file;
	char *filestring, *curfile;
	char olddir[256];
} Add;

void ArchiveFile_Write (void *, int);

void Infile_Read (void *buf, UINT num)
{
	UINT read;
	ReadFile(Add.handle, buf, num, &read, NULL);
	if (read != num)
	{
		errorWithSysError(0, "Error reading from %s", Add.curfile);
		AbortOp = 3;
	}
	PBarAdvance(num);
	make_crc(buf, num);	// the dlls will have to deal with the
}						// crc always being calculated

void Infile_Seek (UINT dest)
{
	int hi = 0;
	SetFilePointer(Add.handle, dest, &hi, FILE_BEGIN);
}

void Infile_Store (UINT size)
{
	void *buf = Dzip_malloc(32768);
	int s;

	s = SetFilePointer(Add.handle, 0, NULL, FILE_CURRENT);
	Infile_Seek(s - size);
	while (size && !AbortOp)
	{
		s = (size > 32768) ? 32768 : size;
		Infile_Read(buf, s);
		ArchiveFile_Write(buf, s);
		size -= s;
	}
	free(buf);
}

void MakeLowercase (char *in)
{
	char out[256];

	if (!Options.LCFilenames)
		return;

	LCMapString(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, in, -1, out, 256);
	strcpy(in, out);
}

int AddFilesFromDirectory (char *dir, int files)
{
	HANDLE ffh;
	WIN32_FIND_DATA fd;
	struct AddFile *AF;
	char srch[256];
	int dirlength = strlen(dir);

// first add this folder to the list
	MakeLowercase(dir);
	AF = Add.file + Add.NumNewFiles++;
	AF->ft.dwHighDateTime = 0xFFFFFFFF;	// used to check for folder
	AF->name = Dzip_malloc(dirlength + 2);
	sprintf(AF->name, "%s\\", dir);

	sprintf(srch, "%s\\*", dir);
	ffh = FindFirstFile(srch, &fd);
	// this will fail if the user does not have
	// permission to browse the directory
	if (ffh == INVALID_HANDLE_VALUE)
		return files;

	do
		files++;
	while (FindNextFile(ffh, &fd));
	FindClose(ffh);
	files -= 2;
	Add.file = Dzip_realloc(Add.file, files * 16);

	ffh = FindFirstFile(srch, &fd);
	do
	{
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{	// dont add "." or ".."  the normal strcmp solution used
			// 133 bytes of code compared to just 32 for this
			if (*(short *)fd.cFileName == '.')
				continue;
			if ('..' == (0xffffff & *(int *)fd.cFileName))
				continue;

			sprintf(srch, "%s\\%s", dir, fd.cFileName);
			files = AddFilesFromDirectory(srch, files + 1);
			if (AbortOp) return 0;
		}
		else if (fd.nFileSizeHigh)
			error("%s is at least 4GB and\ncan not currently be compressed with Dzip", fd.cFileName);
		else
		{
			MakeLowercase(fd.cFileName);
			AF = Add.file + Add.NumNewFiles++;
			AF->name = Dzip_malloc(dirlength + 2 + strlen(fd.cFileName));
			AF->ft = fd.ftLastWriteTime;
			AF->size = fd.nFileSizeLow;
			PBarAddTotal(fd.nFileSizeLow);
			sprintf(AF->name, "%s\\%s", dir, fd.cFileName);
		}
	} while (FindNextFile(ffh, &fd));
	FindClose(ffh);

	return files;
}

void AddThread(void)
{
	char *t, *endoffilestring;
	struct AddFile *AF;
	UINT i, dostime;
	WIN32_FIND_DATA fd;

	if (!lvNumEntries)
		MenuUpdateRFL(temp1);	// very bad to assume temp1 has not changed

// Add.filestring has format of:
// file1\0file2\0...filen\0\0
// unless Add.ofn exists
	i = 1;
	for (t = Add.filestring; *(short *)t; t++)
		if (!*t) i++;

	endoffilestring = t;
	t = Add.filestring;
	if (Add.ofn)	// only true if called from ActionsAdd, need to skip path
		if (i == 1)	// just one file so it is path+file\0\0
			t = GetFileFromPath(t);
		else
		{
			i--;			// it is path\0file1\0...filen\0\0
			t += 1 + strlen(t);
		}

	Add.file = Dzip_malloc(i * 16);
	Add.NumNewFiles = 0;
	PBarReset();
	while (*t)
	{
		fd.dwFileAttributes = fd.nFileSizeHigh = 0;	// in case the find fails because a file was deleted
		FindClose(FindFirstFile(t, &fd));
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			GuiProgressMsg("scanning %s", t);
			i = AddFilesFromDirectory(t, i);
		}
		else if (fd.nFileSizeHigh)
			error("%s is at least 4GB and\ncan not currently be compressed with Dzip", t);
		else
		{
			MakeLowercase(t);
			AF = Add.file + Add.NumNewFiles++;
			AF->name = t;
			AF->ft = fd.ftLastWriteTime;
			AF->size = fd.nFileSizeLow;
			PBarAddTotal(fd.nFileSizeLow);
		}
		t += 1 + strlen(t);
		if (AbortOp) goto AbortedDirectorySearch;
	}

	LVBeginAdding(Add.NumNewFiles);
	gi.BeginAdd(!lvNumEntries);

	AF = Add.file;
	for (i = 0; i < Add.NumNewFiles && !AbortOp; i++, AF++)
	{
		if (AF->ft.dwHighDateTime == 0xFFFFFFFF)
		{
			gi.AddFolder(AF->name);
			continue;
		}
		PBarSetSkip(AF->size);
		Add.handle = CreateFile(AF->name, GENERIC_READ, FILE_SHARE_READ,
			NULL, OPEN_EXISTING, 0, NULL);
		if (Add.handle == INVALID_HANDLE_VALUE)
		{
			errorWithSysError(0, "Could not open %s", AF->name);
			PBarSkip();
			continue;
		}
		Add.curfile = AF->name;
		FileTimeToLocalFileTime(&AF->ft, &fd.ftCreationTime);
		FileTimeToDosDateTime(&fd.ftCreationTime, (short *)&dostime + 1, (short *)&dostime);

		gi.AddFile(AF->name, AF->size, dostime  - (1 << 21));
		CloseHandle(Add.handle);
		PBarSkip();
		if (AbortOp == 3) // reading from file failed, dont abort the rest
			AbortOp = 0;
	}

AbortedDirectorySearch:
	SetCurrentDirectory(Add.olddir);
	AF = Add.file;
	for (i = 0; i < Add.NumNewFiles; i++, AF++)
		if (AF->name < Add.filestring || AF->name >= endoffilestring)
			free(AF->name);
	free(Add.file);

	if (lvNumEntries)
	{
		gi.FinishedAdd();
		ReopenFile(0);
		LVFinishedAdding();
	}
	else
		DeleteCurrentFile();
}

void AddSetBufferSize(void)
{
	OPENFILENAME *ofn = Add.ofn;
	UINT i = 6;
	i += CommDlg_OpenSave_GetSpec(Add.hDlg, temp2, 0);
	i += CommDlg_OpenSave_GetFolderPath(Add.hDlg, temp2, 0);
	if (i > 256)
	{
		ofn->nMaxFile = i;
		ofn->lpstrFile = Dzip_realloc(ofn->lpstrFile, i);
	}
}

long CALLBACK AddSubclassProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_COMMAND && LOWORD(wParam) == IDOK)
		AddSetBufferSize();
	return CallWindowProc(Add.dproc, hDlg, msg, wParam, lParam);
}

UINT CALLBACK AddOFNHookProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NOTIFY)
		if (((NMHDR *)lParam)->code == CDN_INITDONE)
		{
			HWND p = GetParent(hDlg);
			CommDlg_OpenSave_SetControlText(p, stc4, "Add &from:");
			CommDlg_OpenSave_SetControlText(p, IDOK, "&Add");
			Add.hDlg = p;
			Add.dproc = (WNDPROC)SetWindowLong(p, DWL_DLGPROC, (long)AddSubclassProc);
		}
		else if (((NMHDR *)lParam)->code == CDN_SELCHANGE)
			AddSetBufferSize();
	return 0;
}

// temp1 may contain a file name.... dont change it!! (sucks)
int AddGetFilenames(void)
{
	int ret;
	OPENFILENAME ofn;

	memset(&ofn, 0, 76);
	ofn.lStructSize = 76;
	ofn.hwndOwner = MainWnd;
	ofn.lpstrFilter = "All Files\0*.*\0dem,txt,pak,dz Files\0*.dem;*.txt;*.pak;*.dz\0";
	ofn.nFilterIndex = 2 - Options.AllFilesInAdd;
	ofn.lpstrFile = Dzip_calloc(256);
	ofn.nMaxFile = 256;
	ofn.lpstrInitialDir = temp2;
	ofn.lpstrTitle = "Add";
	ofn.Flags = OFN_NODEREFERENCELINKS|OFN_ALLOWMULTISELECT|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST|OFN_EXPLORER|OFN_ENABLEHOOK;
	ofn.lpfnHook = AddOFNHookProc;

	Add.ofn = &ofn;
	GetCurrentDirectory(256, Add.olddir);

	if (!ReadRegValue(HKEY_CURRENT_USER, dzipreg, "addpath", temp2, 256))
		strcpy(temp2, Add.olddir);

	ret = GetOpenFileName(&ofn);
	if (ret)
	{
		GetCurrentDirectory(256, temp2);
		WriteRegValue(HKEY_CURRENT_USER, dzipreg, "addpath", REG_SZ, temp2, 0, 0);

		Add.filestring = ofn.lpstrFile;
		RunThread(AddThread, THREAD_ADD);
	}
	free(ofn.lpstrFile);
	Add.ofn = NULL;
	return ret;
}

void ActionsAdd(void)
{
	AddGetFilenames();
}

void AddDroppedFiles (HDROP hDrop)
{
	int i, j = 1, dirlength;
	int numdropped = DragQueryFile(hDrop, -1, NULL, 0);
	char droppeddir[256];

	SetForegroundWindow(MainWnd);

	if (!lvNumEntries)
	{
		if (numdropped == 1)
		{	// if it's a file type we can open, then do that
			char file[256];
			DragQueryFile(hDrop, 0, file, 256);
			if (IsAnArchive(file))
			{
				OpenArchive(file, 0);
				return;
			}
		}
		if (!GetNewFileName(numdropped))
			return;
	}

	GetCurrentDirectory(256, Add.olddir);

	DragQueryFile(hDrop, 0, droppeddir, 256);
	*GetFileFromPath(droppeddir) = 0;
	dirlength = strlen(droppeddir);
	for (i = 0; i < numdropped; i++)
		j += 1 + DragQueryFile(hDrop, i, NULL, 0) - dirlength;

	Add.filestring = Dzip_malloc(j);

	j = 0;
	for (i = 0; i < numdropped; i++)
	{
		DragQueryFile(hDrop, i, temp2, 256);
		j += 1 + sprintf(Add.filestring + j, "%s", temp2 + dirlength);
	}
	Add.filestring[j] = 0;

	DragFinish(hDrop);

	SetCurrentDirectory(droppeddir);
	RunThread(AddThread, THREAD_ADD);
}

// filestring is formatted as file1"file2"...filen"
// file1 is the dz file to add the rest to if dropped is true
void AddFilesFromCommandLine (char *filestring, int dropped)
{
	int i, numdropped = 0;

	if (dropped)
	{	// use supplied file name
		char *p = filestring;
		if (dropped == 1)	// dropped == 2 already has a file open
		{
			p = strchr(p, '"');
			*p++ = 0;
			if (!OpenArchive(filestring, 1))
				return;	// will rarely fail cuz dll does a preliminary check
		}
		Add.filestring = Dzip_malloc(strlen(p) + 1);
		strcpy(Add.filestring, p);
	// change quotes to nulls for AddThread
		for (filestring = Add.filestring; *filestring; filestring++)
			if (*filestring == '"')
				*filestring = 0;
		RunThread(AddThread, THREAD_ADD);
		return;
	}
	// see if filenames are same, if so use that for dz... if not, get a new one
	Add.filestring = Dzip_malloc(strlen(filestring) + 1);
	strcpy(Add.filestring, filestring);
	for (filestring = Add.filestring; ; filestring++)
		if (*filestring == '"')
		{
			*filestring = 0;
			if (!numdropped++)
			{
				strcpy(temp2, Add.filestring);
				i = FileExtension(temp2) - temp2;
			}
			if (!filestring[1]) break;
			if (strnicmp(temp2, filestring + 1, i))
				temp2[0] = 0;
		}

	if (temp2[0])
	{	// if file names all agreed
		MakeLowercase(temp2);
		// use "default extension" not dz!
		strcpy(FileExtension(temp2), ".dz");
		GetFullPathName(temp2, 256, temp1, NULL);
		if (FileExists(temp1))
		{
			if (!OpenArchive(temp1, 1))
				goto getName;
		}
		else if (!NewArchive(temp1))
			goto getName;
	}
	else
	{
getName:
		if (!GetNewFileName(numdropped))
		{
			free(Add.filestring);
			return;
		}
	}
	if (gi.flags & UNSUPPORTED_ADD)
	{	// error message would be nice
		free(Add.filestring);
		return;
	}
	RunThread(AddThread, THREAD_ADD);
}