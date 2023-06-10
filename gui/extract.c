#include <windows.h>
#include <stdio.h>
#include "gui_import.h"
#include "common.h"
#include "listview.h"
#include "extract.h"
#include "lvhelper.h"
#include "misc.h"
#include "recent.h"
#include "resource.h"
#include "sbar.h"
#include "thread.h"
#include <shlobj.h>

extract_t Extract;

void Outfile_Flush (void *buf, UINT num)
{
	UINT er, wrote;

	if (ThreadType == THREAD_DRAGTOWIN2KEXPLORER)
	{
		PBarAdvance(Extract.bufused);
		Extract.buf = NULL;
		// wait for another buf
		while (!Extract.buf) 
			Sleep(0);
		return;
	}

	if (!buf)	// write whatever is in Extract.buf
	{
		if (!Extract.bufused) return;
		buf = Extract.buf;
		num = Extract.bufused;
		Extract.bufused = 0;
	}

	PBarAdvance(num);
	er = 0;
	if (ThreadType != THREAD_DRAGTOEXPLORER)
	{
		WriteFile(Extract.handle, buf, num, &wrote, NULL);
		if (wrote != num)
			er = GetLastError();
	}
	else if (STG_E_MEDIUMFULL == Extract.stream->lpVtbl->Write(Extract.stream, buf, num, NULL))
		er = ERROR_DISK_FULL;

	if (er)
	{
		errorWithSysError(er, "Error writing to %s", Extract.Curname);
		AbortOp = 1;
	}
}

void Outfile_Write (void *buf, UINT num)
{
	UINT canwrite;

	make_crc(buf, num);

	switch (ThreadType)
	{
	case THREAD_TEST:
		PBarAdvance(num);
		return;
	case THREAD_DRAGTOWIN2KEXPLORER:
		while (!Extract.buf)
			Sleep(0);
		break;
	default:
		if (num > Extract.bufsize)
		{
			Outfile_Flush(NULL, 0);	// clear out whats in Extract.buf
			Outfile_Flush(buf, num);// and then just write what we've got now
			return;
		}
	}

	while (num + Extract.bufused > Extract.bufsize)
	{	// write what we can
		canwrite = Extract.bufsize - Extract.bufused;
		memcpy((char *)Extract.buf + Extract.bufused, buf, canwrite);
		Extract.bufused = Extract.bufsize;
		buf = (char *)buf + canwrite;
		num -= canwrite;
		Outfile_Flush(NULL, 0);
		Extract.bufused = 0;
	}
	memcpy((char *)Extract.buf + Extract.bufused, buf, num);
	Extract.bufused += num;
}

int CALLBACK ExtractOverwriteDlgFunc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INITDIALOG)
	{	// it's not that big a mess anymore
		char msg[280], *ptr = msg;
		lventry_t *lve = (lventry_t *)lParam;
		WIN32_FIND_DATA fd;
		
		SendDlgItemMessage(hDlg, IDC_QICON, STM_SETICON, (int)LoadIcon(NULL, IDI_QUESTION), 0);
		sprintf(msg, "Overwrite %s?", Extract.Curname);
		SendDlgItemMessage(hDlg, IDS_FILENAME, WM_SETTEXT, 0, (int)msg);

		FindClose(FindFirstFile(Extract.Curname, &fd));
	// add filesize of existing file
		ptr += NumberToString(fd.nFileSizeLow / 1024, ptr) - 1;
		ptr += sprintf(ptr, "KB  ");
	// add date of existing file
		ptr += DateTimeToString((UINT)&fd.ftLastWriteTime, 1, 2, ptr);
		ptr[-1] = '\n';
	// add filesize of new file
		ptr += NumberToString(lve->realsize / 1024, ptr) - 1;
		ptr += sprintf(ptr, "KB  ");
	// add date of new file
		if (lve->date)
			DateTimeToString(lve->date, 0, 2, ptr);
		else
			strcpy(ptr, "??????????");
		SendDlgItemMessage(hDlg, IDS_FILEINFO, WM_SETTEXT, 0, (int)msg);

		if (ThreadType == THREAD_DRAGOUT)
			SetForegroundWindow(hDlg);
		return 1;
	}
	if (msg == WM_COMMAND)
		EndDialog (hDlg, LOWORD(wParam));
	return 0;
}

int CALLBACK ExtractAsDlgFunc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INITDIALOG)
	{
		SetWindowText(hDlg, "Extract file as...");
		SetDlgItemText(hDlg, IDC_EDIT1, (char *)lParam);
		return 1;
	}
	if (msg == WM_COMMAND)
		if (LOWORD(wParam) == IDCANCEL)
			EndDialog (hDlg, 0);
		else if (LOWORD(wParam) == IDOK)
		{
			GetDlgItemText(hDlg, IDC_EDIT1, temp2, 258);
			if (CheckForValidFilename(temp2, 0))
				EndDialog (hDlg, 1);
		}
	return 0;
}

