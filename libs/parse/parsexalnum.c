#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include "parse.h"

struct srcdestprot {
	char *src;
	char *srcport;
	char *dest;
	char *destport;
	char *prot;
	char *cos;
};

struct addparamstring {		/* the string form of the above */
	char * sdelay;
		char * sdelsigma;
		char * sdelcorr;
	char * sdrop;
		char * sdropcorr;
	char * sdup;
		char * sdupcorr;
	char * sbandwidth;
	char * sdrdmin;
		char * sdrdmax;
		char * sdrdcongest;
};

/* Parse a set of extended alphanumeric strings separated by non-alpha
 * elements into a set of arrays. Chews up the string, replacing the
 * non-alnum elements with nuls. Returns the total number of arguments found.
 * DOES check the arrays for size.
 *
 * Usage:
 *	parsexalnum(char *string, int arraycount,
 *		char **args1, int limit1, char *extrachars1,
 *		char **args2, int limit2, char *extrachars2,
 *		...);
 * Here, args1,2... are the argument arrays to put the results into,
 * limit1,2... are the sizes of each array, and extrachars1,2...
 * are any extra (nonseparator) characters (beyond alphanumeric) allowed
 * in the arguments in this array. 
 *
 * Stops when the string is exhausted or it runs out of arguments to
 * put it in. In the latter case, everything remaining is tacked on
 * to the last argument. Returns the total number of arguments it found.
 */

__inline__ char *mystrchr(const char *string, int c)
{
	if (!string) return NULL;
	return strchr(string, c);
}
	
int
parsexalnum(char *string, int arraycount, ...)
{
	va_list ap;
	int i, j, total=0;
	char **args;
	int limit;
	char *extrachars;

	va_start(ap, arraycount);

	for (j=0; *string && j < arraycount; ++j) {
		args = va_arg(ap, char **);
		limit = va_arg(ap, int);
		extrachars = va_arg(ap, char *);
		if (!args)
			break;
		for (i=0; *string && i < limit; ++i) {
			while (*string && !isalnum(*string) &&
					!mystrchr(extrachars, *string)) {
				*string++ = '\0';
			}
			if (!*string)
				break;
			args[i] = string;
			while (*string &&
				(isalnum(*string) ||
					mystrchr(extrachars, *string))){
				++string;
			}
		}
		total += i;
	}
	return total;
}

/* For src/dest, we're using period as a separator - src.srcport. I
 * had sort of thought about dotted decimal addresses, but hey, this
 * is big-boy kernel stuff, we can do hex.
 */
int
parsesrcdestprot(char *string, struct srcdestprot *args)
{
	return parsexalnum(string, 1,
	    (char **)args, sizeof(struct srcdestprot)/sizeof(char *), NULL);
}

/* For the various parameters (times and whatnot), the periods are part
 * of the individual arguments.
 */
int
parseaddparamstring(char *string, struct addparamstring *args)
{
	return parsexalnum(string, 1,
	    (char **)args, sizeof(struct addparamstring)/sizeof(char *), ".");
}

int
parsenistnet(char *string,
	struct srcdestprot *sargs, struct addparamstring *aargs)
{
	return parsexalnum(string, 2,
	    (char **)sargs, sizeof(struct srcdestprot)/sizeof(char *), NULL,
	    (char **)aargs, sizeof(struct addparamstring)/sizeof(char *), ".");
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
	struct srcdestprot sargs;
	struct addparamstring aargs;

	while (gets(buffer)) {
		bzero(&sargs, sizeof(sargs));
		bzero(&aargs, sizeof(aargs));
		n = parsenistnet(buffer, &sargs, &aargs);
		printargs((char **)&sargs,
			sizeof(struct srcdestprot)/sizeof(char *));
		printargs((char **)&aargs,
			n - sizeof(struct srcdestprot)/sizeof(char *));
	}
}
#endif
