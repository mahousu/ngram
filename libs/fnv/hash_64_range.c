/* Run the FNV 64 hash algorithm incrementally over a range of lengths.
 * E.g., for input abcdefghij and range 3-10, run it over abc, abcd, ...
 * abcdefghij.
 *
 * Return the results in a provided array.
 *
 * This trivial extension of the fnv code is also in the public domain.
 *
 * Mark Carson
 */

#include <stdlib.h>
#include "fnv.h"
#include "fnvrange.h"


/*
 * FNV-0 defines the initial basis to be zero
 */
#if !defined(HAVE_64BIT_LONG_LONG)
const Fnv64_t fnv0_64_init = { 0UL, 0UL };
#endif /* ! HAVE_64BIT_LONG_LONG */


/*
 * FNV-1 defines the initial basis to be non-zero
 */
#if !defined(HAVE_64BIT_LONG_LONG)
const Fnv64_t fnv1_64_init = { 0x84222325UL, 0xcbf29ce4UL };
#endif /* ! HAVE_64BIT_LONG_LONG */


/*
 * 64 bit magic FNV-0 and FNV-1 prime
 */
#if defined(HAVE_64BIT_LONG_LONG)
#define FNV_64_PRIME ((Fnv64_t)0x100000001b3ULL)
#else /* HAVE_64BIT_LONG_LONG */
#define FNV_64_PRIME_LOW ((unsigned long)0x1b3)	/* lower bits of FNV prime */
#define FNV_64_PRIME_SHIFT (8)		/* top FNV prime shift above 2^32 */
#endif /* HAVE_64BIT_LONG_LONG */


/*
 * fnv_64_buf_range - perform a 64 bit Fowler/Noll/Vo hash on a range of
 * lengths in a buffer
 *
 * input:
 *	buf	- start of buffer to hash
 *	len	- length of buffer in octets
 *	hval	- previous hash value or 0 if first call
 *	low	- start of length range
 *	high	- end of length range
 *
 * returns:
 *	array of high-low+1 64 bit hashes as a static hash type
 *
 * NOTE: To use the 64 bit FNV-0 historic hash, use FNV0_64_INIT as the hval
 *	 argument on the first call to either fnv_64_buf() or fnv_64_str().
 *
 * NOTE: To use the recommended 64 bit FNV-1 hash, use FNV1_64_INIT as the hval
 *	 argument on the first call to either fnv_64_buf() or fnv_64_str().
 */
int
fnv_64_buf_range(void *buf, size_t len, Fnv64_t hval, int low, int high, Fnv64_t *hvalout)
{
    unsigned char *bp = (unsigned char *)buf;	/* start of buffer */
    int i;
#if !defined(HAVE_64BIT_LONG_LONG)
    /* @@ Supporting systems without 64-bit long longs makes for
     * really messy code. I doubt it's really worth it these days,
     * but I'll make an effort. The original code worked; my modifications
     * are however mostly untested.
     */

    unsigned long val[4];			/* hash value in base 2^16 */
    unsigned long tmp[4];			/* tmp 64 bit value */

    /*
     * Convert Fnv64_t hval into a base 2^16 array
     */
    val[0] = hval.w32[0];
    val[1] = (val[0] >> 16);
    val[0] &= 0xffff;
    val[2] = hval.w32[1];
    val[3] = (val[2] >> 16);
    val[2] &= 0xffff;
#endif

    /* Don't go beyond the end! */
    if (high > len) high = len;
    /* Set position 0 to the base hval, in case there's nothing */
    hvalout[0] = hval;

    /*
     * FNV-1 hash each octet of the buffer
     */
     for (i=1; i <= high; ++i) {

#if defined(HAVE_64BIT_LONG_LONG)

	/* multiply by the 64 bit FNV magic prime mod 2^64 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
	hval *= FNV_64_PRIME;
#else /* NO_FNV_GCC_OPTIMIZATION */
	hval += (hval << 1) + (hval << 4) + (hval << 5) +
		(hval << 7) + (hval << 8) + (hval << 40);
#endif /* NO_FNV_GCC_OPTIMIZATION */

	/* xor the bottom with the current octet */
	hval ^= (Fnv64_t)*bp++;
#else /* HAVE_64BIT_LONG_LONG */
	/*
	 * multiply by the 64 bit FNV magic prime mod 2^64
	 *
	 * Using 0x100000001b3 we have the following digits base 2^16:
	 *
	 *	0x0	0x100	0x0	0x1b3
	 *
	 * which is the same as:
	 *
	 *	0x0	1<<FNV_64_PRIME_SHIFT	0x0	FNV_64_PRIME_LOW
	 */
	/* multiply by the lowest order digit base 2^16 */
	tmp[0] = val[0] * FNV_64_PRIME_LOW;
	tmp[1] = val[1] * FNV_64_PRIME_LOW;
	tmp[2] = val[2] * FNV_64_PRIME_LOW;
	tmp[3] = val[3] * FNV_64_PRIME_LOW;
	/* multiply by the other non-zero digit */
	tmp[2] += val[0] << FNV_64_PRIME_SHIFT;	/* tmp[2] += val[0] * 0x100 */
	tmp[3] += val[1] << FNV_64_PRIME_SHIFT;	/* tmp[3] += val[1] * 0x100 */
	/* propagate carries */
	tmp[1] += (tmp[0] >> 16);
	val[0] = tmp[0] & 0xffff;
	tmp[2] += (tmp[1] >> 16);
	val[1] = tmp[1] & 0xffff;
	val[3] = tmp[3] + (tmp[2] >> 16);
	val[2] = tmp[2] & 0xffff;
	/*
	 * Doing a val[3] &= 0xffff; is not really needed since it simply
	 * removes multiples of 2^64.  We can discard these excess bits
	 * outside of the loop when we convert to Fnv64_t.
	 */

	/* xor the bottom with the current octet */
	val[0] ^= (unsigned long)*bp++;

        /*
         * Convert base 2^16 array back into an Fnv64_t
         */
        hval.w32[1] = ((val[3]<<16) | val[2]);
        hval.w32[0] = ((val[1]<<16) | val[0]);

#endif /* HAVE_64BIT_LONG_LONG */

	if (i >= low)
                hvalout[i-low] = hval;
    }
    return i-low;


}


