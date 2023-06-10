#include <windows.h>
typedef unsigned int uInt;
#include "..\gui_export.h"
#include "..\gui_import.h"

gui_export_t ge;
void *buffer;
uInt numfiles;

struct {
	int id;
	uInt offset;
	uInt size;
} pakheader;

typedef struct {
	char name[56];
	uInt ptr;
	uInt len;
} pakentry_t;
pakentry_t *directory;

BOOL WINAPI _DllMainCRTStartup(HINSTANCE hinstDLL, DWORD reason, void *z)
{
	if (reason == DLL_PROCESS_DETACH)
		free(buffer);
	return 1;
}

void pakAddFile (char *name, uInt filesize, uInt filedate)
{
	pakentry_t *pe;
	uInt togo, csize;
	char *ptr;

	if (strlen(name) > 55)
	{
		ge.error("%s\nis too long of a filename for a pak file", name);
		return;
	}

	ge.GuiProgressMsg("adding %s", name);
	togo = filesize;
	while (togo)
	{
		if (*ge.AbortOp)
			return;
		csize = (togo > 65536) ? 65536 : togo;
		ge.Infile_Read(buffer, csize);
		ge.ArchiveFile_Write(buffer, csize);
		togo -= csize;
	}

	directory = ge.realloc(directory, 64 * (numfiles + 1));
	pe = directory + numfiles;
	memset(pe->name, 0, 56);
	strcpy(pe->name, name);
	// change all back slash to forward slash
	ptr = pe->name;
	while (*ptr++)
		if (*ptr == '\\')
			*ptr = '/';
	pe->ptr = pakheader.offset;
	pe->len = filesize;
	pakheader.offset += filesize;
	ge.LVAddFileToListView(pe->name, 0, filesize, filesize, numfiles++, 0);
}

void pakAddFolder (char *name)
{	// void
}

void pakBeginAdd (int newfile)
{
	if (newfile)
	{
		pakheader.id = ('P' + ('A' << 8) + ('C' << 16) + ('K' << 24));
		pakheader.offset = 12;
		numfiles = 0;
		ge.ArchiveFile_Write(&pakheader, 12);
	}
	else
		ge.ArchiveFile_Seek(pakheader.offset);
}

void pakFinishedAdd(void)
{
	pakheader.size = numfiles * 64;
	ge.ArchiveFile_Seek(0);
	ge.ArchiveFile_Write(&pakheader, 12);
	ge.ArchiveFile_Seek(pakheader.offset);
	ge.ArchiveFile_Write(directory, numfiles * 64);
	ge.ArchiveFile_Truncate();
}

void pakCloseFile(void)
{
	free(directory);
}

void pakDeleteFiles (uInt *list, uInt num, void (*Progress)(uInt, uInt))
{
	uInt i, j, m, listnum, fposdelta, pedelta, curpos, togo;
	pakentry_t *pe, *npe;

	listnum = 0;
	fposdelta = 0;
	pedelta = 0;
	m = list[0];	/* used for crude progress bar */

	for (i = 0; i < numfiles; i++)
	{
		pe = directory + i;

		if (listnum < num && list[listnum] == i)
		{	/* this file is being deleted */
			listnum++;
			ge.GuiProgressMsg("deleting %s", pe->name);
			fposdelta += pe->len;
			pedelta++;
			continue;
		}

		if (!pedelta)
			continue;

		npe = pe - pedelta;
		*npe = *pe;
		npe->ptr -= fposdelta;

		if (!fposdelta)
			continue;

		curpos = pe->ptr;
		togo = pe->len;
		while (togo)
		{
			j = (togo > 65536) ? 65536 : togo;
			ge.ArchiveFile_Seek(curpos);
			ge.ArchiveFile_Read(buffer, j);
			ge.ArchiveFile_Seek(curpos - fposdelta);
			ge.ArchiveFile_Write(buffer, j);
			curpos += j;
			togo -= j;
		}
		if (Progress)
			Progress(i - m, numfiles - m - 1);
	}

	numfiles -= pedelta;
	directory = ge.realloc(directory, numfiles * 64);
	pakheader.offset -= fposdelta;
	pakFinishedAdd();
	ge.ArchiveFile_Truncate();
}

void pakExtractFile (uInt filepos, int testing)
{
	uInt togo, csize;
	pakentry_t *pe = directory + filepos;
	ge.ArchiveFile_Seek(pe->ptr);
	ge.GuiProgressMsg("extracting %s", pe->name);

	togo = pe->len;
	while (togo && !*ge.AbortOp)
	{
		if (*ge.AbortOp)
			return;
		csize = (togo > 65536) ? 65536 : togo;
		ge.ArchiveFile_Read(buffer, csize);
		ge.Outfile_Write(buffer, csize);
		togo -= csize;
	}
}

void pakOpenFile (char *name)
{
	uInt i, filesize;
	pakentry_t *pe;
	
	filesize = ge.ArchiveFile_Size();
	if (filesize < 12)
	{
		ge.error("%s is not a valid pak file", name);
		return;
	}

	ge.ArchiveFile_Read(&pakheader, 12);
	if (pakheader.id != ('P' + ('A' << 8) + ('C' << 16) + ('K' << 24))
		|| pakheader.offset + pakheader.size != filesize
		|| (pakheader.size & 0x3f))
	{
		ge.error("%s is not a valid pak file", name);
		return;
	}

	ge.ArchiveFile_Seek(pakheader.offset);
	numfiles = pakheader.size / 64;
	directory = ge.malloc(numfiles * 64);
	ge.ArchiveFile_Read(directory, numfiles * 64);
	ge.LVBeginAdding(numfiles);

	pe = directory;
	for (i = 0; i < numfiles; i++, pe++)
	{
		if (!pe->name[0])
		{
			ge.error("Invalid block in file contents");
			return;
		}
		ge.LVAddFileToListView(pe->name, 0, pe->len, pe->len, i, 0);
	}
}

int pakRenameFile (uInt filepos, char *name)
{
	pakentry_t *pe;
	int tlen;

	tlen = strlen(name);
	pe = directory + filepos;

	if (tlen > 55)
	{
		ge.error("Filename is too long (55 chars max)");
		return 0;
	}

	memset(pe->name, 0, 56);
	strcpy(pe->name, name);
	free(name);

	// change all back slash to forward slash
	for (name = pe->name; *name; name++)
		if (*name == '\\')
			*name = '/';

	pakFinishedAdd();
	return 1;
}

const gui_import_t gi = {
	1,
	"pak",
	UNSUPPORTED_TEST,
	pakAddFile,
	pakAddFolder,
	pakBeginAdd,
	pakCloseFile,
	pakDeleteFiles,
	NULL,
	pakExtractFile,
	pakFinishedAdd,
	pakOpenFile,
	pakRenameFile
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
	buffer = ge.malloc(65536);
	return &gi;
}