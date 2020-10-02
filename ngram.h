/* ngram.h - check for ngram patterns in pcap capture files.
 * Variations:
 * 	1. only for TCP
 * 	2. only for particular protocols on TCP
 * 	3. only for traffic with sufficiently low entropy
 */

#ifndef _CONTENTANOMALY_H
#define _CONTENTANOMALY_H

/* Simple structures and routines for ranges of integers, doubles, etc. */
#include "range.h"

/* For now, we'll restrict ngram sizes to < 20 */
#define NGRAM_RANGEMAX	20

/* I. entropy - byte-level entropy ranges from 0-8.
 *  1. Typical ascii text is around 4 or 5, plus or minus.
 *  2. Binary executables vary from 4.5 - 6.
 *  3. Unicode text is around 6.
 *  4. Encrypted/compressed traffic is very close to 8 for longer ranges,
 *     but for typical 1500-byte packets, 7.8 is more realistic. 
 *  5. A (type-specific) header followed by encrypted/compressed data
 *     is somewhere lower, say 6.5-7.5.
 *
 */
#define OCTETSIZE	256		/* 8 bits */

/* convenience */
#define min(a,b)	((a)<=(b) ? (a) : (b))

extern int entropyflag;
extern DRange entropyrange;
/* Evaluators */
double entropy(u_int8_t *packet, int size);

/* II. snort tagging - We rely on packets already having been tagged
 * by snort, with the information dumped in alert files in ASCII
 * format. That's not because that's a particularly efficient method,
 * but rather because that's how we had already been handling things.
 */

extern int snortflag;	/* 1 - only those tagged by snort
			 * 0 - only those not tagged by snort
			 * -1 - ignore snort tagging (i.e., accept all)
			 */
extern int snorthostflag;	/* The same, except we select by host tagged,
				 * not by packet tagged. */

extern int taggedhostflag;	/* Even simpler - we're just given a list
				 * of tagged hosts. Probably should have
				 * done it this way to begin with. */

extern int startymd, endymd;	/* Time range of snort alerts, yyyymmdd */
/* Evaluators. For now, we use a cheap method to see if a packet
 * was tagged by snort - simply check the packet's timestamp. Since
 * this is to microsecond level resolution, it should be explicit
 * enough. We also throw in the size, since we have it at hand; other,
 * more definitive information for matching (such as protocol and
 * address info) could be supplied if we pushed this evaluation down
 * to, say, the IP level.
 */
int checksnort(struct timeval *ts, u_int32_t size, int startymd, int endymd);

/* This version merely checks whether one of the hosts had any packets
 * tagged by snort during the entire time period.
 */
int checksnorthosts(u_int32_t source, u_int32_t dest);

/* This version is even simpler - it merely checks whether one of the
 * hosts is in a (previously-determined) list of hosts, presumably ones
 * tagged by snort or some other IDS.
 */
int checktaggedhosts(u_int32_t source, u_int32_t dest);

/* III. Other selectors:
 * a. port/protocol ranges - Simple filtering by protocol/port. This
 * is more of a convenience than anything else, since filtering (including
 * much more sophisticated criteria) can also be done by tcpdump and related
 * programs.
 * b. time ranges - Only packets within a given range.
 */

extern int inprotocols;
extern Rangelist *protocols;
extern int inports;
extern Rangelist *ports;

extern int timeflag;
extern struct timeval starttime, endtime; /* range of packets to look at */

/*
 * IV. Ngram counters - there will be two versions of this. For n up
 * to 4, we can just convert the ngram to a 32-bit int and count it.
 * For larger n, we'll use a Bloom filter to get some sort of approximate
 * count.
 *
 * In the future, we may try other methods (e.g., quotient filters or
 * tries), so we'll use a generic structure.
 */

typedef u_int16_t NgramCounter;		/* try 16 bit counters */
#define COUNTER_MAX	0xfff0		/* leave a little space for slop ... */

/* Maximum length of a string to be examined for ngrams. This is simply
 * for the benefit of statistics computations, and faces no particular
 * constraints.
 */
#define NGRAM_MAX	10240

typedef void *NgramFilter;	/* "opaque" type */

typedef struct _ngramfilterset {
	Range	ngramsize;	/* can do array up to 4, Bloom filter to ~20 */
	/* Filter storage - only the slots being used are actually filled */
	NgramFilter filter[NGRAM_RANGEMAX];
} NgramFilterSet;

