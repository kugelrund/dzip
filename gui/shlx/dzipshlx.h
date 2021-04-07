#include <windows.h>
#include <shlobj.h>
#include "resource.h"
#include "..\options.h"
typedef unsigned int uInt;
typedef void * z_stream;
#include "..\gui_export.h"
#include "..\gui_import.h"

class dzClassFactory : public IClassFactory
{
protected:
	ULONG	RefCount;

public:
	inline dzClassFactory();

	//IUnknown members
	STDMETHODIMP			QueryInterface(REFIID, void **);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	//IClassFactory members
	STDMETHODIMP	CreateInstance(IUnknown *, REFIID, void **);
	STDMETHODIMP	LockServer(BOOL) {return S_OK;};
};

enum {CMD_CMP, CMD_EXT, CMD_ALL};	// possible values of ZeroCmd

class dzShellExt : public IShellExtInit, IContextMenu, IPersistFile, IDropTarget
{
protected:
	ULONG		RefCount;
	IDataObject	*DataObj;
	char		*file, *newdir;
	char		alldz, ZeroCmd;

	void		RunDzip(char *, char *);

public:
	inline dzShellExt();

	// IUnknown members
	STDMETHODIMP			QueryInterface(REFIID, void **);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	// IShellExtInit member
	STDMETHODIMP	Initialize(LPCITEMIDLIST, IDataObject *, HKEY);

	// IContextMenu members
	STDMETHODIMP	QueryContextMenu(HMENU, UINT, UINT, UINT, UINT);
	STDMETHODIMP	InvokeCommand(CMINVOKECOMMANDINFO *);
	STDMETHODIMP	GetCommandString(UINT,  UINT, UINT *, char *, UINT);

	// IPersistFile members
	STDMETHODIMP	GetClassID(CLSID *) {return E_FAIL;};
	STDMETHODIMP	GetCurFile(LPOLESTR *) {return E_FAIL;};
	STDMETHODIMP	IsDirty()  {return E_FAIL;};
	STDMETHODIMP	Load(LPCOLESTR, DWORD);
	STDMETHODIMP	Save(LPCOLESTR, BOOL) {return E_FAIL;};
	STDMETHODIMP	SaveCompleted(LPCOLESTR) {return S_OK;};

	// IDropTarget members
	STDMETHODIMP	DragEnter(IDataObject *, DWORD, POINTL, DWORD *);
	STDMETHODIMP	DragOver(DWORD, POINTL, DWORD *);
	STDMETHODIMP	DragLeave() {return S_OK;};
	STDMETHODIMP	Drop(IDataObject *, DWORD, POINTL, DWORD *);
};
