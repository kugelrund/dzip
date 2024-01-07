#include "dzip.h"
#include "dzipcon.h"

#ifdef _MSC_VER
#include <direct.h>
#define getcwd _getcwd
#endif

char flag[NUM_SWITCHES];
char AbortOp, *dzname;
uInt dzsize;
FILE *infile, *outfile, *dzfile;
static char cwd_buffer[260];
static const char* cwd = NULL;

/* read from file being compressed */
void Infile_Read (void *buf, uInt num)
{
	if (fread(buf, 1, num, infile) != num)
	{
		error(": error reading from input: %s", strerror(errno));
		AbortOp = 3;
	}
	make_crc(buf, num);
}

/* change file pointer in file being compressed (used for .pak) */
void Infile_Seek (uInt dest)
{
	fseek(infile, dest, SEEK_SET);
}

/* called when compression was unsuccessful (negative ratio) */
void Infile_Store (uInt size)
{
	void *buf = Dzip_malloc(32768);
	int s;

	Infile_Seek(ftell(infile) - size);
	while (size && !AbortOp)
	{
		s = (size > 32768) ? 32768 : size;
		Infile_Read(buf, s);
		dzFile_Write(buf, s);
		size -= s;
	}
	free(buf);
}

/* write to the file being extracted */
void Outfile_Write (void *buf, uInt num)
{
	if (!flag[SW_VERIFY])
	if (fwrite(buf, 1, num, outfile) != num)
	{
		error(": error writing to output: %s", strerror(errno));
		AbortOp = 1;
	}
	make_crc(buf, num);
}

/* read from dz */
void dzFile_Read (void *buf, uInt num)
{
	if (fread(buf, 1, num, dzfile) != num)
	{
		error(": error reading from %s: %s", dzname, strerror(errno));
		AbortOp = 1;
	}
}

/* write to dz */
void dzFile_Write (void *buf, uInt num)
{
	if (fwrite(buf, 1, num, dzfile) != num)
	{
		error(": error writing to %s: %s", dzname, strerror(errno));
		AbortOp = 1;
	}
}

/*	get filesize of dz, returns zero if it could not
	be determined, most likely because it was >= 4GB */
uInt dzFile_Size(void)
{	/* size was determined in DoFiles */
	return dzsize;
}

/* change file pointer of dz */
void dzFile_Seek (uInt dest)
{
	fseek(dzfile, dest, SEEK_SET);
}

/* chop off end of dz; this is implementation dependent
   and may need adjusting for other platforms */
void dzFile_Truncate(void)
{
#ifdef _MSC_VER
	SetEndOfFile(_get_osfhandle(fileno(dzfile)));
#else
	ftruncate(fileno(dzfile), ftell(dzfile));
#endif
}

/* free directory and close current dz */
void dzClose(void)
{
	int i;
	for (i = 0; i < numfiles; free(directory[i++].name));
	free(directory);
	fclose(dzfile);
}

/* open a dz file, return 0 on failure */
int dzOpen (char *src, int needwrite)
{
	dzname = src;
	dzfile = fopen(src, needwrite ? "rb+" : "rb");
	if (!dzfile)
	{
		error(needwrite ? "could not open %s for writing: %s"
			: "could not open %s: %s", src, strerror(errno));
		return 0;
	}

	if (dzReadDirectory(src))
		return 1;

	fclose(dzfile);
	return 0;
}

FILE *open_create (char *src)
{
	FILE *f;

	if (!flag[SW_FORCE])
		if ((f = fopen(src, "r"))) 
		{
			error("%s exists; will not overwrite", src);
			fclose(f);
			return NULL;
		}

	f = fopen(src,"wb+");
	if (!f) error("could not create %s: %s",src, strerror(errno));
	return f;
}

/* safe set of allocation functions */
void *Dzip_malloc (uInt size)
{
	void *m = malloc(size);
	if (!m)	errorFatal("Out of memory!");
	return m;		
}

void *Dzip_realloc (void *ptr, uInt size)
{
	void *m = realloc(ptr, size);
	if (!m)	errorFatal("Out of memory!");
	return m;		
}

char *Dzip_strdup (const char *str)
{
	char *m = strdup(str);
	if (!m)	errorFatal("Out of memory!");
	return m;		
}

void *Dzip_calloc (void *opaque, uInt items, uInt size)
{
	(void)opaque;
	void *m = calloc(items, size);
	if (!m) errorFatal("Out of memory!");
	return m;
}

void Dzip_free (void *opaque, void *address)
{
	(void)opaque;
	free(address);
}

