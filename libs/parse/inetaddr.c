/*
 * inetaddr.c - simple-minded versions of address and port
 * conversion utilities. These are separated out simply to
 * isolate how I do the conversions.
 */

#include <ctype.h>
#include <sys/types.h>
#include <linux/types.h>
#include <string.h>
#include "parse.h"

/* my_inet_addr - given a string representation of an IP address,
 * either dotted decimal or hex, return the corresponding binary
 * address in network byte order.
 */
__u32
my_inet_addr(char *string)
{
	if (!string) return 0;
	if (index(string, '.')) {
		__u8 bytewise[4];
		int i;

		for (i=0; string && i < 4; ++i) {
			bytewise[i] = atoi(string);
			string = index(string, '.');
			if (string) ++string;
		}
		for (; i < 4; ++i) {
			bytewise[i] = 0;
		}
		return *(__u32 *)bytewise;
		
	} else {
		return ntohl(atoh(string));
	}
}

#ifdef TEST
#include <stdio.h>

main(int argc, char **argv)
{
	int i;

	for (i=1; i < argc; ++i)
		printf("%08x\n", my_inet_addr(argv[i]));
}
#endif