typedef struct _ngramOps {
#ifdef SHMALLOC
	NgramFilterSet *(*newfilterset)(Range ngram, char *shmfilename, int mode);
#else
	NgramFilterSet *(*newfilterset)(Range ngram);
#endif
	/* We may do multiple filters (e.g. snort tagged/not tagged) in the
	 * future, so we include the filter as an argument in the functions
	 * below.
	 */
	/* Each of the following takes the item it is given and divides it
	 * into a set of overlapping ngrams, which it then adds, deletes,
	 * or looks up in the filter.
	 */
	int	(*additemset)(void *item, size_t length, NgramFilterSet *filterset);
	int	(*delitemset)(void *item, size_t length, NgramFilterSet *filterset);
	/* findngram looks up a single ngram in a single filter */
	int	(*findngram)(void *item, int ngram, NgramFilter filter);
	/* findngramset does the same, looking in a set of filters */
	int	(*findngramset)(void *item, size_t length, NgramFilterSet *filterset);
	/* finddist takes the given item as a sort of probe vector into the
	 * contents of the filter. It returns some statistics about the
	 * distribution of its ngrams.
	 */
	int	(*finddist)(void *item, size_t length, int ngram, double *mu,
		double *sigma, double *rho, NgramFilterSet *filter);
	/* diststats is a placeholder for a routine that extracts information
	 * about the overall distribution of a filter's contents.
	 * dump simply dumps the contents of all the filters somewhere.
	 */
	void	(*diststats)(int ngram, double *mu, double *sigma, u_int64_t *max, u_int64_t *min, NgramFilterSet *filterset);
	void	(*dumpset)(FILE *file, NgramFilterSet *filterset);
	void	(*closefilterset)(NgramFilterSet *filterset);
} NgramOps;

typedef struct _ngram {
	NgramFilterSet * f;
	NgramOps * op;
} Ngram;

extern FILE *dumpfile;	/* Where filter contents are dumped */

#ifdef SHMALLOC
/* New, improved version where it's in shared memory */
#include <fcntl.h>
#include "simpleshmfile.h"
extern ShmFile *shmfile;
void * ngram_shmalloc(size_t length, char *filename, int mode);
void ngram_shmfree(void *buffer);
void ngram_peek(char *filename);
#endif
void ngramreadfile(FILE *fp, Ngram *ngram);

/* Our two types for now */
extern Ngram array, bloom;
/* Future ones ... */
extern Ngram quotient, trie;
/* The one we're using */
extern Ngram *ngram;

/* For use in labeling the filter */
#define NGRAM_ARRAY	1
#define NGRAM_BLOOM	2
/* @@ future ... */
#define NGRAM_QUOTIENT	3
#define NGRAM_TRIE	4

/* When we save to disk or shared memory, we label what we've got for use
 * by other programs (including other instances of this program).
 */

typedef struct _nGramLabel {
	int type;
/* Size is the range of ngram sizes - 1 to 4 for array, up to about 20
 * for Bloom filter */
	Range ngramsize;
/* Length is the length (in bytes) of the filters proper */
	size_t length[NGRAM_RANGEMAX];
/* Minimal stats */
	size_t total[NGRAM_RANGEMAX];	/* total number of ngrams */
	size_t distinct[NGRAM_RANGEMAX]; /* distinct ngrams - may be estimate */
} NgramLabel;

extern NgramLabel ngramlabel;

/* This sets whatever information we have about the filter being used */
extern void setngramlabel(int type, Range *ngramsize, int ngram, size_t length, size_t total, size_t distinct);

/* @@ not doing threaded version ... */

/* VI. Protocol readers - There are all kinds of existing structures
 * out there; for now, we'll just use something fairly generic -
 * protocol id (to help out ones that support multiple protocols),
 * pointer to outer header, pointer to this header,
 * length (from this header),
 * give generic src dest pointers (to parent protocol?) as arguments.
 */

struct readproto {
	int type;		/* just do int for now */
	u_int8_t *outheader;
	u_int8_t *header;
	int len;
};

/* Handlers are: int (*handler)(struct readproto *, void *src, void *dest) */
typedef int (*protohandler)(struct readproto *, void *, void *);

struct protoreader {
	int type;		/* just do int for now */
	protohandler handler;
};

extern protohandler readerswitch(int type, struct protoreader *readers);

