#include <stdio.h>
#include <ctype.h>
#include "parse.h"

/* Parse a set of alphanumeric strings separated by non-alpha elements.
 * Chews up the string, replacing the non-alnum elements with nuls.
 * Returns the number of arguments found. Does NOT check the array for size.
 */
int
parsealnum(char *string, char *args[])
{
	int i;

	for (i=0; *string; ++i) {
		while (*string && !isalnum(*string) && *string != '.') {
			*string++ = '\0';
		}
		if (!*string)
			break;
		args[i] = string;
		while (*string && (isalnum(*string) || *string == '.')) {
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
		for (i=1; i < argc; ++i) {
			n = parsealnum(argv[i], args);
			printargs(args, n);
		}
	} else while (gets(buffer)) {
		n = parsealnum(buffer, args);
		printargs(args, n);
	}
}
#endif


