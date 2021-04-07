#include <windows.h>
#include "..\external\zlib\zlib.h"
#include "common.h"
#include "menu.h"
#include "options.h"
#include "resource.h"
#include <commctrl.h>

HBITMAP tbbmp;
HIMAGELIST hdimgl;
HWND Toolbar;
short TBsize;

extern HINSTANCE hInstance;
extern HWND MainWnd;

// why the hell does GetSysColor return stuff in 0x00BBGGRR
// and a bitmap palette wants it in 0x00RRGGBB?  Stupid MS!
__declspec(naked)
int __fastcall BGRSysColor (int color)
{
	__asm {
		push ecx		// ecx = color
		call dword ptr[GetSysColor]
		bswap eax
		shr eax, 8
		ret
	}
}

void DestroyToolbar(void)
{
	if (Options.BigToolbar)
		ImageList_Destroy(hdimgl);
	DeleteObject(tbbmp);
	DestroyWindow(Toolbar);
}

const char tbcmds[8] = {
	ID_FILE_NEW, ID_FILE_OPEN,
	ID_ACTIONS_ADD, ID_ACTIONS_DELETE,
	ID_ACTIONS_EXTRACT, ID_ACTIONS_VIEW,
	ID_ACTIONS_TEST, ID_ACTIONS_ABORT
};

void CreateToolbar(void)
{
	int i;
	RECT r;
	TBBUTTON tb[9];

	for (i = 0; i < 8; i++)
	{
		tb[i].idCommand = tbcmds[i];
		tb[i].iBitmap = tb[i].iString = i;
		tb[i].fsStyle = TBSTYLE_BUTTON;
		tb[i].fsState = MenuDisabled(tb[i].idCommand) ? 0 : TBSTATE_ENABLED;
	}

	if (Options.BigToolbar)
	{
		int c;
 		z_stream zs;
		HDC hdc, compat;
		HGDIOBJ old;
		HBITMAP hbig, hgray;
		BITMAPINFO *gray, *big;
	// decompress the two bitmaps
		big = Dzip_malloc(0x5F04);
		gray = (BITMAPINFO *)((char *)big + 0x3F88);
		zs.zalloc = Dzip_calloc;
		zs.zfree = free;
		inflateInit(&zs);
		zs.avail_in = 0x19C9;
		zs.next_in = LoadResource(hInstance, 
			FindResource(hInstance, MAKEINTRESOURCE(IDB_TOOLBARBIG), "x"));
		zs.avail_out = 0x5F04;
		zs.next_out = (char *)big;
		inflate(&zs, Z_NO_FLUSH);
		inflateEnd(&zs);
	// the [] numbers are based on the image palettes
		c = BGRSysColor(COLOR_3DFACE);
		*(int *)&big->bmiColors[226] = c;
		*(int *)&gray->bmiColors[1] = c;
		*(int *)&gray->bmiColors[0] = BGRSysColor(COLOR_3DSHADOW);
		*(int *)&gray->bmiColors[2] = BGRSysColor(COLOR_3DHILIGHT);
	// create the two HBITMAPs
		hdc = GetDC(NULL);
		hbig = CreateCompatibleBitmap(hdc, 50*8, 38);
		hgray = CreateCompatibleBitmap(hdc, 51*8, 39);
		compat = CreateCompatibleDC(hdc);
		old = SelectObject(compat, hgray);
		StretchDIBits(compat, 0, 0, 51*8, 39, 0, 0, 51*8, 39,
			&gray->bmiColors[16], gray, DIB_RGB_COLORS, SRCCOPY);
		SelectObject(compat, hbig);
		StretchDIBits(compat, 0, 0, 50*8, 38, 0, 0, 50*8, 38,
			&big->bmiColors[256], big, DIB_RGB_COLORS, SRCCOPY);
		SelectObject(compat, old);
		DeleteObject(compat);
		ReleaseDC(NULL, hdc);
	// create disabled image list
		hdimgl = ImageList_Create(51, 39, ILC_COLORDDB, 0, 8);
		ImageList_Add(hdimgl, hgray, 0);
		DeleteObject(hgray);
		free(big);

		tbbmp = hbig;
		Toolbar = CreateToolbarEx(MainWnd, WS_CHILD|WS_VISIBLE,
			0, 8, NULL, (int)hbig, tb, 8, 50, 38, 0, 0, sizeof(TBBUTTON));
		SendMessage(Toolbar, TB_ADDSTRING, 0, (int)"New\0Open\0Add\0Delete\0Extract\0View\0Test\0Abort\0");
		SendMessage(Toolbar, TB_SETSTYLE, 0, TBSTYLE_TOOLTIPS|TBSTYLE_FLAT|CCS_TOP);
		SendMessage(Toolbar, TB_SETDISABLEDIMAGELIST, 0, (int)hdimgl);
	}
	else
	{
		tbbmp = LoadImage(hInstance, MAKEINTRESOURCE(IDB_TOOLBAR), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
		Toolbar = CreateToolbarEx(MainWnd, WS_CHILD|WS_VISIBLE|TBSTYLE_TOOLTIPS,
			0, 8, NULL, (int)tbbmp, tb, 8, 16, 16, 0, 0, sizeof(TBBUTTON));
		tb[8].fsState = tb[8].iBitmap = 0;
		tb[8].fsStyle = TBSTYLE_SEP;
		SendMessage(Toolbar, TB_INSERTBUTTON, 2, (int)(tb + 8));
		SendMessage(Toolbar, TB_INSERTBUTTON, 6, (int)(tb + 8));
		SendMessage(Toolbar, TB_INSERTBUTTON, 8, (int)(tb + 8));
	}
	GetWindowRect(Toolbar, &r);
	TBsize = (short)(r.bottom - r.top);
}