int ExtractSetHandle (lventry_t *lve)
{
	char *name = lve->filename;

tryWithNewName:
	if (Extract.FreeCurname)
	{
		free(Extract.Curname);
		Extract.FreeCurname--;
	}

	if (Extract.NoPaths)
		name = GetFileFromPath(name);
	else if (lve->status.indent)
	{	// combine the path of the pakfile with the path of this file
		char *pakname = lventries[lve->parent].filename;
		char *oldname = name;	// might not be lve->filename because of extract as

		Extract.FreeCurname++;
		
		name = Dzip_malloc(strlen(pakname) + strlen(oldname));
		strcpy(name, pakname);
		strcpy(GetFileFromPath(name), oldname);

		for (oldname = name; *oldname; oldname++)
			if (*oldname == '/')
				*oldname = '\\';
	}
	Extract.Curname = name;

	if (Extract.ToAll == IDYES || !FileExists(name))
	{
overwriteit:
		Extract.handle = CreateNewFile(name);
		return (int)Extract.handle + 1;
	}

	if (Extract.ToAll == IDNO)
		return 0;

	// ask to overwrite or not
	for (;;)
	switch (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_OVERWRITE), MainWnd, ExtractOverwriteDlgFunc, (int)lve))
	{
	case IDC_EXTRACTAS:
		if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_RENAME), MainWnd, ExtractAsDlgFunc, (int)name))
		{
			name = temp2;
			goto tryWithNewName;
		}
		break;	// goes back to switch statement
	case IDYESTOALL:
		Extract.ToAll = IDYES;
	default:	// only other value possible is IDYES
		goto overwriteit;
	case IDNOTOALL:
		Extract.ToAll = IDNO;
		return 0;
	case IDCANCEL:
		AbortOp = 1;
	case IDNO:
		return 0;
	}
}

void ExtractSetup (int type, int directories, const char *olddir)
{
	int i, j = 0, num, indent;
	lventry_t *lve;

// type values:
// 0: extract all files
// 1: extract selected only
// 2: extract only focused

	if (!type)
		num = lvNumEntries;
	else if (type == 1)
		num = LVNumSelected();
	else
		num = 1;
	Extract.list = Dzip_malloc(4 * num);
	memset(&Extract, 0, (int)&(((extract_t*)0)->stream));
	if (!olddir)
		GetCurrentDirectory(256, Extract.olddir);
	else
		strcpy(Extract.olddir, olddir);
	PBarReset();
	if (type == 2)
	{
		lve = lventries + focus;
		Extract.list[j++] = lve;
		PBarAddTotal(lve->realsize);
	}
	else
		for (i = 0; i < lvNumEntries; i++)
		{
			lve = lventries + lvorderarray[i];
			if (type && !lve->status.selected)
				continue;
			if (lve->status.folder && !directories)
				continue;
			Extract.list[j] = lve;
			PBarAddTotal(lve->realsize);
			if (lve->status.expanded)
			{
				if (!type && !directories)	// must be for a test
					continue;	// so add all the sub files
				indent = lve->status.indent;
				do {
					if (++i == lvNumEntries) break;
					lve = lventries + lvorderarray[i];
				} while (lve->status.indent > indent);
				i--;
			}
			j++;
		}

	if (!j) 
	{
		free(Extract.list);
		return;
	}
	Extract.num = j;
	Extract.buf = Dzip_malloc(8192);
	Extract.bufsize = 8192;
}

void ExtractFilesForDragOut(void)
{
	GetTempPath(256, temp2);
	SetCurrentDirectory(temp2);
// we're already in a 2nd thread here
	ExtractThread();
}

// made after extract is done
HGLOBAL ExtractMakeDropfiles(void)
{
	DROPFILES *df;
	tempfiles_t *ct;
	char *p;
	int i, size = 1 + sizeof(*df);
	int j = GetTempPath(256, temp1);

	ct = tempfiles;
	for (i = 0; i < Extract.numSuccessfull; i++, ct = ct->next)
		size += 1 + j + strlen(ct->name);

// the hglobals I allocate are freed by ole32.dll
	df = GlobalAlloc(0, size);
	df->pFiles = sizeof(*df);
	df->fWide = 0;
	p = (char *)df + sizeof(*df);

	ct = tempfiles;
	for (i = 0; i < Extract.numSuccessfull; i++, ct = ct->next)
		p += 1 + sprintf(p, "%s%s", temp1, ct->name);
	*p = 0;

	return df;
}

