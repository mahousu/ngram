#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <strings.h>
#include <memory.h>
#include "entropy.h"

#define OCTETSIZE	256		/* 8 bits */

/* We are given a range of bytes, and calculate their statistical
 * (byte-wise) entropy, on a range from 0 to 8.
 */

void
entropyp(u_int8_t *x, int limit, double *entropy)
{
	int count;
	int bytes[OCTETSIZE];

	int n=0, i;
	double sum=0.0;

	memset((void *)bytes, 0, OCTETSIZE*sizeof(int));
	for (i=0; i < limit; ++i) {
		++bytes[x[i]&0xff];	/* belt and suspenders */
	}
	/* Shall we be stupid about it? Yes, we shall. */
	*entropy = 0;
	for (i=0; i<OCTETSIZE; ++i) {
		if (bytes[i])
			*entropy += ((double)bytes[i]/(double)limit)*log2((double)limit/((double)bytes[i]));
	}
}

double
entropy(u_int8_t *x, int limit)
{
	double answer;

	entropyp(x, limit, &answer);
	return answer;
}

#ifdef TEST
fileentropy(FILE *fp)
{
	char buffer[BUFSIZ];
	int count, i;
	double ent;

	for (i=0; (count = fread(buffer, 1, BUFSIZ, fp)) > 0; ++i) {
		entropyp(buffer, count, &ent);
		printf("%d (%d): %8.4f\n", i, count, ent);
	}
}

main(int argc, char **argv)
{
	int i;
	FILE *fp;

	if (argc < 2) {
		fp = stdin;
	} else {
		fp = fopen(argv[1], "r");
		if (!fp) {
			perror(argv[1]);
			exit(1);
		}
	}
	fileentropy(fp);
}
#endif
