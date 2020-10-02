#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <ctype.h>
#include <sys/types.h>
#include "ngram.h"
#include "arrayngram.h"
#include "libstats.h"

/* The "generic" structure */
NgramFilterSet arrayset = {
	{0,0},
	NULL
};

NgramOps arrayops = {
	allocngramarrayrange,
	readngramsrange,
	delngramarrayrange,
	findarray,
	findngramarrayrange,
	finddistarray,
	distarrayrange,
	dumparrayrange,
	closearrayrange
};

Ngram array = {
	&arrayset,
	&arrayops
};

/* This code uses the ngram value as a direct index into the counter
 * array. Theoretically, on a 64-bit machine, this could allow ngrams
 * of length up to 8, though obviously, there's the slight issue of
 * allocating that big an array. In practice, we'll max out at 4.
 */
#define NGRAMSIZE(i)	(long)(1L<<(8*i))
#define NGRAMMASK(i)	(long)(NGRAMSIZE(i)-1L)

/* For checking adequacy of counter size */
int overflows;
int underflows;

/* For statistics sake */
size_t totalentries;
size_t distinctentries;

/* Calculate the ngram value; for the sake of definiteness, we always
 * do this in what amounts to big-endian fashion. Note that we could
 * do a cast+mask, on systems that don't constrain integers to be
 * even-aligned, if we didn't care about byte ordering.
 */
inline u_int32_t tongram(u_int8_t *p, int size) {
	u_int32_t answer=0;

	while (--size >= 0) {
		answer <<= 8;
		answer += p[size];
	}
	return answer;
}

u_int8_t *
tonarray(u_int32_t ngram, int size)
{
	static u_int8_t answer[4];

	memset(answer, 0, 4);
	while (--size >= 0) {
		answer[size] = ngram&NGRAMMASK(1);
		ngram >>= 8;
	}
	return answer;
}

NgramCounter *
#ifdef SHMALLOC
allocngramarray(int ngram, char *shmfilename, int mode)
#else
allocngramarray(int ngram)
#endif
{
	NgramCounter *ngrams;
	size_t bytesize;

	bytesize = sizeof(NgramCounter)*NGRAMSIZE(ngram);
	setngramlabel(NGRAM_ARRAY, NULL, ngram, bytesize, 0, 0);
#ifdef SHMALLOC
	ngrams = (NgramCounter *)ngram_shmalloc(bytesize, shmfilename, mode);
#else
	ngrams = (NgramCounter *)malloc(bytesize);
	if (ngrams)
		memset((void *)ngrams, 0, bytesize);
#endif
	return ngrams;
}

int
arrayfiltersetsize(Range ngram)
{
	return sizeof(NgramFilterSet) + (ngram.max-ngram.min+1)*sizeof(NgramCounter *);
}

NgramFilterSet *
#ifdef SHMALLOC
allocngramarrayrange(Range ngram, char *shmfilename, int mode)
#else
allocngramarrayrange(Range ngram)
#endif
{
	NgramFilterSet *answer;
	int ng;
	size_t bytesize = arrayfiltersetsize(ngram);

	/* To be honest, there's not a huge amount of reason to allocate
	 * this here, as the range of valid array sizes are for ng = 1 to 4.
	 * But this makes it uniform with the other methods.
	 */
	#ifdef SHMALLOC
	answer = (NgramFilterSet *)ngram_shmalloc(bytesize,
		shmfilename, mode);
#else
	answer = (NgramFilterSet *)malloc(bytesize);
	if (answer)
		memset((void *)answer, 0, bytesize);
#endif
	if (!answer) return NULL;
	answer->ngramsize = ngram;
	for (ng=ngram.min; ng <= ngram.max; ++ng) {
		answer->filter[ng] =
#ifdef SHMALLOC
			(NgramFilter)allocngramarray(ng, shmfilename, mode);
#else
			(NgramFilter)allocngramarray(ng);
#endif
	}
	return answer;

}

void
closearray(void *array)
{
#ifdef SHMALLOC
	ngram_shmfree(array);
#else
	free(array);
#endif
}

void
closearrayrange(NgramFilterSet *array)
{
	int ng;

	if (!array) return;
	for (ng=array->ngramsize.min; ng <= array->ngramsize.max; ++ng) {
		if (array->filter[ng]) {
			closearray(array->filter[ng]);
		}
	}
#ifdef SHMALLOC
	ngram_shmfree(array);
#else
	free(array);
#endif
}

