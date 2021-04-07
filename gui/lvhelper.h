void __fastcall LVDeleteAllItems(void);
void __fastcall LVDeleteColumn(int column);
void __fastcall LVEditLabel(int item);
void __fastcall LVEnsureVisible(int item);
void __fastcall LVInsertColumn(void *lvcolumn, int location);
void __fastcall LVItemRect(RECT *r);
void __fastcall LVScroll(int cy);
void __fastcall LVSetColumnOrder(int num, int *array);
void __fastcall LVSetColumnWidth(int column, int width);
void __fastcall LVSetImageList(int imgl);
void __fastcall LVSetItemCount(int count);
void __fastcall LVSetItemState(int item, int state);// tampers with temp2[]
void __fastcall LVSetItemState_Mask(int item, int state, int mask);// tampers with temp2[]
int __fastcall LVColumnWidth(int column);
int __fastcall LVNumSelected(void);
int __fastcall LVTopIndex(void);
HWND __fastcall LVGetHeader(void);
