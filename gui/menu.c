#include <windows.h>
#include <stdio.h>
#include "gui_import.h"
#include "listview.h"
#include "common.h"
#include "menu.h"
#include "misc.h"
#include "options.h"
#include "recent.h"
#include "resource.h"
#include "thread.h"
#include <commctrl.h>

menu_t Menu;

const char * const MenuShortcut[] = {
	"Ctrl+N", "Ctrl+O", "Ctrl+F4", NULL, NULL, "Alt+F4",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	"Shift+A", "Shift+D", "Shift+E", "Shift+V", NULL,
	"F2", "Ctrl+A", "Ctrl+I", "Shift+T", "Esc", NULL
};

char FileRenameText[20], FileMoveText[20];	// buffer overflow alert

void ChangeCommand (int id, int enable)
{
	EnableMenuItem(Menu.h, id, !enable);
	SendMessage(Toolbar, TB_ENABLEBUTTON, id, enable);
}

int MenuDisabled (int menuid)
{
	return MF_GRAYED & GetMenuState(Menu.h, menuid, 0);
}

void MenuReset (int justRFL)
{
	int i;
	MENUITEMINFO miim;
	miim.cbSize = 44;
	miim.fMask = MIIM_TYPE;
	miim.dwTypeData = "";
	
	for (i = 0; i < NUMOWNERDRAWN; i++)
	{
// this crap courtesy of windows 2000 refusing to
// send another MEASUREMENU message
		miim.fType = MF_STRING;
		SetMenuItemInfo(Menu.h, i, 0, &miim);

		miim.fType = MF_OWNERDRAW;
		SetMenuItemInfo(Menu.h, i, 0, &miim);
		if (justRFL) return;
	}
	DeleteObject(Menu.hFont);
	Menu.Initialized = 0;
}

void MenuUpdateRFL (char *newfile)
{
	int i, flags;
	
	if (newfile)	// the RFL is still maintained even if they dont use it
		RecentAddString(&Recent.FileList, &Recent.NumFiles, newfile);

	if (!Options.RFLsize) return;
	
	if (!Recent.NumFiles) // user just deleted only file on the list
	{
		DeleteMenu(Menu.file, FILEMENULENGTH, MF_BYPOSITION);
		DeleteMenu(Menu.file, FILEMENULENGTH, MF_BYPOSITION);
		MenuReset(1);
		return;
	}
	flags = MF_OWNERDRAW;
	if (ThreadType == THREAD_ADD)
		flags |= MF_GRAYED;	// only if we made a new file just as we're about to add
	if (GetMenuItemCount(Menu.file) == FILEMENULENGTH) // first item on the list
		AppendMenu(Menu.file, MF_SEPARATOR, 0, 0);
	else
		while (DeleteMenu(Menu.file, FILEMENULENGTH + 1, MF_BYPOSITION));
	for (i = 0; i < Options.RFLsize && i < Recent.NumFiles; i++)
	{
		char copy[520];
		char *ptr = Recent.FileList[i];
		if (strchr(ptr, '&'))
		{	// change & into && If only this was perl!
			char *ptr2 = ptr;
			ptr = copy;
			do
			{
				if (*ptr2 == '&')
					*ptr++ = '&';
				*ptr++ = *ptr2;
			} while (*ptr2++);
			ptr = copy;
		}
		sprintf(Menu.RFLtext[i], "&%i ", i + 1);
		if (strlen(Recent.FileList[i]) < 60)
			strcpy(Menu.RFLtext[i] + 3, ptr);
		else
		{
			char *file = GetFileFromPath(ptr);
			int flen = strlen(file);
			if (flen < 57)
				sprintf(Menu.RFLtext[i] + 3, "%.*s...\\%s", 56 - flen, ptr, file);
			else if (file - Recent.FileList[i] > 7)
				sprintf(Menu.RFLtext[i] + 3, "%.3s...\\%.36s...%s", ptr, file, file + flen - 14);
			else
				sprintf(Menu.RFLtext[i] + 3, "%.43s...%s", ptr, file + flen - 14);
		}
		AppendMenu(Menu.file, flags, ID_FILE_FILE1 + i, Menu.RFLtext[i]);
	}
}

