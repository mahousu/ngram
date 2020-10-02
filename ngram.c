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
#include <string.h>
#include <getopt.h>
#include "ngram.h"
#include "snortcheck.h"
#include "snorthostcheck.h"
#include "taggedhostcheck.h"
#include "parse.h"
#include "ymd.h"

/* entropy related */
int entropyflag = -1;	/* -1 ignore entropy; 0 out of range; 1 in range */
DRange entropyrange;

/* protocol/port related */
int inprotocols=1;	/* default select */
Rangelist *protocols;
int inports=1;
Rangelist *ports;

/* snort related */
int snortflag = -1;	/* -1 ignore snort; 0 untagged; 1; tagged */
char *snortlist;	/* Directories and/or files with snort alerts */
int snorthostflag = -1;	/* -1 ignore snort hosts; 0 untagged; 1; tagged */
char *snortokhosts;		/* List of hosts we're fine with */
int taggedhostflag = -1;/* -1 ignore tagged hosts; 0 untagged; 1; tagged */
char *taggedhosts;		/* List of tagged hosts */
int startymd=20140101, endymd=20150101;	/* range of snort entries which are relevant - HACK! */

/* time related */
int timeflag = 0;	/* 0 don't check time; 1 check */
struct timeval starttime, endtime;	/* range of packets to look at */

FILE *dumpfile;
int dumplevel = 1;

#ifdef SHMALLOC
char *shmfilename;
int shmmode = (O_CREAT|O_TRUNC);
#endif

/* ngram - skeleton code which reads pcap capture files and
 * extracts a few bits of information for a packet database.
 *
 * We read through each packet, parsing the headers as we go, check
 * if it's one of the types we're tracking (e.g., TCP to certain ports),
 * and then check its entropy. If it's a type we're tracking with
 * entropy below a certain threshold, we then record its ngram
 * distribution in one of two tables, depending on whether it was
 * tagged by snort.
 *
 */

/* Generic switch used at each protocol level to determine the next one to call */
protohandler
readerswitch(int type, struct protoreader *readers)
{
	int i;

	for (i=0; readers[i].type != -1; ++i)
		if (type == readers[i].type)
			return readers[i].handler;
	return readers[i].handler;
}

void
Usage(void)
{
fprintf(stderr,
"Usage: ngram <arguments> pcap files ...\n"
"-I yes/no	- select/deselect based on protocol\n"
"-P protocol	- select/deselect only this protocol/set of protocols\n"
"-i yes/no	- select/deselect based on port\n"
"-p port		- select/deselect only this port/set of ports\n"
"		- both of the above can be comma-separated sets of ranges,\n"
"		-6,34-90\n"
"\n"
"-E yes/no	- select/deselect based on entropy\n"
"-e entropy	- range of entropy to select/deselect\n"
"\n"
"-S yes/no	- select packets tagged/not tagged by snort\n"
"-s d1,d2,...	- directories and/or files of snort alerts to use\n"
"-t begin-end	- time range of snort alerts in yyyymmdd format\n"
"\n"
"-H yes/no	- select hosts tagged/not tagged by snort\n"
"-h h1,h2,...	- hosts and/or subnets exempt from tagging\n"
"\n"
"-L yes/no	- select/deselect listed hosts\n"
"-l h1,h2,...	- listed hosts for select/deselect\n"
"\n"
"\tlists may also be given as file=<file with list info>\n"
"\n"
"-N filter	- which ngram filter to use (bloom, array ...)\n"
"-n low-high	- range of length of ngrams\n"
"\n"
"-D dumpfile    - dump filter contents to this file\n"
"-d dumplevel	- debug level\n"
"\n"
"-A shmfile	- allocate filter in this (permanent) shared memory region\n"
"\n"
"-T start,end	- start and end time of packets to select\n"
);

	exit(1);
}

