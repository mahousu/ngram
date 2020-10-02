#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "parse.h"

/* filearg - for reading a list of things, either from a string (i.e.,
 * command-line argument) or from a file. Returns a semicolon-separated
 * list.
 * Its arguments are:
 *	arg - either the list itself, or "file=filename" which has it
 *	space, length - if the list is in a file, space to read it into.
 *		If no space is provided (space=NULL, length=0), it allocates
 *		it as needed.
 */

/* semicolonize - only set up temporary outposts */
void
semicolonize(char *space)
{
	while (*space) {
		if (isspace(*space)) {	/* duh? */
			if (*(space+1))
				*space = ';';
			else
				*space = '\0';
		}
		++space;
	}
}

char *
filetoarg(char *file, char *space, size_t length)
{
	FILE *fp;
	int actual;
	struct stat statbuf;

	if (!(fp = fopen(file, "r"))) {
		perror(file);
		return NULL;
	}
	if (!space || !length) {
		if (fstat(fileno(fp), &statbuf) >= 0) {
			length = statbuf.st_size;
		} else {
			/* ummm */
			length = BUFSIZ;
		}
		space = (char *)malloc(length+1);
		if (!space) return NULL;
	}
	actual = fread((void *)space, 1, length, fp);
	space[actual] = '\0';
	fclose(fp);
	semicolonize(space);
	return space;
}

char *
filearg(char *arg, char *space, int length)
{
	char *value;
	char *const tokens[] = {
		"file",
		NULL
	};

	while (*arg != '\0') {
		switch (getsubopt(&arg, tokens, &value)) {
			case 0:
				if (value == NULL) {
					fprintf(stderr, "Mising filename\n");
					return NULL;
				} else
					return filetoarg(value, space, length);
			default:
				return value;
		}
	}
}

#ifdef TEST
main(int argc, char **argv)
{
	int c;
	int i;
	char aspace[20];
	char bspace[30];
	char cspace[40];

	while ((c = getopt(argc, argv, "a:b:c:d:")) >= 0) {
		switch (c) {
		case 'a':
			printf("a arg: %s\n", optarg);
			printf("\t%s\n", filearg(optarg, aspace, 20));
			break;
		case 'b':
			printf("b arg: %s\n", optarg);
			printf("\t%s\n", filearg(optarg, bspace, 30));
			break;
		case 'c':
			printf("c arg: %s\n", optarg);
			printf("\t%s\n", filearg(optarg, cspace, 40));
			break;
		case 'd':
			printf("d arg: %s\n", optarg);
			printf("\t%s\n", filearg(optarg, NULL, 0));
			break;
		default:
			printf("unknown arg: %s\n", optarg);
			break;
		}
	}
	for (i=optind; i < argc; ++i) {
		printf("other arg: %s\n", argv[i]);
	}
}
#endif