int MenuShortcutKey(int item, HMENU menu)
{
	if (item >= 'A' && item <= 'Z')
		item -= 'A' - 'a';
	if (menu == Menu.file)
		switch (item)
		{
		case 'n': return (MNC_EXECUTE << 16);
		case 'o': return (MNC_EXECUTE << 16) + 1;
		case 'c': return (MNC_EXECUTE << 16) + 2;
		case 'm': return (MNC_EXECUTE << 16) + 4;
		case 'r': return (MNC_EXECUTE << 16) + 5;
		case 'x': return (MNC_EXECUTE << 16) + 7;
		case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9':
			if (item - '0' > Recent.NumFiles || item - '0' > Options.RFLsize)
				return 0;
			return (MNC_EXECUTE << 16) + FILEMENULENGTH + item - '0';
		default: return 0;
		}
	if (menu == Menu.actions)
		switch (item)
		{
		case 'a': return (MNC_EXECUTE << 16);
		case 'd': return (MNC_EXECUTE << 16) + 1;
		case 'e': return (MNC_EXECUTE << 16) + 2;
		case 'v': return (MNC_EXECUTE << 16) + 3;
		case 'r': return (MNC_EXECUTE << 16) + 4;
		case 'x': return (MNC_EXECUTE << 16) + 5;
		case 's': return (MNC_EXECUTE << 16) + 7;
		case 'i': return (MNC_EXECUTE << 16) + 8;
		case 't': return (MNC_EXECUTE << 16) + 10;
		case 'b': return (MNC_EXECUTE << 16) + 11;
		default: return 0;
		}
	if (menu == Menu.help)
		if (item == 'a')
			return (MNC_EXECUTE << 16);
	return 0;
}

void MenuDraw (DRAWITEMSTRUCT *dis)
{
	int i;

// these need _WIN32_WINNT #defined as >= 0x500 but I dont want to do that
	#define ODS_NOACCEL     0x0100
	#define DSS_HIDEPREFIX  0x0200
	#define DT_HIDEPREFIX   0x00100000

	HDC hdc = dis->hDC;
	RECT *r = &dis->rcItem;
	char gray = (dis->itemState & ODS_GRAYED) ? 1 : 0;
	char selected = (dis->itemState & ODS_SELECTED) ? 1 : 0;
	char id = dis->itemID;
	char noprefix = (dis->itemState & ODS_NOACCEL) ? 1 : 0;
	HICON hIcon = Options.MenuIcons ? Menu.hIcon[id] : NULL;
	int len = r->right - r->left;

	if (hIcon)
	{
		short y = 0;
		if (Menu.ysize > 18)
			y = (Menu.ysize - 17) / 2;
		i = DST_ICON;
		if (gray) i |= DSS_DISABLED;
		DrawState(hdc, NULL, NULL, (long)hIcon, 0, r->left + 1, r->top + y, 16, 16, i);
		r->left += 18;
	}

	if (selected)
	{
		FillRect(hdc, r, (HBRUSH)(COLOR_HIGHLIGHT + 1));
		SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
		i = (gray ? COLOR_GRAYTEXT : COLOR_HIGHLIGHTTEXT); 
		SetTextColor(hdc, GetSysColor(i));
	}
	else if (dis->itemAction & ODA_SELECT)
		FillRect(hdc, r, (HBRUSH)(COLOR_MENU + 1));

	if (hIcon)
	{
		r->left += 3;
		if (Menu.ysize < 17)
			r->top += (20 - Menu.ysize) / 3;
	}
	else
		r->left += 21;

	r->top++;

	if (!gray || selected)
		DrawText(hdc, (char *)dis->itemData, -1, r, DT_NOCLIP | (noprefix ? DT_HIDEPREFIX : 0));
	else
		DrawState(hdc, NULL, NULL, dis->itemData, 0, r->left,
		r->top, len, 0, DST_PREFIXTEXT|DSS_DISABLED | (noprefix ? DSS_HIDEPREFIX : 0));

	if (MenuShortcut[id])
	{
		r->left += (id < ID_ACTIONS_ADD) ? Menu.fileSCofs : Menu.actionsSCofs;
		if (!gray || selected)
			DrawText(hdc, MenuShortcut[id], -1, r, DT_NOCLIP);
		else
			DrawState(hdc, NULL, NULL, (long)MenuShortcut[id], 0, r->left,
				r->top, len, 0, DST_PREFIXTEXT|DSS_DISABLED);
	}
}