int
yesno(const char *string)
{
	if (atoi(string) > 0 || !strncasecmp(string, "yes", 1) ||
		!strncasecmp(string, "true", 1) ||
		!strncasecmp(string, "on", 2) ||
		!strncasecmp(string, "in", 1) ||
		!strncasecmp(string, "select", 1) ) return 1;
	if (atoi(string) <= 0 || !strncasecmp(string, "no", 1) ||
		!strncasecmp(string, "false", 1) ||
		!strncasecmp(string, "off", 2) ||
		!strncasecmp(string, "out", 2) ||
		!strncasecmp(string, "deselect", 1) ) return 0;
	fprintf(stderr, "What does %s mean?\n", string);
	return -1;
}

int
handle_arguments(int argc, char **argv)
{
	int c;
	void datemskinit(void);
	int ngramtype=0;
	Range ngramsize={0,0};

	while ((c = getopt(argc, argv,
			"I:P:i:p:S:s:t:H:h:L:l:N:n:E:e:D:d:A:a:T:")) >= 0) {
		switch (c) {
		case 'I':
			inprotocols = yesno(optarg);
			break;
		case 'P':
			protocols = readprotorangelist(optarg);
			break;
		case 'i':
			inports = yesno(optarg);
			break;
		case 'p':
			ports = readportrangelist(optarg);
			break;
		case 'S':
			snortflag = yesno(optarg);
			break;
		case 's':
			snortlist = filearg(optarg, NULL, 0);
			if (snortlist == optarg)
				snortlist = strdup(optarg);
			break;
		case 'H':
			snorthostflag = yesno(optarg);
			break;
		case 'h':
			snortokhosts = filearg(optarg, NULL, 0);
			if (snortokhosts == optarg)
				snortokhosts = strdup(optarg);
			break;
		case 'L':
			taggedhostflag = yesno(optarg);
			break;
		case 'l':
			taggedhosts = filearg(optarg, NULL, 0);
			if (taggedhosts == optarg)
				taggedhosts = strdup(optarg);
			break;
		case 't':
			sscanf(optarg, "%d-%d", &startymd, &endymd);
			break;
		case 'N':
			if (!strncasecmp(optarg, "bloom", 5)) {
				ngramtype = NGRAM_BLOOM;
			} else if (!strncasecmp(optarg, "array", 5)) {
				ngramtype = NGRAM_ARRAY;
			} else if (!strncasecmp(optarg, "quotient", 8)) {
				ngramtype = NGRAM_QUOTIENT;
				/*@@*/
				fprintf(stderr, "Quotient not implemented\n");
				exit(1);
			} else if (!strncasecmp(optarg, "trie", 4)) {
				ngramtype = NGRAM_TRIE;
				/*@@*/
				fprintf(stderr, "Trie not implemented\n");
				exit(1);
			} else {
				fprintf(stderr, "Unknown filter %s\n",
					optarg);
				Usage();
			}
			setngramlabel(ngramtype, &ngramsize, 0, 0, 0, 0);
			break;
		case 'n':
			(void) readintrange(optarg, &ngramsize);
			if (ngramsize.max-ngramsize.min > NGRAM_RANGEMAX) {
				fprintf(stderr, "Too many ngram sizes %d > %d",
					ngramsize.max-ngramsize.min,
					NGRAM_RANGEMAX);
				Usage();
			}
			setngramlabel(ngramtype, &ngramsize, 0, 0, 0, 0);
			break;
		case 'E':
			entropyflag = yesno(optarg);
			break;
		case 'e':
			(void) readdoublerange(optarg, &entropyrange);
			break;
		case 'D':
			dumpfile = fopen(optarg, "w");
			if (!dumpfile) perror(optarg);
			break;
		case 'd':
			dumplevel = atoi(optarg);
			break;
#ifdef SHMALLOC
		case 'a':
			shmfilename = optarg;
			ngram_peek(shmfilename);
			shmmode = (O_CREAT);
			break;
		case 'A':
			shmfilename = optarg;
			break;
#endif
		case 'T':
			++timeflag;
			datemskinit();
			(void) readtimerange(optarg, &starttime, &endtime);
			break;
		case '?':
		default:
			Usage();
			break;
		}
	}
	return optind;
}

