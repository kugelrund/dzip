#include "dzipshlx.h"

char **exts;
gui_export_t ge;
UINT	  DllCount, numext;
HINSTANCE hDllInst;
HBITMAP   bmpdz;

// entry point
STDAPI _DllMainCRTStartup(HINSTANCE hInstance, DWORD reason, void *reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		// this is from dllcode.c to search for dzip_*.dll files
		// and get a list of the supported file extensions
		int dirofs, extlength = 0;
		char dll[260];
		WIN32_FIND_DATA fd;
		HANDLE ffh;
		const char *(*GetExtentions)();

		ge.malloc = malloc;
		GetModuleFileName(hInstance, dll, 260);
		dirofs = strrchr(dll, '\\') - dll + 1;
		strcpy(dll + dirofs, "dzip_*.dll");

		numext = 0;
		ffh = FindFirstFile(dll, &fd);
		if (ffh == INVALID_HANDLE_VALUE)
			return 0;	// something's wrong so just dont load

		do {
			strcpy(dll + dirofs, fd.cFileName);
			hDllInst = LoadLibrary(dll);
			if (!hDllInst)
				continue;
			GetExtentions = (const char *(*)())GetProcAddress(hDllInst, "GetExtensions");
			if (!GetExtentions)
				continue;
			if (!(numext & 7))
				if (!(exts = (char **)realloc(exts, 4 * (numext + 8))))
					return 0;
			if (!(exts[numext] = strdup(GetExtentions())))
				return 0;
			numext++;
			FreeLibrary(hDllInst);
		} while (FindNextFile(ffh, &fd));
		FindClose(ffh);

		hDllInst = hInstance;
		bmpdz = LoadBitmap(hDllInst, MAKEINTRESOURCE(IDB_DZ));
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		while (numext--)
			free(exts[numext]);
		free(exts);
		DeleteObject(bmpdz);
	}

	return 1;
}
// exported
STDAPI DllCanUnloadNow()
{
	return DllCount;
}
// exported, first thing called by the shell
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppvOut)
{
	return (new dzClassFactory)->QueryInterface(riid, ppvOut);
}

// is ext in the list of exts?
int IsValidExtension (char *ext)
{
	uInt i;
	for (i = 0; i < numext; i++)
		if (!stricmp(exts[i], ext))
			return 1;
	return 0;
}

dzClassFactory::dzClassFactory()
{
	RefCount = 0;
	DllCount++;	
}
																
STDMETHODIMP dzClassFactory::QueryInterface(REFIID riid, void **ppv)
{	// Any interface on this object is the object pointer
	*ppv = (IClassFactory *)this;
	AddRef();
	return S_OK;
}	

STDMETHODIMP_(ULONG) dzClassFactory::AddRef()
{
	return ++RefCount;
}

STDMETHODIMP_(ULONG) dzClassFactory::Release()
{
	if (--RefCount)
		return RefCount;

	DllCount--;
	delete this;
	return 0;
}

STDMETHODIMP dzClassFactory::CreateInstance
	(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
{
	*ppvObj = NULL;

	if (pUnkOuter)	// dont think this will happen...
		return CLASS_E_NOAGGREGATION;

	dzShellExt *ShellExt = new dzShellExt();

	if (!ShellExt)
		return E_OUTOFMEMORY;

	return ShellExt->QueryInterface(riid, ppvObj);
}

dzShellExt::dzShellExt()
{
	RefCount = 0;
	DataObj = NULL;
	file = newdir = NULL;

	alldz = 0;	// 2.8

	DllCount++;
}

STDMETHODIMP dzShellExt::QueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IShellExtInit)
		*ppv = (IShellExtInit *)this;
	else if (riid == IID_IContextMenu)
		*ppv = (IContextMenu *)this;
	else if (riid == IID_IPersistFile)
		*ppv = (IPersistFile *)this;
	else if (riid == IID_IDropTarget)
		*ppv = (IDropTarget *)this;
	else
		return E_NOINTERFACE;

	AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) dzShellExt::AddRef()
{
	return ++RefCount;
}

STDMETHODIMP_(ULONG) dzShellExt::Release()
{
	if (--RefCount)
		return RefCount;

	if (DataObj)
		DataObj->Release();

	DllCount--;

	if (file) delete file;
	if (newdir) delete newdir;

	delete this;
	return 0;
}

STDMETHODIMP dzShellExt::Initialize
	(LPCITEMIDLIST pidl, IDataObject *NewDataObj, HKEY hRegKey)
{
	if (DataObj)
		DataObj->Release();

	if (NewDataObj)
	{
		DataObj = NewDataObj;
		DataObj->AddRef();
	}

	if (pidl)
	{
		char dir[256];
		SHGetPathFromIDList(pidl, dir);
		newdir = strdup(dir);
	}

	return S_OK;
}

