#include <stdio.h>
#include <windows.h>
#include "common.h"
#include "listview.h"
#include "lvhelper.h"
#include "menu.h"
#include "misc.h"
#include "options.h"
#include "recent.h"
#include "resource.h"
#include "sbar.h"
#include "thread.h"
#include <shlobj.h>

options_t Options;

void OptionsToolbar(void)
{
	Options.Toolbar ^= 1;
	if (Options.Toolbar)
		CreateToolbar();
	else
	{
		DestroyToolbar();
		TBsize = 0;
	}
	LVResize();
}

void OptionsChangeToolbarSize(void)
{
	if (Options.Toolbar)
	{
		DestroyToolbar();
		Options.BigToolbar ^= 1;
		CreateToolbar();
		LVResize();
		return;
	}
	Options.BigToolbar ^= 1;
}

void OptionsMenuIcons(void)
{
	Options.MenuIcons ^= 1;
	MenuReset(0);
}

void OptionsStatusBar(void)
{
	Options.StatusBar ^= 1;
	if (Options.StatusBar)
		CreateStatusBar();
	else
	{
		DestroyWindow(SBar);
		SBsize = 0;
	}
	LVResize();
}

void OptionsSmoothPBar()
{
	Options.SmoothPBar ^= 1;
	if (ThreadType)
	{	// cant just set the window style, the dumb thing
		// won't notice that it has changed
		DestroyWindow(PBar);
		CreatePBar();
	}
}

void OptionsShowSelected(void)
{
	Options.ShowSelected ^= 1;
	if (!ThreadType)
		UpdateSBar();
}

void OptionsTypeCol(void)
{
	Options.TypeCol ^= 1;
	if (!Options.TypeCol)
	{
		int i = LVColumnWidth(5);
		WriteRegValue(HKEY_CURRENT_USER, dzipreg, "lvtype", REG_DWORD, (char *)&i, 1, 4);
		LVDeleteColumn(5);
	}
	else
	{
		LVCOLUMN column;
		column.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;
		column.fmt = LVCFMT_LEFT;
		if (!ReadRegValue(HKEY_CURRENT_USER, dzipreg, "lvtype", (char *)&column.cx, 4))
			column.cx = 86;
		column.pszText = "Type";
		LVInsertColumn(&column, 5);
		if ((SortType & 0x7f) == 5)
			LVMoveSortArrow(SortType, SortType);
	}
	LVSaveState();
}

void OptionsPathCol(void)
{
	int i;
	Options.PathCol ^= 1;
	if (!Options.PathCol)
	{
		i = LVColumnWidth(5 + Options.TypeCol);
		WriteRegValue(HKEY_CURRENT_USER, dzipreg, "lvpath", REG_DWORD, (char *)&i, 1, 4);
		LVDeleteColumn(5 + Options.TypeCol);
	}
	else
	{
		LVCOLUMN column;
		column.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;
		column.fmt = LVCFMT_LEFT;
		if (!ReadRegValue(HKEY_CURRENT_USER, dzipreg, "lvpath", (char *)&column.cx, 4))
			column.cx = 70;
		column.pszText = "Path";
		LVInsertColumn(&column, 6);
	}
//	if sort is path+filename, we need to move the arrow
	if (!(SortType & 0x7f))
		LVMoveSortArrow(6, SortType);
	LVSaveState();
}

