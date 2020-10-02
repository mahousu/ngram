#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <malloc.h>
#include <math.h>
#include <sys/mman.h>
#include "libstats.h"
/* For the "generic" ngram structure */
#include "ngram.h"
#include <stdint.h>
#include "fnv.h"
#include "fnvrange.h"

#include "bloom.h"

NgramFilterSet bloomset = {
	{0,0},
	NULL
};
NgramOps bloomops = {
	NewBloomNgramFilterSet,
	AddBloomNgramFilterSet,
	DeleteBloomNgramFilterSet,
	FindBloomFilter,
	FindBloomNgramFilterSet,
	FindBloomNgramDistFilter,
	DistBloomNgramFilter,
	DumpBloomNgramFilterSet,
	CloseBloomNgramFilterSet
};

Ngram bloom = {
	&bloomset,
	&bloomops
};

/* Guess at a good size, based on projected number of entries.
 */
static inline size_t
BloomSizeByEntries(size_t entries)
{
	/* p = exp(-(m/n)*(ln 2)^2) so
	 * m = -n * ln p / (ln 2)^2
	 * or m = n * (ln 10^6 - ln r)/(ln 2)^2
	 * where r is error rate per million.
	 * Call this n * a.
	 * A few calculations give this approximate table
	 *	r	a
	 *	.1	33.54
	 *	.5	30.2
	 *	1	28.75
	 *	10	23.96
	 *	100	19.17
	 * Let's just go with a = 32.
	 */
#ifdef SMALLBLOOM
	return entries*20;
#else
	return entries*32;
#endif
}

/* Guess at a number of entries, based on n-gram length */
static inline size_t
BloomEntries(int ngram)
{
	/* 2^(n+15) - just a wild guess based on some preliminary
	 * measurements. It's about right for 5 <= n <= 10, at least.
	 * Probably should be increased for lower n, and decreased
	 * for higher n.
	 */
	/* After a few more trials, I'm going with this:
	 * 2^(n/4 + 23). Let's just use a table.
	 */
#define MAXNGRAM 25
	size_t bloomsizes[MAXNGRAM+1] = {
		0,		/* heh, heh */
		/* Bloom filters for n < 4 (and even n=4) don't make
		 * much sense, but let's include values for them.
		 *
		 * 1 to 3 are absolute (for our hash functions) - that is,
		 * they result in no clashes whatsoever. The numbers aren't
		 * tightly tuned, but it's not that important.
		 */
		600,
		100000,
		20000000,	/* yes, bigger than the next two ... */
		/* 4 and up are from the formula */
		16777216,	/*4*/
		19951585,	/*5*/
		23726566,	/*6*/
		28215802,	/*7*/
		33554432,	/*8*/
		39903169,	/*9*/
		47453133,	/*10*/
		56431603,	/*11*/
		67108864,	/*12*/
		79806339,	/*13*/
		94906266,	/*14*/
		112863206,	/*15*/
		/* Anything above this size is unlikely to be useful */
		134217728,	/*16*/
		159612677,	/*17*/
		189812531,	/*18*/
		225726413,	/*19*/
		268435456,	/*20*/
		319225354,	/*21*/
		379625063,	/*22*/
		451452825,	/*23*/
		536870912,	/*24*/
		638450708	/*25 - about 38G; that's enough ... */
	};

	if (ngram < 0 || ngram > MAXNGRAM)
		ngram = MAXNGRAM;
	/*return (size_t)rint(exp(log(2.0)*(23.0 + (double)ngram/4.0)));*/
	return bloomsizes[ngram];
}

/* The two combined */
size_t
BloomSize(int ngram)
{
	return BloomSizeByEntries(BloomEntries(ngram));
}

/* Now package this with the above */
BloomFilter *
#ifdef SHMALLOC
NewBloomNgramFilter(int ngram, char *shmfilename, int mode)
#else
NewBloomNgramFilter(int ngram)
#endif
{
	BloomFilter *answer;
	size_t size, bytesize;

	size = BloomSize(ngram);
	bytesize = sizeof(BloomFilter) + size*sizeof(NgramCounter);
	setngramlabel(NGRAM_BLOOM, NULL, ngram, bytesize, 0, 0);
#ifdef SHMALLOC
	answer = (BloomFilter *)ngram_shmalloc(bytesize,
		shmfilename, mode);
#else
	answer = (BloomFilter *)malloc(bytesize);
	if (answer)
		memset((void *)answer, 0, bytesize);
#endif
	if (!answer) {
		fprintf(stderr, "Couldn't allocate %ld\n", bytesize);
		perror("malloc");
		return NULL;
	}
	answer->m = size;
	/* k = (m/n)*ln 2. Here (see below), we're using a fixed value
	 * for m/n of 32. So k = 22 or 23.
	 */
#ifdef SMALLBLOOM
	answer->k = 14;
#else
	answer->k = 23;
#endif
	return answer;
}

