#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include "parse.h"
#include "ymd.h"

static int lastday[13] = 
	{0, 31, 28, 31, 30,	/* j f** m a* - not a leap year */
	31, 30, 31, 31,		/* m j* j a */
	30, 31, 30, 31};	/* s* o n* d */

int
nextymd(int ymd)
{
	int year = ymdyear(ymd);
	int month = ymdmonth(ymd);
	int day = ymdday(ymd);

	if (month > 12) month = 1;	/* huh? */
	++day;
	if (day > lastday[month]) {
		++month;
		if (month > 12) {
			month = 1;
			++year;
		}
		day = 1;
	}
	return toymd(year, month, day);
}

int
prevymd(int ymd)
{
	int year = ymdyear(ymd);
	int month = ymdmonth(ymd);
	int day = ymdday(ymd);

	if (month > 12) month = 1;	/* huh? */
	--day;
	if (day <= 0) {
		--month;
		if (month <= 0) {
			month = 12;
			--year;
		}
		day = lastday[month];
	}
	return toymd(year, month, day);
}

int
tmtoymd(struct tm *t)
{
	/* tm format is a little odd ... */
	return toymd(t->tm_year+1900, t->tm_mon+1, t->tm_mday);
}

/* timeval uses macros in slightly inconvenient ways ... */
int timevalcmp(const struct timeval *a, const struct timeval *b)
{
	return (a->tv_sec != b->tv_sec ? a->tv_sec - b->tv_sec :
		a->tv_usec - b->tv_usec);
}

/* -1: t is before start of range
 *  0: t is in range
 *  1: t is past end of range
 */
int
intimerange(const struct timeval *t, const struct timeval *start,
	const struct timeval *end)
{
	if (start) {
		if (timevalcmp(t, start) < 0) return -1;
	}
	if (end) {
		/* An end of 0, 0 is infinity ... */
		if (end->tv_sec || end->tv_usec) {
			if (timevalcmp(t, end) > 0) return 1;
		}
	}
	return 0;
}

struct timeval
tmtotimeval(struct tm *t, int usec)
{
	struct timeval answer;

	if (t)
		answer.tv_sec = mktime(t);
	else
		answer.tv_sec = 0;
	answer.tv_usec = usec;
	return answer;
}

struct tm
timevaltotm(struct timeval *t)
{
	struct tm answer;

	answer = *localtime((time_t *)&t->tv_sec);
	return answer;
}

/* Very simple-minded print routine. */
char *
sprinttimeval(const struct timeval *time)
{
	static char answer[BUFSIZ];

	sprintf(answer, "%d.%06d", time->tv_sec, time->tv_usec);
	return answer;
}

/* getdate() needs a file of possible date templates. It'd be nice
 * if it had a builtin list, or a way to supply one, but it's written
 * to require a file. There's a hacked-together one with the code
 * here.
 */
char *
tryfile(char *file)
{
	struct stat dummy;
	static char longpath[PATH_MAX];
	char *home;

	/* Try the straight pathname */
	if (!stat(file, &dummy))
		return file;
	/* Check for homedir. Note I'm not bothering with ~user */
	if (*file == '~') {
		/* There's the possibility of a hack, using a bogus
		 * environmental variable. Why someone would hack
		 * themselves this way is beyond me, but ...
		 */
		home = getenv("HOME");
		if (!home) return NULL;
		if ((strlen(home) + strlen(file)) >= PATH_MAX)
			return NULL;
		sprintf(longpath, "%s%s", home, file+1);
		if (!stat(longpath, &dummy))
			return longpath;
	}
	return NULL;
}

