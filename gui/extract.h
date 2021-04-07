void ExtractFilesForDragOut(void);
void ExtractFile(lventry_t *);
HGLOBAL ExtractMakeFGD(void);
HGLOBAL ExtractMakeDropfiles(void);
void ExtractSetup(int, int, const char *);
void Outfile_Flush(void *, UINT);
void Outfile_Write(void *, UINT);

typedef struct {
	char *Curname;
	char ToAll, FreeCurname, NoPaths, reserved;
	int num, numSuccessfull;
	int NextFileToExplorer;
	void *buf;
	UINT bufsize, bufused;
	HANDLE handle;
	IStream *stream;
	lventry_t **list;	// contains which items will be extracted
	char olddir[256];
} extract_t;

extern extract_t Extract;