/* does not return if -e was specified on cmd line */
void error (const char *msg, ...)
{
	FILE *f = flag[SW_HALT] ? stderr : stdout;
	va_list	the_args;
	va_start(the_args,msg);
	if (flag[SW_HALT]) {
		if (*msg == ':')
		{
			fprintf(stderr, "\nERROR: ");
			msg += 2;
		}
		else
			fprintf(stderr,"ERROR: ");
	} /* stupid compilers bitch about ambigious else */

	vfprintf(f,msg,the_args);
	fprintf(f,"\n");
	va_end(the_args);
	if (flag[SW_HALT])
		exit(1);
}

/* does not return ever */
void errorFatal (const char *msg, ...)
{
	va_list	the_args;
	va_start(the_args,msg);
	fprintf(stderr,"ERROR: ");
	vfprintf(stderr,msg,the_args);
	fprintf(stderr,"\n");
	va_end(the_args);
	exit(1);
}

struct { char name; int flag; }
optname[] = {
	{ 'o', SW_OUTFILE },
	{ 'l', SW_LIST },
	{ 'x', SW_EXTRACT },
	{ 's', SW_VIEW },
	{ 'v', SW_VERIFY },
	{ 'a', SW_ADD },
	{ 'd', SW_DELETE },
	{ 'f', SW_FORCE },
	{ 'e', SW_HALT },
	{ 'p', SW_FULLPATHS },
	{ 0, 0 }
};

void parse_switch (char *arg)
{
	int i;

	while (*++arg)
		if (*arg >= '0' && *arg <= '9')
			zlevel = *arg - '0';
		else
		{
			for (i = 0; optname[i].name; i++)
				if (*arg == optname[i].name)
				{
					flag[optname[i].flag]++;
					break;
				}
			if (!optname[i].name)
				errorFatal("unknown switch %s",arg);
		}
}

void usage(void)
{
	fprintf(stdout,
	"Dzip v%u.%u: specializing in Quake demo compression\n\n"

	"Compression:         dzip <filenames> [-o <outputfile>]\n"
	"Add to existing dz:  dzip -a <dzfile> <filenames>\n"
	"Delete from a dz:    dzip -d <dzfile> <filenames>\n"
	"Decompression:       dzip -x <filenames>\n"
	"View one file:       dzip -s <dzfile> [file to view]\n"
	"Verify dz file:      dzip -v <filenames>\n"
	"List dz file:        dzip -l <filenames>\n\n"

	"Options:\n"
	"  -0 to -9: set compression level\n"
	"  -e: quit program on first error\n"
	"  -f: overwrite existing files\n"
	"  -p: keep given full paths in archive\n\n"

	"Copyright 2000-2005 Stefan Schwoon, Nolan Pflug\n",
	MAJOR_VERSION, MINOR_VERSION
    );

    exit(0);
}

void DoFiles (char *fname, void (*func)(char *));

void DoDirectory (char *dname)
{
	char fullname[260], *ptr;

	if (!strcmp(dname, ".") || !strcmp(dname, ".."))
		return;
	ptr = &fullname[sprintf(fullname, "%s\\", dname)];
	dzAddFolder(fullname);

	/* scan directory, more system specific stuff */
	{
#ifdef WIN32
	HANDLE f;
	WIN32_FIND_DATA fd;

	*(short *)ptr = '*';
	f = FindFirstFile(fullname, &fd);
	if (f == INVALID_HANDLE_VALUE)
		return;	/* happens if user doesn't have browse permission */

	do {
		if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, ".."))
			continue;
		strcpy(ptr, fd.cFileName);
		DoFiles(fullname, NULL);
	} while (!AbortOp && FindNextFile(f, &fd));

	FindClose(f);
#else
	struct dirent *e;
	DIR *d = opendir(dname);
	if (!d) return;
	ptr[-1] = '/';

	while (!AbortOp && (e = readdir(d)))
	{
		if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
			continue;
		strcpy(ptr, e->d_name);
		DoFiles(fullname, NULL);
	}

	closedir(d);
#endif
	}
}

static char* StripCurrentWorkingDirectory(char* fullname)
{
	if (!cwd) {
		return fullname;
	}

	const size_t cwd_length = strlen(cwd);
	if (strlen(fullname) <= cwd_length) {
		return fullname;
	}
	if (strncmp(fullname, cwd, cwd_length) != 0) {
		return fullname;
	}

	char* name_without_cwd = fullname + cwd_length;
	while (*name_without_cwd == '/' || *name_without_cwd == '\\') {
		++name_without_cwd;
	}

	if (*name_without_cwd == '\0') {
		return fullname;
	}
	return name_without_cwd;
}

