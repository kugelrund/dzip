typedef struct {
	BYTE Toolbar : 1;
	BYTE MenuIcons : 1;
	BYTE StatusBar : 1;
	BYTE ShowSelected : 1;
	BYTE RFLsize : 4;
	BYTE TypeCol : 1;
	BYTE PathCol : 1;
	BYTE AutoClose : 1;
	BYTE LCFilenames : 1;
	BYTE AllFilesInAdd : 1;
	BYTE BigToolbar : 1;
	BYTE ExpCmpWDz : 1;
	BYTE SmoothPBar : 1;
} options_t;

extern options_t Options;

#define dzGUID "{DB45CF00-6209-11D4-8F10-CE19C874A858}"

#define SetDefaultOptions() \
{ \
	Options.Toolbar = 1; \
	Options.BigToolbar = 1; \
	Options.MenuIcons = 1; \
	Options.StatusBar = 1; \
	Options.ShowSelected = 1; \
	Options.RFLsize = 4; \
	Options.TypeCol = 1; \
	Options.PathCol = 0; \
	Options.AutoClose = 1; \
	Options.LCFilenames = 1; \
	Options.AllFilesInAdd = 1; \
	Options.ExpCmpWDz = 1; \
	Options.SmoothPBar = 1; \
}