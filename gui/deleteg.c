#include <windows.h>
#include "common.h"
#include "gui_import.h"
#include "lvhelper.h"
#include "listview.h"
#include "misc.h"
#include "menu.h"
#include "thread.h"
#include "recent.h"

void PBarGenericProgress (UINT, UINT);
void FileRecentFile(int);

void DeleteCurrentFile(void)
{
	int i;
	FileClose();
	DeleteFile(Recent.FileList[0]);
	Recent.NumFiles--;
	for (i = 0; i < Recent.NumFiles; i++)
		strcpy(Recent.FileList[i], Recent.FileList[i + 1]);
	MenuUpdateRFL(NULL);
}

int intcmp (const UINT *arg1, const UINT *arg2)
{
	if (*arg1 < *arg2) return -1;
	return 1;
}

void DeleteThread(void)
{
	int i, j, numsel;
	UINT *array;
	lventry_t *lve = lventries;

	// first thing to do is collapse any files that are selected
	for (i = 0; i < lvNumEntries; i++, lve++)
		if (lve->status.expanded && lve->status.selected)
			CollapseFile(lve);

	numsel = LVNumSelected();
	if (lvNumEntries == numsel)
	{	// they deleted them all!
		DeleteCurrentFile();
		return;
	}

	// make an array of all the filepos that are selected for delete
	array = Dzip_malloc(4 * numsel);
	lve = lventries;
	j = 0;
	for (i = 0; i < lvNumEntries; i++, lve++)
		if (lve->status.selected)
			array[j++] = lve->filepos;

// sort the array
	qsort(array, j, 4, intcmp);
	gi.DeleteFiles(array, j, PBarGenericProgress);

	FileRecentFile(0);
	free(array);
}

void ActionsDelete(void)
{
	if (LVNumSelected())
		if (YesNo("Just making sure you really\nwant to delete those...", "Delete confirmation", MainWnd, 0))
			RunThread(DeleteThread, THREAD_DELETE);
}