void DoFiles (char *fname, void (*func)(char *))
{
	uInt filetime;

/* handle wildcards for Win32, assume that
   other systems will handle them for me! */
#ifdef WIN32
	HANDLE f;
	WIN32_FIND_DATA fd;
	char fullname[260];
	int p = GetFileFromPath(fname) - fname;

	f = FindFirstFile(fname, &fd);
	if (f == INVALID_HANDLE_VALUE)
	{	/* file not found, try adding .dem if no wildcards */
		if (!*FileExtension(fname) && !strpbrk(fname, "*?"))
		{
			strcat(fname, ".dem");
			f = FindFirstFile(fname, &fd);
			if (f == INVALID_HANDLE_VALUE)
				*FileExtension(fname) = 0;
		}
		if (f == INVALID_HANDLE_VALUE)
		{
			error("could not open %s", fname);
			return;
		}
	}

	do
	{
		strncpy(fullname, fname, p);
		strcpy(fullname + p, fd.cFileName);

		if (func)
		{	/* either dzList or dzUncompress */
			if (fd.nFileSizeHigh)
				dzsize = 0;
			else
				dzsize = fd.nFileSizeLow;
			func(fullname);
			continue;
		}

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			DoDirectory(fullname);
			continue;
		}

		/* cant add the current file to itself */
		if (!strcasecmp(fullname, dzname))
			continue;

		if (fd.nFileSizeHigh)
		{
			error("%s is 4GB or more and can't be compressed", fullname);
			return;
		}

		infile = fopen(fullname, "rb");
		if (!infile)
		{
			error("could not open %s: %s", fullname, strerror(errno));
			continue;
		}

		FileTimeToLocalFileTime(&fd.ftLastWriteTime, &fd.ftLastAccessTime);
		FileTimeToDosDateTime(&fd.ftLastAccessTime, (short *)&filetime + 1, (short *)&filetime);

		char* name_without_cwd = StripCurrentWorkingDirectory(fullname);
		dzCompressFile(name_without_cwd, fd.nFileSizeLow, filetime - (1 << 21));
		fclose(infile);
		if (AbortOp == 3)	/* had a problem reading from infile */
			AbortOp = 0;	/* but still try the rest of the files */

	} while (!AbortOp && FindNextFile(f, &fd));
	FindClose(f);

#else

	struct stat64 filestats;
	struct tm *trec;

	if (stat64(fname, &filestats))
	{	/* file probably doesn't exist if this failed */
		error("could not open %s: %s", fname, strerror(errno));
		return;
	}

	if (func)
	{	/* either dzList or dzUncompress */
		if (filestats.st_size > 0xFFFFFFFF)
			dzsize = 0;
		else
			dzsize = filestats.st_size;
		func(fname);
		return;
	}

	if (filestats.st_mode & S_IFDIR)
	{
		DoDirectory(fname);
		return;
	}

	/* cant add the current file to itself */
	if (!strcasecmp(fname, dzname))
		return;

	if (filestats.st_size > 0xFFFFFFFF)
	{
		error("%s is 4GB or more and can't be compressed", fname);
		return;
	}

	infile = fopen(fname, "rb");
	if (!infile)
	{
		error("could not open %s: %s", fname, strerror(errno));
		return;
	}

	trec = localtime(&filestats.st_mtime);
	filetime = (trec->tm_sec >> 1) + (trec->tm_min << 5)
		+ (trec->tm_hour << 11) + (trec->tm_mday << 16)
		+ (trec->tm_mon << 21) + ((uInt)(trec->tm_year - 80) << 25);

	char* name_without_cwd = StripCurrentWorkingDirectory(fname);
	dzCompressFile(name_without_cwd, filestats.st_size, filetime);
	fclose(infile);
	if (AbortOp == 3)	/* had a problem reading from infile */
		AbortOp = 0;	/* but still try the rest of the files */
#endif
}

