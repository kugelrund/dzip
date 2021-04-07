void CreatePBar(void);
void CreateStatusBar(void);
void PBarReset(void);
void PBarAddTotal(UINT);
void PBarAdvance(UINT);
void PBarSetSkip(UINT);
void PBarSkip(void);
void SBarResize(void);
void UpdateSBar(void);

extern HWND SBar, PBar;
extern short SBsize;