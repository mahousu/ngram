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
#include <netinet/ip6.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netdb.h>
#include <math.h>
#include "ngram.h"
#include "entropy.h"
#include "protocols.h"

int transportprotocols[MAX_PROTOCOLS] = {
	ICMP_PROTO,
	IGMP_PROTO,
	IPV4_PROTO,
	TCP_PROTO,
	UDP_PROTO,
	IPV6_PROTO,
	RSVP_PROTO,
	GRE_PROTO,
	ESP_PROTO,
	AH_PROTO,
	OSPF_PROTO,
	-1
};

int
checkicmpchecksum(struct icmphdr *icmp)
{
	return 0;
}

int
checktcpchecksum(struct tcphdr *tcp)
{
	return 0;
}

int
checkudpchecksum(struct udphdr *udp)
{
	return 0;
}

int readicmp(struct readproto *r, void *src, void *dest)
{
	struct icmphdr *icmp = (struct icmphdr *)r->header;
	u_int32_t *isrc, *idest;
	struct readproto rp;
	double ent;

	isrc = (u_int32_t *)src;
	idest = (u_int32_t *)dest;
	/* Very few generic checks here ... */
	if (checkicmpchecksum(icmp) < 0) {
		/* We blame the source for a bad checksum */
		return -1;
	}

	/* Now pass to the next level */
	/* icmp has type and code; we'll just look at type for filtering */
	rp.type = icmp->type;
	rp.outheader = r->header;
	rp.header = r->header + sizeof(struct icmphdr);
	rp.len = r->len - sizeof(struct icmphdr);

	/* Check if we are filtering this */
	if (inrangelist(rp.type, ports) != inports)
		return 0;


	/* Evaluate entropy of packet body */
	if (entropyflag >= 0) {
		if (rp.len > 0) {	/* ignore dataless packets for this */
			ent = entropy(rp.header, rp.len);
			if (indoublerange(ent, &entropyrange) != entropyflag) {
				/* @@ maybe do high entropy stuff instead? */
				return 0;
			}
		}
	}

	/* For the first try, we will just mash together all
	 * the ngrams.
	 */

	(*ngram->op->additemset)(rp.header, rp.len, ngram->f);

#ifdef notdef
	/* Do any custom processing for individual protocols */
	if ((*readerswitch(rp.type, typereaders))(&rp, src, dest) < 0) {
		return -1;
	}
#endif
		
	return 0;
}

int readigmp(struct readproto *r, void *src, void *dest)
{
	return 0;
}

int readtcp(struct readproto *r, void *src, void *dest)
{
	struct tcphdr *tcp = (struct tcphdr *)r->header;
	u_int32_t *isrc, *idest;
	struct readproto rp;
	u_int16_t tsource, tdest;
	double ent;

	isrc = (u_int32_t *)src;
	idest = (u_int32_t *)dest;

	/* We will do more detailed examination of tcp peculiarities
	 * at some point, but here are a few we can check now.
	 *
	 * Things to check: header size,  checksum (assuming we are
	 * not the originating node), sequence number?, urgent pointer,
	 * odd window sizes, maybe some other flag handling. Perhaps
	 * some looking at options would help too, but unlike ip
	 * options, tcp options seem to be in common use, particularly
	 * timestamps.
	 */
	if (tcp->doff < 5 || tcp->doff*4 > r->len)  {
		/* corrupted header; can't proceed */
		return -1;
	}
	if (checktcpchecksum(tcp) < 0) {
		return -1;
	}
	/* @@ worry about sequence numbers and windows later */
	if (tcp->urg) {		/* hey bud, what's the rush? */
		/* Check the urgent pointer itself */
		if (tcp->urg_ptr > r->len - tcp->doff*4)
			return -1;
	}

	/* For passing to the next level */
	tsource = ntohs(tcp->source);
	tdest = ntohs(tcp->dest);
/*fprintf(stderr, "%x:%d -> %x:%d\n", *isrc, tsource, *idest, tdest);*/
	/* for now, we only associate with the lower-numbered port,
	 * under the assumption this is the "service."
	 */
	rp.type = min(tsource, tdest);
	rp.outheader = r->header;
	rp.header = r->header + tcp->doff*4;
	rp.len = r->len - tcp->doff*4;

	/* Check if we are filtering this */
	if (inrangelist(rp.type, ports) != inports)
		return 0;

	/* Evaluate entropy of packet body */
	if (entropyflag >= 0) {
		if (rp.len > 0) {	/* ignore dataless packets for this */
			ent = entropy(rp.header, rp.len);
			if (indoublerange(ent, &entropyrange) != entropyflag) {
				/* @@ maybe do high entropy stuff instead? */
				return 0;
			}
		}
	}

	/* For the first try, we will just mash together all
	 * the ngrams.
	 */
	(*ngram->op->additemset)(rp.header, rp.len, ngram->f);
	
	
#ifdef notdef
	/* Do any custom processing for individual protocols */

	if ((*readerswitch(rp.type, portreaders))(&rp, src, dest) < 0) {
		return -1;
	}
#endif

	return 1;
}

