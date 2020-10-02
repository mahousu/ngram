#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <getopt.h>
#include "ngram.h"

/* Simple random walk with history. This super-simple version
 * has only 256 possible positions. We model this with an array,
 * where position (i,j) has a vector of cumulative probability 
 * distribution for next steps [0, ... 255]. We generate a random
 * number to figure out the next value k, then repeat with (j, k).
 *
 * We'll try a vector of 16-bit ints for the cumulative distribution,
 * normalized to 0->1 => 0->65535.
 *
 * For unsparse representation, this requires:
 *
 * 1-step (Markov)	128K
 * 2-step		32M
 * 3-step		8G
 *
 * Obviously, anything large would need a sparse/approximate representation.
 * Currently, the Bloom filter sets 25 as the maximum ngram size, so we'll
 * set 32 as a reasonable maximum history length.
 *
 * For this version, I'm using 32-bit ints for storage, since I'm not
 * caching results. If I do start caching, I'll use 32-bit for calculations
 * and 16-bit for storing.
 */
#define MAXSTEPS	32

#define VALUES	256

typedef struct _cumulative {
	int total;
	u_int32_t c[VALUES];
} Cumulative;

#ifdef SHMALLOC
char *shmfilename;
int shmmode = (O_CREAT);
#endif

/* simple hack - start vector of printing characters only */
int startprint = 1;

#ifdef DEBUG
char *
myprint1(int c)
{
	static char answer[BUFSIZ];

	if (isprint(c))
		sprintf(answer, "%c", c);
	else
		sprintf(answer, "\\%02x", c);
	return answer;
}
#endif


Cumulative *
newcumulative(void)
{
	Cumulative *c;

	c = (Cumulative *)malloc(sizeof(Cumulative));
	if (!c) return NULL;
	memset((void *)c, 0, sizeof(Cumulative));
	return c;
}

/* Yes, it's a little pointless to have a single static array to
 * store results, but this is a placeholder in case I change to
 * allocated/cached results.
 */
Cumulative *
fillcumulative(u_int8_t *steps, int nsteps, NGram *ngram, void *filter)
{
	static Cumulative answer;
	u_int8_t trialstep[MAXSTEPS];
	int i;

	memset(&answer, 0, sizeof(answer));
	memcpy(trialstep, steps, nsteps*sizeof(u_int8_t));
	answer.total = 0;
	for (i=0; i < VALUES; ++i) {
		trialstep[nsteps] = i;
		answer.c[i] = (*ngram->finditem)(trialstep+1,
			nsteps, nsteps, filter);
		answer.total += answer.c[i];
	}
	if (!answer.total) return NULL;
	return &answer;
}

/* Rewrite a frequency array into a cumulative distribution array */
void
crewrite(Cumulative *c)
{
	int i;
	long int accum;

	if (!c) return;
	for (i=0, accum=0; i < VALUES; ++i) {
		/* Ignore empties */
		if (c->c[i]) {
			accum += c->c[i];
			c->c[i] = ((accum*65536)/c->total - 1);
		}
	}
}

int
nextvalue(u_int8_t *steps, int nsteps, NGram *ngram, void *filter)
{
	int i, lasti=0;
	unsigned int target;
	Cumulative *cvals;

#ifdef DEBUG
	printf("\n");
	for (i=0; i < nsteps; ++i) {
		printf("%s", myprint1(steps[i]));
	}
	printf(": ");
#endif
	target = (unsigned int)(random()&0xffff);
	/* In a real version, we'd have caching of as many as
	 * would fit to avoid recalculation.
	 */
	cvals = fillcumulative(steps, nsteps, ngram, filter);
	if (!cvals)
		return -1;
	if (!cvals->total) {	/* Somehow landed on an empty */
		return -1;
	}
	crewrite(cvals);
	/* Eventually this will be replace with binary search */
	for (i=0; i < VALUES; ++i) {
		if (cvals->c[i]) {
#ifdef DEBUG
			printf("%s: %u ", myprint1(i), cvals->c[i]);
#endif
			lasti = i;
			if (cvals->c[i] >= target) {
#ifdef DEBUG
			printf("found\n");
#endif
				return i;
			}
		}
	}
#ifdef DEBUG
			printf("none\n");
#endif
	return lasti;
}

/* Fill out a steps array. Assumes there's enough space, i.e. nfill+nsteps */
void
fillsteps(u_int8_t *steps, int nsteps, NGram *ngram, void *filter, int nfill)
{
	int i;
	int n;
	int lastfail = -1, failhits=0;

	for (i=0; i < nfill; ++i) {
		n = nextvalue(steps+i, nsteps, ngram, filter);
		if (n < 0) {
fprintf(stderr, "Backup at %d\n", i);
			if (i == lastfail) {
				++failhits;
				i -= failhits*nsteps;
				if (i < 0) i = 0;
			} else {
				lastfail = i;
				failhits = 0;
				i -= 2;
			}
		} else
			steps[i+nsteps] = n;
	}
}


