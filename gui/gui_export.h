typedef struct {
	char *AbortOp;
	uInt *crcval;
	uInt (*ArchiveFile_Read)(void *, uInt);
	void (*ArchiveFile_Seek)(uInt);
	uInt (*ArchiveFile_Size)(void);
	void (*ArchiveFile_Truncate)(void);
	void (*ArchiveFile_Write)(void *, uInt);
	void *(*malloc)(uInt);
	void *(*realloc)(void *, uInt);
	void *(*calloc)(void *, uInt, uInt);
	void (*free)(void *, void *);
	char *(*strdup)(const char *);
	void (*GuiProgressMsg)(const char *, ...);
	void (*Infile_Read)(void *, uInt);
	void (*Infile_Seek)(uInt);
	void (*Infile_Store)(uInt);
	void (*LVAddFileToListView)(char *, uInt, uInt, uInt, uInt, int);
	void (*LVBeginAdding)(uInt);
	void (*Outfile_Write)(void *, uInt);
	void (*error)(const char *, ...);
	int (*YesNo)(const char *, const char *, void *, int);
	int (*deflate)(struct z_stream_s *, int);
	int (*deflateEnd)(struct z_stream_s *);
	int (*deflateInit)(struct z_stream_s *, int);
	int (*inflate)(struct z_stream_s *, int);
	int (*inflateEnd)(struct z_stream_s *);
	int (*inflateInit)(struct z_stream_s *);
	void (*make_crc)(unsigned char *ptr, int len);
} gui_export_t;

// const in dll, non-const in exe
extern
#ifndef _USRDLL
const
#endif
gui_export_t ge;