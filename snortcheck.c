#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "snortcheck.h"
#include "ymd.h"
#include "readtree.h"

/* This code shouldn't really exist. It's a weird artifact of the
 * way we've stored snort information, plus a modicum of desperation
 * when I first wrote this up for a statistics class. I'm planning
 * on replacing this with something which keeps all times in timeval
 * format, which will greatly simplify things.
 */

/* snort times can be off one hour from pcap times, perhaps because
 * of some disagreement about DST. The date is either the start date
 * or one more, since the captures start and end midday.
 */

/* Just be simple-minded */

#define TIMESMAX	100000

struct snorttime {
	int ymd;		/* Superfluous, but was used in older code */
	u_int32_t length;
	struct timeval time;
} snorttimes[TIMESMAX];

int maxtimes;

/* For now, we match strictly on the time. When/if I have more
 * confidence in the length values, I'll use them, too.
 */
int cmpsnort(const void *a, const void *b)
{
	const struct snorttime *as, *bs;

	as = (const struct snorttime *)a;
	bs = (const struct snorttime *)b;
	if (!as) {
		if (!bs) return 0;
		return -1;
	}
	if (!bs)
		return 1;
	return timevalcmp(&as->time, &bs->time);
}

/* Rather than do an insertion sort as items are added, I simply dump
 * them all together, then qsort them at the end. There's no defense
 * against duplicate entries, but oh well. Just choose input that doesn't
 * have duplicates.
 */
void
sortsnorttimes(void)
{
	qsort(snorttimes, maxtimes, sizeof(struct snorttime), cmpsnort);
}

struct snorttime *
searchsnort(struct timeval *thistime,  u_int32_t size)
{
	struct snorttime thisone;

	memset(&thisone, 0, sizeof(thisone));
	thisone.time = *thistime;
	return (struct snorttime *)bsearch(&thisone, snorttimes,
		maxtimes, sizeof(struct snorttime), cmpsnort);
}

void
dumpsnorttimes(void)
{
	int i;

	for (i=0; i < maxtimes; ++i) {
		printf("%04d/%02d/%02d %s %d\n",
			ymdyear(snorttimes[i].ymd),
			ymdmonth(snorttimes[i].ymd),
			ymdday(snorttimes[i].ymd),
			sprinttimeval(&snorttimes[i].time),
			snorttimes[i].length);
	}
}

struct timeval
parsetime(char *timestring, int ymd)
{
	struct tm t;
	int usec;

	/* Snort date format is 03/15-16:20:13.512886 */

	sscanf(timestring, "%02d/%02d-%02d:%02d:%02d.%d ",
		&t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec,
		&usec);
	/* Check ... */
	if (ymd) {
		if (ymdmonth(ymd) != t.tm_mon || ymdday(ymd) != t.tm_mday) {
			fprintf(stderr, "Warning: ymd %d != tm %d/%d\n",
				ymd, t.tm_mon, t.tm_mday);
		}
	}
	/* tm weirdness */
	--t.tm_mon;
	t.tm_year = ymdyear(ymd)-1900;
	t.tm_isdst = -1;
	return tmtotimeval(&t, usec);
}

int
parsedgmlength(char *snortline)
{
	char *dgm;

	/* Snort line format is
	 * TCP TTL:63 TOS:0x0 ID:6219 IpLen:20 DgmLen:981 DF
	 */
	dgm = strstr(snortline, "DgmLen:");
	if (!dgm) return 0;
	dgm += strlen("DgmLen:");
	return atoi(dgm);
}


/* Read in a snort ASCII file and store the date/time info. Since this
 * is down to the microsecond, this is probably good enough in terms of
 * identifying a packet, but we could also pull in address/port info.
 */
int
readsnort(char *file, int startymd, int endymd)
{
	int ymd;
	int total=0;
	char line[BUFSIZ], line2[BUFSIZ], grepstring[20], *timestring;
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

	/* Read through the file line by line, pulling out date and
	 * length fields.
	 */
	while (fgets(line, BUFSIZ, fp)) {
		for (ymd = startymd; ymd != endymd; ymd = nextymd(ymd)) {
			sprintf(grepstring, "%02d/%02d-", (ymd/100)%100, ymd%100);
			if ((timestring = strstr(line, grepstring))) {
				snorttimes[maxtimes].ymd = ymd;
				snorttimes[maxtimes].time =
					parsetime(timestring, ymd);
				/* Next line should have length */
				if (fgets(line2, BUFSIZ, fp))
					snorttimes[maxtimes].length =
						parsedgmlength(line2);
				++maxtimes;
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
readsnortlist(char *list, int startymd, int endymd)
{
	return readallfiles(list, startymd, endymd, readsnort);
}


int
checksnort(struct timeval *thistime, u_int32_t size, int startymd, int endymd)
{
	int i;
	struct snorttime *found;

/*@@printf("%s\n",
	sprinttimeval(thistime));*/

	if (found = searchsnort(thistime, size)) {
		if (inymdrange(found->ymd, startymd, endymd))
			return 1;
	}
	if (found = searchsnort(prevhour(thistime), size)) {
		if (inymdrange(found->ymd, startymd, endymd))
			return 1;
	}
	if (found = searchsnort(nexthour(thistime), size)) {
		if (inymdrange(found->ymd, startymd, endymd))
			return 1;
	}
	return 0;
}


#ifdef TEST
main(int argc, char **argv)
{
	int startymd=20140101;
	int endymd=20150101;
	int total;
	char trystring[BUFSIZ];
	struct timeval trytime;

	/* @@ allow start/end args */
	total = readsnortlist(argv[1], startymd, endymd);
	sortsnorttimes();
	printf("Read %d, maxtimes %d\n", total, maxtimes);
	dumpsnorttimes();
	while (1) {
		printf("Give me a time: ");
		gets(trystring);
		memset(&trytime, 0, sizeof(trytime));
		sscanf(trystring, "%d.%d", &trytime.tv_sec, &trytime.tv_usec);
		printf("Check returns %d\n",
			checksnort(&trytime, 0, startymd, endymd));
		fflush(stdout);
	}
}
#endif

