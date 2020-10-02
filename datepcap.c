#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <pcap/pcap.h>
#include <memory.h>
#include <getopt.h>
#include "ymd.h"

/*  pull the date from a pcap file */

int
datepcap(char *pcap, struct timeval *tpcap)
{
	FILE *fp;
	pcap_t *readp;
	char errbuf[PCAP_ERRBUF_SIZE];
	struct pcap_pkthdr *pkt_header;
	u_int8_t *pkt_data;

	if (!(fp = fopen(pcap, "r")))
		return -1;
	if (!(readp = pcap_fopen_offline(fp, errbuf))) {
		fclose(fp);
		return -1;
	}
	if ((pcap_next_ex(readp, &pkt_header,
			(const u_char **)&pkt_data)) <= 0) {
		pcap_close(readp);
		return -1;
	}

	*tpcap = pkt_header->ts;
	pcap_close(readp);
	return 0;
}

#ifdef TEST
int
main(int argc, char **argv)
{
	int i;
	struct timeval t;
	struct tm tm;

	for (i=1; i < argc; ++i) {
		if (datepcap(argv[i], &t) < 0) {
			perror(argv[i]);
		} else {
			tm = timevaltotm(&t);
			printf("%s: %s %s", argv[i],
				sprinttimeval(&t), asctime(&tm));
		}
	}
	return 0;
}
#else
int
main(int argc, char **argv)
{
	int i;
	struct timeval t;
	void datemskinit(void);
	struct timeval starttime, endtime;
	int starti=0, endi=0;
	int limitonly=0;
	int c;

	while ((c = getopt(argc, argv, "T:l"))>=0) {
		switch (c) {
		case 'T':
			datemskinit();
			(void) readtimerange(optarg, &starttime, &endtime);
			break;
		case 'l':
			++limitonly;
			break;
		default:
			fprintf(stderr, "Huh?\n");
			exit(1);
		}
	}
	/* Select starting with the last one before the start, and ending
	 * with the first one after the end (in case there are a couple
	 * out of sequence);
	 */
	for (i = optind; i < argc; ++i) {
		if (datepcap(argv[i], &t) < 0) {
			perror(argv[i]);
		} else {
			switch (intimerange(&t, &starttime, &endtime)) {
			case -1:	/* before range */
				break;
			case 0:		/* in range */
				if (!starti) {
					if (i > optind)
						starti = i-1;
					else
						starti = i;
					
				}
				break;
			case 1:		/* after range */
				if (!endi) {
					if (i < argc-1)
						endi = i+1;
					else
						endi = i;
				}
				goto printout;
				break;
			}
		}
	}
	/* Fell through; set end to last arg */
	if (!endi) endi = argc-1;
printout:
	if (limitonly) {
		printf("%s ", argv[starti]);
		printf("%s\n", argv[endi]);
	} else {
		for (i = starti; i <= endi; ++i) {
			printf("%s ", argv[i]);
		}
		printf("\n");
	}
	return 0;
}
#endif
