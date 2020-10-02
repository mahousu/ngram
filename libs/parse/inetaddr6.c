/*
 * inetaddr.c - simple-minded versions of address and port
 * conversion utilities. These are separated out simply to
 * isolate how I do the conversions.
 */

#include <ctype.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <string.h>
#include <memory.h>
#include "parse.h"

/* my_inet_pton - given a string representation of an IPv4 or v6
 * address, either dotted decimal or hex, convert to the corresponding
 * binary address in network byte order and return an indication of
 * which address family it is.
 *
 * The addr pointer has to point to space big enough to hold either
 * form of address, unless you want to crash your program. But if
 * the latter is all you're interested in, there are much easier
 * ways to go about doing it.
 */

int
my_inet_pton(char *string, void *addr)
{
	int af, worked;

	if (!string) return 0;
	/* v6's tend to have colons in them. It's just one of those things. */
	if (index(string, ':')) {
		af = AF_INET6;
		worked = inet_pton(af, string, addr);
		if (worked > 0)
			return af;
		else
			return worked;
	}
	/* Otherwise, try for v4 */
	af = AF_INET;
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
		memcpy(addr, (void *)bytewise, sizeof(struct in_addr));
		return af;
	} else {
		__u32 wordwise;

		wordwise = ntohl(atoh(string));
		memcpy(addr, (void *)&wordwise, sizeof(struct in_addr));
		return af;
	}
}

#ifdef TEST
#include <stdio.h>

main(int argc, char **argv)
{
	int i, j, lim, af;
	union {
		struct in_addr a4;
		struct in6_addr a6;
	} addr;
	unsigned char *b;
	char space[BUFSIZ];

	printf("I am but a naive single-file program, but I know struct in_addr is %d bytes and struct in6_addr is %d bytes\n",
		sizeof(struct in_addr), sizeof(struct in6_addr));

	for (i=1; i < argc; ++i) {
		af = my_inet_pton(argv[i], &addr);
		switch (af) {
		case AF_INET:
			lim = sizeof(struct in_addr);
			printf("%s ", inet_ntop(af, &addr, space, BUFSIZ));
			break;
		case AF_INET6:
			lim = sizeof(struct in6_addr);
			printf("%s ", inet_ntop(af, &addr, space, BUFSIZ));
			break;
		default:
			lim = 0;
			printf("Couldn't parse %s", argv[i]);
			break;
		}
		b = (unsigned char *)&addr;
		for (j=0; j < lim; ++j) {
			printf(":%x", b[j]);
		}
		printf("\n");
	}
}
#endif