int
readngrams(void *item, size_t length, int ngram, void *filter)
{
	u_int32_t ngramwork;
	size_t count=0, distinct=0;
	long int where;
	int i;
	u_int8_t *input = (u_int8_t *)item;
	NgramCounter *ngrams = (NgramCounter *)filter;

	/* Ignore tiny fragments */
	if (length < ngram) return 0;
	/* Initialize - note fill direction! */
	ngramwork = 0;
	for (i=0; i < ngram; ++i) {
		ngramwork <<= 8;
		ngramwork += input[i];
	}
	for (; i <= length; ++i) {
		++count;
		where = ((unsigned long)ngramwork&NGRAMMASK(ngram));
		if (!ngrams[where])
			++distinct;
		if (ngrams[where] < COUNTER_MAX)
			++ngrams[where];
		else {
			/* fprintf(stderr, "Item overflow in %d\n", spot); */
			++overflows;
		}
		if (i < length) {
			ngramwork <<= 8;
			ngramwork += input[i];
		}
	}
	totalentries += count;
	distinctentries += distinct;
	setngramlabel(NGRAM_ARRAY, NULL, ngram, 0, totalentries, distinctentries);
	return count;
}

int
readngramsrange(void *item, size_t length, NgramFilterSet *vfilter)
{
	int ret=0;
	int ng;

	for (ng=vfilter->ngramsize.min; ng <= vfilter->ngramsize.max; ++ng)
		ret += readngrams(item, length, ng, vfilter->filter[ng]);
	return ret;
}

/* For the delete, the only cautionary note is when a bucket is full */
int
delngramarray(void *item, size_t length, int ngram, void *filter)
{
	u_int32_t ngramwork;
	size_t count=0, indistinct=0;;
	long int where;
	int i;
	u_int8_t *input = (u_int8_t *)item;
	NgramCounter *ngrams = (NgramCounter *)filter;

	/* Ignore tiny fragments */
	if (length < ngram) return 0;
	/* Initialize - note fill direction! */
	ngramwork = 0;
	for (i=0; i < ngram; ++i) {
		ngramwork <<= 8;
		ngramwork += input[i];
	}
	for (; i <= length; ++i) {
		++count;
		where = ((unsigned long)ngramwork&NGRAMMASK(ngram));
		if (ngrams[where] < COUNTER_MAX)
			--ngrams[where];
		else {
			/* fprintf(stderr, "Item underflow in %d\n", spot); */
			++underflows;
		}
		if (!ngrams[where])
			++indistinct;
		if (i < length) {
			ngramwork <<= 8;
			ngramwork += input[i];
		}
	}
	totalentries -= count;
	distinctentries -= indistinct;
	setngramlabel(NGRAM_ARRAY, NULL, ngram, 0, totalentries, distinctentries);
	return count;
}

int
delngramarrayrange(void *item, size_t length, NgramFilterSet *vfilter)
{
	int ret=0;
	int ng;

	for (ng=vfilter->ngramsize.min; ng <= vfilter->ngramsize.max; ++ng)
		ret += delngramarray(item, length, ng, vfilter->filter[ng]);
	return ret;
}


/* Find an individual ngram in the array */
int
findarray(void *item, int ngram, void *filter)
{
	u_int32_t ngramwork;
	int count;
	long int where;
	int i;
	u_int8_t *input = (u_int8_t *)item;
	NgramCounter *ngrams = (NgramCounter *)filter;

	/* Initialize - note fill direction! */
	ngramwork = 0;
	for (i=0; i < ngram; ++i) {
		ngramwork <<= 8;
		ngramwork += input[i];
	}
	/* This is where the array is nice ... */
	where = ((unsigned long)ngramwork&NGRAMMASK(ngram));
	return ngrams[where];
}

/* I should make the following generic ... */
/* Chop an item into ngrams, and total up their (approximate) frequencies */
int
findngramarray(void *item, size_t length, int ngram, void *filter)
{
	int i;
	int frequency=0;

	if (length == ngram)
		return findarray((u_int8_t *)item, ngram, filter);

	for (i=0; i+ngram <= length; ++i) {
		frequency +=
			findarray((u_int8_t *)item + i, ngram, filter);
	}
	return frequency;
}

int
findngramarrayrange(void *item, size_t length, NgramFilterSet *vfilter)
{
        int i;
        int ret=0;

        for (i=vfilter->ngramsize.min; i <=vfilter->ngramsize.max; ++i) {
                ret += findngramarray(item, length, i, &vfilter->filter[i]);
        }
        return ret;
}


int
finddistarray(void *item, size_t length, int ng, double *mu,
		double *sigma, double *rho, NgramFilterSet *vfilter)
{
	int i;
	int total=0;
	static int frequencies[NGRAM_MAX];
	void *filter = &vfilter->filter[ng];

	for (i=0; i+ng <= length && i < NGRAM_MAX; ++i) {
		frequencies[i] =
			findarray((u_int8_t *)item + i, ng, filter);
		total += frequencies[i];
	}
	intarraystats(frequencies, i, mu, sigma, rho);
#ifdef notdef
	/* Scale to relative frequencies */
	*mu /= (double)total;
	*sigma /= (double)total;
	*rho /= (double)total;
#endif
	return total;
}