// file menu width is determined by "Rename dz..." or recent file list
// actions menu by Abort Operation+shortcut
void MenuMeasure (MEASUREITEMSTRUCT *mis)
{
	SIZE size;
	HDC hdc = GetDC(MainWnd);
	short id = mis->itemID;

	if (!Menu.Initialized)
	{
		NONCLIENTMETRICS ncm;
		ncm.cbSize = sizeof(ncm);
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
		Menu.hFont = CreateFontIndirect(&ncm.lfMenuFont);
		Menu.Initialized++;
		SelectObject(hdc, Menu.hFont);
		GetTextExtentPoint32(hdc, " ", 1, &size);
		Menu.ysize = size.cy + 4;
		GetTextExtentPoint32(hdc, "Ctrl+F4", 7, &size);
		Menu.ExtentAF4 = (short)size.cx;
		GetTextExtentPoint32(hdc, "Rename dz...   ", 15, &size);
		Menu.ExtentRename = (short)size.cx;
	}
	else
		SelectObject(hdc, Menu.hFont);

	if (Menu.hIcon[id] && Menu.ysize < 18 && Options.MenuIcons)
		mis->itemHeight = 18;
	else
		mis->itemHeight = Menu.ysize;

	if (id == ID_FILE_NEW)
	{
		Menu.fileSCofs = Menu.ExtentRename;
		mis->itemWidth = Menu.ExtentRename + Menu.ExtentAF4 + 9;
	}
	else if (id == ID_FILE_FILE1)
	{	// measure the whole RFL to find the largest
		Menu.fileSCofs = Menu.ExtentRename;
		mis->itemWidth = Menu.ExtentRename + Menu.ExtentAF4 + 9;
		for (id = 0; id < Recent.NumFiles && id < Options.RFLsize; id++)
		{
			GetTextExtentPoint32(hdc, Menu.RFLtext[id] + 1, strlen(Menu.RFLtext[id] + 1), &size);
			if (size.cx + 9 > (int)mis->itemWidth)
				mis->itemWidth = size.cx + 9;
			size.cx = size.cx - Menu.ExtentAF4;
			if (size.cx > Menu.fileSCofs)
				Menu.fileSCofs = (short)size.cx;
		}
	}
	else if (id < ID_ACTIONS_ABORT)
		mis->itemWidth = 1;
	else if (id == ID_ACTIONS_ABORT)
	{
		GetTextExtentPoint32(hdc, "Abort Operation   ", 18, &size);
		Menu.actionsSCofs = (short)size.cx;
		GetTextExtentPoint32(hdc, "Shift+V", 7, &size);
		mis->itemWidth = size.cx + Menu.actionsSCofs + 9;
	}
	else
	{
		GetTextExtentPoint32(hdc, "About...", 8, &size);
		mis->itemWidth = size.cx + 30;
	}

	ReleaseDC(MainWnd, hdc);
}

void MenuActivateCommands(void)
{
	int i;
	int v = lvNumEntries && !ThreadType;	// true only if a file is open
											// and there is no thread running
	Menu.ExitText[5] = ThreadType ? '.' : 0;
	ChangeCommand(ID_FILE_NEW, !ThreadType);
	ChangeCommand(ID_FILE_OPEN, !ThreadType);
	ChangeCommand(ID_FILE_CLOSE, v);
	ChangeCommand(ID_FILE_MOVE, v);
	ChangeCommand(ID_FILE_RENAME, v);
// ID_FILE_EXIT is always enabled
	for (i = ID_FILE_FILE1; i < ID_ACTIONS_ADD; i++)
		ChangeCommand(i, !ThreadType);

	ChangeCommand(ID_ACTIONS_ADD, v && !ReadOnly && !(gi.flags & UNSUPPORTED_ADD));
	ChangeCommand(ID_ACTIONS_DELETE, v && !ReadOnly && !(gi.flags & UNSUPPORTED_DELETE));
	ChangeCommand(ID_ACTIONS_EXTRACT, v && !(gi.flags & UNSUPPORTED_EXTRACT));
	ChangeCommand(ID_ACTIONS_VIEW, v && !(gi.flags & UNSUPPORTED_EXTRACT));
	ChangeCommand(ID_ACTIONS_MAKEEXE, v && !(gi.flags & UNSUPPORTED_MAKEEXE));
	ChangeCommand(ID_ACTIONS_RENAME, v && !ReadOnly && !(gi.flags & UNSUPPORTED_RENAME));
	ChangeCommand(ID_ACTIONS_SELECTALL, v);
	ChangeCommand(ID_ACTIONS_INVERT, v);
	ChangeCommand(ID_ACTIONS_TEST, v && !(gi.flags & UNSUPPORTED_TEST));
		
	ChangeCommand(ID_ACTIONS_ABORT, // activated only if there is a thread and its not deleting
		ThreadType != THREAD_NONE && ThreadType != THREAD_DELETE);

	// true if no open file
	DragAcceptFiles(MainWnd, !ThreadType && !ReadOnly && !(gi.flags & UNSUPPORTED_ADD));

	// update file move/rename text based on current extension
	if (!lvNumEntries)
	{
		strcpy(FileMoveText, "&Move ...");
		strcpy(FileRenameText, "&Rename ...");
	}
	else
	{
		char *ext = 1 + FileExtension(Recent.FileList[0]);
		sprintf(FileMoveText + 6, "%s...", ext);
		sprintf(FileRenameText + 8, "%s...", ext);
	}
}

