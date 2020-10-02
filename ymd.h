#ifndef _YMD_H
#define _YMD_H

/* ymd format is an integer yyyymmdd. Some simple manipulations of it: */
#define ymdyear(ymd)	((ymd)/10000)
#define ymdmonth(ymd)	(((ymd)/100)%100)
#define ymdday(ymd)	((ymd)%100)
#define toymd(y,m,d)	(((y)*10000)+((m)*100)+d)

int nextymd(int ymd);
int prevymd(int ymd);
int tmtoymd(struct tm *t);
int inymdrange(int ymd, int startymd, int endymd);
int ymdrange(FILE *fp, int *startymd, int *endymd);

/* Other time routines */
int timevalcmp(const struct timeval *a, const struct timeval *b);
struct timeval tmtotimeval(struct tm *t, int usec);
struct tm timevaltotm(struct timeval *t);
char * sprinttimeval(const struct timeval *time);
struct timeval * nexthour(struct timeval *thistime);
struct timeval * prevhour(struct timeval *thistime);

struct timeval scantime(const char *string);
int intimerange(const struct timeval *t, const struct timeval *start,
        const struct timeval *end);
void readtimerange(char *string, struct timeval *start, struct timeval *end);


#endif
