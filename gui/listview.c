#include <stdio.h>
#include <windows.h>
#include "gui_import.h"
#include "common.h"
#include "file.h"
#include "listview.h"
#include "lvhelper.h"
#include "menu.h"
#include "misc.h"
#include "options.h"
#include "resource.h"
#include "sbar.h"
#include "thread.h"
#include <commctrl.h>

void ExtractDragFilesOut(void);
void ViewFile(int);

static HWND LVhdr;
static HBITMAP bmpArrow1, bmpArrow2;
static WNDPROC LVProc, LVHeaderProc;

BYTE SortType, IndentOfNewFiles, Renaming, DivDblClk;
int foldericon, lvNumEntries, parent, focus, *lvorderarray;
char folderstring[80];
lventry_t *lventries;

void LVResize(void)
{
	RECT r;
	GetClientRect(MainWnd, &r);
	MoveWindow(LView, 0, TBsize, r.right, r.bottom - (TBsize + SBsize), 1);
}

void LVSetType (lventry_t *lve)
{
	SHFILEINFO shfi;
	lventry_t *lvet = lventries;
// damn the speed of this function
	SHGetFileInfo(lve->filename, 0, &shfi, sizeof(shfi), SHGFI_SYSICONINDEX|SHGFI_TYPENAME|SHGFI_USEFILEATTRIBUTES);
	lve->icon = shfi.iIcon;
	if (!*shfi.szTypeName)
	{	// work around bug in NT4 and 2000
		char *ext = FileExtension(lve->filename);
		HINSTANCE shell = GetModuleHandle("shell32.dll");
		if (LoadString(shell, 0x2780, shfi.szDisplayName, 80) ||
			LoadString(shell, 715, shfi.szDisplayName, 80))
		{	// sfzi.szDisplayName now contains format for generic file type
			// such as "%s File" for english or "%s-fil" for Danish
			if (*ext)
			{
				LCMapString(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE,
					ext + 1, -1, shfi.szDisplayName + 80, 256);
				shfi.szDisplayName[161 - strlen(shfi.szDisplayName)] = 0;	// truncate extension if too big for 80 chars
			}
			else 
			{	// no extension, get rid of character after %s (if there is one)
				ext = strstr(shfi.szDisplayName, "%s");
				if (ext && ext[2])
					strcpy(ext + 2, ext + 3);
				shfi.szDisplayName[80] = 0;
			}

			sprintf(shfi.szTypeName, shfi.szDisplayName, shfi.szDisplayName + 80);
		}
	}
	lve->filetype = Dzip_strdup(shfi.szTypeName);
}

int LVSortStrcmp (const char *a, const char *b)
{
	return CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, a, -1, b, -1) - 2;
}

int LVqsortfunc (const int *i1, const int *i2)
{
	int result, i1indent, i2indent;
	lventry_t *lv1, *lv2;
	
	lv1 = lventries + *i1;
	lv2 = lventries + *i2;

	i1indent = lv1->status.indent;
	i2indent = lv2->status.indent;

	if (!i1indent && !i2indent)
		; // neither file is in a pak, do nothing (most likely case)
	else if (i1indent && i2indent)
	{	// both files are in a pak
		if (lv1->parent != lv2->parent)	// if files are in the same pak, do
		{	// nothing, otherwise compare based on the pak file they're in
			lv1 = lventries + lv1->parent;
			lv2 = lventries + lv2->parent;
		}
	}
	else if (i1indent)	// just i is in a pak
	{
		if (*i2 == lv1->parent)	// comparing it to the pak file it's in,
			return 1;		// it will always be after it
		lv1 = lventries + lv1->parent;	// otherwise compare based on pak it's in
	}
	else	// just j is in a pak (treat same as just i)
	{
		if (*i1 == lv2->parent)
			return -1;
		lv2 = lventries + lv2->parent;
	}

	result = lv1->status.folder - lv2->status.folder;

	switch (SortType & 0x7f)
	{
	case 0: // path + filename
		result = CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE,
			lv1->filename, lv1->filenameonly_offset,
			lv2->filename, lv2->filenameonly_offset) - 2;
		if (!result) 
			result = LVSortStrcmp(lv1->filename + lv1->filenameonly_offset,
				lv2->filename + lv2->filenameonly_offset);
		break;
	case 1: // date
		if (lv1->date > lv2->date) result = -1;
		else result = (lv1->date < lv2->date);
		break;
	case 2: // size
		if (!result)
			if (lv1->realsize > lv2->realsize) result = -1;
			else result = (lv1->realsize < lv2->realsize);
		break;
	case 3: // ratio
		if (!result)
			if (lv1->ratio > lv2->ratio) result = -1;
			else result = (lv1->ratio < lv2->ratio);
		break;
	case 4: // packed
		if (!result)
			if (lv1->compressedsize > lv2->compressedsize) result = -1;
			else result = (lv1->compressedsize < lv2->compressedsize);
		break;
	case 5: // type
		if (!result)
		{
			if (!lv1->filetype) LVSetType(lv1);
			if (!lv2->filetype) LVSetType(lv2);
			result = LVSortStrcmp(lv1->filetype, lv2->filetype);
		}
		break;
	case 6:	// filename only
		result = LVSortStrcmp(lv1->filename + lv1->filenameonly_offset,
			lv2->filename + lv2->filenameonly_offset);
		break;
	case 7:	// extension
		if (!result)
			result = LVSortStrcmp(lv1->filename + lv1->extension_offset,
				lv2->filename + lv2->extension_offset);
		break;
	case 8: // none
		result = (lv1->filepos > lv2->filepos) ? 1 : -1;
	}
	if (!result)	// never return 0, retain the original order instead
		return (lv1->lvpos > lv2->lvpos) ? 1 : -1;
	if (SortType & 0x80) return -result;
	return result;
}

