#include <windows.h>
#include "common.h"
#include "misc.h"
#include "resource.h"

HINSTANCE hInstance;
HWND MainWnd, LView;
char temp1[260], temp2[260];	// need them all the time :)
char AbortOp, ReadOnly;
tempfiles_t *tempfiles;

void ActionsAbort(void)
{
	AbortOp = 1; 
}

const char abouttext[] =
	"Dzip -- Compression program\n"
	"Works great on Quake demo files!\n\n"
	"Version 3.0 -- Released 18.04.2021\n"
	"Win32 version by Nolan Pflug\n"
	"(radix@planetquake.com)\n"
	"Initial version by Stefan Schwoon\n"
	"(schwoon@in.tum.de)\n"
	"Contributions by Sphere since 2021\n"
	"(sphere@speeddemosarchive.com)\n"
	"Compression using zlib library\n\n"
	"http://speeddemosarchive.com/dzip/";

int CALLBACK HelpAboutDlgFunc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INITDIALOG)
	{
		SetDlgItemText(hDlg, IDC_ABOUTTEXT, abouttext);
		CenterDialog(hDlg);
		return 1;
	}
	if (msg == WM_COMMAND)
		EndDialog (hDlg, 0);
	return 0;
}

void HelpAbout(void)
{
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_HELPABOUT), MainWnd, HelpAboutDlgFunc);
}

/* returns pointer to filename from a full path */
char *GetFileFromPath (char *in)
{
	char *x = in - 1;

	while (*++x)
		if (*x == '/' || *x == '\\')
			in = x + 1;
	return in;
}

/* returns a pointer directly to the period of the extension,
   or it there is none, to the nul character at the end */
char *FileExtension (char *in)
{
	char *e = in + strlen(in);

	in = GetFileFromPath(in);
	while ((in = strchr(in, '.')))
		e = in++;

	return e;
}