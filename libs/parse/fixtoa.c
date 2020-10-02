#include <stdio.h>
#include <ctype.h>
#include "parse.h"

/* How big can the decimal string be?
 * Answer: sign + 20 digits (for 64-bit) + point + decimal places
 * How many decimal places should there be?
 * Answer: considering this is a number to be converted back and
 * forth, there's no point in going beyond another 20 decimal places
 * or so. But, no big harm in allowing fairly ridiculous lengths.
 * 
 */
#define MAXPLACES BUFSIZ-22

/* Convert scaled/fixed point number to decimal string representation.
 * I.e., given an integer m which has been scaled up by a factor n,
 * return a string version xxx.yyy of the decimal representation of m/n.
 * This has to be done only using integral arithmetic so that it's
 * suitable for kernel use. The representation is stopped at the
 * given number of decimal places.
 */
char *
fixtoa(int num, int factor, int places)
{
	int div=0, rem=0;
	int whole, mantissa=0;
	int negative;
	static char answer[BUFSIZ];
	char *c;

	/* Don't let them goof with us */
	if (places < 0) places = 0;
	if (places > MAXPLACES) places = MAXPLACES;
	if (!factor) return NULL;

	/* I can never get the signs right, so punt the issue */
	negative = (num < 0)^(factor < 0);
	if (num < 0) num = -num;
	if (factor < 0) factor = -factor;

	whole = num/factor;
	rem = num%factor;

	if (negative)
		sprintf(answer, "-%d", whole);
	else
		sprintf(answer, "%d", whole);

	c = answer + strlen(answer);
	if (rem > 0 && places > 0) {
		*c++ = '.';
		while (rem > 0 && places > 0) {
			*c++ = (rem*10)/factor + '0';
			rem = (rem*10)%factor;
			--places;
		}
		*c = '\0';
	}
	return answer;
}

#ifdef TEST

main(int argc, char **argv)
{
	int factor, places;

	--argc;
	if (argc < 2) {
		fprintf(stderr, "fixtoa factor places num1 num2 ... (or < num)\n");
		exit(1);
	}
	factor = atoi(*++argv); --argc;
	places = atoi(*++argv); --argc;
	if (argc > 0) {
		while (argc > 0) {
			++argv; --argc;
			printf("%s -> %s\n", *argv,
				fixtoa(atoi(*argv), factor, places));
		}
	} else {
		char buffer[BUFSIZ];

		while (gets(buffer)) {
			printf("%s -> %s\n", buffer,
				fixtoa(atoi(buffer), factor, places));
		}
	}
}
#endif
	


