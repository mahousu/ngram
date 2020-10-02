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
 * string, replacing the blanks with nuls. Allows for quote characters for
 * escaping separators. Returns the number of arguments
 * found. Does NOT check the array for size.
 *
 * Arguments:
 *	string	(in/out, changes) - string to be parsed
 *	args	(out, filled) - where to put result pointers
 *	seps	(in) - separator chars instead of space, NULL if none
 *	quotes	(in) - quote characters instead of ", NULL if none
 */
int
parsequoteargs(char *string, char *args[], char *seps, char *quotes)
{
	int i;
	static char stdquotes[] = "\"\'";

	if (!quotes) quotes = stdquotes;
	for (i=0; *string; ++i) {
		while (*string && mstrchr(seps, *string))
			*string++ = '\0';
		if (!*string)
			break;
		if (mstrchr(quotes, *string)) {
			*string++ = '\0';
			args[i] = string;
			while (*string && !mstrchr(quotes, *string))
				++string;
			*string++ = '\0';
		} else {
			args[i] = string;
			while (*string && !mstrchr(seps, *string) &&
				!mstrchr(quotes, *string))
			++string;
		}
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
	char buffer[BUFSIZ];
	char *args[BUFSIZ];

	if (argc > 1) {
		if (argc > 2) {
			for (i=2; i < argc; ++i) {
				n = parsequoteargs(argv[i], args, argv[1], "\"\'");
				printargs(args, n);
			}
		} else {
			n = parsequoteargs(argv[1], args, NULL, NULL);
			printargs(args, n);
		}
	} else while (gets(buffer)) {
		n = parsequoteargs(buffer, args, NULL, NULL);
		printargs(args, n);
	}
}
#endif