// called when user clicks right mouse button (or file menu) on any number of selected files
STDMETHODIMP dzShellExt::QueryContextMenu
	(HMENU hMenu, UINT indexMenu, UINT CmdFirst, UINT CmdLast, UINT flags)
{
	FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM medium;
	int numfiles;

	if (FAILED(DataObj->GetData(&fmte, &medium)))
		return 0;	// right click on my computer only known cause for failure
	numfiles = DragQueryFile((HDROP)medium.hGlobal, -1, 0, 0);
	if (numfiles == 1)
	{
		char filename[256];
		char *ext;
		DragQueryFile((HDROP)medium.hGlobal, 0, filename, 256);
		GlobalFree(medium.hGlobal);
		ext = strrchr(filename, '\\');
		if (!ext[1])	// right click on a drive
			return 0;
		ext = strrchr(ext, '.');
		if (ext)
			if (IsValidExtension(ext + 1))
			{	// right click (perhaps drag) of valid file, add extract here
				ZeroCmd = CMD_EXT;
				InsertMenu(hMenu, indexMenu, MF_STRING|MF_BYPOSITION, CmdFirst, "Extract Here");
				SetMenuItemBitmaps(hMenu, indexMenu, MF_BYPOSITION, bmpdz, NULL);
				file = strdup(filename);
				return 1;
			}
		if (!flags)	// flags is true for a normal right click, false for a click and drag
			return 0;	// right click and drag of non dz, do nothing
	}
	else
	{	// 2.8: more than one file, are they all files we can work with?
		char filename[256];
		char *ext;
		int i;

		for (i = 0; i < numfiles; i++)
		{
			DragQueryFile((HDROP)medium.hGlobal, i, filename, 256);
			ext = strrchr(filename, '\\');
			if (!ext[1]) break;
			ext = strrchr(ext, '.');
			if (!ext) break;
			if (!IsValidExtension(ext + 1)) break;
		}
		GlobalFree(medium.hGlobal);
		if (i == numfiles)
		{
			alldz++;
			ZeroCmd = CMD_ALL;
			InsertMenu(hMenu, indexMenu, MF_STRING|MF_BYPOSITION, CmdFirst, "Extract all here");
			SetMenuItemBitmaps(hMenu, indexMenu, MF_BYPOSITION, bmpdz, NULL);
		}
	}
	if (flags)
	{	// right click on non dz, or more than one file: add compress with dz
		// 2.8: make sure the option is turned on
		HKEY hKey;
		options_t Options;
		DWORD i, s = sizeof(Options);

		i = RegOpenKey(HKEY_CURRENT_USER, "Software\\SDA\\Dzip", &hKey);

		if (i != ERROR_SUCCESS)
			return alldz;

		i = RegQueryValueEx(hKey, "options", 0, &i, (BYTE *)&Options, &s);
		RegCloseKey(hKey);

		if (i != ERROR_SUCCESS || !Options.ExpCmpWDz)
			return alldz;

		if (!alldz) ZeroCmd = CMD_CMP;
		InsertMenu(hMenu, indexMenu, MF_STRING|MF_BYPOSITION, CmdFirst + alldz, "Compress with Dzip");
		SetMenuItemBitmaps(hMenu, indexMenu, MF_BYPOSITION, bmpdz, NULL);
		return alldz + 1;
	}
	return alldz;	// right click and drag on non dz, or more than one file, do nothing
}

