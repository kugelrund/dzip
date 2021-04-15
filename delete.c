#include "dzip.h"

void dzDeleteFiles (uInt *list, uInt num, void (*Progress)(uInt, uInt))
{
	uInt i, j, m, listnum, fposdelta, dedelta, curpos, togo;
	unsigned short pakdelta;
	direntry_t *de, *nde;

	listnum = 0;
	fposdelta = 0;
	dedelta = 0;
	pakdelta = 0;
	m = list[0];	/* used for crude progress bar */

	for (i = 0; i < (uInt)numfiles; i++)
	{
		de = directory + i;

		if (de->pak)
			de->pak -= pakdelta;
		else
			pakdelta = 0;

		if (listnum < num && list[listnum] == i)
		{	/* this file is being deleted */
			listnum++;
		#ifdef GUI
			GuiProgressMsg("deleting %s", de->name);
		#endif
			printf("deleting %s\n", de->name);
			free(de->name);
			fposdelta += de->size;
			if (de->type == TYPE_PAK)
			{
				dedelta += de->pak;
				i += de->pak;
				for (j = 1; j <= de->pak; j++)
					free(de[j].name);
			}
			else if (de->pak)
			{
				pakdelta++;
				nde = de - de->pak - dedelta;
				nde->pak--;
				nde->real -= 64 + de->real;
				nde->size -= de->size;
			}
			dedelta++;
			continue;
		}

		if (!dedelta)
			continue;

		nde = de - dedelta;
		*nde = *de;
		nde->ptr -= fposdelta;

		if (!fposdelta || de->type == TYPE_DIR || de->type == TYPE_PAK)
			continue;

		curpos = de->ptr;
		togo = de->size;
		while (togo)
		{
			j = (togo > p_blocksize * 2) ? p_blocksize * 2 : togo;
			dzFile_Seek(curpos);
			dzFile_Read(inblk, j);
			dzFile_Seek(curpos - fposdelta);
			dzFile_Write(inblk, j);
			curpos += j;
			togo -= j;
		}
		if (Progress)
			Progress(i - m, numfiles - m - 1);
	}

	numfiles -= dedelta;
	directory = Dzip_realloc(directory, numfiles * sizeof(direntry_t));
	dzWriteDirectory();
	dzFile_Truncate();
}

#ifndef GUI

#include "dzipcon.h"

int intcmp (const void *arg1, const void *arg2)
{
	const uInt lhs = *(const uInt*)arg1;
	const uInt rhs = *(const uInt*)arg2;
	if (lhs < rhs) return -1;
	return 1;
}

/* create list[] array from command prompt files list */
void dzDeleteFiles_MakeList (char **files, int num)
{
	int i, j, k = num;
	direntry_t *de;
	uInt *list = Dzip_malloc(4 * num);

	for (i = 0; i < num; i++)
	{
		de = directory;
		for (j = 0; j < numfiles; j++, de++)
			if (!strcasecmp(files[i], de->name))
				break;
		if (j == numfiles)
		{
			error("%s does not contain a file named %s", dzname, files[i]);
			return;
		}
		list[i] = j;
		if (de->type == TYPE_PAK)
			k += de->pak;
	}

	if (k == numfiles) /* deleting everything */
	{
		dzClose();
		if (remove(dzname))	/* i doubt this would ever fail */
			error("unable to delete %s: %s", dzname, strerror(errno));
		else
			printf("deleted %s\n", dzname);
	}
	else
	{	// sort the list
		qsort(list, num, 4, intcmp);
		dzDeleteFiles(list, num, NULL);
		dzClose();
	}
	free(list);
}
#endif
