#include <windows.h>
#include "..\..\dzip.h"
#include "..\gui_import.h"

gui_export_t ge;
z_stream zs;

BOOL WINAPI _DllMainCRTStartup(HINSTANCE hinstDLL, DWORD reason, void *z)
{
	if (reason == DLL_PROCESS_DETACH)
		free(inblk);
	return 1;
}

void dzAddFile(char *, uInt, uInt);
void dzAddFolder_GUI(char *);
void dzBeginAdd(int);
void dzCloseFile(void);
void dzDeleteFiles(uInt *, uInt, void (*)(uInt, uInt));
void dzExpandFile(uInt);
void dzFinishedAdd(void);
void dzMakeExe(char *);
void dzOpenFile(char *);
int dzRenameFile(uInt, char *);
int dzSFXstart(void);

const gui_import_t gi = {
	1,
	"dz",
	0,
	dzAddFile,
	dzAddFolder_GUI,
	dzBeginAdd,
	dzCloseFile,
	dzDeleteFiles,
	dzExpandFile,
	dzExtractFile,
	dzFinishedAdd,
	dzMakeExe,
	dzOpenFile,
	dzRenameFile,
	dzSFXstart
};

__declspec(dllexport)
const char *GetExtensions()
{
	return gi.extlist;
}

__declspec(dllexport)
const gui_import_t *GetDllFuncs (gui_export_t *ex)
{
	ge = *ex;

	inblk = ge.malloc(32768 * 3);
	outblk = inblk + 32768;
	tmpblk = outblk + 32768 / 2;
	zbuf = tmpblk + 32768 / 2;

	zs.zalloc = ge.calloc;
	zs.zfree = free;

	return &gi;
}