int
BloomFilterSetSize(Range ngram)
{
	return sizeof(NgramFilterSet) + (ngram.max-ngram.min+1)*sizeof(BloomFilter *);
}

NgramFilterSet *
#ifdef SHMALLOC
NewBloomNgramFilterSet(Range ngram, char *shmfilename, int mode)
#else
NewBloomNgramFilterSet(Range ngram)
#endif
{
	NgramFilterSet *answer;
	int ng;
	size_t bytesize = BloomFilterSetSize(ngram);

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
			(NgramFilter)NewBloomNgramFilter(ng, shmfilename, mode);
#else
			(NgramFilter)NewBloomNgramFilter(ng);
#endif
	}
	return answer;
}


void
CloseBloomNgramFilter(BloomFilter *array)
{
if (!array) return;
#ifdef SHMALLOC
	ngram_shmfree((void *)array);
#else
	free((void *)array);
#endif
}

void
CloseBloomNgramFilterSet(NgramFilterSet *array)
{
	int ng;

	if (!array) return;
	for (ng=array->ngramsize.min; ng <= array->ngramsize.max; ++ng) {
		if (array->filter[ng]) {
			CloseBloomNgramFilter((BloomFilter *)array->filter[ng]);
		}
	}
#ifdef SHMALLOC
	ngram_shmfree(array);
#else
	free(array);
#endif
}

/* Bloom filters need a family of hash functions. The usual
 * method for producing them is to start with two functions h and g,
 * and  then use h + i*g as the ith hash function.
 *
 * Here, we use the (updated) 64-bit version of the FNV hash, divided
 * into two 32-bit hash values. If we ever needed to expand beyond
 * this, we could rework the hash to squeeze out a few more bits.
 */
/* This function isn't actually used, since we always do a range of
 * hashes at once.
 */
u_int32_t
BloomHash(int i, void *item, size_t length)
{
	u_int32_t h, g;
	Hash64 hval;

	hval.ab = (u_int64_t)fnv_64_buf(item, length, FNV1_64_INIT);
	h = hval.h.a;
	g = hval.h.b;
	return h + (u_int32_t)i*g;
}


/* Add an item to a Bloom filter. Done with atomic operations, so
 * this can be run in parallel.
 *
 * Again, this function isn't actually used, since we always do a range of
 * hashes at once.
 */
int
AddBloomFilter(void *item, size_t length, BloomFilter *filter)
{
	u_int32_t i;
	u_int32_t h, g;
	size_t spot;
	int distinct = 0;
	int ret=0;
	u_int32_t newval;
	Hash64 hval;

	hval.ab = (u_int64_t)fnv_64_buf(item, length, FNV1_64_INIT);
	h = hval.h.a;
	g = hval.h.b;
	/* @@ Trivial optimization */
	if (h > filter->m)
		h %= filter->m;
	if (g > filter->m)
		g %= filter->m;
	spot = h;
	for (i=0; i < filter->k; ++i) {
		/* @@ Trivial optimization part 2 */
		if (spot > filter->m)
			spot -= filter->m;
#ifdef NGRAM_PARALLEL
		newval = __atomic_add_fetch(filter->counter+spot, (u_int32_t)1,
				__ATOMIC_RELAXED);
#else
		newval = ++filter->counter[spot];
#endif
		/*@@ check for overflow */
		if (newval >= COUNTER_MAX) {
			/* fprintf(stderr, "Item overflow in %d\n", spot); */
#ifdef NGRAM_PARALLEL
			(void) __atomic_add_fetch(&filter->overflows,
				(size_t)1, __ATOMIC_RELAXED);
			__atomic_store_n (filter->counter+spot,
				(u_int32_t)COUNTER_MAX, __ATOMIC_RELAXED);
#else
			++filter->overflows;
			filter->counter[spot] = COUNTER_MAX;
#endif
			++ret;
		} else if (newval == 1) {
			/* We can't always tell if an item is new, but
			 * if this spot was empty, then it has to be.
			 */
				distinct = 1;
		}
		spot += g;
	}
	/* total number of items */
#ifdef NGRAM_PARALLEL
	(void) __atomic_add_fetch(&filter->n, (size_t)1,
		__ATOMIC_RELAXED);
#else
		++filter->n;
#endif
	/* number of distinct items */
	if (distinct) {
#ifdef NGRAM_PARALLEL
		(void) __atomic_add_fetch(&filter->d, (size_t)distinct,
			__ATOMIC_RELAXED);
#else
		++filter->d;
#endif
	}
	return ret;
}

