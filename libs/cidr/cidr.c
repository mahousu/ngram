#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include "../parse/parse.h"
#include "cidr.h"

#define MAXCIDR		1000

void
cidr_to_mask(char *cidr, struct in_addr *spart, struct in_addr *mpart, struct in_addr *hpart)
{
	char *findslash;
	int slash;
	uint32_t hostmask, subnetmask;
	struct in_addr subnet, host, mask;


	findslash = strchr(cidr, '/');
	if (!findslash) {
		/*@@ for mask match, need host -> host/32 */
		inet_pton(AF_INET, cidr, &subnet);
		host.s_addr = 0;
		mask.s_addr = ~0;
	} else {
		*findslash++ = '\0';
		inet_pton(AF_INET, cidr, &host);
		subnet = host;
		slash = atoi(findslash);
		if (!slash) {
			mask.s_addr = 0;
			subnet.s_addr = 0;
		} else if (slash >= 32) {
			mask.s_addr = ~0;
			host.s_addr = 0;
		} else {
			hostmask = ((1<<(32-slash))-1);
			subnetmask = ~hostmask;
			subnet.s_addr &= htonl(subnetmask);
			host.s_addr &= htonl(hostmask);
			mask.s_addr = htonl(subnetmask);
		}
	}
	if (spart) *spart = subnet;
	if (mpart) *mpart = mask;
	if (hpart) *hpart = host;
	return;
}

void
strtocidr(char *str, cidr_t *cidr)
{
	cidr_to_mask(str, &cidr->subnet, &cidr->mask, NULL);
}

cidr_t *
strtocidrlist(char *str)
{
	char *strlist[MAXCIDR];
	int i, n;
	cidr_t *answer;

	n = parsenargs(str, strlist, ",; ", MAXCIDR);
	answer = (cidr_t *)malloc((n+1)*sizeof(cidr_t));
	for (i=0; i < n; ++i)
		strtocidr(strlist[i], answer+i);
	memset(answer+n, 0, sizeof(cidr_t));
	return answer;
}

int
incidr(struct in_addr host, cidr_t *cidr)
{
	if (!cidr) return 0;
	return (cidr->mask.s_addr & host.s_addr) == cidr->subnet.s_addr;
}

int
incidrlist(struct in_addr host, cidr_t *cidrlist)
{
	while (cidrlist && cidrlist->subnet.s_addr) {
		if (incidr(host, cidrlist))
			return 1;
		++cidrlist;
	}
	return 0;
}

#ifdef TEST1

main(int argc, char **argv)
{
	struct in_addr spart, mask, hpart;
	char sout[BUFSIZ], mout[BUFSIZ], hout[BUFSIZ];
	int i;

	for (i=1; i < argc; ++i) {
		cidr_to_mask(argv[i], &spart, &mask, &hpart);
		inet_ntop(AF_INET, &spart, sout, BUFSIZ);
		inet_ntop(AF_INET, &mask, mout, BUFSIZ);
		inet_ntop(AF_INET, &hpart, hout, BUFSIZ);
		printf("%s/%s %s\n", sout, hout, mout);
	}
}
#endif

#ifdef TEST2

main(int argc, char **argv)
{
	cidr_t *cidrlist;
	struct in_addr host;
	char input[BUFSIZ];

	if (argc > 1)
		cidrlist = strtocidrlist(argv[1]);
	while (1) {
		printf("Host? "); gets(input);
		inet_pton(AF_INET, input, &host);
		printf("In list: %d\n", incidrlist(host, cidrlist));
	}
}
#endif