/*
 * 1. Application level
 *
 * For this, type is the port (UDP/TCP) or type (ICMP) number
 * outheader is the transport header
 * header is the packet data (past the transport header)
 * len is smaller of actual len remaining, capture len remaining
 * A pointer to the "server" port info is included as destination;
 * eventually we may do the same for the "client" port.
 */

/* A quick selection of some protocols. No doubt this will be revised
 * considerably with experience.
 *
 * These will be called from both the udp and tcp sides.
 */
int readftp(struct readproto *r, void *client, void *server);
int readssh(struct readproto *r, void *client, void *server);
int readtelnet(struct readproto *r, void *client, void *server);
int readsmtp(struct readproto *r, void *client, void *server);
int readdns(struct readproto *r, void *client, void *server);
int readboot(struct readproto *r, void *client, void *server);
int readfinger(struct readproto *r, void *client, void *server);
int readhttp(struct readproto *r, void *client, void *server);
int readauth(struct readproto *r, void *client, void *server);
int readntp(struct readproto *r, void *client, void *server);
int readimap(struct readproto *r, void *client, void *server);
int readsnmp(struct readproto *r, void *client, void *server);
int readhttps(struct readproto *r, void *client, void *server);
int readimaps(struct readproto *r, void *client, void *server);
int readportother(struct readproto *r, void *client, void *server);

extern struct protoreader portreaders[];

/* A few ICMP/ICMPv6 types we'll read. Where possible, we'll share
 * handlers between the two because why not.
 */
int readecho(struct readproto *r, void *src, void *dest);
int readunreach(struct readproto *r, void *src, void *dest);
int readtoobig(struct readproto *r, void *src, void *dest);
int readquench(struct readproto *r, void *src, void *dest);
int readredirect(struct readproto *r, void *src, void *dest);
int readtimexceed(struct readproto *r, void *src, void *dest);
int readparamprob(struct readproto *r, void *src, void *dest);
int readtrace(struct readproto *r, void *src, void *dest);
int readmld(struct readproto *r, void *src, void *dest);
int readneighdisc(struct readproto *r, void *src, void *dest);
int readtypeother(struct readproto *r, void *client, void *server);

extern struct protoreader typereaders[];
extern struct protoreader type6readers[];

/* 2. Transport, i.e. "protocol" */
/* We only really look at ICMP/ICMPv6, UDP and TCP in any detail.
 * These are stored right in the ip-level tables.
 */

extern struct protoreader transportreaders[];

/* 3. IP level
 *
 * For this, type is IP protocol number
 * outheader is IP header
 * header is packet/transport-level body (past IP header and options)
 * len is smaller of actual len remaining, capture len remaining
 * handler is given this structure why not
 * src and dest are the ipinfo things above
 * counter is set here
 */

int readicmp(struct readproto *r, void *src, void *dest);
int readigmp(struct readproto *r, void *src, void *dest);
int readipv4(struct readproto *r, void *src, void *dest);
int readtcp(struct readproto *r, void *src, void *dest);
int readudp(struct readproto *r, void *src, void *dest);
int readipv6(struct readproto *r, void *src, void *dest);
int readrsvp(struct readproto *r, void *src, void *dest);
int readgre(struct readproto *r, void *src, void *dest);
int readesp(struct readproto *r, void *src, void *dest);
int readah(struct readproto *r, void *src, void *dest);
int readospf(struct readproto *r, void *src, void *dest);
/* Catchall for the remainder */
int readotherip(struct readproto *r, void *src, void *dest);

extern struct protoreader transportreaders[];

/* 4. Ethernet level
 *
 * For this, type is ethertype
 * outheader is the pcap "packet header" (actually time info)
 * header is ethernet header
 * len is smaller of actual len and capture len
 * src and dest are pointers to ethernet source and destination
 */

int readip(struct readproto *r, void *src, void *dest);
int readarp(struct readproto *r, void *src, void *dest);
int readrarp(struct readproto *r, void *src, void *dest);
int readvlan(struct readproto *r, void *src, void *dest);
int readipv6(struct readproto *r, void *src, void *dest);
int readswitch(struct readproto *r, void *src, void *dest);
int readother(struct readproto *r, void *src, void *dest);


extern struct protoreader etherreaders[];

/* Debugs */
extern int dumplevel;
int nonzerobyte(void *range, int size);



#endif /* _CONTENTANOMALY_H */