/* Split a 64-bit hash into two 32-bit parts, and use them to
 * generate Bloom filter entries.
 *
 * Done with atomic operations, so this can be run in parallel.
 *
 * Returns number of overflows, if any.
 */
int
AddBloomHash64(Hash64 hash, BloomFilter *filter)
{
	u_int32_t h, g;
	size_t spot;
	u_int32_t i;
	u_int32_t newval;
	int ret=0;
	int distinct = 0;

	h = hash.h.a;
	g = hash.h.b;
	/* @@ Trivial optimization */
	if (h > filter->m)
		h %= filter->m;
	if (g > filter->m)
		g %= filter->m;

	spot = h;
	for (i=0; i < filter->k; ++i) {
		/* @@ Trivial optimization part 2 */
		/* equal to spot = (h + i*k)%filter->m */
		if (spot > filter->m)
			spot -= filter->m;
#ifdef NGRAM_PARALLEL
		newval = __atomic_add_fetch(filter->counter+spot, (u_int32_t)1,
				__ATOMIC_RELAXED);
#else
		newval = ++filter->counter[spot];
#endif
		if (newval >= COUNTER_MAX) {
			/* fprintf(stderr, "Item overflow in %d\n", spot); */
#ifdef NGRAM_PARALLEL
			(void) __atomic_add_fetch(&filter->overflows,
				(size_t)1, __ATOMIC_RELAXED);
			__atomic_store_n (filter->counter+spot,
				(u_int32_t)COUNTER_MAX, __ATOMIC_RELAXED);
#else
			++filter->overflows;
			filter->counter[spot] = COUNTER_MAX;
#endif
			++ret;
		} else if (newval == 1) {
			/* We can't always tell if an item is new, but
			 * if this spot was empty, then it has to be.
			 */
				distinct = 1;
		}
		spot += g;
	}
	/* total number of items */
#ifdef NGRAM_PARALLEL
	(void) __atomic_add_fetch(&filter->n, (size_t)1,
		__ATOMIC_RELAXED);
#else
		++filter->n;
#endif
	/* number of distinct items */
	if (distinct) {
#ifdef NGRAM_PARALLEL
		(void) __atomic_add_fetch(&filter->d, (size_t)distinct,
			__ATOMIC_RELAXED);
#else
		++filter->d;
#endif
	}
	return ret;
}

/* Add a range of items to a Bloom filter set.
 */
int
AddBloomFilterRange(void *item, int length, Range range, NgramFilterSet *filter)
{
	u_int32_t i;
	int ret=0;
	Hash64 hvals[RANGEMAX];
	

	/* We limit the range to the length of the item. */
	if (range.min > length) return 0;
	if (range.max > length) range.max = length;
	
	/* Hash over the range */
	fnv_64_buf_range(item, length, FNV1_64_INIT,
		range.min, range.max, (Fnv64_t *)hvals);
	/* Now use each 64-bit hash to generate k spots in the bloom filter */
	for (i=range.min; i <= range.max; ++i) {
		ret += AddBloomHash64(hvals[i-range.min], filter->filter[i]);
	}
	return ret;
}

/* Chop an item into ngrams, and add them to a Bloom filter. Here, we
 * do overlapping ngrams.
 */
int
AddBloomNgramFilter(void *item, size_t length, int ngram, BloomFilter *filter)
{
	int i;
	int ret=0;

	for (i=0; i+ngram <= length; ++i) {
		ret += AddBloomFilter((u_int8_t *)item + i, ngram, filter);
	}
	/* Record stats */
	setngramlabel(NGRAM_BLOOM, NULL, ngram, 0, filter->n, filter->d);
	return ret;
}

