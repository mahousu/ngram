int
readfilesdir(char *dir, int startymd, int endymd,
	int (*fileread)(char *file, int startymd, int endymd));

int
readallfiles(char *list, int startymd, int endymd,
	int (*fileread)(char *file, int startymd, int endymd));