int readudp(struct readproto *r, void *src, void *dest)
{
	struct udphdr *udp = (struct udphdr *)r->header;
	u_int32_t *isrc, *idest;
	struct readproto rp;
	u_int16_t usource, udest;
	u_int16_t udplen;
	double ent;

	isrc = (u_int32_t *)src;
	idest = (u_int32_t *)dest;

	udplen = ntohs(udp->len);
	if (udplen < sizeof(struct udphdr) || udplen > r->len)  {
		/* corrupted header; can't proceed */
		return -1;
	}
	if (checkudpchecksum(udp) < 0) {
		/* We blame the source for a bad checksum */
		return -1;
	}

	/* For passing to the next level */
	usource = ntohs(udp->source);
	udest = ntohs(udp->dest);
	/* for now, we only associate with the lower-numbered port,
	 * under the assumption this is the "service."
	 */
	rp.type = min(usource, udest);
	rp.outheader = r->header;
	/* udp header is fixed at 8 bytes */
	rp.header = r->header + sizeof(struct udphdr);
	rp.len = r->len - sizeof(struct udphdr);

	/* Check if we are filtering this */
	if (inrangelist(rp.type, ports) != inports)
		return 0;

	/* Evaluate entropy of packet body */
	if (entropyflag >= 0) {
		if (rp.len > 0) {	/* ignore dataless packets for this */
			ent = entropy(rp.header, rp.len);
			if (indoublerange(ent, &entropyrange) != entropyflag) {
				/* @@ maybe do high entropy stuff instead? */
				return 0;
			}
		}
	}

	/* For the first try, we will just mash together all
	 * the ngrams.
	 */
	(*ngram->op->additemset)(rp.header, rp.len, ngram->f);
	
	/* Do any custom processing for individual protocols */

#ifdef notdef
	if ((*readerswitch(rp.type, portreaders))(&rp, src, dest) < 0) {
		return -1;
	}
#endif

	return 1;
}

int readrsvp(struct readproto *r, void *src, void *dest)
{
	return 0;
}
int readgre(struct readproto *r, void *src, void *dest)
{
	return 0;
}
int readesp(struct readproto *r, void *src, void *dest)
{
	return 0;
}
int readah(struct readproto *r, void *src, void *dest)
{
	return 0;
}
int readospf(struct readproto *r, void *src, void *dest)
{
	return 0;
}
int readotherip(struct readproto *r, void *src, void *dest)
{
	return 0;
}


struct protoreader transportreaders[] = {
	{ICMP_PROTO, readicmp},
	{IGMP_PROTO, readigmp},
	{IPV4_PROTO, readipv4},
	{TCP_PROTO, readtcp},
	{UDP_PROTO, readudp},
	{IPV6_PROTO, readipv6},
	{RSVP_PROTO, readrsvp},
	{GRE_PROTO, readgre},
	{ESP_PROTO, readesp},
	{AH_PROTO, readah},
	{OSPF_PROTO, readospf},
	{-1, readotherip},
};
