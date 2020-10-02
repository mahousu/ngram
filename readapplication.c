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
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include "entropy.h"
#include "ngram.h"

int readftp(struct readproto *r, void *client, void *server)
{
	/* For the ftp control channel, we expect low entropy.
	 * We don't really expect the data channel to be used
	 * at all ("active" mode), since everything should be
	 * passive nowadays. But if it does show up, its
	 * entropy depends on what's being transferred.
	 */
	return 0;
}
int readssh(struct readproto *r, void *client, void *server)
{
	/* For ssh, we expect high entropy except for the first
	 * packet, which is probably only moderately high.
	 */
	return 0;
}
int readtelnet(struct readproto *r, void *client, void *server)
{
	/* telnet showing up at all nowadays is an anomaly.
	 * If it does occur, we expect low entropy and very
	 * small packet sizes.
	 */
	return 0;
}
int readsmtp(struct readproto *r, void *client, void *server)
{
	/* smtp traffic will almost certainly have low to moderate
	 * entropy. Even when mailing binaries, they'll presumably
	 * be base64 encoded, which limits entropy to 6 bits.
	 */
	return 0;
}
int readdns(struct readproto *r, void *client, void *server)
{
	/* dns traffic should be low entropy, except dnssec. However,
	 * the packet size may be small enough that a good entropy
	 * estimate is hard. Perhaps more of interest are deviations
	 * from standard sizes.
	 */
	return 0;
}
int readboot(struct readproto *r, void *client, void *server)
{
	/* boot/dhcp traffic should only be local; any of it
	 * showing up is a concern! Typically, the entropy
	 * is low.
	 */
	return 0;
}
int readfinger(struct readproto *r, void *client, void *server)
{
	/* 1983 called, and they want their protocol back. I
	 * agree they should have it.
	 */
	return 0;
}
int readhttp(struct readproto *r, void *client, void *server)
{
	/* http entropy depends entirely on the kind of data
	 * being transferred. Headers will always be low
	 * entropy.
	 */
	return 0;
}
int readauth(struct readproto *r, void *client, void *server)
{
	/* We expect small packets, low entropy; however most
	 * of the time this traffic is blocked by firewalls.
	 */
	return 0;
}
int readntp(struct readproto *r, void *client, void *server)
{
	/* Almost certainly should be short, standard sized,
	 * moderate entropy.
	 */
	return 0;
}
int readimap(struct readproto *r, void *client, void *server)
{
	/* like smtp in terms of expectations */
	return 0;
}
int readsnmp(struct readproto *r, void *client, void *server)
{
	/* snmp should be relatively low entropy (depending on
	 * what exactly is being transmitted), but in any case
	 * should almost certainly be blocked by the firewall.
	 */
	return 0;
}
int readhttps(struct readproto *r, void *client, void *server)
{
	/* We expect moderately high (for initial packet) or
	 * very high (for subsequent packets) entropy.
	 */
	return 0;
}
int readimaps(struct readproto *r, void *client, void *server)
{
	/* We expect moderately high (for initial packet) or
	 * very high (for subsequent packets) entropy.
	 */
	return 0;
}
int readportother(struct readproto *r, void *client, void *server)
{
	/* Probably the most important thing to note about this
	 * traffic is that it exists. It either means we need to
	 * write another handler, or that there's something here
	 * to look at.
	 */
	return 0;
}

/* ICMP and ICMPv6 types */
int readecho(struct readproto *r, void *src, void *dest)
{	return 0;
}
int readunreach(struct readproto *r, void *src, void *dest)
{	return 0;
}
int readtoobig(struct readproto *r, void *src, void *dest)
{	return 0;
}
int readquench(struct readproto *r, void *src, void *dest)
{	return 0;
}
int readredirect(struct readproto *r, void *src, void *dest)
{	return 0;
}
int readtime_exceeded(struct readproto *r, void *src, void *dest)
{	return 0;
}
int readparameterprob(struct readproto *r, void *src, void *dest)
{	return 0;
}
int readmld(struct readproto *r, void *src, void *dest)
{	return 0;
}
int readneighdisc(struct readproto *r, void *src, void *dest)
{	return 0;
}
int readtypeother(struct readproto *r, void *client, void *server)
{	return 0;
}

/* A quick selection of some protocols. No doubt this will be revised
 * considerably with experience.
 *
 * These will be called from both the udp and tcp sides.
 */

#define FTP_PROTO	21
#define SSH_PROTO	22
#define TELNET_PROTO	23
#define SMTP_PROTO	25
#define DNS_PROTO	53
#define BOOTPS_PROTO	67
#define BOOTPC_PROTO	68
#define TFTP_PROTO	69
#define FINGER_PROTO	79
#define HTTP_PROTO	80
#define AUTH_PROTO	113
#define NTP_PROTO	123
#define IMAP_PROTO	143
#define SNMP_PROTO	161
#define HTTPS_PROTO	443
#define IMAPS_PROTO	993

struct protoreader portreaders[] = {
	{FTP_PROTO,	readftp},
	{SSH_PROTO,	readssh},
	{TELNET_PROTO,	readtelnet},
	{SMTP_PROTO,	readsmtp},
	{DNS_PROTO,	readdns},
	{BOOTPS_PROTO,	readboot},
	{BOOTPC_PROTO,	readboot},
	{TFTP_PROTO,	readboot},
	{FINGER_PROTO,	readfinger},
	{HTTP_PROTO,	readhttp},
	{AUTH_PROTO,	readauth},
	{NTP_PROTO,	readntp},
	{IMAP_PROTO,	readimap},
	{SNMP_PROTO,	readsnmp},
	{HTTPS_PROTO,	readhttps},
	{IMAPS_PROTO,	readimaps},
	{-1,		readportother}
};

/* A few ICMP/ICMPv6 types we'll read. Where possible, we'll share
 * handlers between the two because why not.
 */

struct protoreader typereaders[] = {
	{ICMP_ECHO,		readecho},
	{ICMP_ECHOREPLY,	readecho},
	{ICMP_DEST_UNREACH,	readunreach},
	{ICMP_SOURCE_QUENCH,	readquench},
	{ICMP_REDIRECT,	readredirect},
	{ICMP_TIME_EXCEEDED,	readtime_exceeded},
	{ICMP_PARAMETERPROB,	readparameterprob},
	{-1,		readtypeother},
};

struct protoreader type6readers[] = {
	{ICMP6_DST_UNREACH,	readunreach},
	{ICMP6_PACKET_TOO_BIG,	readtoobig},
	{ICMP6_TIME_EXCEEDED,	readtime_exceeded},
	{ICMP6_PARAM_PROB,	readparameterprob},
	{ICMP6_ECHO_REQUEST,	readecho},
	{ICMP6_ECHO_REPLY,	readecho},
	{MLD_LISTENER_QUERY,	readmld},
	{MLD_LISTENER_REPORT,	readmld},
	{MLD_LISTENER_REDUCTION,	readmld},
	{ND_ROUTER_SOLICIT,	readneighdisc},
	{ND_ROUTER_ADVERT,	readneighdisc},
	{ND_NEIGHBOR_SOLICIT,	readneighdisc},
	{ND_NEIGHBOR_ADVERT,	readneighdisc},
	{ND_REDIRECT,	readneighdisc},
	{-1,		readtypeother},
};
