typedef struct {
	int version;
	const char *extlist;	// list of extensions that the dll supports
	int flags;
	void (*AddFile)(char *name, unsigned int filesize, unsigned int filedate);
	void (*AddFolder)(char *name);
	void (*BeginAdd)(int newfile);
	void (*CloseFile)(void);
	void (*DeleteFiles)(unsigned int *list, unsigned int num, void (*Progress)(unsigned int, unsigned int));
	void (*ExpandFile)(unsigned int filepos);
	void (*ExtractFile)(unsigned int filepos, int testing);
	void (*FinishedAdd)(void);
	void (*OpenFile)(char *name);
	int (*RenameFile)(unsigned int filepost, char *name);
} gui_import_t;

// const in exe, non-const in dll
extern
#ifdef _USRDLL
const
#endif
gui_import_t gi;

#define UNSUPPORTED_ADD 1
#define UNSUPPORTED_DELETE 2
#define UNSUPPORTED_EXTRACT 4
#define UNSUPPORTED_RENAME 8
#define UNSUPPORTED_TEST 32