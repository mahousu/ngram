/* Run the FNV 32 hash algorithm incrementally over a range of lengths.
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
 * 32 bit magic FNV-0 and FNV-1 prime
 */
#define FNV_32_PRIME ((Fnv32_t)0x01000193)


/*
 * fnv_32_buf_range - perform a 32 bit Fowler/Noll/Vo hash on a range of
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
 *	array of high-low+1 32 bit hashes as a static hash type
 *
 * NOTE: To use the 32 bit FNV-0 historic hash, use FNV0_32_INIT as the hval
 *	 argument on the first call to either fnv_32_buf() or fnv_32_str().
 *
 * NOTE: To use the recommended 32 bit FNV-1 hash, use FNV1_32_INIT as the hval
 *	 argument on the first call to either fnv_32_buf() or fnv_32_str().
 */
int
fnv_32_buf_range(void *buf, size_t len, Fnv32_t hval, int low, int high, Fnv32_t *hvalout)
{
    unsigned char *bp = (unsigned char *)buf;	/* start of buffer */
    int i;

    /* Don't go beyond the end! */
    if (high > len) high = len;
    /* Set position 0 to the base hval, in case there's nothing */
    hvalout[0] = hval;
    /*
     * FNV-1 hash each octet in the buffer
     */
    for (i=1; i <= high; ++i) {
#if defined(NO_FNV_GCC_OPTIMIZATION)
	hval *= FNV_32_PRIME;
#else
	hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#endif

	/* xor the bottom with the current octet */
	hval ^= (Fnv32_t)*bp++;

	if (i >= low)
		hvalout[i-low] = hval;
    }
    return i-low;
}


/*
 * fnv_32_str - perform a 32 bit Fowler/Noll/Vo hash on a string
 *
 * input:
 *	str	- string to hash
 *	hval	- previous hash value or 0 if first call
 *	low	- start of length range
 *	high	- end of length range
 *
 * returns:
 *	array of high-low+1 32 bit hashes as a static hash type
 *
 * NOTE: To use the 32 bit FNV-0 historic hash, use FNV0_32_INIT as the hval
 *	 argument on the first call to either fnv_32_buf() or fnv_32_str().
 *
 * NOTE: To use the recommended 32 bit FNV-1 hash, use FNV1_32_INIT as the hval
 *	 argument on the first call to either fnv_32_buf() or fnv_32_str().
 */
int
fnv_32_str_range(char *str, Fnv32_t hval, int low, int high, Fnv32_t *hvalout)
{
    unsigned char *s = (unsigned char *)str;	/* unsigned string */
    int i;

    /*
     * FNV-1 hash each octet in the buffer
     */
    if (!*s) {
    	/* Weirdness - null string should return base hval in first
	 * position ...
	 */
    	hvalout[0] = hval;
    } else for (i=1; *s && i <= high; ++i) {

	/* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
	hval *= FNV_32_PRIME;
#else
	hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#endif

	/* xor the bottom with the current octet */
	hval ^= (Fnv32_t)*s++;

	if (i >= low)
		hvalout[i-low] = hval;
    }

    return i-low;
}