/* Here, we add the pieces to a set of filters in parallel. Well,
 * parallel-ish, at any rate.
 */
int
AddBloomNgramFilterSet(void *item, size_t length, NgramFilterSet *vfilter)
{
	int i, j;
	int ret=0;
	Range r = vfilter->ngramsize;
	
	/* We do the whole range each time until we butt up against
	 * the end, where we have to ramp down and only do the amount
	 * that fits.
	 */
	for (i=0; i+r.max <= length; ++i) {
		ret += AddBloomFilterRange(item+i, length, r, vfilter);
	}
	/* Ramp down to get the rest of the hashes */
	for (--r.max; r.max >= r.min && i+r.max <= length;
			++i, --r.max) {
		ret += AddBloomFilterRange(item+i, length, r, vfilter);
	}
	/* Now record the stats */
	for (i=bloomset.ngramsize.min; i <= bloomset.ngramsize.max; ++i) {
		setngramlabel(NGRAM_BLOOM, NULL, i, 0,
			((BloomFilter *)vfilter->filter[i])->n,
			((BloomFilter *)vfilter->filter[i])->d);
	}
	return ret;
}


/* Delete an item from a Bloom filter. */
int
DeleteBloomFilter(void *item, size_t length, BloomFilter *filter)
{
	u_int32_t i;
	u_int32_t h, g;
	size_t spot;
	int indistinct=0;
	int ret=0;
	Hash64 hval;

	hval.ab = (u_int64_t)fnv_64_buf(item, length, FNV1_64_INIT);
	h = hval.h.a;
	g = hval.h.b;
	/* First, check if it's there ... */
	for (i=0; i < filter->k; ++i) {
		spot = ((size_t)h+(size_t)i*g)%filter->m;
		if (!filter->counter[spot]) {
			/* fprintf(stderr, "Item not found\n"); */
			return -1;
		}
	}
	for (i=0; i < filter->k; ++i) {
		spot = ((size_t)h+(size_t)i*g)%filter->m;
		/*@@ check if we're at max */
		if (filter->counter[spot] == COUNTER_MAX) {
			/* fprintf(stderr, "Item underflow in %d\n", spot); */
			++filter->underflows;
			++ret;
		} else {
			--filter->counter[spot];
			if (!filter->counter[spot])
				++indistinct;
		}
	}
	--filter->n;	/* total number of items */
	if (indistinct)	/* because of clashes, can't get a exact count, but..*/
		--filter->d;
	return ret;
}

/* Chop an item into ngrams, and delete them from a Bloom filter. Here, we
 * do overlapping ngrams.
 */
int
DeleteBloomNgramFilter(void *item, size_t length, int ngram,
	void *vfilter)
{
	int i;
	int ret=0;
	BloomFilter *filter = (BloomFilter *)vfilter;

	for (i=0; i+ngram <= length; ++i) {
		ret += DeleteBloomFilter((u_int8_t *)item + i, ngram, filter);
	}
	/* Record stats */
	setngramlabel(NGRAM_BLOOM, NULL, ngram, 0, filter->n, filter->d);
	return ret;
}

int
DeleteBloomNgramFilterSet(void *item, size_t length, NgramFilterSet *vfilter)
{
	int i;
	int ret=0;

	for (i=vfilter->ngramsize.min; i <=vfilter->ngramsize.max; ++i) {
		ret += DeleteBloomNgramFilter(item, length, i,
				&vfilter->filter[i]);
	}
	return ret;
}

/* Find an item in a Bloom filter, and return its (approximate) frequency */
int
FindBloomFilter(void *item, int ngram, void *vfilter)
{
	u_int32_t i;
	u_int32_t h, g;
	size_t spot;
	int frequency=0;
	BloomFilter *filter = (BloomFilter *)vfilter;
	Hash64 hval;

	hval.ab = (u_int64_t)fnv_64_buf(item, (size_t)ngram, FNV1_64_INIT);
	h = hval.h.a;
	g = hval.h.b;
	for (i=0; i < filter->k; ++i) {
		spot = ((size_t)h+(size_t)i*g)%filter->m;
		if (!frequency)
			frequency = filter->counter[spot];
		else if (frequency > filter->counter[spot])
			frequency = filter->counter[spot];
		if (!frequency) {
			/* fprintf(stderr, "Item not found\n"); */
			return 0;
		}
	}
	return frequency;
}

