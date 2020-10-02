#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "readtree.h"
#include "ymd.h"
#include "cidr.h"
#include "parse.h"
#define _SNORTHOSTCHECKINTERNAL
#include "snorthostcheck.h"

/* This version tags any "outside" host which has had a snort record
 * within the given period, under the assumption that all of its
 * traffic is likely suspicious. Outside here means outside the
 * supplied list of trusted hosts - e.g., 129.6.100/24
 */

/* This should of course be made protocol-independent */

/* These are the bad hosts */
struct in_addr snorthosts[HOSTSMAX];
int maxhosts;

/* And these are the good ones */
cidr_t *goodhosts;

/* Since we'd expect many duplicates in the host list, let's go
 * ahead and do insertion sort when adding.
 */

/* find the first location with value >= host. If none, return maxhosts.
 * This is a sloppy implementation of binary search.
 */
int
findsnorthost(struct in_addr host)
{
	int here, bottom, top;

	if (!maxhosts) return 0;

	bottom = 0;
	top = maxhosts;
	for (bottom=0, top=maxhosts; bottom != top; ) {
		here = (bottom+top)/2;
		if (snorthosts[here].s_addr == host.s_addr) {
			return here;
		} else if (snorthosts[here].s_addr < host.s_addr) {
			if (bottom != here) {
				bottom = here;
			} else {
				bottom = here+1;
			}
		} else /* snorthosts[here].s_addr > host.s_addr */ {
			if (top != here) {
				top = here;
			} else {
				top = here-1;
			}
		}
	}
	return bottom;
}

/* add a host address into the array. Returns 1 if newly added, 0 if
 * already present, and -1 if no room left.
 */
int
addsnorthost(struct in_addr host)
{
        int where;

        where = findsnorthost(host);
        if (where == maxhosts) {
                if (maxhosts >= HOSTSMAX) return -1;
                snorthosts[where] = host;
        } else if (snorthosts[where].s_addr == host.s_addr) {
                return 0;
        } else {
                /* Shove array down */
                if (maxhosts >= HOSTSMAX) return -1;
                memmove(snorthosts+where+1, snorthosts+where,
                        (maxhosts-where)*sizeof(struct in_addr));
                snorthosts[where] = host;
        }
        ++maxhosts;
        return 1;
}

/* Select whichever hosts in a snort message have not been
 * deemed "safe," and add them to the list to watch.
 */
int
addmiscreanthost(struct in_addr source, struct in_addr dest, cidr_t *safe)
{
	int added;

	added=0;
	if (!incidrlist(source, safe))
		added += addsnorthost(source);
	if (!incidrlist(dest, safe))
		added += addsnorthost(dest);
	return added;
}

int
addsnortokhosts(char *list)
{
	goodhosts = strtocidrlist(list);
	return (goodhosts != NULL);
}


/* Read in a snort ASCII file and store the miscreant host info. */
/* Format:
 *
 * [**] [1:17317:4] SPECIFIC-THREATS OpenSSH sshd Identical Blocks DOS attempt [**]
 * [Classification: Attempted Administrator Privilege Gain] [Priority: 1] 
 * 06/23-06:21:24.680462 129.6.140.210:33619 -> 129.6.100.251:22
 * TCP TTL:59 TOS:0x0 ID:3894 IpLen:20 DgmLen:100 
 *
 * So the host information is in the third line, the one with the date+time
 */
int
parsehosts(char *line, struct in_addr *hosta, struct in_addr *hostb)
{
	char *args[5];
	int n;
	int ret=0;

	/* 129.6.140.210:33619 -> 129.6.100.251:22 becomes
	 * 0 129.6.140.210
	 * 1 33619
	 * 2 129.6.100.251
	 * 3 22
	 */
	n = parsenargs(line, args, ": ->", 5);
	ret += (inet_aton(args[0], hosta) > 0);
	if (n >= 3)
	ret += (inet_aton(args[2], hostb) > 0);
	return ret;
}

int
readsnorthosts(char *file, int startymd, int endymd)
{
	int ymd;
	int total=0;
	char line[BUFSIZ], grepstring[20], *hoststring;
	struct in_addr hosta, hostb;
	FILE *fp;

	fp = fopen(file, "r");
	if (!fp) {
		perror(file);
		return 0;
	}
	/* Unfortunately, snort ASCII files omit the year. We can
	 * either supply a year/date range specification as arguments,
	 * or pick up the file's date (assuming the latter hasn't been
	 * touched since). Best case is when we have both.
	 */
	if (ymdrange(fp, &startymd, &endymd) < 0) {
		/* No file info; make sure we've got something to work with */
		if (!startymd) {
			fprintf(stderr, "Warning: no start date available\n");
			/* Just guess */
			startymd = 20140101;
		} 
		if (!endymd) {
			fprintf(stderr, "Warning: no end date available\n");
			/* Just guess */
			endymd = 20150101;
		} 
		
	}

	/* Read through the file line by line, looking for date lines. */
	while (fgets(line, BUFSIZ, fp)) {
		for (ymd = startymd; ymd != endymd; ymd = nextymd(ymd)) {
			sprintf(grepstring, "%02d/%02d-", (ymd/100)%100, ymd%100);
			if ((strstr(line, grepstring))) {
				/* Skip to the hosts */
				hoststring = strchr(line, ' ');
				++hoststring;
				parsehosts(hoststring, &hosta, &hostb);
				addmiscreanthost(hosta, hostb, goodhosts);
				++total;
				break;
			}
		}
	}
	fclose(fp);
	return total;
}

/* Read all the snort alert files in a list of files and/or directories */
int
readsnorthostlist(char *list, int startymd, int endymd)
{
	return readallfiles(list, startymd, endymd, readsnorthosts);
}


int
checksnorthosts(u_int32_t source, u_int32_t dest)
{
	struct in_addr src, dst;
	int where;

	src.s_addr = source;
	dst.s_addr = dest;
	where = findsnorthost(src);
        if (where < maxhosts) {
		if (snorthosts[where].s_addr == src.s_addr)
			return 1;
	}
	where = findsnorthost(dst);
        if (where < maxhosts) {
		if (snorthosts[where].s_addr == dst.s_addr)
			return 1;
	}
	return 0;
}


#ifdef TEST
void
dumpsnorthosts(void)
{
	int i;

	for (i=0; i < maxhosts; ++i) {
		printf("%s\n", inet_ntoa(snorthosts[i]));
	}
}

main(int argc, char **argv)
{
	int startymd=20140101;
	int endymd=20150101;
	int listarg = 1;
	int total;
	char trystring[BUFSIZ];
	struct in_addr tryhost;

	/* @@ allow start/end args */
	if (!strcmp(argv[1], "-h")) {
		addsnortokhosts(argv[2]);
		listarg = 3;
	}
	total = readsnorthostlist(argv[listarg], startymd, endymd);
	printf("Read %d, maxhosts %d\n", total, maxhosts);
	dumpsnorthosts();
	while (1) {
		printf("Give me a host: ");
		gets(trystring);
		(void) inet_aton(trystring, &tryhost);

		printf("Check returns %d\n",
			checksnorthosts(tryhost.s_addr, tryhost.s_addr));
		fflush(stdout);
	}
}
#endif