// made before extract starts
HGLOBAL ExtractMakeFGD(void)
{
	FILEGROUPDESCRIPTOR *fgd;
	FILEDESCRIPTOR *fd;
	FILETIME ft;
	lventry_t *lve;
	int i;

	i = 4 + Extract.num * sizeof(FILEDESCRIPTOR);
	fgd = GlobalAlloc(0, i);
	fgd->cItems = Extract.num;

	fd = fgd->fgd;
	for (i = 0; i < Extract.num; i++)
	{
		lve = Extract.list[i];
	// before IE 4, only the cFileName is looked at by the shell
		fd->dwFlags = FD_WRITESTIME|FD_FILESIZE;//|FD_ATTRIBUTES;
//		fd->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		fd->nFileSizeHigh = 0;
		fd->nFileSizeLow = lve->realsize;
		DosDateTimeToFileTime(HIWORD(lve->date + (1 << 21)),
			LOWORD(lve->date), &ft);
		LocalFileTimeToFileTime(&ft, &fd->ftLastWriteTime);
		strcpy(fd->cFileName, GetFileFromPath(lve->filename));
		fd++;
	}
	return fgd;
}

void ExtractFile (lventry_t *lve)
{
	PBarSetSkip(lve->realsize);
	gi.ExtractFile(lve->filepos, ThreadType == THREAD_TEST);
	if (AbortOp)
		return;
	Outfile_Flush(NULL, 0);
	PBarSkip();
}

void TestThread(void)
{
	int i;

	ExtractSetup(0, 0, NULL);
	for (i = 0; i < Extract.num && !AbortOp; i++)
		ExtractFile(Extract.list[i]);
}

void ActionsTest(void)
{
	RunThread(TestThread, THREAD_TEST);
}

// used by THREAD_DRAGOUT, THREAD_EXTRACT and THREAD_VIEW
void ExtractThread(void)
{
	int i;
	lventry_t *lve;
	FILETIME ft1, ft2;
	HANDLE h;

	for (i = 0; i < Extract.num && !AbortOp; i++)
	{
		lve = Extract.list[i];
		if (lve->status.folder)
		{
			CreateDirectories(lve->filename);
			continue;
		}

		if (!ExtractSetHandle(lve))
		{
			PBarAdvance(lve->realsize);
			continue;
		}

		ExtractFile(lve);
		CloseHandle(Extract.handle);
		if (AbortOp)
		{
			DeleteFile(Extract.Curname);
			break;
		}
		if (ThreadType != THREAD_EXTRACT)
		{	// only if we weren't aborted
			tempfiles_t *newtf = Dzip_malloc(sizeof(*newtf));
			newtf->next = tempfiles;
			newtf->name = Dzip_strdup(Extract.Curname);
			tempfiles = newtf;
		}
		Extract.numSuccessfull++;
		if (lve->date)
		{
			h = CreateFile(Extract.Curname, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
			DosDateTimeToFileTime(HIWORD(lve->date + (1 << 21)),
				LOWORD(lve->date), &ft1);
			LocalFileTimeToFileTime(&ft1, &ft2);
			SetFileTime(h, NULL, NULL, &ft2);
			CloseHandle(h);
		}
	}

	if (Extract.FreeCurname)
		free(Extract.Curname);

	if (ThreadType == THREAD_DRAGOUT)
		return;
	
	SetCurrentDirectory(Extract.olddir);
	free(Extract.list);
	free(Extract.buf);
}

int CALLBACK ExtractDlgFunc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		int i;
		Recent.NumExtractPaths = RecentRead(&Recent.ExtractPaths, "path", 10);
		for (i = 0; i < Recent.NumExtractPaths; i++)
			SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (int)Recent.ExtractPaths[i]);
		if (!Recent.NumExtractPaths)
		{
			GetCurrentDirectory(256, temp1);
			SetDlgItemText(hDlg, IDC_COMBO1, temp1);
		}
		else
			SetDlgItemText(hDlg, IDC_COMBO1, Recent.ExtractPaths[0]);
		if (LVNumSelected())
			CheckDlgButton(hDlg, IDC_SELECTED, 1);
		else
		{
			CheckDlgButton(hDlg, IDC_ALLFILES, 1);
			EnableWindow(GetDlgItem(hDlg, IDC_SELECTED), 0);
		}
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
			RecentAddString(&Recent.ExtractPaths, &Recent.NumExtractPaths, temp1);
			RecentWrite(Recent.ExtractPaths, "path", Recent.NumExtractPaths);
			ExtractSetup(IsDlgButtonChecked(hDlg, IDC_SELECTED), 1, temp2);
			if (IsDlgButtonChecked(hDlg, IDC_OVERWRITE))
				Extract.ToAll = IDYES;
			Extract.NoPaths = IsDlgButtonChecked(hDlg, IDC_NOPATHS);
			EndDialog(hDlg, 1);
			break;
		case IDCANCEL:
			EndDialog(hDlg, 0);
		}
		return 1;
	}
	return 0;
}

void ActionsExtract(void)
{
	GetCurrentDirectory(256, temp2);
	if (!DialogBox(hInstance, MAKEINTRESOURCE(IDD_EXTRACT), MainWnd, ExtractDlgFunc))
		return;

	RunThread(ExtractThread, THREAD_EXTRACT);
}