/* Chop an item into ngrams, and total up their (approximate) frequencies */
int
FindBloomNgramFilter(void *item, size_t length, int ngram, BloomFilter *filter)
{
	int i;
	int frequency=0;

	if (length == ngram)
		return FindBloomFilter((u_int8_t *)item, ngram, filter);

	for (i=0; i+ngram <= length; ++i) {
		frequency +=
			FindBloomFilter((u_int8_t *)item + i, ngram, filter);
	}
	return frequency;
}

int
FindBloomNgramFilterSet(void *item, size_t length, NgramFilterSet *vfilter)
{
	int i;
	int ret=0;

	for (i=vfilter->ngramsize.min; i <=vfilter->ngramsize.max; ++i) {
		ret += FindBloomNgramFilter(item, length, i,
				(BloomFilter *)&vfilter->filter[i]);
	}
	return ret;
}

/* The same as above, except extract some more refined(?) statistics */
int
FindBloomNgramDistFilter(void *item, size_t length, int ngram, double *mu, double *sigma, double *rho, NgramFilterSet *vfilter)
{
	int i;
	int total=0;
	static int frequencies[NGRAM_MAX];

	for (i=0; i+ngram <= length && i < NGRAM_MAX; ++i) {
		frequencies[i] =
			FindBloomFilter((u_int8_t *)item + i, ngram,
				vfilter->filter[ngram]);
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

/* Dump some simple statistics about the whole filter */
void
DistBloomNgramFilter(int ngram, double *mu, double *sigma,
	u_int64_t *max, u_int64_t *min, NgramFilterSet *vfilter)
{
	double chisquare;
	size_t nonzero;
	BloomFilter *filter = (BloomFilter *)vfilter->filter[ngram];

	/*printf("Bloom filter k %d m %ld n %ld d %ld\n",
		filter->k, filter->m, filter->n, filter->d);*/
	bloomarraystats(filter->counter, sizeof(NgramCounter),
		filter->m, mu, sigma, max, min, &chisquare);
	/*printf("mu %9.6lf sigma %9.6lf sigma2/mu %9.6lf chisquare %9.6lf\n",
		mu, sigma, sigma*sigma/mu, chisquare);*/
	/* Do some histogram stuff here ... */
}

void
DumpBloomNgramFilter(FILE *dumpfile, BloomFilter *filter)
{
	size_t i;

	if (!dumpfile) return;
	for (i=0; i < filter->m; ++i)
		if (filter->counter[i]) 
			fprintf(dumpfile, "%ld %d\n", i, filter->counter[i]);
	fclose(dumpfile);
	dumpfile = NULL;
}

void
DumpBloomNgramFilterSet(FILE *dumpfile, NgramFilterSet *vfilter)
{
	return;
}

#ifdef TEST
void
getngrams(FILE *fp, int ngram, BloomFilter *filter)
{
	char line[BUFSIZ];
	size_t len;

	while ((len = fread(line, 1, BUFSIZ, fp)) > 0) {
		AddBloomNgramFilter((void *)line, len, ngram, (void *)filter);
	}
}

FILE *dumpfile=NULL;

main(int argc, char **argv)
{
	int n, ngram = 3;
	FILE *fp;
	int i;
	int c;
	char line[BUFSIZ];
	int len;
	int count;
	BloomFilter *filter=NULL;
	double mu, sigma, rho;

	
	while  ((c = getopt(argc, argv, "n:")) >= 0) {
		switch (c) {
		case 'n':
			ngram = atoi(optarg);
			break;
		}
	}
	filter = NewBloomNgramFilter(ngram);
	if (optind >= argc)
		getngrams(stdin, ngram, filter);
	else {
		fp = fopen(argv[optind], "r");
		getngrams(fp, ngram, filter);
		fclose(fp);
	}

	DumpBloomNgramFilter(stdout, filter);

#ifdef notdef
	while (1) {
		printf("Lookup? "); gets(line);
		if (strlen(line) < ngram) break;
		count = FindBloomNgramDistFilter((void *)line, strlen(line), ngram, &mu, &sigma, &rho, filter);
		printf("Ngrams found %d times out of %d (%d distinct)\n", count, filter->n, filter->d);
		printf("Mu %10.8lf sigma %10.8lf rho %10.8lf\n", mu, sigma, rho);
	}
#endif
}
#endif