void LVSort (int from, int to)
{
	int i;

	if (!to) return;

// during this time i cant let the listview ask for display info
// but its ok to add new items onto the end while this is going on
	qsort(lvorderarray + from, to - from, 4, LVqsortfunc);
	if (LVNumSelected())
		for (i = from; i < to; i++)
			LVSetItemState_Mask(i,
			lventries[lvorderarray[i]].status.selected ? LVIS_SELECTED : 0,
			LVIS_SELECTED);

	for (i = from; i < to; i++)
		lventries[lvorderarray[i]].lvpos = i;

	i = lventries[focus].lvpos;
	LVSetItemState_Mask(i, LVIS_FOCUSED, LVIS_FOCUSED);
	if (lventries[focus].status.selected)
		LVEnsureVisible(i);

	InvalidateRgn(LView, NULL, 1);	// force an update of everything now
}

int LVGetColumnFromSortType (int i)
{
	i &= 0x7f;
	if (i == 6) i = 0;	// filename only
	else if (!i && Options.PathCol) i = 6 - !Options.TypeCol; // path+filename with pathcol on
	else if (i == 5 && !Options.TypeCol) i = 6;	// type with no type col, so set it to 6 which won't exist
	return i;
}

void LVMoveSortArrow (int from, int to)
{
	int i;
	HDITEM hdi;
	hdi.mask = HDI_BITMAP|HDI_FORMAT;
// clear the old arrow
	i = LVGetColumnFromSortType(from);
	Header_GetItem(LVhdr, i, &hdi);
	hdi.fmt &= ~(HDF_BITMAP|HDF_BITMAP_ON_RIGHT);
	Header_SetItem(LVhdr, i, &hdi);

// set the new one
	i = LVGetColumnFromSortType(to);
	Header_GetItem(LVhdr, i, &hdi);
	hdi.fmt |= HDF_BITMAP|HDF_BITMAP_ON_RIGHT;
	hdi.hbm = (to & 0x80) ? bmpArrow2 : bmpArrow1;
	Header_SetItem(LVhdr, i, &hdi);
}

