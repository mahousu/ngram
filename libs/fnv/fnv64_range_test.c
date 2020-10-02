#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "longlong.h"
#include "fnv.h"
#include "fnvrange.h"

#define	MAXRANGE	8192

main(int argc, char **argv)
{
	int i, j;
	int low, high;
	u_int8_t buffer[MAXRANGE];
	unsigned char strbuffer[MAXRANGE];
	Fnv64_t basehval, hval;
	Fnv64_t hvalout[MAXRANGE];
	long long int hashcount=0;

	
	basehval = FNV1_64_INIT;
	srandom(time(NULL));
	for (j=0; j < 2000000; ++j) {
		for (i=0; i < MAXRANGE; ++i) {
			buffer[i] = (random() & 0xff);
			strbuffer[i] = (random() & 0x7f);
		}
		low = (random()&(MAXRANGE-1));
		high = (random()&(MAXRANGE-1));
		if (low > high) {
			int flip = low;

			low = high;
			high = flip;
		}
		fnv_64_buf_range(buffer, MAXRANGE, basehval, low, high, hvalout);
		for (i=low; i <= high; ++i) {
			hval = fnv_64_buf(buffer, i, basehval);
			if (hval != hvalout[i-low]) {
				fprintf(stderr, "low %d high %d error %d\n",
					low, high, i);
				fprintf(stderr, "%llx vs %llx\n",
					hvalout[i-low], hval);
				abort();
			}
		}
		hashcount += 2*(high-low+1);

		high = strlen(strbuffer);
		low = 0;
		fnv_64_str_range(strbuffer, basehval, low, high, hvalout);
		hval = fnv_64_str(strbuffer, basehval);
		if (hval != hvalout[high-low]) {
			fprintf(stderr, "low %d high %d strlen %d\n",
				low, high, high);
			fprintf(stderr, "%llx vs %llx\n",
				hvalout[high-low], hval);
			abort();
		}
		hashcount += 2*high;

		if (j%10000 == 0) {
			printf("%lld hashes\n", hashcount);
		}
	}
}