void
datemskinit(void)
{
	static char *datemsklist[] = {
		"./datemsk",
		"~/datemsk",
		"/etc/datemsk",
		"/tmp/datemsk",
		NULL
	};
	char *file;
	/* @@ Sloppy with lengths */
	char datebuf[PATH_MAX+20];
	int i;

	if (!getenv("DATEMSK")) {
		/* Look for it */
		for (i=0; datemsklist[i]; ++i) {
			if ((file=tryfile(datemsklist[i]))) {
				sprintf(datebuf, "DATEMSK=%s", file);
				putenv(datebuf);
				return;
			}
		}
	}
	/* @@ Last ditch - write out our default list to /tmp and
	 * use that instead.
	 */
	{
	static char defaultlist[] =
"%a %B %d %H:%M:%S %Y\n"
"%a %B %d %H:%M:%S %y\n"
"%A %B %d, %Y %H:%M:%S\n"
"%A\n"
"%B\n"
"%m/%d/%y %I %p\n"
"%d,%m,%Y %H:%M\n"
"%m/%d/%y\n"
"%H:%M\n"
"%m/%d/%y %H:%M\n"
"%B %Y\n"
"%a %d %b %Y\n"
"%y/%m/%d %H:%M:%S\n"
"%Y/%m/%d %H:%M:%S\n"
"%y/%m/%d %I:%M %p\n"
"%Y/%m/%d %I:%M %p\n"
"%y/%m/%d\n"
"%Y/%m/%d\n"
"%y-%m-%d %H:%M:%S\n"
"%y-%m-%d %H:%M.%S\n"
"%Y-%m-%d %H:%M:%S\n"
"%Y-%m-%d %H:%M.%S\n"
"%y-%m-%d %I:%M %p\n"
"%Y-%m-%d %I:%M %p\n"
"%y-%m-%d\n"
"%Y-%m-%d\n"
"%m/%d/%y %I:%M:%S %p\n"
"%m/%d/%Y %I:%M:%S %p\n"
"%m/%d/%y %H:%M:%S\n"
"%m/%d/%Y %H:%M:%S\n"
"%m/%d/%y %I:%M %p\n"
"%m/%d/%Y %I:%M %p\n"
"%m/%d/%y %H:%M\n"
"%m/%d/%Y %H:%M\n"
"%m/%d/%y\n"
"%m/%d/%Y\n"
"%b %d, %Y %I:%M:%S %p\n"
"%b %d, %Y %H:%M:%S\n"
"%B %d, %Y %I:%M:%S %p\n"
"%B %d, %Y %H:%M:%S\n"
"%b %d, %Y %I:%M %p\n"
"%b %d, %Y %H:%M\n"
"%B %d, %Y %I:%M %p\n"
"%B %d, %Y %H:%M\n"
"%b %d, %Y\n"
"%B %d, %Y\n"
"%b %d\n"
"%B %d\n"
"%m%d%H%M%y\n"
"%m%d%H%M%Y\n"
"%m%d%H%M\n"
"%m%d\n";
	FILE *datefp;

	datefp = fopen("/tmp/datemsk", "w");
	if (!datefp)
		return;
	fwrite(defaultlist, 1, strlen(defaultlist), datefp);
	fclose(datefp);
	sprintf(datebuf, "DATEMSK=/tmp/datemsk");
	putenv(datebuf);
	}
}


struct timeval
scantime(const char *string)
{
	static struct timeval zero;

	if (!string || !*string)
		return zero;
	return tmtotimeval(getdate(string), 0);
}

void
readtimerange(char *string, struct timeval *start, struct timeval *end)
{
	char *times[2];
	int n;

	/* Date strings use all sorts of separators internally, so
	 * we've got to be careful about what we allow to split two
	 * of them. I'm going with just commas, since they're used
	 * for separating other argument types as well. It does mean
	 * you can't have, e.g, "Sep 12, 1984 10:35" as a date.
	 */
	n = parsenargs(string, times, ",", 2);
	*start = scantime(times[0]);
	if (n == 2)
		*end = scantime(times[1]);
	else
		memset((void *)end, 0, sizeof(struct timeval));
}


	

int
inymdrange(int ymd, int startymd, int endymd)
{
	int i;

	for (i = startymd; i != endymd; i = nextymd(i))
		if (ymd == i) return 1;
	return 0;
}

struct timeval *
nexthour(struct timeval *thistime)
{
	static struct timeval answer;

	answer = *thistime;
	answer.tv_sec += 3600;
	return &answer;
}

struct timeval *
prevhour(struct timeval *thistime)
{
	static struct timeval answer;

	answer = *thistime;
	answer.tv_sec -= 3600;
	return &answer;
}

/* Determine the applicable date range for a snort alert file. We collect
 * these by the week, so the start is 7 days before. The end is the date
 * of the file plus one, since we exclude the end date in the checks below.
 */
int
ymdrange(FILE *fp, int *startymd, int *endymd)
{
	struct stat statbuf;
	struct tm alerttime;
	int ymd, fstartymd, fendymd;
	int i;

	if (fstat(fileno(fp), &statbuf) < 0)
		return -1;
	(void) gmtime_r(&statbuf.st_mtime, &alerttime);
	ymd = tmtoymd(&alerttime);
	fendymd = nextymd(ymd);
	fstartymd = prevymd(ymd);
	/* Just be stupid about this for now */
	for (i=0; i < 6; ++i)
		fstartymd = prevymd(fstartymd);
	/* Double-check against any passed-in values. */
	if (*startymd > ymd)
		fprintf(stderr,
			"Warning: start date %d is after file date %d\n",
				*startymd, ymd);
	else if (!*startymd)
		*startymd = fstartymd;
	if (*endymd < ymd)
		fprintf(stderr,
			"Warning: end date %d is before file date %d\n",
				*endymd, ymd);
	else if (!*endymd)
		*endymd = fendymd;
	return 0;
}

/*@@debug*/
char *
stimeval(const struct timeval *t, char *buf)
{
	strcpy(buf, sprinttimeval(t));
	return buf;
}
