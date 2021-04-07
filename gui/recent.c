#include <stdio.h>
#include <windows.h>
#include "common.h"
#include "file.h"
#include "menu.h"
#include "misc.h"
#include "recent.h"
#include "thread.h"
#include <shlobj.h>

extern int lvNumEntries;

struct recent_t Recent;
/*static char Synchronizing;

void RecentBroadcast(int code, char *string)
{
	HWND wnd = NULL;
	ATOM a;

	if (Synchronizing)
		return;

	if (string)
		a = GlobalAddAtom(string);

	while (wnd = FindWindowEx(NULL, wnd, "DzipWndClass", NULL))
		if (wnd != MainWnd)
		SendMessageTimeout(wnd, WM_APP + 1, code, a, SMTO_NORMAL, 200, NULL);

	if (string)
		GlobalDeleteAtom(a);
}
*/
// add newstring to a recent list
void RecentAddString (char ***dest, char *max, char *newstring)
{
	int i, j = *max;
	for (i = 0; i < j; i++)
		if (!stricmp((*dest)[i], newstring))
			break;
	if (i == j)	// not already in the list
		if (j < 10)	// need to expand list
		{
			char **oldlist = *dest;
			j = ++*max;
			*dest = Dzip_malloc(j * 260);
			for (i = 0; i < j; i++)
				(*dest)[i] = (char *)*dest + 4 * j + 256 * i;
			for (i = 0; i < j - 1; i++)
				strcpy((*dest)[i], oldlist[i]);
			free(oldlist);
		}
		else // list is at max size, move #10 out
			i = 9;

	while (i--)
		strcpy((*dest)[i + 1], (*dest)[i]);
	strcpy((*dest)[0], newstring);

//	RecentBroadcast(dest - &Recent.FileList, newstring);
}

int RecentRead (char ***list, char *name, int num)
{
/*	char *ptr, *buf;
	char tmp[16];

	buf = Dzip_malloc(2600);
	sprintf(tmp, "recent%s", name);

	ReadRegValue(HKEY_CURRENT_USER, dzipreg, tmp, buf, 0);
	free(buf);

	return 0;
*/
	int i;
	if (!num) return 0;
	*list = Dzip_malloc(260 * num);
	sprintf(temp1, "%s0", name);
	for (i = 0; i < num; i++)
	{
		(*list)[i] = (char *)*list + 4 * num + 256 * i;
		if (!ReadRegValue(HKEY_CURRENT_USER, dzipreg, temp1, (*list)[i], 256))
			break;
		temp1[4]++;
	}
	return i;	// num is what it should be but perhaps someone deleted them from the registry
}

void RecentWrite (char **list, char *name, int num)
{
/*	int i;
	char tmp[16];
	char *ptr, *buf;

	buf = Dzip_malloc(2600);
	ptr = buf;
	sprintf(tmp, "recent%s", name);
	for (i = 0; i < num; i++)
		ptr += sprintf(ptr, "%s\"", list[i]);

	WriteRegValue(HKEY_CURRENT_USER, dzipreg, tmp, REG_SZ, buf, 0, 0);
	free(buf);
*/
	int i;
	char tmp[6];

	strcpy(tmp, name);
	tmp[4] = '0';
	tmp[5] = 0;
	for (i = 0; i < num; i++, tmp[4]++)
		WriteRegValue(HKEY_CURRENT_USER, dzipreg, tmp, REG_SZ, list[i], 0, 0);
}

void RecentClear (char *name)
{
	HKEY k;
	sprintf(temp1, "%s0", name);
	RegOpenKey(HKEY_CURRENT_USER, dzipreg, &k);
	while (RegDeleteValue(k, temp1) == ERROR_SUCCESS)
		temp1[4]++;
	RegCloseKey(k);
}
/*
void RecentSynchronize (int code, int atom)
{
	char str[256];
	Synchronizing++;
	if (atom)
		GlobalGetAtomName((short)atom, str, 256);
	switch (code)
	{
	case 0:	// add to RFL
		MenuUpdateRFL(str);
		if (lvNumEntries)	// if a file is open we have to keep its name in [0]
		{
			strcpy(str, Recent.FileList[1]);
			MenuUpdateRFL(str);
		}
		break;
	case 1:	// add to extract paths
	case 2: // add to move paths
		RecentAddString(&Recent.FileList + code, &Recent.NumFiles + code, str);
		break;			
	}
	Synchronizing--;
}
*/
void FileRecentFile (int i)
{
	strcpy(temp1, Recent.FileList[i]);
	if (lvNumEntries) CloseArchive();
	OpenArchive(temp1, 0);
	if (!lvNumEntries)
	{	// if opening it failed, remove it from the list
		Recent.NumFiles--;
		while (i++ < Recent.NumFiles)
			strcpy(Recent.FileList[i - 1], Recent.FileList[i]);
		MenuUpdateRFL(NULL);
	}
	else
		SetDir(Recent.FileList[0]);
}

void FileRecentFile1(void) {if (!lvNumEntries) FileRecentFile(0);}
void FileRecentFile2(void) {FileRecentFile(1);}
void FileRecentFile3(void) {FileRecentFile(2);}
void FileRecentFile4(void) {FileRecentFile(3);}
void FileRecentFile5(void) {FileRecentFile(4);}
void FileRecentFile6(void) {FileRecentFile(5);}
void FileRecentFile7(void) {FileRecentFile(6);}
void FileRecentFile8(void) {FileRecentFile(7);}
void FileRecentFile9(void) {FileRecentFile(8);}