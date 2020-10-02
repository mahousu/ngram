#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "parse.h"

static int
mstrchr(const char *s, int c)
{
	if (!s) return isspace(c);
	return strchr(s, c) != NULL;
}

/* Parse a string into space-separated (or other) arguments. Chews up the
 * string, replacing the blanks with nuls. Returns the number of arguments
 * found. DOES check the array for size.
 *
 * Arguments:
 *	string	(in/out, changes) - string to be parsed
 *	args	(out, filled) - where to put result pointers
 *	seps	(in) - separator chars instead of space, NULL if none
 *	n	size of args array
 */
int
parsenargs(char *string, char *args[], char *seps, int n)
{
	int i;

	for (i=0; i < n && *string; ++i) {
		while (*string && mstrchr(seps, *string))
			*string++ = '\0';
		if (!*string)
			break;
		args[i] = string;
		while (*string && !mstrchr(seps, *string))
			++string;
	}
	return i;
}

#ifdef TEST
static void
printargs(char *args[], int n)
{
	int i;

	for (i=0; i < n; ++i)
		printf("%d %s ", i, args[i]);
	printf("\n");
}

main(int argc, char **argv)
{
	int i, n;
	int limit = BUFSIZ;
	char buffer[BUFSIZ];
	char *args[BUFSIZ];

	if (argc > 1) {
		if (argc > 2) {
			n = parsenargs(argv[i], args, argv[1], atoi(argv[2]));
			printargs(args, n);
		} else {
			n = parsenargs(argv[1], args, NULL, limit);
			printargs(args, n);
		}
	} else while (gets(buffer)) {
		n = parsenargs(buffer, args, NULL, limit);
		printargs(args, n);
	}
}
#endif


