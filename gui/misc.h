extern HINSTANCE hInstance;
extern HWND MainWnd, LView, Toolbar;
extern char temp1[260], temp2[260];	// need them all the time :)
extern short TBsize;
extern char AbortOp, ReadOnly;

typedef struct {
	const char *ext;
	const char *dllname;
	UINT isDzips;
} ext_dll_t;
extern ext_dll_t *ext_dll_list;
extern char *filterList;

typedef struct tempfiles_s {
	char *name;
	struct tempfiles_s *next;
} tempfiles_t;
extern tempfiles_t *tempfiles;

void CreateToolbar(void);
void DestroyToolbar(void);