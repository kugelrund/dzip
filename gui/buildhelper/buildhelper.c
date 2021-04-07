#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

__declspec(dllimport) long __cdecl _get_osfhandle(int);

void StripDz (char *dzname)
{
	unsigned char datname[256], strname[256], var[256], block[16];
	int togo, realsize;
	FILE *dz, *dat;
		
	dz = fopen(dzname, "rb");
	if (!dz)
	{
		printf("StripDz: couldn't open %s\n", dzname);
		exit(1);
	}

	fread(block, 12, 1, dz);
	togo = *(int *)(block + 4) - 12;
	if (block[0] != 'D' || block[1] != 'Z' || block[2] != 2 || togo <= 0)
	{
		printf("StripDz: %s is not a v2 .dz file\n", dzname);
		exit(1);
	}

	fseek(dz, togo + 20, SEEK_SET);
	fread(&realsize, 4, 1, dz);
	fseek(dz, 12, SEEK_SET);

	strcpy(strname, dzname);
	strname[strlen(dzname) - 3] = 0;

	strcpy(datname, dzname);
	strcpy(datname + strlen(datname) - 2, "h");
	dat = fopen(datname, "w");
	if (!dat)
	{
		printf("StripDz: couldn't open %s\n", datname);
		exit(1);
	}

	fprintf(dat, "extern const unsigned char %s[];\n", strname);
	strcpy(var, strname);
	strupr(var);
	fprintf(dat, 
		"#define %s_REAL_SIZE %i\n"
		"#define %s_COMPRESSED_SIZE %i",
		var, realsize, var, togo);
	printf(
		"%s_REAL_SIZE %i (%x)\n"
		"%s_COMPRESSED_SIZE %i (%x)\n",
		var, realsize, realsize, var, togo, togo);
	fclose(dat);
	datname[strlen(datname) - 1] = 'c';

	dat = fopen(datname, "w");
	if (!dat)
	{
		printf("StripDz: couldn't open %s\n", datname);
		exit(1);
	}

	fprintf(dat, "const unsigned char %s[] = {\n", strname);

	while (togo > 16)
	{
		fread(block, 16, 1, dz);
		fprintf(dat, "\t0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, "
			"0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, "
			"0x%02x, 0x%02x, 0x%02x,\n",
			block[0], block[1], block[2], block[3], block[4], block[5],
			block[6], block[7], block[8], block[9], block[10], block[11],
			block[12], block[13], block[14], block[15]);
		togo -= 16;
	}
	fprintf(dat, "\t");
	for (;;)
	{
		fprintf(dat, "0x%02x", (unsigned int)fgetc(dz));
		if (--togo)
			fprintf(dat, ", ");
		else
		{
			fprintf(dat, "\n};");
			exit(0);
		}
	}
}

