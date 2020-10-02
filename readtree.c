#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "readtree.h"
#include "parse.h"

/* Generic functions to recursively read all the files in a directory
 * tree. Well, sort of generic, at any rate.
 */

/* Read all the files in a directory */
int
readfilesdir(char *dir, int startymd, int endymd,
	int (*fileread)(char *file, int startymd, int endymd))
{
	DIR *dp;
	struct dirent *dirp;
	char cwd[BUFSIZ], *usecwd;
	int total=0;

	dp = opendir(dir);
	if (!dp) {
		perror(dir);
		return -1;
	}
	if (!getcwd(cwd, BUFSIZ)) {	/* oops */
		usecwd = getcwd(NULL, 0);
	} else {
		usecwd = cwd;
	}

	if (chdir(dir) < 0) {
		perror(dir);
		return -1;
	}
	while ((dirp = readdir(dp))) {
		/* Unfortunately, the implementation of readdir() is
		 * broken for certain file system types, including xfs.
		 * So we have to throw this in.
		 */
		if (dirp->d_type == DT_UNKNOWN) {
			struct stat statbuf;

			if (stat(dirp->d_name, &statbuf) < 0)
				continue;
			/* fake it */
			if (S_ISDIR(statbuf.st_mode)) {
				dirp->d_type = DT_DIR;
			} else if (S_ISREG(statbuf.st_mode)) {
				dirp->d_type = DT_REG;
			}
			/* else ignore ... */
		}
		if (dirp->d_type == DT_DIR) {
			/* Be careful ... */
			if (!strcmp(dirp->d_name, ".")
					|| !strcmp(dirp->d_name, ".."))
				continue;
			total += readfilesdir(dirp->d_name, startymd, endymd,
				fileread);
		} else if (dirp->d_type == DT_REG) {
			total += (*fileread)(dirp->d_name, startymd, endymd);
		}
	}
	closedir(dp);
	chdir(usecwd);
	/* Check if we had to allocate */
	if (usecwd != cwd)
		free((void *)usecwd);
	return total;
}

int
readallfiles(char *list, int startymd, int endymd,
	int (*fileread)(char *file, int startymd, int endymd))
{
	char *flist[BUFSIZ];	/* plenty */
	int nitems, i;
	int total=0;
	struct stat statbuf;

	nitems = parsenargs(list, flist, " ,;", BUFSIZ);
	for (i=0; i < nitems; ++i) {
		if (stat(flist[i], &statbuf) < 0) {
			perror(flist[i]);
			continue;
		}
		if (S_ISDIR(statbuf.st_mode)) {
			total += readfilesdir(flist[i], startymd, endymd,
				fileread);
		} else if (S_ISREG(statbuf.st_mode)) {
			total += (*fileread)(flist[i], startymd, endymd);
		}
		/* else ignore ... */
	}
	return total;
}
