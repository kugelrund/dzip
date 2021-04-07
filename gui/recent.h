void RecentAddString(char ***, char *, char *);
void RecentClear(char *);
int RecentRead(char ***, char *, int);
void RecentSynchronize(int, int);
void RecentWrite(char **, char *, int);

struct recent_t {	// Recent.FileList[0] is name of the open file
	char NumFiles, NumExtractPaths, NumMovePaths;
	char **FileList, **ExtractPaths, **MovePaths;
};

extern struct recent_t Recent;