void SortByPathFilename(void)	{LVChangeSort(SortType & 0x80);}
void SortByModified(void)	{LVChangeSort((SortType & 0x80) | 1);}
void SortBySize(void)		{LVChangeSort((SortType & 0x80) | 2);}
void SortByRatio(void)		{LVChangeSort((SortType & 0x80) | 3);}
void SortByPacked(void)		{LVChangeSort((SortType & 0x80) | 4);}
void SortByType(void)		{LVChangeSort((SortType & 0x80) | 5);}
void SortByFilename(void)	{LVChangeSort((SortType & 0x80) | 6);}
void SortByExtension(void)	{LVChangeSort((SortType & 0x80) | 7);}
void SortByNone(void)		{LVChangeSort((SortType & 0x80) | 8);}
void SortReverse(void)		{LVChangeSort(SortType ^ 0x80);}

void (* const MenuFunc[NUMMENU])(void) = {
	FileNew, FileOpen, FileClose, FileMove, FileRename, FileExit,
	FileRecentFile1, FileRecentFile2, FileRecentFile3, FileRecentFile4, 
	FileRecentFile5, FileRecentFile6, FileRecentFile7, FileRecentFile8, 
	FileRecentFile9, ActionsAdd, ActionsDelete,
	ActionsExtract, ActionsView, ActionsMakeExe, ActionsRename,
	ActionsSelectAll, ActionsInvert, ActionsTest, ActionsAbort,
	HelpAbout, SortByPathFilename, SortByModified, SortBySize,
	SortByRatio, SortByPacked, SortByType, SortByFilename,
	SortByExtension, SortByNone, SortReverse, OptionsOptions,
	OptionsFileTypes, OptionsReset, OptionsUninstall
};

const BYTE MenuIconList[] = {
	ID_FILE_NEW, ID_FILE_OPEN, ID_ACTIONS_ADD, ID_ACTIONS_DELETE,
	ID_ACTIONS_EXTRACT, ID_ACTIONS_VIEW, ID_ACTIONS_TEST,
	ID_ACTIONS_ABORT, ID_HELP_ABOUT, ID_FILE_EXIT, ID_ACTIONS_MAKEEXE
};

