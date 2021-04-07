#include "sfx.h"
#include "..\..\dzip.h"

void dzFinishedAdd(void)
{
	dzWriteDirectory();
	dzFile_Truncate();
}

// cleans up stuff from the currently open file when it's closed
void dzCloseFile(void)
{
	direntry_t *de = directory;
	int i;

	for (i = 0; i < numfiles; i++, de++)
		free(de->name);
	free(directory); directory = NULL;

	numfiles = 0;
}

void dzExpandFile (uInt filepos)
{
	direntry_t *de;
	uInt i, num;
	
	de = directory + filepos;
	num = de->pak;
	ge.LVBeginAdding(num);
	for (i = 0; i < num; i++)
	{
		de++;
		ge.LVAddFileToListView(de->name, 0, de->real,
			de->size, de - directory, 0);
	}
}

void dzOpenFile (char *fname)
{
	int i;
	direntry_t *de;

	if (!dzReadDirectory(fname))
		return;

	if (maj_ver == 1)
	{
		error("Version 1 .dz files are currently not supported\nPlease use v2.6 or the command line version");
		dzCloseFile();
		return;
	}

	de = directory;
	ge.LVBeginAdding(numfiles);
	for (i = 0; i < numfiles; i++, de++)
		if (!de->pak || de->type == TYPE_PAK)
			ge.LVAddFileToListView(de->name, de->date, de->real,
				de->size, i, de->type == TYPE_PAK);
}

int dzRenameFile (uInt filepos, char *text)
{
	direntry_t *de;
	int tlen;

	tlen = strlen(text);
	de = directory + filepos;

	if (de->pak && de->type != TYPE_PAK)
	{	// change all back slash to forward slash for inside pak
		char *ptr;
		for (ptr = text; *ptr; ptr++)
			if (*ptr == '\\')
				*ptr = '/';
		if (tlen > 55)
		{
			error("Filename is too long (no longer than 55 chars inside a .pak)");
			return 0;
		}
	}

	free(de->name);
	de->name = text;
	de->len = tlen + 1;
	dzFinishedAdd();
	return 1;
}

void dzBeginAdd (int newfile)
{
	if (newfile)
	{
		int i = 'D' + ('Z' << 8) + (MAJOR_VERSION<<16) + (MINOR_VERSION<<24);
		dzFile_Write(&i, 12);	// write i into the first int, then garbage into next 2
		totalsize = 12;
		numfiles = 0;
	}
	else
	{
		dzFile_Seek(4);
		dzFile_Read(&totalsize, 4);
		dzFile_Seek(totalsize);
	}
}

void dzAddFolder_GUI (char *name)
{
	dzAddFolder(name);
	ge.LVAddFileToListView(directory[numfiles - 1].name, 0, 0, 0,
		numfiles - 1, 0);
}

void dzAddFile (char *name, uInt filesize, uInt filedate)
{
	direntry_t *de;

	int oldn = numfiles;
	GuiProgressMsg("compressing %s", name);
	dzCompressFile(name, filesize, filedate);
	if (numfiles == oldn) return;
	de = directory + numfiles - 1;
	de -= de->pak;
	ge.LVAddFileToListView(de->name, de->date, de->real, de->size,
		de - directory, de->type == TYPE_PAK);
}

void dzMakeExe (char *exepath)
{
	uInt filelen, size;

	filelen = dzFile_Size() - 4;
	dzFile_Seek(4);

	inflateInit(&zs);
	zs.avail_in = SFX_COMPRESSED_SIZE;
	zs.next_in = (char *)sfx;
	zs.avail_out = SFX_REAL_SIZE;
	zs.next_out = inblk;
	inflate(&zs, Z_NO_FLUSH);
	inflateEnd(&zs);

	strcpy(inblk + (SFX_REAL_SIZE - 0xc00), exepath);
	Outfile_Write(inblk, SFX_REAL_SIZE);

	while (filelen && !AbortOp)
	{
		size = (filelen > p_blocksize * 2) ? p_blocksize * 2 : filelen;
		dzFile_Read(inblk, size);
		Outfile_Write(inblk, size);
		filelen -= size;
	}
}

int dzSFXstart(void)
{
	return SFX_REAL_SIZE - 4;
}