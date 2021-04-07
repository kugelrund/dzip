typedef struct {
	char *filename;
	UINT date;
	UINT realsize;
	UINT compressedsize;
	UINT filepos;
	int lvpos;	// dont mess with the order above this
	int parent;
	float ratio;
	int icon;
	char *filetype;
	BYTE filenameonly_offset;
	BYTE extension_offset;
	struct {
		BYTE folder : 1;
		BYTE selected : 1;
		BYTE expandable : 1;
		BYTE expanded : 1;
		BYTE reserved : 4;
		BYTE indent : 3;
		BYTE reserved2 : 5;
	} status;
} lventry_t;

void CollapseFile(lventry_t *);
void CreateListView(void);
void LVAddFileToListView (char *, UINT, UINT, UINT, UINT, int);
void LVBeginAdding(UINT);
void LVChangeSort(int);
void LVFinishedAdding(void);
int LVGetColumnFromSortType(int);
void LVGetDispInfo(NMHDR *);
void LVLoadArrows(void);
void LVMoveSortArrow (int, int);
int LVNotification(NMHDR *);
void LVResize(void);
void LVSaveState(void);
void RecreateListView(void);

extern lventry_t *lventries;
extern BYTE SortType;
extern int focus, lvNumEntries, *lvorderarray;