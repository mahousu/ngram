/* Here is a simple implementation of a counting Bloom filter,
 * intended for use in classifying n-grams drawn from network
 * packet data. Hopefully, though, it's written in a generic
 * enough fashion to be generally useful.
 */

/* "opaque" type */
struct _BloomFilter;
typedef struct _BloomFilter BloomFilter;

/* A Bloom filter needs a (parametrized) family of hash functions.
 * The usual theoretical analysis assumes all the hash functions
 * are independent, but in practice, we use a cheat: start with
 * two (good) hash functions h and g, then make hash function
 * h_i = h + i*g. It's not obvious that it should, but this tends
 * to work well enough in practice.
 *
 * We currently use 32-bit hash functions. This is under the assumption
 * that a hash table with 2^32 entries is big enough for anything we're
 * likely to use. If this proves false, we can rig things to squeeze some
 * more bits out of the hashes.
 *
 * As a further cheat, we use the two halves of the output of one 64-bit
 * hash function rather than two independent 32-bit ones. This at least
 * has justification in that the two halves of a good 64-bit hash should
 * appear independent.
 */
/* Everybody seems to define this on their own, so I will too. */
typedef union _hash64 {
	struct _h {
		u_int32_t a;
		u_int32_t b;
	} h;
	u_int64_t ab;
} Hash64;

int AddBloomHash64(Hash64 hash, BloomFilter *filter);

/* This definition is mostly suggestive;
 * in practice, we calculate all the hash values together rather than
 * one at a time.
 */
u_int32_t BloomHash(int i, void *item, size_t length);

/* The ideal size of a Bloom filter and the number of hash functions
 * to be used depend on the number of items to be inserted and the
 * acceptable false positive rate. The size of the counter should also
 * be somewhat dependent on these factors, but I figure, for now,
 * we'll just try 16 and 32 bits.
 */


#ifdef notdef
/*@@ don't need this - just use generic type */
/* A set of these */
struct _BloomFilterSet {
	Range size;
	BloomFilter *filters;
};
typedef struct _BloomFilterSet BloomFilterSet;
#endif /* notdef */

/* Create a counting Bloom filter of the requested size */
BloomFilter *
#ifdef SHMALLOC
NewBloomNgramFilter(int ngram, char *shmfilename, int mode);
#else
NewBloomNgramFilter(int ngram);
#endif

NgramFilterSet *
#ifdef SHMALLOC
NewBloomNgramFilterSet(Range ngram, char *shmfilename, int mode);
#else
NewBloomNgramFilterSet(Range ngram);
#endif

/* Basic operations */
/* Add the given string to the filter */
int
AddBloomFilter(void *item, size_t length, BloomFilter *filter);
/* Chop the given item into ngrams, and add them to the filter */
int
AddBloomNgramFilter(void *item, size_t length, int ngram, BloomFilter *vfilter);
int
AddBloomNgramFilterSet(void *item, size_t length, NgramFilterSet *vfilter);
/* Delete is sort of approximate - in the unlikely event all of the
 * counters are maxxed out, it won't work.
 */
int
DeleteBloomFilter(void *item, size_t length, BloomFilter *filter);
int
DeleteBloomNgramFilter(void *item, size_t length, int ngram, void *vfilter);
int
DeleteBloomNgramFilterSet(void *item, size_t length, NgramFilterSet *vfilter);
/* Find returns the (approximate) number of instances found, or 0 for none */
int
FindBloomFilter(void *item, int ngram, void *vfilter);
int
FindBloomNgramFilter(void *item, size_t length, int ngram, BloomFilter *filter);
int
FindBloomNgramFilterSet(void *item, size_t length, NgramFilterSet *vfilter);
int
FindBloomNgramDistFilter(void *item, size_t length, int ngram, double *mu, double *sigma, double *rho, NgramFilterSet *vfilter);
void DistBloomNgramFilter(int ngram, double *mu, double *sigma, u_int64_t *max, u_int64_t *min, NgramFilterSet *vfilter);
void DumpBloomNgramFilter(FILE *file, BloomFilter *vfilter);
void DumpBloomNgramFilterSet(FILE *file, NgramFilterSet *vfilter);
void CloseBloomNgramFilter(BloomFilter *vfilter);
void CloseBloomNgramFilterSet(NgramFilterSet *vfilter);


/* "Private" */

typedef struct _BloomFilter {
	/* k - number of hash functions
	 * m - size of hash table (i.e., number of 16 or 32 bit counters)
	 * n - total number of items inserted into filter
	 * d - number of distinct items inserted into filter (approximate)
	 */
	int k;
	/* For checking adequacy of counter size */
	size_t overflows;
	size_t underflows;
	size_t m, n, d;
	NgramCounter counter[0];
} BloomFilter;




