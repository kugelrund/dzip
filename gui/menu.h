void FileClose(void);
void FileExit(void);
void FileMove(void);
void FileRecentFile1(void);
void FileRecentFile2(void);
void FileRecentFile3(void);
void FileRecentFile4(void);
void FileRecentFile5(void);
void FileRecentFile6(void);
void FileRecentFile7(void);
void FileRecentFile8(void);
void FileRecentFile9(void);
void FileRename(void);
void FileNew(void);
void FileOpen(void);
void ActionsAdd(void);
void ActionsAbort(void);
void ActionsDelete(void);
void ActionsExtract(void);
void ActionsInvert(void);
void ActionsRename(void);
void ActionsSelectAll(void);
void ActionsTest(void);
void ActionsView(void);
void OptionsOptions(void);
void OptionsFileTypes(void);
void OptionsReset(void);
void OptionsUninstall(void);
void HelpAbout(void);

void CreateMenuBar(void);
void MenuActivateCommands(void);
int MenuDisabled (int);
void MenuDraw(DRAWITEMSTRUCT *);
void MenuMeasure(MEASUREITEMSTRUCT *);
void MenuReset(int);
int MenuShortcutKey(int, HMENU);
void MenuUpdateRFL(char *);

#define FILEMENULENGTH 8

enum {
	ID_FILE_NEW, ID_FILE_OPEN, ID_FILE_CLOSE, ID_FILE_MOVE,
	ID_FILE_RENAME, ID_FILE_EXIT, ID_FILE_FILE1, ID_FILE_FILE2,
	ID_FILE_FILE3, ID_FILE_FILE4, ID_FILE_FILE5, ID_FILE_FILE6,
	ID_FILE_FILE7, ID_FILE_FILE8, ID_FILE_FILE9, ID_ACTIONS_ADD,
	ID_ACTIONS_DELETE, ID_ACTIONS_EXTRACT, ID_ACTIONS_VIEW,
	ID_ACTIONS_RENAME, ID_ACTIONS_SELECTALL,
	ID_ACTIONS_INVERT, ID_ACTIONS_TEST, ID_ACTIONS_ABORT, ID_HELP_ABOUT,
	NUMOWNERDRAWN,
	ID_SORT_PATHFILENAME = NUMOWNERDRAWN,	// dont waste a number :)
	ID_SORT_MODIFIED, ID_SORT_SIZE, ID_SORT_RATIO, ID_SORT_PACKED,
	ID_SORT_TYPE, ID_SORT_FILENAME, ID_SORT_EXTENSION, ID_SORT_NONE,
	ID_SORT_REVERSE, ID_OPTIONS_OPTIONS, ID_OPTIONS_FILETYPES,
	ID_OPTIONS_RESET, ID_OPTIONS_UNINSTALL,
	NUMMENU
};

typedef struct {
	HMENU h, file, actions, help;
	HFONT hFont;
	short Initialized;
	short fileSCofs, actionsSCofs;
	short ysize, ExtentAF4, ExtentRename;
	char ExitText[9];
	char RFLtext[9][64];
	HICON hIcon[NUMOWNERDRAWN];
	char *ToolTips[NUMOWNERDRAWN];
} menu_t;

extern menu_t Menu;
extern void (* const MenuFunc[NUMMENU])(void);