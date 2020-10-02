#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <pcap/pcap.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <memory.h>
#include "ngram.h"
#include "snorthostcheck.h"
#include "taggedhostcheck.h"
#include "entropy.h"


/* For now, we are only looking at IPv4 packets. Eventually, we'll
 * add IPv6, or maybe look at them separately. There doesn't seem
 * to be much reason to look at any other packet types for this
 * study, at least.
 */
int readarp(struct readproto *r, void *src, void *dest)
{
return 0;
}

int readrarp(struct readproto *r, void *src, void *dest)
{
return 0;
}

int readvlan(struct readproto *r, void *src, void *dest)
{
return 0;
}

int readipv6(struct readproto *r, void *src, void *dest)
{
return 0;
}

int readswitch(struct readproto *r, void *src, void *dest)
{
return 0;
}

int readother(struct readproto *r, void *src, void *dest)
{
return 0;
}


/* For IPv4, we select the protocol types that have payloads
 * we might wish to examine.
 */
int readipv4(struct readproto *r, void *esrc, void *edest)
{
	struct iphdr *ip = (struct iphdr *)r->header;
	struct readproto rp;
	double ent;
	int iplen, len, entindex;

	/* First, check for errors; don't try to proceed
	 * too far with a messed-up packet.
	 */
	iplen = ntohs(ip->tot_len);
	if (ip->version != 4 || ip->ihl < 5 || iplen < ip->ihl*4)
                /* too risky to try to parse these further */
		return -1;
	/* Again, be careful */
	len = min(r->len, iplen);
	len -= ip->ihl*4;
	if (len <= 0) { 		/* error or incomplete capture */
		return -1;
	}

	/* Check if we are filtering this */
	/* Note: ip->protocol is one byte, so no swapping needed */
	if (inrangelist(ip->protocol, protocols) != inprotocols)
		return 0;

	/* Check against snort and/or tagged host list */
	if (snorthostflag >= 0) {
		if (checksnorthosts(ip->saddr, ip->daddr) != snorthostflag)
			return 0;
	}
	if (taggedhostflag >= 0) {
		if (checktaggedhosts(ip->saddr, ip->daddr) != taggedhostflag)
			return 0;
	}

	/* Now pass to the next level */
	rp.type = ip->protocol;
	rp.outheader = r->header;
	rp.header = r->header + ip->ihl*4;
	rp.len = len;

	if ((*readerswitch(rp.type, transportreaders))(&rp, &ip->saddr, &ip->daddr) < 0) {
		return -1;
	}
	return 1;
}