int
reportngram(FILE *file, u_int16_t count, int size, long int i)
{
	u_int8_t *p;
	int j;

	fprintf(file, "%d: ", count);
	p = tonarray(i, size);
	for (j=0; j < size; ++j) {
		if (isprint(p[j]))
			fprintf(file, " %c", p[j]);
		else
			fprintf(file, " (%d)", p[j]);
	}
	fprintf(file, "\n");
}

void
reportngrams(FILE *file, u_int16_t *ngrams, int size)
{

	long int i;
	int byte;
	long int total, ngramno, ngram10no, ngram1no, ngram01no;

	if (!file) return;
	total = ngramno = ngram10no = ngram1no = ngram01no = 0;
	for (i=0; i < NGRAMSIZE(size); ++i) {
		if (ngrams[i]) {
			++ngramno;
			total += ngrams[i];
			if (ngrams[i] >= 100) {
				if (dumplevel == 1)
					reportngram(file, ngrams[i], size, i);
				++ngram01no;
				if (ngrams[i] >= 1000) {
					++ngram1no;
					if (ngrams[i] >= 10000) {
						++ngram10no;
					}
				}
			}

			if (dumplevel > 1)
				reportngram(file, ngrams[i], size, i);
		}
	}
	fprintf(file, "ngram %d: %ld total %ld distinct %ld %ld %ld\n", size,
		total, ngramno, ngram10no, ngram1no, ngram01no);

	/*bloomarraystats(ngrams, sizeof(NgramCounter),
		NGRAMSIZE(size), &mu, &sigma, &sigmamu, &chisquare);*/
	/*printf("mu %9.6lf sigma %9.6lf sigma2/mu %9.6lf chisquare %9.6lf\n",
		mu, sigma, sigmamu, chisquare);*/
	fflush(stdout);
}

void
distarray(int ngram, double *mu, double *sigma, u_int64_t *max, u_int64_t *min, void *filter)
{
	NgramCounter *ngrams = (NgramCounter *)filter;
	double chisquare;

	bloomarraystats(ngrams, sizeof(NgramCounter),
		ngram, mu, sigma, max, min, &chisquare);
}

void
distarrayrange(int ngram, double *mu, double *sigma, u_int64_t *max, u_int64_t *min, NgramFilterSet *filter)
{
	return;
}


void
dumparray(FILE *file, int ngram, void *filter)
{
	NgramCounter *ngrams = (NgramCounter *)filter;

	reportngrams(file, ngrams, ngram);

}

void
dumparrayrange(FILE *file, NgramFilterSet *filter)
{
}

#ifdef TEST
long int
freadngrams(FILE *fp, int size, u_int16_t *ngrams)
{
	int length;
	u_int8_t buffer[BUFSIZ];
	long int totalcount = 0L;

	while ((length = fread(buffer, 1, BUFSIZ, fp))) {
		totalcount += 
			readngrams(buffer, length, size, ngrams);
	}
	return totalcount;
}

void
usage(void)
{
	fprintf(stderr, "Usage: ngramcount [-n ngram size] file1 file2 ...\n");
	fprintf(stderr, "ngram size from 1 to 4\n");
}

int dumplevel;

main(int argc, char **argv)
{
	int size=3;
	long int totalcount;
	FILE *fp;
	u_int16_t *ngrams;
	int c;
	extern char *optarg;
	extern int optind, opterr, optopt;


	while ((c = getopt(argc, argv, "n:d:")) >= 0) {
		switch (c) {
		case 'n':
			size = atoi(optarg);
			if (size > 4 || size < 1) {
				usage();
				exit(1);
			}
			break;
		case 'd':
			dumplevel = atoi(optarg);
			break;
		default:
			usage();
			exit(1);
		}
	}
	ngrams = allocngramarray(size);
	if (!ngrams) {
		perror("alloc");
		exit(2);
	}
	if (optind >= argc)
		totalcount = freadngrams(stdin, size, ngrams);
	else {
		totalcount = 0;
		for (; optind < argc; ++optind) {
			fp = fopen(argv[optind], "r");
			if (!fp) {
				perror(argv[optind]);
				exit(3);
			}
			totalcount += freadngrams(fp, size, ngrams);
			fclose(fp);
		}
	}

	if (dumplevel > 1)
		printf("%ld ngrams:\n", totalcount);
	reportngrams(stdout, ngrams, size);
	free((void *)ngrams);

	exit(0);
}
#endif