void LVLoadArrows(void)
{
	DeleteObject(bmpArrow1);	// these are NULL when the program starts
	DeleteObject(bmpArrow2);	// its for if a system color changes
	bmpArrow1 = LoadImage(hInstance, MAKEINTRESOURCE(IDB_ARROW1), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
	bmpArrow2 = LoadImage(hInstance, MAKEINTRESOURCE(IDB_ARROW2), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
}

void LVChangeSortIndicators (int NewSort)
{
	LVMoveSortArrow(SortType, NewSort);

// set the menu checks
	CheckMenuItem(Menu.h, ID_SORT_PATHFILENAME + (SortType & 0x7f), MF_UNCHECKED);
	CheckMenuItem(Menu.h, ID_SORT_PATHFILENAME + (NewSort & 0x7f), MF_CHECKED);
	CheckMenuItem(Menu.h, ID_SORT_REVERSE, (NewSort & 0x80) ? MF_CHECKED : MF_UNCHECKED);
}

void LVChangeSort (int NewSort)
{
	LVChangeSortIndicators(NewSort);
	SortType = NewSort;
	LVSort(0, lvNumEntries);
}

void LVFinishedAdding(void)
{
	LVSort(0, lvNumEntries);
	LVSetItemCount(lvNumEntries);
}

void LVBeginAdding (UINT numadding)
{	// this must be synchonized properly with other things going on
	SendMessage(MainWnd, WM_APP, 0, numadding + lvNumEntries);
}

void LVSetOffsets (lventry_t *lve)
{
	char *filename = lve->filename;
	char *name = GetFileFromPath(filename);

	if (!*name)
		for (name--; name > filename; name--)
			if (name[-1] == '\\')
				break;
	
	lve->filenameonly_offset = name - filename;
	lve->extension_offset = FileExtension(filename) - filename;
}

void LVAddFileToListView (char *fullfilename, UINT date, UINT realsize,
					  UINT compressedsize, UINT filepos, int expandable)
{
	lventry_t *lve;
	int folder;
	
	fullfilename = Dzip_strdup(fullfilename);	// dont trust the dlls to not muck with this

	// this will change / to \ 
	if (!CheckForValidFilename(fullfilename, 4))
		return;

	folder = (fullfilename[strlen(fullfilename) - 1] == '\\');

	lve = lventries + lvNumEntries;
	memcpy(&lve->filename, &fullfilename, 20);
	/*	that memcpy does this:
	lve->filename = fullfilename;
	lve->date = date;
	lve->realsize = realsize;
	lve->compressedsize = compressedsize;
	lve->filepos = filepos;
	*/

	lve->parent = parent;	// looked at only if indent > 0
	lve->ratio = 0;

	lvorderarray[lvNumEntries] = lvNumEntries;
	lve->status.selected = 0;
	lve->status.folder = folder;
	lve->status.expandable = expandable;
	lve->status.expanded = 0;
	lve->status.indent = IndentOfNewFiles;

	LVSetOffsets(lve);

	if (folder)
	{
		lve->icon = foldericon;
		lve->filetype = folderstring;
	}
	else
	{
		if (compressedsize)
			lve->ratio = 100 - (100 * (float)compressedsize / (float)realsize);
		else if (realsize)
			lve->ratio = 100;

		lve->filetype = NULL;	// set when its needed
	}

	// set item count to lvNumEntries + 1 then increment lvNumEntries
	// to avoid problems with sorting at 'just the right time'
	if (ThreadType == THREAD_ADD)
		LVSetItemCount(lvNumEntries + 1);

	lve->lvpos = lvNumEntries++;
}

void CollapseFile (lventry_t *lve)
{
	int i, j, k, indent, deleted, min = lvNumEntries;

	if (!lve->status.expanded) return;
	j = lve->lvpos + 1;
	indent = lve->status.indent;
	lve->status.expanded = 0;
// find the first file that's indented the same as the file we're collapsing
// and free the filenames while we go
	for (i = j; i < lvNumEntries; i++)
	{
		k = lvorderarray[i];
		if (lventries[k].status.indent == indent)
			break;
		else if (k < min)
			min = k;
		free(lventries[k].filename);
	}

	deleted = i - j;
	lvNumEntries -= deleted;
// shift the orderarray left by the amount we deleted
	memcpy(lvorderarray + j, lvorderarray + i, (lvNumEntries - j) * 4);
// adjust the .lvpos of all files after this file
// and reset their selection status
	for (; j < lvNumEntries; j++)
	{
		lve = lventries + lvorderarray[j];
		lve->lvpos -= deleted;
		LVSetItemState(j, lve->status.selected ? LVIS_SELECTED : 0);
	}
// get rid of a gap in the lventries array, then
// any lventries that were past lve+min must have their
// lvorderarray values reduced since they moved
	lve = lventries + min;
	memcpy(lve, lve + deleted, sizeof(lventry_t) * (lvNumEntries - (lve - lventries)));
	for (; lve < lventries + lvNumEntries; lve++)
		lvorderarray[lve->lvpos] -= deleted;
	LVSetItemCount(lvNumEntries);
	if (!ThreadType)	// dont do this if collapsing for a delete op
		LVEnsureVisible(lventries[focus].lvpos);
	InvalidateRgn(LView, NULL, 1);
	UpdateSBar();
}

void ExpandFile (int item)
{
	int i, j, k, numadded;
	lventry_t *lve;
	
	lve = lventries + item;
	if (lve->status.expanded) return;

	i = lvNumEntries;
	parent = item;
	lve->status.expanded = 1;
	IndentOfNewFiles = lve->status.indent + 1;
	j = lve->lvpos;	// lve may be invalid after dll call
					// because of realloc of lventries
	gi.ExpandFile(lve->filepos);

	IndentOfNewFiles = 0;
	numadded = lvNumEntries - i;
	LVSetItemCount(lvNumEntries);

// shift the orderarray right by the amount we added,
// increase the .lvpos of all items after the expanded file
// and reset their selection status
	j += numadded;
	for (i = lvNumEntries - 1; i > j; i--)
	{
		lvorderarray[i] = lvorderarray[i - numadded];
		lve = lventries + lvorderarray[i];
		lve->lvpos = i;
		LVSetItemState_Mask(i, lve->status.selected ? LVIS_SELECTED : 0, LVIS_SELECTED);
	}
// set info for the new items, then sort them
	j -= numadded;
	k = lvNumEntries - 1;
	while (i > j)
	{
		lvorderarray[i] = k--;
		lventries[lvorderarray[i]].lvpos = i;
		i--;
	}
	j++;
	LVSort(j, j + numadded);
	UpdateSBar();
}

int LVEndLabelEdit (NMLVDISPINFO *dispinfo)
{
	int i;
	char *text, *copy;
	lventry_t *lve;

	text = dispinfo->item.pszText;
	if (!text)	// pushed escape
		return 0;

	lve = lventries + lvorderarray[dispinfo->item.iItem];
	if (!CheckForValidFilename(text, lve->status.folder ? 3 : 1))
		return 0;

	// make sure folders end in a backslash
	if (lve->status.folder)
	{
		i = strlen(text);
		if (text[i - 1] != '\\')
			*(short *)&text[i] = '\\';
	}

	copy = Dzip_strdup(text);
	if (!gi.RenameFile(lve->filepos, copy))
	{
		free(copy);
		return 0;
	}
	lve->filename = Dzip_strdup(text);
	LVSetOffsets(lve);
	if (!lve->status.folder)
	{
		free(lve->filetype);
		LVSetType(lve);
	}
	ReopenFile(0);

	// update the sort if it was one of the filename sorts
	if (!(SortType & 0x7f) || (SortType & 0x7f) == 6)
		LVSort(0, lvNumEntries);
	return 1;
}

void ActionsRename(void)
{
	Renaming = 1;
	LVEditLabel(lventries[focus].lvpos);
}

void ActionsSelectAll(void)
{
	LVSetItemState_Mask(-1, LVIS_SELECTED, LVIS_SELECTED);
}

void ActionsInvert(void)
{
	int i;
	lventry_t *lve = lventries;
	for (i = 0; i < lvNumEntries; i++, lve++)
		LVSetItemState_Mask(lve->lvpos,
			lve->status.selected ? 0 : LVIS_SELECTED, LVIS_SELECTED);
}

void LVSaveState(void)
{
	int cols[7], i, j;

	j = 7;
	if (!Options.TypeCol) j--;
	if (!Options.PathCol) j--;
	ListView_GetColumnOrderArray(LView, j, cols);
	i = SortType;
	for (j = 0; j < 7; j++)
		i += cols[j] << (8 + j * 3);
	WriteRegValue(HKEY_CURRENT_USER, dzipreg, "lvorder", REG_DWORD, (char *)&i, 0, 4);
	
	for (i = 0; i < 5; i++)
		cols[i] = LVColumnWidth(i);
	WriteRegValue(HKEY_CURRENT_USER, dzipreg, "lvsize", REG_BINARY, (char *)cols, 0, 20);

	if (Options.TypeCol)
	{
		i = LVColumnWidth(5);
		WriteRegValue(HKEY_CURRENT_USER, dzipreg, "lvtype", REG_DWORD, (char *)&i, 1, 4);
	}
	if (Options.PathCol)
	{
		i = LVColumnWidth(5 + Options.TypeCol);
		WriteRegValue(HKEY_CURRENT_USER, dzipreg, "lvpath", REG_DWORD, (char *)&i, 1, 4);
	}
}

void LVGetItemString (lventry_t *lve, LVITEM *lvi)
{
	switch (lvi->iSubItem)
	{
	case 0:
	{
		int offs;
		if (!Options.PathCol ||
			(Renaming && focus == lvorderarray[lvi->iItem]))
			offs = 0;
		else
			offs = lve->filenameonly_offset;
		strcpy(lvi->pszText, lve->filename + offs);
		break;
	}
	case 1:
		if (lve->date)
			DateTimeToString(lve->date, 0, 1, lvi->pszText);
		break;
	case 2:
		if (!lve->status.folder)
			NumberToString(lve->realsize, lvi->pszText);
		break;
	case 3:
		if (!lve->status.folder)
			sprintf(lvi->pszText, "%.0f%%", lve->ratio);
		break;
	case 4:
		if (!lve->status.folder)
			NumberToString(lve->compressedsize, lvi->pszText);
		break;
	case 5:
		if (Options.TypeCol)
		{
			if (lve->filetype)
				strcpy(lvi->pszText, lve->filetype);
			break;
		}
	case 6:
		sprintf(lvi->pszText, "%.*s", lve->filenameonly_offset, lve->filename);
	}
}

void LVGetDispInfo (NMHDR *hdr)
{
	NMLVDISPINFO *lvdi = (NMLVDISPINFO *)hdr;
	LVITEM *lvi = &lvdi->item;
	lventry_t *lve = lventries + lvorderarray[lvi->iItem];

	lvi->iIndent = lve->status.indent;
	if (!(lvi->mask & LVIF_TEXT))
		return;

	if (!lve->filetype) LVSetType(lve);
	lvi->iImage = lve->icon;
	LVGetItemString(lve, lvi);
}

int LVNotification (NMHDR *hdr)
{
	int i;
	lventry_t *lve;

	switch (hdr->code)
	{
	case LVN_ITEMCHANGED:
	{
		NMLISTVIEW *nmlv = (NMLISTVIEW *)hdr;
		if (nmlv->uNewState & LVIS_FOCUSED)
			focus = lvorderarray[nmlv->iItem];
		if (!((nmlv->uOldState | nmlv->uNewState) & LVIS_SELECTED))
			return 0;	// message about focus only
		if (nmlv->iItem == -1)	// every item changed selection status
			for (i = 0; i < lvNumEntries; i++)
				lventries[i].status.selected = 
					(nmlv->uNewState & LVIS_SELECTED) ? 1 : 0;
		else
			lventries[lvorderarray[nmlv->iItem]].status.selected = 
				(nmlv->uNewState & LVIS_SELECTED) ? 1 : 0;
		UpdateSBar();
		return 0;
	}
	case LVN_ODSTATECHANGED:
	{
		NMLVODSTATECHANGE *odsc = (NMLVODSTATECHANGE *)hdr;
		for (i = odsc->iFrom; i <= odsc->iTo; i++)
			lventries[lvorderarray[i]].status.selected = 1;
		UpdateSBar();
		return 0;
	}
	case NM_DBLCLK:
		// make sure the focused item is selected for a double click
		// if its not, they must have double clicked in an empty area
		if (!lvNumEntries || !lventries[focus].status.selected)
			return 0;
	case NM_RETURN:
		lve = lventries + focus;
		if (!lve->status.expandable)
			ViewFile(1);
		else if (lve->status.expanded)
			CollapseFile(lve);
		else
			ExpandFile(focus);
		return 0;
	case LVN_COLUMNCLICK:
		i = ((NMLISTVIEW *)hdr)->iSubItem;
		if (i == 5 && !Options.TypeCol || i == 6) i = 0;	// click on path column
		else if (!i) 
			if (Options.PathCol)
				i = 6;	// click on filename means "filename only" if pathcol is on
			else  // otherwise cycle through four sort choices
				if (SortType == 0x80)
				{	LVChangeSort(6); return 0;
				}
				else if (SortType == 6)
				{	LVChangeSort(0x86); return 0;
				}
		if ((SortType & 0x7f) == i)	// reverse the current sort
			LVChangeSort(SortType ^ 0x80);
		else
			LVChangeSort(i);
		return 0;
	case LVN_BEGINLABELEDIT:
		if (!Renaming) return 1;
		return --Renaming;
	case LVN_ENDLABELEDIT:
		return LVEndLabelEdit((NMLVDISPINFO *)hdr);
	case LVN_ODFINDITEM:
	{	// sent when user types to find an item
		NMLVFINDITEM *lvfi = (NMLVFINDITEM *)hdr;
		int ofs, len = strlen(lvfi->lvfi.psz);

		for (i = lvfi->iStart--;; i++)
		{
			i %= lvNumEntries;
			if (i == lvfi->iStart)
				return -1;
			lve = lventries + lvorderarray[i];
			ofs = Options.PathCol ? lve->filenameonly_offset : 0;
			if (!strnicmp(lve->filename + ofs, lvfi->lvfi.psz, len))
				return i;
		}
	}
	case LVN_BEGINDRAG:
		if (!(gi.flags & UNSUPPORTED_EXTRACT))
			ExtractDragFilesOut();
		return 0;
	case LVN_DELETEALLITEMS:
		lve = lventries;
		for (i = 0; i < lvNumEntries; i++)
		{
			if (!lve->status.folder && lve->filetype)
				free(lve->filetype);
			free(lve->filename);
			lve++;
		}
		lvNumEntries = 0; focus = 0;
		free(lventries); lventries = NULL;
		free(lvorderarray); lvorderarray = NULL;
		return 1;
	default:
		return 0;
	}
}

int CALLBACK LVSubclassProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int i;
	lventry_t *lve = lventries;

	if (ThreadType)
		switch (msg)
		{
		case WM_CHAR:
			if (wParam == 27)	// Esc
				if (YesNo("Abort current operation?", "Dzip", MainWnd, 1))
					if (ThreadType)
						ActionsAbort();
		case WM_KEYDOWN:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:	// ignore all of these during an op
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
			return 0;
		default:
			return CallWindowProc(LVProc, hWnd, msg, wParam, lParam);
		}

// I process these here instead of LVNotification like I should because
// I'm already subclassing the thing and this is the only way to prevent
// it from beeping on the +/-/* keys
	if (msg == WM_KEYDOWN && lvNumEntries)
	switch (wParam)
	{
	case VK_ADD:
	case VK_SUBTRACT:
		lve += focus;
		if (lve->status.expandable)
			if (wParam == VK_ADD)
				ExpandFile(focus);
			else
				CollapseFile(lve);
		return 0;
	case VK_BACK:
		lve += focus;
		if (lve->status.indent)
		{	// deselect any selected items then move selection/focus to pak file
			LVSetItemState(-1, 0);
			i = lve->parent;
			i = lventries[i].lvpos;
			LVSetItemState(i, LVIS_FOCUSED|LVIS_SELECTED);
			LVEnsureVisible(i);
		}
		return 0;
	case VK_MULTIPLY:
		for (i = 0; i < lvNumEntries; i++, lve++)
			if (lve->status.expandable)
			{
				ExpandFile(i);
				lve = lventries + i;	// lventries may change!
			}
		return 0;
	}
	else if (msg == WM_CHAR)
		switch (wParam)
		{
		case '+':
		case '-':
		case '*':
			return 0;	// ignore these here because we dont know which keys they are
		}
	else if (msg == WM_NOTIFY)
	{
		NMHEADER *hdr = (NMHEADER *)lParam;
		if (hdr->hdr.code == HDN_DIVIDERDBLCLICKW)
			DivDblClk++;
		else if (DivDblClk && hdr->hdr.code == HDN_ITEMCHANGEDW)
		{	// needed for scroll bar adjustment
			DivDblClk--;
			LVScroll(0);
		}
		else if (DivDblClk && hdr->hdr.code == HDN_ITEMCHANGINGW)
		{
			LVITEM lvi;
			HDC hdc;
			SIZE sz;
			char text[260];
			int i, maxw, maxi;

			if (!lvNumEntries) return DivDblClk--;

			lvi.iSubItem = hdr->iItem;
			lvi.pszText = text;
			maxw = -1;
			hdc = GetDC(hWnd);
			for (i = 0; i < lvNumEntries; i++, lve++)
			{
				LVGetItemString(lve, &lvi);
				GetTextExtentPoint32(hdc, text, strlen(text), &sz);
				if (sz.cx > maxw)
				{
					maxw = sz.cx;
					maxi = i;
				}
			}
			ReleaseDC(hWnd, hdc);
			LVGetItemString(lventries + maxi, &lvi);
			maxw = CallWindowProc(LVProc, hWnd, LVM_GETSTRINGWIDTH, 0, (int)text);
			hdr->pitem->cxy = maxw + 12;
			if (!hdr->iItem) hdr->pitem->cxy += 12;
			return 0;
		}
	}

	return CallWindowProc(LVProc, hWnd, msg, wParam, lParam);
}

// just to set the hourglass cursor over the header :)
int CALLBACK LVHeaderSubclassProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ThreadType && msg == WM_SETCURSOR)
		return 1;
	return CallWindowProc(LVHeaderProc, hWnd, msg, wParam, lParam);
}

