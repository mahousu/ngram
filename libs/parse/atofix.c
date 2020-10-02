#include <stdio.h>
#include <ctype.h>
#include "parse.h"

/* Convert decimal string representation to a scaled/fixed point number.
 * I.e., given a string xxx.yyy and a factor n, return (int)(xxx.yyy*n).
 * This has to be done only using integral arithmetic so that it's
 * suitable for kernel use. Since it is used on numbers read in from
 * userspace, we do have to be a bit careful about overflow.
 */
int
atofix(char *string, int factor)
{
	int answer=0, chop=10;
	int div=0, rem=0;
	int whole, mantissa=0;
	int negative;
	int round=0;

	/* Don't bother if they're goofing with us */
	if (!string || !factor) return 0;
	/* Look for an explicit minus sign for negatives */
	while (isspace(*string)) ++string;
	negative = (*string == '-');
	if (negative)
		++string;
	/* Avoid foolishness with negative factors */
	if (factor < 0) {
		negative = !negative;
		factor = -factor;
	}

	whole = atoi(string);
	if (string = index(string, '.')) {
		char *end;

		/* Easiest to do by going to the end and backing up */
		end = string;
		++end;
		while (isdigit(*end))
			++end;
		--end;
		while (end != string) {
			mantissa += factor*(*end - '0');
			/* Should we round up? Eh, I guess... */
			round += 10*(mantissa%10 >= 5);
			mantissa /= 10;
			round /= 10;
			--end;
		}
	}
	answer = whole*factor + mantissa + round;
	return negative ? -answer : answer;
}

#ifdef TEST

main(int argc, char **argv)
{
	int factor;

	--argc;
	if (argc < 1) {
		fprintf(stderr, "atofix factor str1 str2 ... (or < string)\n");
		exit(1);
	}
	factor = atoi(*++argv); --argc;
	if (argc > 0) {
		while (argc > 0) {
			++argv; --argc;
			printf("%s -> %d\n", *argv, atofix(*argv, factor));
		}
	} else {
		char buffer[BUFSIZ];

		while (gets(buffer)) {
			printf("%s -> %d\n", buffer, atofix(buffer, factor));
		}
	}
}
#endif
	


