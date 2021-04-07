#include <windows.h>
#include "common.h"
#include "listview.h"
#include "menu.h"
#include "misc.h"
#include "options.h"
#include "thread.h"
#include "recent.h"
#include "sbar.h"
#include <commctrl.h>

void AddDroppedFiles(HDROP);

long CALLBACK WndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NOTIFY)
	{	// fast check for listview needing stuff to display
		NMHDR *hdr = (NMHDR *)lParam;
		if (hdr->code == LVN_GETDISPINFO)
			LVGetDispInfo(hdr);
		else if (hdr->hwndFrom == LView)
			return LVNotification(hdr);
		else if (hdr->code == TTN_NEEDTEXT)
		{
			((NMTTDISPINFO *)hdr)->lpszText = Menu.ToolTips[hdr->idFrom];
			((NMTTDISPINFO *)hdr)->uFlags |= TTF_DI_SETITEM;
		}
		return 0;
	}

	switch (msg)
	{
	case WM_SETCURSOR:
		if (!ThreadType)
			break;
		SetCursor(LoadCursor(NULL, IDC_WAIT));
		return 1;
	case WM_DRAWITEM:
		MenuDraw((void *)lParam);
		return 1;
	case WM_MEASUREITEM:
		MenuMeasure((void *)lParam);
		return 1;
	case WM_COMMAND:
		if (!HIWORD(wParam) || HIWORD(wParam) == 1)
			MenuFunc[LOWORD(wParam)]();
		return 0;
	case WM_ACTIVATE:
		SetFocus(LView);
		return 0;
	case WM_SIZE:
		SendMessage(SBar, WM_SIZE, wParam, lParam);
		SendMessage(Toolbar, TB_AUTOSIZE, 0, 0);
		LVResize();
		SBarResize();
		return 0;
	case WM_CLOSE:
		FileExit();
		return 0;
	case WM_DROPFILES:
		AddDroppedFiles((HDROP)wParam);
		return 0;
	case WM_CONTEXTMENU:
		if (!lvNumEntries) return 0;

		if (lParam == -1)
		{	// pressed menu key
			RECT r;
			GetWindowRect(LView, &r);
			lParam = r.left + (r.top << 16);
		}
		TrackPopupMenuEx(Menu.actions, TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RIGHTBUTTON|TPM_NONOTIFY,
			LOWORD(lParam), HIWORD(lParam), MainWnd, NULL);
		return 0;
	case WM_MENUCHAR:
		return MenuShortcutKey(LOWORD(wParam), (HMENU)lParam);
	case WM_APP:
		lventries = Dzip_realloc(lventries, sizeof(lventry_t) * lParam);
		lvorderarray = Dzip_realloc(lvorderarray, 4 * lParam);
		return 0;
//	case WM_APP + 1:// broadcast from another Dzip process
//		RecentSynchronize(wParam, lParam);
//		return 0;
	case WM_SYSCOLORCHANGE:
		LVLoadArrows();
		LVMoveSortArrow(SortType, SortType);

		if (Options.BigToolbar && Options.Toolbar)
		{	// this is to change the colors in the disabled imglist
			DestroyToolbar();
			CreateToolbar();
		}
		return 0;
	case WM_SETTINGCHANGE:
		if (wParam == SPI_SETNONCLIENTMETRICS)
		{
			if (hThread) SuspendThread(hThread);
			MenuReset(0);
			if (Options.StatusBar)
			{
				DestroyWindow(SBar);
				CreateStatusBar();
			}
			RecreateListView();
			if (hThread) ResumeThread(hThread);
		}
		SetNumberFormat();
		InvalidateRgn(LView, NULL, 1);
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void MsgLoop(void)
{
	extern HACCEL hacl;
	MSG msg;

	GetMessage(&msg, NULL, 0, 0);
	if (msg.hwnd == LView && TranslateAccelerator(MainWnd, hacl, &msg))
		return;
	TranslateMessage(&msg);
	DispatchMessage(&msg);
}