// whoever invented ListViews needs lots of praise for making them...
// but they could use more descriptive documentation.
// ok scratch that, the LV docs are superb compared to the docs for
// making a shell extension and OLE drag and drop
void CreateListView(void)
{
	LVCOLUMN column;
	SHFILEINFO shfi;
	int cols[7], i, j;

	LView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
		WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_SHOWSELALWAYS|LVS_SHAREIMAGELISTS|LVS_OWNERDATA|LVS_EDITLABELS,
		0, 0, CW_USEDEFAULT, CW_USEDEFAULT, MainWnd, NULL, hInstance, NULL);

	ListView_SetExtendedListViewStyle(LView, LVS_EX_HEADERDRAGDROP);
	LVResize();

	i = ReadRegValue(HKEY_CURRENT_USER, dzipreg, "lvsize", (char *)cols, 20);

	column.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;
	column.fmt = LVCFMT_LEFT;
	column.cx = i ? cols[0] : 106;
	column.pszText = "Filename";
	LVInsertColumn(&column, 0);

	column.cx = i ? cols[1] : 108;
	column.pszText = "Modified";
	LVInsertColumn(&column, 1);

	column.cx = i ? cols[2] : 72;
	column.fmt = LVCFMT_RIGHT;
	column.pszText = "Size";
	LVInsertColumn(&column, 2);

	column.cx = i ? cols[3] : 38;
	column.pszText = "Ratio";
	LVInsertColumn(&column, 3);

	column.cx = i ? cols[4] : 72;
	column.pszText = "Packed";
	LVInsertColumn(&column, 4);

	column.fmt = LVCFMT_LEFT;

	if (Options.TypeCol)
	{
		if (!ReadRegValue(HKEY_CURRENT_USER, dzipreg, "lvtype", (char *)&column.cx, 4))
			column.cx = 86;
		column.pszText = "Type";
		LVInsertColumn(&column, 5);
	}

	if (Options.PathCol)
	{
		if (!ReadRegValue(HKEY_CURRENT_USER, dzipreg, "lvpath", (char *)&column.cx, 4))
			column.cx = 90;
		column.pszText = "Path";
		LVInsertColumn(&column, 6);
	}

	if (ReadRegValue(HKEY_CURRENT_USER, dzipreg, "lvorder", (char *)&i, 4))
	{
		for (j = 0; j < 7; j++)
			cols[j] = (i >> (8 + j * 3)) & 7;
		SortType = i & 0xff;
		LVSetColumnOrder(7 - !Options.TypeCol - !Options.PathCol, cols);
	}