void
main(int argc, char **argv)
{
	pcap_t *readp;
	FILE *fp;
	int i;
	char errbuf[PCAP_ERRBUF_SIZE];
	long int totalcount = 0L;
	int readpcap(pcap_t *readp, int *flag);
	void flushpackets(void);
	double mu, sigma;
	u_int64_t max, min;
	int atend=0;

	/* @@ handle flags */
	i = handle_arguments(argc, argv);

	/* Do any preprocessing */
	/* Sanity check */
	if (endymd < startymd) {
		fprintf(stderr,
			"Warning: adjusting end date %d to %d\n",
			endymd, startymd);
		endymd = startymd;
	}

	/* Only one of snortflag/snorthostflag/taggedhostflag
	 * should be specified, probably, but we'll accept any ... */
	if (snorthostflag >= 0) {
		if (snortokhosts)
			addsnortokhosts(snortokhosts);
		if (snortlist)
			(void) readsnorthostlist(snortlist, startymd, endymd);
	}
	if (snortflag >= 0) {
		if (snortlist) {
			(void) readsnortlist(snortlist, startymd, endymd);
			sortsnorttimes();
		}
	}
	if (taggedhostflag >=0) {
		if (taggedhosts)
			(void) readtaggedhosts(taggedhosts);
	}

	if (!(ngram->f =
#ifdef SHMALLOC
			(*ngram->op->newfilterset)(ngramlabel.ngramsize,
					shmfilename, shmmode)
#else
			(*ngram->op->newfilterset)(ngramlabel.ngramsize)
#endif
			)) {
		perror("Allocating ngram filters");
		exit(1);
	}


	/* Read and process all the capture files */
	if (i < argc) {
		for (; i < argc; ++i) {
			fp = fopen(argv[i], "r");
			if (!fp) {
				perror(argv[i]);
				continue;
			}
			readp = pcap_fopen_offline(fp, errbuf);
			if (!readp) {
				perror(errbuf);
				continue;
			}
			totalcount += readpcap(readp, &atend);
			pcap_close(readp);
			/* Note - pcap_close includes an fclose, somehow */
			/* We'll do one more file after the first ending,
			 * in case a few packets are out of sequence.
			 */
			if (atend>1) break;
		}
	} else {
		readp = pcap_fopen_offline(stdin, errbuf);
		if (!readp) {
			perror(errbuf);
		} else {
			totalcount += readpcap(readp, &atend);
			pcap_close(readp);
		}
	}

	/* Now go through the accumulated results, dumping ngram info */
	{
	int n;

	for (n = ngramlabel.ngramsize.min; n <= ngramlabel.ngramsize.max; ++n) {
		(*ngram->op->diststats)(n, &mu, &sigma, &max, &min, ngram->f);
		printf("ngram %d mu %10.8lf sigma %10.8lf max %ld min %ld sigma2/mu %10.8lf\n",
			n, mu, sigma, max, min, (sigma*sigma)/mu);
	}
	}
#ifdef TEST
	/* Simple mini-tester */
	while (1) {
		char line[BUFSIZ];
		int count;
		double mu, sigma, rho;
		int i;

		printf("Lookup? "); gets(line);
		for (i=ngram->f.ngramsize.min; i <= ngram->f.ngramsize.max; ++i) {
			if (strlen(line) < i) continue;
			count = (*ngram->op->finddist)((void *)line, strlen(line),
					i, &mu, &sigma, &rho, ngram->f->filter);
			printf("%d-grams found %d times: ", i, count);
			printf("Mu %10.8lf sigma %10.8lf rho %10.8lf\n",
				mu, sigma, rho);
		}
	}
#endif
	/* Done */
	(*ngram->op->closefilterset)(ngram->f);
	exit(0);
}