void dzShellExt::RunDzip (char *cmd, char *directory)
{
	int i;
	char dzipexe[256];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	
	GetModuleFileName(hDllInst, dzipexe, 256);
	strcpy(strrchr(dzipexe, '\\') + 1, "dzipgui.exe");
	if (!cmd)
	{
		int numfiles, cmdsize, dirsize;
		char filename[260], commondir[256], *p1, *p2, *p3;
		FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
		STGMEDIUM medium;

		if (FAILED(DataObj->GetData(&fmte, &medium)))
			return;

		numfiles = DragQueryFile((HDROP)medium.hGlobal, -1, 0, 0);
		cmdsize = 6 + DragQueryFile((HDROP)medium.hGlobal, 0, filename, 260);
		dirsize = 1 + strrchr(filename, '\\') - filename;
		strncpy(commondir, filename, dirsize);

		for (i = 1; i < numfiles; i++)
		{
			cmdsize += 1 + DragQueryFile((HDROP)medium.hGlobal, i, filename, 260);
			// compare the directories
			p1 = commondir;
			p2 = filename;
			p3 = commondir + dirsize;
			while (p1 < p3)
				if (*p1++ != *p2++)
				{
					dirsize -= p3 - p1 + 1;
					p3 = p1;
					break;
				}
		}
		cmdsize -= dirsize * numfiles;
		if (file)	// files were dropped onto 'file' (otherwise it was just right click and dzip will ask for file)
			cmdsize += 1 + lstrlen(file);
		else if (alldz) // 2.8
			cmdsize += newdir ? lstrlen(newdir) : dirsize;
		cmd = new char[cmdsize];
		strcpy (cmd, " /a ");	// the first token gets parsed away by CreateProcess so I need a space before /a
		cmdsize = 4;
		if (alldz) // 2.8
		{
			cmd[2] = 'e';
			alldz--;

			filename[dirsize] = 0;
		 	cmdsize += wsprintf(cmd + cmdsize, "%s", filename);
			if (newdir) directory = strdup(newdir);
			else directory = strdup(filename);
		}
		else
		{
			if (dirsize)
			{	// if there was no common root directory i'll just let windows
				// set the starting path for me, its what other programs do
				directory = new char[dirsize + 1];
				strncpy(directory, commondir, dirsize);
				directory[dirsize] = 0;
			}
			if (file)
			{
				cmd[2] = 'd';
				cmdsize += wsprintf(cmd + cmdsize, "%s\"", file);
			}
		}
		for (i = 0; i < numfiles; i++)
		{
			DragQueryFile((HDROP)medium.hGlobal, i,  filename, 256);
			cmdsize += wsprintf(cmd + cmdsize, "%s\"", filename + dirsize);
		}
		GlobalFree(medium.hGlobal);
	}
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	CreateProcess(dzipexe, cmd, NULL, NULL, 0,
		CREATE_NEW_PROCESS_GROUP, NULL, directory, &si, &pi);

	delete cmd;
	if (directory) delete directory;
}

// called when user picks the command I added
STDMETHODIMP dzShellExt::InvokeCommand(CMINVOKECOMMANDINFO *cmi)
{
	if (file)
	{	// file is only true if we added "extract here" for a dz file
		char *cmd, *directory;
		if (newdir)	// right click and drag
			directory = strdup(newdir);
		else
		{
			int i = 1 + strrchr(file, '\\') - file;
			directory = new char[i + 1];
			strncpy(directory, file, i);
			directory[i] = 0;
		}
		cmd = new char[5 + strlen(file)];
		wsprintf(cmd, " /e %s", file);
		RunDzip(cmd, directory);
	}
	else
	{
		if (alldz && cmi->lpVerb) // both commands were added and they picked compress
			alldz--;
		RunDzip(NULL, NULL);
	}

	return NOERROR;
}

// called to display info in the status bar (not called for right click and drag)
STDMETHODIMP dzShellExt::GetCommandString
	(UINT idCmd, UINT flags, UINT *reserved, char *name, UINT max)
{
	static const char * const text[3] = {
		"Compress files with Dzip",
		"Extract to this folder",
		"Extract all selected files to this folder"
	};
	int i = ZeroCmd;
	if (idCmd) i = CMD_CMP;	// idCmd == 1 would always be compress with Dzip

	if (flags == GCS_HELPTEXTA)
		strcpy(name, text[i]);
	else if (flags == GCS_HELPTEXTW)
	{
		wchar_t *wptr = (wchar_t*)name;
		const char *cptr = text[i];

		while (*cptr)
			*wptr++ = *cptr++;
		*wptr = 0;
	}
	return NOERROR;
}

// called when a file is initially dragged over a dz
STDMETHODIMP dzShellExt::Load(LPCOLESTR FileName, DWORD mode)
{
/*	struct {
		char id0, id1, maj_ver, min_ver;
		unsigned int dirloc;
		int numfiles;
	} dz;
	HANDLE f;
*/	int i = lstrlenW(FileName) + 1;	// FileName always comes in as UNICODE
	file = new char[i];
	while (i--)
		file[i] = (char)FileName[i];
/*	f = CreateFile(file, GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (f == INVALID_HANDLE_VALUE)
		return E_FAIL;
	if (!ReadFile(f, &dz, 12, (ULONG *)&i, NULL))
		i = E_FAIL;
	else if (dz.id0 != 'D' || dz.id1 != 'Z' || dz.maj_ver < 2
		|| dz.dirloc + dz.numfiles * 32 > GetFileSize(f, NULL))
		i = E_FAIL;
	else i = S_OK;

	CloseHandle(f);
	return i; */
	return S_OK;
}

STDMETHODIMP dzShellExt::DragEnter(IDataObject *pObj, DWORD KeyState, POINTL pt, DWORD *effect)
{
	*effect = DROPEFFECT_COPY;
	return S_OK;
}

STDMETHODIMP dzShellExt::DragOver(DWORD KeyState, POINTL pt, DWORD *effect)
{
	*effect = DROPEFFECT_COPY;
	return S_OK;
}

STDMETHODIMP dzShellExt::Drop(IDataObject *pObj, DWORD KeyState, POINTL pt, DWORD *effect)
{
	DataObj = pObj;
	RunDzip(NULL, NULL);
	DataObj = NULL;
	return S_OK;
}