/*
 * fnv_64_str - perform a 64 bit Fowler/Noll/Vo hash on a string
 *
 * input:
 *      str     - string to hash
 *      hval    - previous hash value or 0 if first call
 *      low     - start of length range
 *      high    - end of length range
 *
 * returns:
 *      array of high-low+1 64 bit hashes as a static hash type
 *
 * NOTE: To use the 64 bit FNV-0 historic hash, use FNV0_64_INIT as the hval
 *	 argument on the first call to either fnv_64_buf() or fnv_64_str().
 *
 * NOTE: To use the recommended 64 bit FNV-1 hash, use FNV1_64_INIT as the hval
 *	 argument on the first call to either fnv_64_buf() or fnv_64_str().
 */
int
fnv_64_str_range(char *str, Fnv64_t hval, int low, int high, Fnv64_t *hvalout)
{
    unsigned char *s = (unsigned char *)str;	/* unsigned string */
    int i;

#if !defined(HAVE_64BIT_LONG_LONG)
    unsigned long val[4];	/* hash value in base 2^16 */
    unsigned long tmp[4];	/* tmp 64 bit value */

    /*
     * Convert Fnv64_t hval into a base 2^16 array
     */
    val[0] = hval.w32[0];
    val[1] = (val[0] >> 16);
    val[0] &= 0xffff;
    val[2] = hval.w32[1];
    val[3] = (val[2] >> 16);
    val[2] &= 0xffff;
#endif


    if (!*s) {
        /* Weirdness - null string should return base hval in first
         * position ...
         */
        hvalout[0] = hval;
    } else for (i=1; *s && i <= high; ++i) {
    /*
     * FNV-1 hash each octet of the string
     */

#if defined(HAVE_64BIT_LONG_LONG)

	/* multiply by the 64 bit FNV magic prime mod 2^64 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
	hval *= FNV_64_PRIME;
#else /* NO_FNV_GCC_OPTIMIZATION */
	hval += (hval << 1) + (hval << 4) + (hval << 5) +
		(hval << 7) + (hval << 8) + (hval << 40);
#endif /* NO_FNV_GCC_OPTIMIZATION */

	/* xor the bottom with the current octet */
	hval ^= (Fnv64_t)*s++;

#else /* !HAVE_64BIT_LONG_LONG */
	/*
	 * multiply by the 64 bit FNV magic prime mod 2^64
	 *
	 * Using 1099511628211, we have the following digits base 2^16:
	 *
	 *	0x0	0x100	0x0	0x1b3
	 *
	 * which is the same as:
	 *
	 *	0x0	1<<FNV_64_PRIME_SHIFT	0x0	FNV_64_PRIME_LOW
	 */
	/* multiply by the lowest order digit base 2^16 */
	tmp[0] = val[0] * FNV_64_PRIME_LOW;
	tmp[1] = val[1] * FNV_64_PRIME_LOW;
	tmp[2] = val[2] * FNV_64_PRIME_LOW;
	tmp[3] = val[3] * FNV_64_PRIME_LOW;
	/* multiply by the other non-zero digit */
	tmp[2] += val[0] << FNV_64_PRIME_SHIFT;	/* tmp[2] += val[0] * 0x100 */
	tmp[3] += val[1] << FNV_64_PRIME_SHIFT;	/* tmp[3] += val[1] * 0x100 */
	/* propagate carries */
	tmp[1] += (tmp[0] >> 16);
	val[0] = tmp[0] & 0xffff;
	tmp[2] += (tmp[1] >> 16);
	val[1] = tmp[1] & 0xffff;
	val[3] = tmp[3] + (tmp[2] >> 16);
	val[2] = tmp[2] & 0xffff;
	/*
	 * Doing a val[3] &= 0xffff; is not really needed since it simply
	 * removes multiples of 2^64.  We can discard these excess bits
	 * outside of the loop when we convert to Fnv64_t.
	 */

	/* xor the bottom with the current octet */
	val[0] ^= (unsigned long)(*s++);

        /*
         * Convert base 2^16 array back into an Fnv64_t
         */
        hval.w32[1] = ((val[3]<<16) | val[2]);
        hval.w32[0] = ((val[1]<<16) | val[0]);

#endif /* !HAVE_64BIT_LONG_LONG */

	if (i >= low)
                hvalout[i-low] = hval;
    }

    return i-low;
}