int CALLBACK OptionsDlgFunc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INITDIALOG)
	{
		char s[2];
		s[1] = 0;
		for (s[0] = '0'; s[0] <= '9'; s[0]++)
			SendDlgItemMessage(hDlg, IDC_RFL, CB_ADDSTRING, 0, (int)s);
		SendDlgItemMessage(hDlg, IDC_RFL, CB_SETCURSEL, Options.RFLsize, 0);
		CheckDlgButton(hDlg, IDC_TOOLBAR, Options.Toolbar);
		CheckDlgButton(hDlg, IDC_TOOLBARSMALL + Options.BigToolbar, 1);
		CheckDlgButton(hDlg, IDC_MENUICONS, Options.MenuIcons);
		CheckDlgButton(hDlg, IDC_TYPECOL, Options.TypeCol);
		CheckDlgButton(hDlg, IDC_PATHCOL, Options.PathCol);
		CheckDlgButton(hDlg, IDC_SBAR, Options.StatusBar);
		CheckDlgButton(hDlg, IDC_SELECTED, Options.ShowSelected);
		CheckDlgButton(hDlg, IDC_AUTOCLOSE, Options.AutoClose);
		CheckDlgButton(hDlg, IDC_LCFILENAMES, Options.LCFilenames);
		CheckDlgButton(hDlg, IDC_EXPCMPWDZ, Options.ExpCmpWDz);
		CheckDlgButton(hDlg, IDC_SMOOTHPBAR, Options.SmoothPBar);
		CheckDlgButton(hDlg, IDC_ALLFILES, Options.AllFilesInAdd);
		return 1;
	}
	if (msg == WM_COMMAND)
	{
		if (LOWORD(wParam) == IDOK)
		{
			int x;

			if (Options.Toolbar != IsDlgButtonChecked(hDlg, IDC_TOOLBAR))
				OptionsToolbar();
			if (Options.BigToolbar != IsDlgButtonChecked(hDlg, IDC_TOOLBARLARGE))
				OptionsChangeToolbarSize();
			if (Options.MenuIcons != IsDlgButtonChecked(hDlg, IDC_MENUICONS))
				OptionsMenuIcons();
			if (Options.TypeCol != IsDlgButtonChecked(hDlg, IDC_TYPECOL))
				OptionsTypeCol();
			if (Options.PathCol != IsDlgButtonChecked(hDlg, IDC_PATHCOL))
				OptionsPathCol();
			if (Options.StatusBar != IsDlgButtonChecked(hDlg, IDC_SBAR))
				OptionsStatusBar();
			if (Options.ShowSelected != IsDlgButtonChecked(hDlg, IDC_SELECTED))
				OptionsShowSelected();
			if (Options.SmoothPBar != IsDlgButtonChecked(hDlg, IDC_SMOOTHPBAR))
				OptionsSmoothPBar();
			if (Options.AutoClose != IsDlgButtonChecked(hDlg, IDC_AUTOCLOSE))
				Options.AutoClose ^= 1;
			if (Options.LCFilenames != IsDlgButtonChecked(hDlg, IDC_LCFILENAMES))
				Options.LCFilenames ^= 1;
			if (Options.ExpCmpWDz != IsDlgButtonChecked(hDlg, IDC_EXPCMPWDZ))
				Options.ExpCmpWDz ^= 1;
			if (Options.AllFilesInAdd != IsDlgButtonChecked(hDlg, IDC_ALLFILES))
				Options.AllFilesInAdd ^= 1;

			x = SendDlgItemMessage(hDlg, IDC_RFL, CB_GETCURSEL, 0, 0);
			if (Options.RFLsize != x)
			{
				Options.RFLsize = x;
				while (DeleteMenu(Menu.file, FILEMENULENGTH, MF_BYPOSITION));
				MenuReset(1);
				MenuUpdateRFL(NULL);
			}
			// write Options back to registry immediately
			// in case ExpCmpWDz changed or program is killed/crashes
			WriteRegValue(HKEY_CURRENT_USER, dzipreg, "options", REG_BINARY, (char *)&Options, 0, sizeof(Options));

			EndDialog(hDlg, 1);
		}
		else if (LOWORD(wParam) == IDCANCEL)
			EndDialog(hDlg, 0);
	}
	return 0;
}

void OptionsOptions(void)
{
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_OPTIONS), MainWnd, OptionsDlgFunc);
}

int CALLBACK OptionsResetDlgFunc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INITDIALOG)
	{
		CenterDialog(hDlg);
		return 1;
	}
	if (msg == WM_COMMAND)
		if (LOWORD(wParam) == IDCANCEL)
			EndDialog (hDlg, 0);
		else
		{
			char *amp;
			GetWindowText((HWND)lParam, temp2, 256);
			amp = strchr(temp2, '&');
			strcpy(amp, amp + 1);
			if (YesNo("Are you sure?", temp2, hDlg, 0))
				switch (LOWORD(wParam))
				{
				case IDC_CLEARRMENU:
					RecentClear("file");
					lParam = FILEMENULENGTH;
					if (lvNumEntries) lParam += 2;
					while (DeleteMenu(Menu.file, lParam, MF_BYPOSITION));
					MenuReset(1);
					Recent.NumFiles = !!lvNumEntries;
					break;
				case IDC_CLEARREXTRACT:
					RecentClear("path");
					Recent.NumExtractPaths = 0;
					break;
				case IDC_CLEARRMOVE:
					RecentClear("move");
					Recent.NumMovePaths = 0;
					break;
				case IDC_RESETCOLUMNS:
				{
					int cols[7] = {0, 1, 2, 3, 4, 5, 6};
					LVSetColumnOrder(7 - !Options.TypeCol - !Options.PathCol, cols);
					LVSetColumnWidth(0, 106);
					LVSetColumnWidth(1, 108);
					LVSetColumnWidth(2, 72);
					LVSetColumnWidth(3, 38);
					LVSetColumnWidth(4, 72);
					if (Options.TypeCol)
						LVSetColumnWidth(5, 86);
					else
					{
						int i = 86;
						WriteRegValue(HKEY_CURRENT_USER, dzipreg, "lvtype", REG_DWORD, (char *)&i, 1, 4);
					}
					if (Options.PathCol)
						LVSetColumnWidth(5 + Options.TypeCol, 90);
					else
					{
						int i = 90;
						WriteRegValue(HKEY_CURRENT_USER, dzipreg, "lvpath", REG_DWORD, (char *)&i, 1, 4);
					}
					LVSaveState();
					InvalidateRgn(LView, NULL, 0);
				}
				}
		}
	return 0;
}

void OptionsReset(void)
{
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_RESET), MainWnd, OptionsResetDlgFunc);
}