#define MAXTRIALS	80000000

/* Start off a steps array with something plausible. */
void
initialsteps(u_int8_t *steps, int nsteps, NGram *ngram, void *filter)
{
	int trial;
	int i;

	for (trial = 0; trial < MAXTRIALS; ++trial) {
		for (i = 0; i < nsteps; ++i) {
			do {
				steps[i] = (random()&0xff);
			} while (startprint && (!isprint(steps[i])));
		}
		if ((*ngram->finditem)(steps, nsteps, nsteps, filter))
			return;
	}
	fprintf(stderr, "Couldn't generate a start vector!\n");
	exit(1);
}


void
reporttotals(NGram *ngram, void *filter)
{
	double mu, sigma;
	u_int64_t max, min;

	(*ngram->diststats)(&mu, &sigma, &max, &min, filter);
	printf("distribution mu %lf sigma %lf max %ld min %ld\n",
		mu, sigma, max, min);
}

#ifdef TEST

FILE *dumpfile;
int dumplevel = 1;


void
main(int argc, char **argv)
{
	int ngramtype=0;
	int nsteps=2;
	int generate=20;
	int c;
	FILE *fp, *ifp;
	int i;
	u_int8_t *steps=NULL;
	u_int8_t *initial=NULL;
	char *initialfile=NULL;
	static u_int8_t initialbuffer[MAXSTEPS];
	extern NGram *ngram;
	void *filter;

	while ((c = getopt(argc, argv, "N:n:g:I:i:A:a:rPp")) >= 0) {
		switch (c) {
		case 'N':
			if (!strncasecmp(optarg, "bloom", 5)) {
				ngramtype = NGRAM_BLOOM;
			} else if (!strncasecmp(optarg, "array", 5)) {
				ngramtype = NGRAM_ARRAY;
			} else {
				fprintf(stderr, "Unknown filter %s\n",
					optarg);
				exit(1);
			}
                        setngramlabel(ngramtype, NULL, nsteps, 0, 0, 0);
                        break;
		case 'n':
			nsteps = atoi(optarg);
			if (nsteps > MAXSTEPS) {
				fprintf(stderr, "%d steps? No can do, boss\n",
					nsteps);
				exit(1);
			}
                        setngramlabel(ngramtype, NULL, nsteps, 0, 0, 0);
			break;
		case 'g':
			generate = atoi(optarg);
			break;
		case 'i':
			initial = (u_int8_t *)optarg;
			break;
		case 'I':
			initialfile = (u_int8_t *)optarg;
			break;
		case 'r':	/* random start */
			srandom(time(NULL));
			break;
#ifdef SHMALLOC
		case 'a':
			shmfilename = optarg;
			ngram_peek(shmfilename);
			shmmode = (O_CREAT);
			break;
		case 'A':
			shmfilename = optarg;
			shmmode = (O_CREAT|O_TRUNC);
			break;
#endif
		case 'p':
			startprint = 0;
			break;
		case 'P':
			startprint = 1;
			break;

		}
	}
	if (initialfile) {
		ifp = fopen(initialfile, "r");
		if (!ifp) {
			perror(initialfile);
			exit(1);
		}
		fread(initialbuffer, 1, nsteps, ifp);
		fclose(ifp);
		initial = initialbuffer;
	}
	steps = (u_int8_t *)malloc((generate+nsteps)*sizeof(u_int8_t));
#ifdef SHMALLOC
	filter = (*ngram->newfilter)(nsteps, shmfilename, shmmode);
#else
	filter = (*ngram->newfilter)(nsteps);
#endif
	if (!filter) {
		perror("malloc");
		exit(1);
	}
	if (optind >= argc) {
		ngramreadfile(stdin, nsteps, ngram, filter);
	} else {
		for (i=optind; i < argc; ++i) {
			fp = fopen(argv[i], "r");
			if (!fp) {
				perror(argv[i]);
				continue;
			}
			ngramreadfile(fp, nsteps, ngram, filter);
			fclose(fp);
		}
	}
	/*reporttotals(ngram, filter);*/
	/*dumpcumulative(nsteps, storage);*/
	/*dumpcumulative(nsteps, storage);*/
	if (initial) {
		memcpy(steps, initial, strlen((char *)initial));
	} else {
		initialsteps(steps, nsteps, ngram, filter);
	}
	fillsteps(steps, nsteps, ngram, filter, generate);
	for (i=0; i < generate; ++i) {
		putchar(steps[i]);
	}
	fflush(stdout);
	exit(0);
}
#endif /* TEST */