// get system image list, and foldericon too
	LVSetImageList(SHGetFileInfo("x", FILE_ATTRIBUTE_DIRECTORY, &shfi, sizeof(shfi),
		SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_SMALLICON|SHGFI_TYPENAME));
	foldericon = shfi.iIcon;
	strcpy(folderstring, shfi.szTypeName);

	LVhdr = LVGetHeader();
	LVLoadArrows();
	LVChangeSortIndicators(SortType);

// subclass them both
	LVProc = (WNDPROC)SetWindowLong(LView, GWL_WNDPROC, (int)LVSubclassProc);
	LVHeaderProc = (WNDPROC)SetWindowLong(LVhdr, GWL_WNDPROC, (int)LVHeaderSubclassProc);
}

void RecreateListView(void)
{
	int i, top;
	RECT r;

	top = LVTopIndex();
	LVSaveState();
	DestroyWindow(LView);
	CreateListView();
	if (lvNumEntries)
	{
		LVSetItemCount(lvNumEntries);
		for (i = 0; i < lvNumEntries; i++)
			LVSetItemState(i, lventries[lvorderarray[i]].status.selected ? LVIS_SELECTED : 0);

		LVSetItemState_Mask(lventries[focus].lvpos, LVIS_FOCUSED, LVIS_FOCUSED);
		LVItemRect(&r);
		LVScroll(top * (r.bottom - r.top));
	}
}
