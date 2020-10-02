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
#include "entropy.h"
#include "ymd.h"

/*  - skeleton code which reads pcap capture files and
 * checks entropy and/or ngram distributions of the packet body.
 *
 * We read through each packet, parsing the headers as we go,
 * then run some simple tests on the packet body (if any). We
 * store ngram results in a table.
 *
 */


#include <net/ethernet.h>
struct protoreader etherreaders[] = {
		{ETHERTYPE_IP, readipv4},
		{ETHERTYPE_ARP, readarp},
		{ETHERTYPE_REVARP, readrarp},
		{ETHERTYPE_VLAN, readvlan},
		{ETHERTYPE_IPV6, readipv6},
		/* The next several are various switch protocols,
		 * which likely are not that interesting.
		 */
		{0x6002,/* DEC DNA remote console */ readswitch},
		{0x2e,/* Spanning tree for bridges */ readswitch},
		{0x1ab,/* Cisco Discovery Protocol */ readswitch},
		{-1, readother},
};

#ifdef notdef
int
readprotocol(struct iphdr *hdr, int pcaplen, struct ipinfo *src,
	struct ipinfo *dest)
{
	/* Let's see how much packet we've got left */
	int bodylen = pcaplen - (int)4*hdr->ihl;
	u_int8_t *body;
	int i;

	if (bodylen <= 0) {
		body = NULL;
	} else {
		body = (u_int8_t *)hdr + (int)4*hdr->ihl;
	}

	for (i=0; protoreaders[i].proto >= 0; ++i) {
		if (hdr->protocol == protoreaders[i].proto) {
			return (*protoreaders[i].reader)(body, bodylen, src, dest);
		}
	}
	return readother(hdr->protocol, body, bodylen, src, dest);
}
#endif

int
nonzerobyte(void *range, int size)
{
	int i;
	u_int8_t *run = (u_int8_t *)range;

	if (!range) return 0;
	for (i=0; i < size; ++i) {
		if (run[i]) return 1;
	}
	return 0;
}

/* Lowest level - reads the actual pcap file, determines its type,
 * and passes it up to the next level.
 */

int
readpcap(pcap_t *readp, int *atend)
{
	struct pcap_pkthdr *pkt_header;
	u_int8_t *pkt_data;
	u_int8_t *pkt;
	struct readproto rp;
	struct ether_header *eth;
	struct etypes *ttype;
	u_int32_t caplen;
	int ret;
	int len;
	int afterrange=0;
	int process_packet(int len, u_int8_t *data);

	while ((ret = pcap_next_ex(readp, &pkt_header, (const u_char **)&pkt_data)) > 0) {
		/* The pcap_pkthdr structure isn't actually the
		 * packet header. Rather, it just has the time
		 * the packet was received and the length of the
		 * packet. We can use that information to see if it
		 * is in our time range, or has been tagged by snort.
		 */
		/* Check time range */
		if (timeflag) {
			switch (intimerange(&pkt_header->ts,
					&starttime, &endtime)) {
			case -1:	/* before range */
				continue;
			case 0:		/* in range */
				break;
			case 1:		/* after range */
				++afterrange;
				/* We keep looking for a few packets
				 * after the end of the range just in 
				 * case they are slightly out of order
				 */
#define AFTERMAX	10
				if (afterrange > AFTERMAX) {
					*atend++;
					return ret;
				} else {
					continue;
				}
			}
		}
		/* Check against snort list */
		if (snortflag >= 0) {
			if (checksnort(&pkt_header->ts, pkt_header->len,
				startymd, endymd) != snortflag)
					continue;
		}

		/* Now process the packet data */
		ret = process_packet(pkt_header->caplen, pkt_data);
	}
	return ret;
}

int
process_packet(int len, u_int8_t *data)
{
	struct readproto rp;
	int ret = 0;

	rp.type = ntohs(((struct ether_header*)
					data)->ether_type);
	rp.outheader = data;
	rp.header = data + sizeof(struct ether_header);
	rp.len = len;

	if ((*readerswitch(rp.type, etherreaders))(&rp,
		((struct ether_header*)data)->ether_shost,
		((struct ether_header*)data)->ether_dhost) < 0)
			/* ethernet error - ignore for now */
			return -1;
	return 0;
}