void CreateMenuBar(void)
{
	HIMAGELIST imgl;
	HMENU options, sort;
	HBITMAP bmp;
	int i;

	bmp = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TOOLBAR));
	imgl = ImageList_Create(16, 16, ILC_MASK|ILC_COLORDDB, 11, 0);
	ImageList_AddMasked(imgl, bmp, 0xc0c0c0);
	DeleteObject(bmp);

	Menu.file = CreatePopupMenu();
	AppendMenu(Menu.file, MF_OWNERDRAW, ID_FILE_NEW, "&New...");
	AppendMenu(Menu.file, MF_OWNERDRAW, ID_FILE_OPEN, "&Open...");
	AppendMenu(Menu.file, MF_OWNERDRAW, ID_FILE_CLOSE, "&Close");
	AppendMenu(Menu.file, MF_SEPARATOR, 0, 0);
	AppendMenu(Menu.file, MF_OWNERDRAW, ID_FILE_MOVE, FileMoveText);
	AppendMenu(Menu.file, MF_OWNERDRAW, ID_FILE_RENAME, FileRenameText);
	AppendMenu(Menu.file, MF_SEPARATOR, 0, 0);
	AppendMenu(Menu.file, MF_OWNERDRAW, ID_FILE_EXIT, Menu.ExitText);
	memcpy(Menu.ExitText, "E&xit\0..", 9);

	Menu.actions = CreatePopupMenu();
	AppendMenu(Menu.actions, MF_OWNERDRAW, ID_ACTIONS_ADD, "&Add...");
	AppendMenu(Menu.actions, MF_OWNERDRAW, ID_ACTIONS_DELETE, "&Delete...");
	AppendMenu(Menu.actions, MF_OWNERDRAW, ID_ACTIONS_EXTRACT, "&Extract...");
	AppendMenu(Menu.actions, MF_OWNERDRAW, ID_ACTIONS_VIEW, "&View...");
	AppendMenu(Menu.actions, MF_OWNERDRAW, ID_ACTIONS_RENAME, "&Rename");
	AppendMenu(Menu.actions, MF_OWNERDRAW, ID_ACTIONS_MAKEEXE, "Make E&xe...");
	AppendMenu(Menu.actions, MF_SEPARATOR, 0, 0);
	AppendMenu(Menu.actions, MF_OWNERDRAW, ID_ACTIONS_SELECTALL, "&Select All");
	AppendMenu(Menu.actions, MF_OWNERDRAW, ID_ACTIONS_INVERT, "&Invert Selection");
	AppendMenu(Menu.actions, MF_SEPARATOR, 0, 0);
	AppendMenu(Menu.actions, MF_OWNERDRAW, ID_ACTIONS_TEST, "&Test");
	AppendMenu(Menu.actions, MF_OWNERDRAW, ID_ACTIONS_ABORT, "A&bort Operation");

	sort = CreatePopupMenu();
	AppendMenu(sort, MF_STRING, ID_SORT_PATHFILENAME, "Path + &Filename");
	AppendMenu(sort, MF_STRING, ID_SORT_FILENAME, "Filename &only");
	AppendMenu(sort, MF_STRING, ID_SORT_MODIFIED, "&Modified");
	AppendMenu(sort, MF_STRING, ID_SORT_SIZE, "&Size");
	AppendMenu(sort, MF_STRING, ID_SORT_RATIO, "&Ratio");
	AppendMenu(sort, MF_STRING, ID_SORT_PACKED, "&Packed");
	AppendMenu(sort, MF_STRING, ID_SORT_TYPE, "&Type");
	AppendMenu(sort, MF_STRING, ID_SORT_EXTENSION, "E&xtension");
	AppendMenu(sort, MF_STRING, ID_SORT_NONE, "&None");
	AppendMenu(sort, MF_SEPARATOR, 0, 0);
	AppendMenu(sort, MF_STRING, ID_SORT_REVERSE, "Re&verse Sort");

	options = CreatePopupMenu();
	AppendMenu(options, MF_POPUP|MF_STRING, (int)sort, "&Sort by");
	AppendMenu(options, MF_STRING, ID_OPTIONS_OPTIONS, "Dzip &Options...");
	AppendMenu(options, MF_STRING, ID_OPTIONS_FILETYPES, "&File Types...");
	AppendMenu(options, MF_STRING, ID_OPTIONS_RESET, "&Reset Settings...");
	AppendMenu(options, MF_SEPARATOR, 0, 0);
	AppendMenu(options, MF_STRING, ID_OPTIONS_UNINSTALL, "&Uninstall...");

	Menu.help = CreatePopupMenu();
	AppendMenu(Menu.help, MF_OWNERDRAW, ID_HELP_ABOUT, "&About...");

	Menu.h = CreateMenu();
	AppendMenu(Menu.h, MF_POPUP, (int)Menu.file, "&File");
	AppendMenu(Menu.h, MF_POPUP, (int)Menu.actions, "&Actions");
	AppendMenu(Menu.h, MF_POPUP, (int)options, "&Options");
	AppendMenu(Menu.h, MF_POPUP, (int)Menu.help, "&Help");

	if (Recent.NumFiles) MenuUpdateRFL(0);
	SetMenu(MainWnd, Menu.h);

	Menu.ToolTips[ID_FILE_NEW] = "Create a new file";
	Menu.ToolTips[ID_FILE_OPEN] = "Open an existing file";
	Menu.ToolTips[ID_ACTIONS_ADD] = "Add files to archive";
	Menu.ToolTips[ID_ACTIONS_DELETE] = "Delete files from archive";
	Menu.ToolTips[ID_ACTIONS_EXTRACT] = "Extract files to disk";
	Menu.ToolTips[ID_ACTIONS_VIEW] = "View selected file";
	Menu.ToolTips[ID_ACTIONS_TEST] = "Test archive for errors";
	Menu.ToolTips[ID_ACTIONS_ABORT] = "Abort current operation";

	for (i = 0; i < sizeof(MenuIconList); i++)
		Menu.hIcon[MenuIconList[i]] = ImageList_GetIcon(imgl, i, ILD_NORMAL);

	ImageList_Destroy(imgl);
	MenuActivateCommands();
}