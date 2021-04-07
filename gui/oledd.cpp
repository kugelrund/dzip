extern "C" {

#include <windows.h>
#include "common.h"
#include "listview.h"
#include "extract.h"
#include "misc.h"
#include "sbar.h"
#include "thread.h"
#include <shlobj.h>

/*	when explorer is stupid and asks for cf_hdrop first, the files
	go to the temp directory.  When it finally asks for filecontents
	I'll stream them from the temps that exist which is alot faster
	than decompressing them again */
int ExtractToExplorerFromTemp(void)
{
	lventry_t *lve = Extract.list[Extract.NextFileToExplorer];
	DWORD read, chunck, left = lve->realsize;
	void *buf;

	Extract.handle = CreateFile(lve->filename + lve->filenameonly_offset,
		GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_FLAG_DELETE_ON_CLOSE, NULL);
	if (Extract.handle == INVALID_HANDLE_VALUE)
		return E_UNEXPECTED;

	buf = Dzip_malloc(Extract.bufsize);
	while (left && !AbortOp)
	{
		chunck = (left > Extract.bufsize) ? Extract.bufsize : left;
		ReadFile(Extract.handle, buf, Extract.bufsize, &read, NULL);
		Outfile_Write(buf, chunck);
		left -= chunck;
	}

	Outfile_Flush(NULL, 0);
	free(buf);
	CloseHandle(Extract.handle);
	return S_OK;
}

// I start a new thread for each file that is extracted
DWORD WINAPI ExtractToWin2kThreadFunc(void *FromTemp)
{
	if (FromTemp)
		ExtractToExplorerFromTemp();
	else
		ExtractFile(Extract.list[Extract.NextFileToExplorer]);
	Extract.buf = NULL;	// Read() will returns 0 to end the current file
	CloseHandle(hThread);
	hThread = NULL;
	return 0;
}

}

#ifdef _DEBUG
#define OLELOG
#endif

#ifdef OLELOG
void OutputDebugStringf(char *x, ...)
{
	char msg[256];
	va_list	the_args;
	va_start(the_args, x);
	wvsprintf(msg, x, the_args);
	va_end(the_args);
	OutputDebugString(msg);
}
#endif

class dzDD : public IDropSource, IDataObject, IEnumFORMATETC, IStream
{
public:	// who needs protection???
	char ExtractPhase, EnumCounter;
//	char IsExplorer;	// used to reject the cf_hdrops requested by explorer
	UINT cfstr_filedescriptor, cfstr_filecontents;
	DWORD effect;
#ifdef OLELOG
	char name[256];
	DWORD lasteffect;
#endif

	/* Meaning of ExtractPhase values
		0: drop has not occured
		1: drop has occured and no data has been requested yet
		2: files were extracted to temp directory
		3: something other than CF_HDROP was asked for first,
		     any future CF_HDROP requests will be rejected
		4: file contents were asked for after an initial cf_hdrop so files
		     are already in temp directory
		5: file contents were asked for and will be streamed out of dzip
		6: same as 4 but with Read instead of CopyTo
		7: same as 5 but with Read instead of CopyTo
	*/

	dzDD(void)
	{
		ExtractPhase = 0;
		EnumCounter = 0;
//		IsExplorer = 0;
		cfstr_filedescriptor = RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
		cfstr_filecontents = RegisterClipboardFormat(CFSTR_FILECONTENTS);

		Extract.NoPaths++;
		DragAcceptFiles(MainWnd, 0);
		OleInitialize(NULL);

		DoDragDrop(this, this, DROPEFFECT_COPY, &effect);

		OleUninitialize();

		if (!ReadOnly)
			DragAcceptFiles(MainWnd, 1);

		if (ExtractPhase > 1)
			ThreadCleanUp();
		if (ExtractPhase < 6)
			free(Extract.buf);
		free(Extract.list);
		SetCurrentDirectory(Extract.olddir);
	}

	STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
	{
#ifdef OLELOG
		wchar_t str[256];
		char regpath[256] = "Interface\\";
		HKEY hk;
		StringFromGUID2(riid, str, 256);
		WideCharToMultiByte(CP_ACP, 0, str, -1,
			regpath + 10, 246, NULL, NULL);
		if (!RegOpenKey(HKEY_CLASSES_ROOT, regpath, &hk))
		{
			char riidname[256];
			long twofivesix = 256;
			if (!RegQueryValue(hk, NULL, riidname, &twofivesix))
				OutputDebugStringf("QueryInterface for: %s\n", riidname);
			else
				OutputDebugStringf("QueryInterface for: %s\n", regpath + 10);
			RegCloseKey(hk);
		}
		else
			OutputDebugStringf("QueryInterface for: %s\n", regpath + 10);
#endif
		if (riid == IID_IUnknown || riid == IID_IDataObject)
		{
			*ppv = (IDataObject *)this;
			return S_OK;
		}
		if (riid == IID_IStream)
		{
			*ppv = (IStream *)this;
			return S_OK;
		}
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	STDMETHODIMP QueryContinueDrag(BOOL esc, DWORD keys)
	{
//		HINSTANCE psapi;

		if (esc) return DRAGDROP_S_CANCEL;
		if (keys & MK_LBUTTON) return S_OK;
		ExtractPhase = 1;
#ifdef OLELOG
		OutputDebugStringf("DROPPED\n");
#endif
/*		psapi = LoadLibrary("psapi.dll");
		if (psapi)
		{
			POINT pt;
			HWND wnd;

			GetCursorPos(&pt);
			wnd = WindowFromPoint(pt);

			if (wnd)
			{
				HANDLE hproc;
				char pname[16];

				GetWindowThreadProcessId(wnd, &keys);
				hproc = OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_INFORMATION, 0, keys);
				if (hproc)
				{
					int module;
					int (WINAPI * EnumProcessModules)(HANDLE, int *, int, HWND *);
					int (WINAPI * GetModuleBaseName)(HANDLE, int, char *, int);
					EnumProcessModules = (int (WINAPI *)(HANDLE, int *, int, HWND *))GetProcAddress(psapi, "EnumProcessModules");
					GetModuleBaseName = (int (WINAPI *)(HANDLE, int, char *, int))GetProcAddress(psapi, "GetModuleBaseNameA");
					EnumProcessModules(hproc, &module, 4, &wnd);
					GetModuleBaseName(hproc, module, pname, 16);
					IsExplorer = !stricmp(pname, "explorer.exe");
					CloseHandle(hproc);
#ifdef OLELOG
					OutputDebugStringf("IsExplorer: %i\n", IsExplorer);
#endif
				}
			}
			FreeLibrary(psapi);
		}
*/
		return DRAGDROP_S_DROP;
	}

	STDMETHODIMP GiveFeedback(DWORD effect)
	{
#ifdef OLELOG
		const char *str[] = {
			"DROPEFFECT_NONE", "DROPEFFECT_COPY",
			"DROPEFFECT_MOVE", "DROPEFFECT_LINK"
		};
		if ((effect&15) != lasteffect)
		{
			OutputDebugStringf("GiveFeedback: %s\n", str[effect&15]);
			lasteffect = effect&15;
		}
#endif
		return DRAGDROP_S_USEDEFAULTCURSORS;
	}

	STDMETHODIMP GetData(FORMATETC *fmt, STGMEDIUM *medium)
	{
#ifdef OLELOG
		if (fmt->cfFormat == CF_HDROP)
			OutputDebugStringf("GetData: CF_HDROP\n");
		else
		{
			GetClipboardFormatName(fmt->cfFormat, name, 256);
			OutputDebugStringf("GetData: %s\n", name);
		}
#endif
		// first time this is called after the drop
		// do all the stuff done by starting a thread
		// but this doesn't really run a thread
		if (ExtractPhase == 1)
			RunThread(NULL, THREAD_DRAGOUT);

		medium->tymed = TYMED_HGLOBAL;
		medium->pUnkForRelease = NULL;

		if (fmt->cfFormat == CF_HDROP)
		{	// if drop has not occured, or something other
			// than hdrop was asked for first, return invalid
			if (ExtractPhase < 1 || ExtractPhase > 2)
				return E_INVALIDARG;
			if (ExtractPhase == 1)
			{	// some programs ask for it twice, so I do this to make
				// sure I only extract the files once
				ExtractPhase = 2;
				ExtractFilesForDragOut();	// doesn't return until all selected
			}								// files are put in temp directory
			medium->hGlobal = ExtractMakeDropfiles();
			return S_OK;
		}

		// first request is for something other than hdrop,
		// reject all future hdrops  (explorer is stupid)
		if (ExtractPhase == 1)
			ExtractPhase = 3;

		if (fmt->cfFormat == cfstr_filedescriptor)
		{
			medium->hGlobal = ExtractMakeFGD();
			return S_OK;
		}

		// dont allow anything to ask for file contents before the drop!
		// and anything other than contents is invalid
		if (!ExtractPhase || fmt->cfFormat != cfstr_filecontents)
			return E_INVALIDARG;

		if (AbortOp)
			return E_UNEXPECTED;
		if (ExtractPhase < 4)
		{	// it was either 2 or 3
			ExtractPhase += 2;
			ThreadType = THREAD_DRAGTOEXPLORER;
		}
		medium->tymed = TYMED_ISTREAM;
		medium->pstm = this;

		// advance progress bar if files were skipped
		for (int i = Extract.NextFileToExplorer; i < fmt->lindex - 1; i++)
			PBarAdvance(Extract.list[i]->realsize);

		Extract.NextFileToExplorer = fmt->lindex;
		// set Curname in case there's an error
		Extract.Curname = Extract.list[fmt->lindex]->filename + Extract.list[fmt->lindex]->filenameonly_offset;
		if (ThreadType == THREAD_DRAGTOWIN2KEXPLORER)
		{	// for first file, we won't know if it's to win2k,
			// so thread is created in Read() when we find out
			ULONG crap;
			hThread = CreateThread(NULL, 0,	ExtractToWin2kThreadFunc, (void *)(ExtractPhase == 6), 0, &crap);
		}
		return S_OK;
	}

	STDMETHODIMP EnumFormatEtc(DWORD, IEnumFORMATETC **ppenum)
	{
#ifdef OLELOG
		OutputDebugStringf("EnumFormatEtc\n");
#endif
		EnumCounter = 1;
		*ppenum = this;
		return S_OK;
	}

	STDMETHODIMP Next(ULONG, FORMATETC *fmt, ULONG *)
	{
#ifdef OLELOG
		OutputDebugStringf("Next\n");
#endif
		if (EnumCounter == 1)
		{ 
			fmt->cfFormat = cfstr_filecontents;
			fmt->tymed = TYMED_ISTREAM;
		}
		else if (EnumCounter == 2)
		{
			fmt->cfFormat = cfstr_filedescriptor;
			fmt->tymed = TYMED_HGLOBAL;
		}
		else return S_FALSE;
		fmt->ptd = NULL;
		fmt->dwAspect = DVASPECT_CONTENT;
		fmt->lindex = -1;
		EnumCounter++;
		return S_OK;
	}

	STDMETHODIMP Reset(void) {EnumCounter = 1; 
#ifdef OLELOG
		OutputDebugStringf("Reset\n");
#endif
	return S_OK;}

	STDMETHODIMP QueryGetData(FORMATETC *fmt)
	{
#ifdef OLELOG
		OutputDebugStringf("QueryGetData: %i\n", fmt->cfFormat);
#endif
		if (fmt->cfFormat == CF_HDROP ||
			fmt->cfFormat == cfstr_filedescriptor ||
			fmt->cfFormat == cfstr_filecontents)
			return S_OK;
		return DV_E_FORMATETC;
	}

// this is called by 95/98
// this *does not* start another thread since then the IStream
// functions don't work
	STDMETHODIMP CopyTo(IStream *stm, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *)
	{
#ifdef OLELOG
		OutputDebugStringf("CopyTo\n");
#endif
		Extract.stream = stm;

		// happens with win98 drop on desktop
		if (ExtractPhase == 4)
			return ExtractToExplorerFromTemp();
		else
			ExtractFile(Extract.list[Extract.NextFileToExplorer]);
		return S_OK;
	}

// this is called by 2000/xp
	STDMETHODIMP Read(void *buf, ULONG num, ULONG *read)
	{
#ifdef OLELOG
		OutputDebugStringf("Read\n");
#endif
		if (ExtractPhase < 6)	// will be either 4, 5, 6 or 7
		{
			ExtractPhase += 2;
			free(Extract.buf);	// isn't needed
			Extract.buf = NULL;
			ThreadType = THREAD_DRAGTOWIN2KEXPLORER;
			hThread = CreateThread(NULL, 0,	ExtractToWin2kThreadFunc, (void *)(ExtractPhase == 6), 0, read);
		}

		Extract.bufused = 0;
		Extract.bufsize = num;

		/*	once I set Extract.buf, the other thread starts writing
			stuff into it; it sets it to NULL when it's written
			the max number of bytes */
		Extract.buf = buf;
		while (Extract.buf)
			Sleep(0);

		*read = Extract.bufused;
		return S_OK;
	}

	// I dont need to worry about reference counting
	STDMETHODIMP_(ULONG) AddRef(void)  {return 1;}
	STDMETHODIMP_(ULONG) Release(void) {return 1;}

// I've never seen these called but they should exist
	STDMETHODIMP GetDataHere(FORMATETC *, STGMEDIUM *) {return E_NOTIMPL;}
	STDMETHODIMP GetCanonicalFormatEtc(FORMATETC *, FORMATETC *) {return E_NOTIMPL;}
	STDMETHODIMP SetData(FORMATETC *, STGMEDIUM *, BOOL) {return E_NOTIMPL;}
	STDMETHODIMP Skip(ULONG) {return E_NOTIMPL;}
	STDMETHODIMP Clone(IEnumFORMATETC **) {return E_NOTIMPL;}
	STDMETHODIMP DAdvise(FORMATETC *, DWORD, IAdviseSink *, DWORD *) {return E_NOTIMPL;}
	STDMETHODIMP DUnadvise(DWORD) {return E_NOTIMPL;}
	STDMETHODIMP EnumDAdvise(IEnumSTATDATA **) {return E_NOTIMPL;}
	STDMETHODIMP Write(const void *, ULONG, ULONG *) {return E_NOTIMPL;}
	STDMETHODIMP Seek(LARGE_INTEGER, DWORD, ULARGE_INTEGER *) {return E_NOTIMPL;}
	STDMETHODIMP SetSize(ULARGE_INTEGER) {return E_NOTIMPL;}
	STDMETHODIMP Commit(DWORD) {return E_NOTIMPL;}
	STDMETHODIMP Revert(void) {return E_NOTIMPL;}
	STDMETHODIMP LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) {return E_NOTIMPL;}
	STDMETHODIMP UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) {return E_NOTIMPL;}
	STDMETHODIMP Stat(STATSTG *, DWORD) {return E_NOTIMPL;}
	STDMETHODIMP Clone(IStream **) {return E_NOTIMPL;}
};

HANDLE ddThread;

DWORD WINAPI ExtractDragFilesOutThread (void *zilch)
{
	dzDD::dzDD();	// just call the constructor
	CloseHandle(ddThread);
	ddThread = NULL;
	return 0;
}

EXTERN_C void ExtractDragFilesOut(void)
{
	ULONG ddThreadID;
	ExtractSetup(1, 0, NULL);
	if (!Extract.num)
		return;

	ddThread = CreateThread(NULL, 0, ExtractDragFilesOutThread, 0, 0, &ddThreadID);
	while (!AttachThreadInput(ddThreadID, GetCurrentThreadId(), 1))
		Sleep(0);	// won't work until the 2nd thread calls a msg-related function
	while (ddThread)
		MsgLoop();
}