void ShrinkExe(char *exename, char *dosmsg)
{
	IMAGE_DOS_HEADER *idh;
	IMAGE_NT_HEADERS *inh;
	IMAGE_SECTION_HEADER *ish;

	FILE *exe;
	int peoffs, curpos, togo;
	short numsections;
	char header[1024];
	char *block;
		
	exe = fopen(exename, "rb+");
	if (!exe)
	{
		printf("ShrinkExe: couldn't open %s\n", exename);
		exit(1);
	}

	fread(header, 1024, 1, exe);

	idh = (void *)header;
	inh = (void *)&header[idh->e_lfanew];

	if (idh->e_magic != IMAGE_DOS_SIGNATURE || 
		inh->Signature != IMAGE_NT_SIGNATURE)
	{
		printf("ShrinkExe: %s is not a PE file\n", exename);
		exit(1);
	}
	if (dosmsg)
	{
		int len = strlen(dosmsg);
		if (len > 0x31)
		{
			printf("ShrinkExe: dosmsg is too long!\n");
			exit(1);
		}
		dosmsg[len++] = '$';
		memcpy(header + 0x4e, dosmsg, len);
		peoffs = 0x4e + len;
		if (peoffs & 7)
			peoffs += 8 - (peoffs & 7);
	}
	else
		peoffs = 0x80;

	memcpy(header + peoffs, header + idh->e_lfanew, 512);
	idh->e_lfanew = peoffs;
	inh = (void *)&header[peoffs];
	ish = (void *)(inh + 1);

	if (inh->OptionalHeader.FileAlignment != 0x200)
	{
		printf("ShinkExe: file alignment is not 0x200\n");
		exit(0);
	}
	if (inh->OptionalHeader.SizeOfHeaders != 0x400)
	{
		printf("ShinkExe: header size is not currently 0x400\n");
		exit(0);
	}
	
	inh->OptionalHeader.SizeOfHeaders = 0x200;
	numsections = inh->FileHeader.NumberOfSections;
	if (peoffs + sizeof(*inh) + numsections * sizeof(*ish) > 0x200)
	{
		printf("ShinkExe: too many sections to shrink header\n");
		exit(0);
	}

	while (numsections--)
		if (ish[numsections].PointerToRawData)
			ish[numsections].PointerToRawData -= 0x200;

	fseek(exe, 0, SEEK_SET);
	fwrite(header, 512, 1, exe);

	block = malloc(32768);
	fseek(exe, 0, SEEK_END);
	togo = ftell(exe) - 1024;
	curpos = 1024;

	while (togo > 32768)
	{
		fseek(exe, curpos, SEEK_SET);
		fread(block, 32768, 1, exe);
		fseek(exe, curpos - 512, SEEK_SET);
		fwrite(block, 32768, 1, exe);
		togo -= 32768;
		curpos += 32768;
	}
	fseek(exe, curpos, SEEK_SET);
	fread(block, togo, 1, exe);
	fseek(exe, curpos - 512, SEEK_SET);
	fwrite(block, togo, 1, exe);
	SetEndOfFile((void *)_get_osfhandle(fileno(exe)));

// decrease pointer to pdb name, if it exists
	curpos = inh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
	if (curpos)
	{
		fseek(exe, curpos - 0xE00 + 24, SEEK_SET);
		fread(&curpos, 4, 1, exe);
		curpos -= 0x200;
		fseek(exe, -4, SEEK_CUR);
		fwrite(&curpos, 4, 1, exe);
	}

	
	if (!stricmp(exename, "c:\\games\\quake\\dzipgui.exe"))
	{
		FILE *map = fopen("c:\\games\\quake\\dzip\\gui\\release\\dzipgui.map", "r");
		char *mapc;
		int len;

		fseek(map, 0, SEEK_END);
		len = ftell(map);
		mapc = malloc(len);
		fseek(map, 0, SEEK_SET);
		fread(mapc, 1, len, map);
		printf("%.8s\t", strstr(strstr(mapc, "MsgL"), "00"));
		printf("%.8s\n", strstr(strstr(mapc, "__NULL"), "00"));
	}
	exit(0);
}

void MakeBackwardsCompatible(char *exename)
{
	IMAGE_DOS_HEADER *idh;
	IMAGE_NT_HEADERS *inh;

	FILE *exe;
	char header[1024];
		
	exe = fopen(exename, "rb+");
	if (!exe)
	{
		printf("ShrinkExe: couldn't open %s\n", exename);
		exit(1);
	}

	fread(header, 1024, 1, exe);

	idh = (void *)header;
	inh = (void *)&header[idh->e_lfanew];

	if (idh->e_magic != IMAGE_DOS_SIGNATURE || 
		inh->Signature != IMAGE_NT_SIGNATURE)
	{
		printf("ShrinkExe: %s is not a PE file\n", exename);
		exit(1);
	}

	inh->OptionalHeader.MajorSubsystemVersion = 4;
	inh->OptionalHeader.MinorSubsystemVersion = 0;
	inh->OptionalHeader.MajorOperatingSystemVersion = 4;
	inh->OptionalHeader.MinorOperatingSystemVersion = 0;

	fseek(exe, 0, SEEK_SET);
	fwrite(header, 1024, 1, exe);
	exit(0);
}

int main (int argc, char **argv)
{
	if (argc < 3)
		return -1;

	if (!strcmp(argv[1], "-s"))	// strip a .dz to a .dat
		StripDz(argv[2]);

	if (!strcmp(argv[1], "-h"))	// change exe header to reduce by .5K
		ShrinkExe(argv[2], argc == 3 ? NULL : argv[3]);

	if (!strcmp(argv[1], "-h2")) // add \r\r\n to end of argv[3]
	{
		char *dosmsg = malloc(strlen(argv[3]) + 4);
		strcpy(dosmsg, argv[3]);
		*(int*)(dosmsg + strlen(dosmsg)) = 0x0a0d0d;
		ShrinkExe(argv[2], dosmsg);
	}

	if (!strcmp(argv[1], "-b"))	// change exe header win32 version to 4.0 for backwards compatibility
		MakeBackwardsCompatible(argv[2]);

	return -1;
}