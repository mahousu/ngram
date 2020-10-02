#ifndef _PROTOCOLS_H
#define _PROTOCOLS_H
/* We'll just define and deal with a few protocols here for now. */
#define	ICMP_PROTO	1
#define	IGMP_PROTO	2
#define	IPV4_PROTO	4
#define	TCP_PROTO	6
#define	UDP_PROTO	17
#define	IPV6_PROTO	41
#define	RSVP_PROTO	46
#define	GRE_PROTO	47
#define	ESP_PROTO	50
#define	AH_PROTO	51
#define	OSPF_PROTO	89

#define	ICMP_INDEX	0
#define	IGMP_INDEX	1
#define	IPV4_INDEX	2
#define	TCP_INDEX	3
#define	UDP_INDEX	4
#define	IPV6_INDEX	5
#define	RSVP_INDEX	6
#define	GRE_INDEX	7
#define	ESP_INDEX	8
#define	AH_INDEX	9
#define	OSPF_INDEX	10
#define OTHER_INDEX	11

#define MAX_PROTOCOLS	OTHER_INDEX+1

/* A quick selection of some "application" protocols. No doubt this will
 * be revised considerably with experience.
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


#endif /* _PROTOCOLS_H */