int main (int argc, char **argv)
{
	int i, switchflag = 0;
	int fileargs = 0;
	char *optr = NULL;
	char **files;
	char *fname;
	char dzheader[12];

#ifdef WIN32
	WIN32_FIND_DATA fd;
#else
	struct stat64 filestats;
#endif

	if (argc < 2)
		usage();

	files = Dzip_malloc(argc * sizeof(char*));
	for (i = 1; i < argc; i++)
		if (*argv[i] == '-' && switchflag != 1)
		{	/* check for '--' switch first */
			if (argv[i][1] == '-' && !argv[i][2])
			{
				switchflag = 1;
				continue;
			}
			parse_switch(argv[i]);
			if (flag[SW_OUTFILE] && switchflag != 2)
			{	/* if switch contained an o, get the output file */
				if (i == argc - 1)
					errorFatal("-o without argument");
				optr = argv[++i];
				switchflag = 2;
			}
		}
		else
			files[fileargs++] = argv[i];

	if (!fileargs)
		errorFatal("no input files");
	if (1 < !!flag[SW_LIST] + !!flag[SW_EXTRACT] + !!flag[SW_VIEW]
		+ !!flag[SW_VERIFY] + !!flag[SW_ADD] + !!flag[SW_DELETE]
		+ !!flag[SW_OUTFILE])
		errorFatal("conflicting switches");

	if (!flag[SW_FULLPATHS])
		cwd = getcwd(cwd_buffer, sizeof(cwd_buffer));

	crc_init();
	inblk = Dzip_malloc(p_blocksize * 3);
	outblk = inblk + p_blocksize;
	tmpblk = outblk + p_blocksize / 2;
	zbuf = tmpblk + p_blocksize / 2;
	zs.zalloc = Dzip_calloc;
	zs.zfree = Dzip_free;

	if (flag[SW_VIEW])
	{
		if (fileargs > 2)
			errorFatal("only one file can be viewed at once");
	#ifdef WIN32
		FindClose(FindFirstFile(files[0], &fd));
		if (!fd.nFileSizeHigh)
			dzsize = fd.nFileSizeLow;
	#else
		if (!stat64(files[0], &filestats))
			if (!(filestats.st_size > 0xFFFFFFFF))
				dzsize = filestats.st_size;
	#endif

		dzViewFile(files[0], (fileargs < 2) ? NULL : files[1]);
		exit(0);
	}

	if (flag[SW_LIST] || flag[SW_EXTRACT] || flag[SW_VERIFY])
	{
		void (*func)(char *) = flag[SW_LIST] ? dzList : dzUncompress;
		for (i = 0; i < fileargs && !AbortOp; i++)
		{
			fname = Dzip_malloc(strlen(files[i])+4);
			strcpy(fname,files[i]);
#ifdef WIN32 /* only add extension automatically on windows */
			if (!*FileExtension(fname))
				strcat(fname, ".dz");
#endif
			DoFiles(fname, func);
			free(fname);
		}
		exit(0);
	}

	if (!flag[SW_ADD] && !flag[SW_DELETE])
	{	/* create a new dz and compress files */
		if (!optr)
		{
			optr = Dzip_malloc(strlen(files[0]) + 4);
			strcpy(optr, files[0]);
			strcpy(FileExtension(optr), ".dz");
		}
	#ifdef WIN32
		else if (!*FileExtension(optr))
		{
			char *t = Dzip_malloc(strlen(optr) + 4);
			sprintf(t, "%s.dz", optr);
			optr = t;
		}
	#endif
		dzname = optr;
		dzfile = open_create(optr);
		if (!dzfile) exit(1);
		/* Write .dz header. First 4 bytes are the id. Following bytes are for
		   offset and numfiles, so they can only be filled later on. For now
		   write zeros to reserve the space for them. */
		memset(dzheader, 0, sizeof(dzheader));
		dzheader[0] = 'D';
		dzheader[1] = 'Z';
		dzheader[2] = MAJOR_VERSION;
		dzheader[3] = MINOR_VERSION;
		dzFile_Write(dzheader, sizeof(dzheader));
		totalsize = sizeof(dzheader);

		directory = Dzip_malloc(sizeof(direntry_t));

		for (i = 0; i < fileargs; i++) 
		{
			fname = Dzip_malloc(strlen(files[i])+5);
			strcpy(fname,files[i]);
			DoFiles(fname, NULL);
			free(fname);
		}

		free(files);
		dzWriteDirectory();
		dzClose();
		if (!numfiles)
			remove(dzname);
		exit(0);
	}

	/* add or delete */
	if (fileargs < 2)
		errorFatal("no input files");
#ifdef WIN32
	fname = Dzip_malloc(strlen(files[0])+4);
	strcpy(fname,files[0]);
	if (!*FileExtension(fname))
		strcat(fname, ".dz");
#else
	fname = files[0];
#endif

	/* figure out dzsize */
#ifdef WIN32
	FindClose(FindFirstFile(fname, &fd));
	if (!fd.nFileSizeHigh)
		dzsize = fd.nFileSizeLow;
#else
	if (!stat64(fname, &filestats))
		if (!(filestats.st_size > 0xFFFFFFFF))
			dzsize = filestats.st_size;
#endif

	if (!dzOpen(fname, 1))	/* add & delete need write access */
		exit(0);

	if (flag[SW_ADD])
	{
		dzFile_Seek(4);
		dzFile_Read(&totalsize, 4);
		dzFile_Seek(totalsize);
		for (i = 1; i < fileargs; i++) 
		{
			fname = Dzip_malloc(strlen(files[i])+5);
			strcpy(fname,files[i]);
			DoFiles(fname, NULL);
			free(fname);
		}
		dzWriteDirectory();
		dzClose();
	}
	else
		dzDeleteFiles_MakeList(files + 1, fileargs - 1);
	free(files);
	exit(0);
}
