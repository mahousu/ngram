#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include "parse.h"
#include "range.h"

/* range.c - some simple routines for handling ranges of integer
 * or float values. No doubt there's some standard version of these
 * things that I'm blanking out on, but they're easy enough to
 * implement, so why not.
 */

/* Ranges can be presented in a few ways:
 * 	a-b,a:b 	all from a to b
 * 	a		just a
 * 	, ; 	separators for multiple ranges
 *	porta-portb	can also use port/protocol names
 */

/* straight integer version, one range. Returns the number of
 * integers read - 2 means a-b type; 1 means a type; 0 means error.
 * For now, we ignore the return value, but we may have a use for
 * it later.
 */
int
readintrange(char *string, Range *range)
{
	char *limits[2];
	int nlim;

	nlim = parsenargs(string, limits, "-: ", 2);
	errno = 0;
	range->min = strtol(limits[0], NULL, 0);
	if (errno)
		range->min = 0;
	if (nlim > 1) {
		errno = 0;
		range->max = strtol(limits[1], NULL, 0);
		if (errno) {
			range->max = 0;
			return 1;
		}
		return 2;
	} else {
		range->max = range->min;
		return 1;
	}
	return 0;
}

Rangelist *
readintrangelist(char *string)
{
	char *ranges[RANGEMAX];
	int nranges;
	Rangelist *answer;
	int i;

	nranges = parsenargs(string, ranges, ",; ", RANGEMAX);
	answer = (Rangelist *)malloc(sizeof(Rangelist) + nranges*sizeof(Range));
	if (!answer) return NULL;
	answer->nranges = nranges;
	for (i=0; i < nranges; ++i) {
		/* Probably should do error handling, heh, heh. */
		(void) readintrange(ranges[i], &answer->ranges[i]);
	}
	return answer;
}

/* The same for double/float values. Used for entropies. */
int
readdoublerange(char *string, DRange *range)
{
	char *limits[2];
	int nlim;

	nlim = parsenargs(string, limits, "-: ", 2);
	errno = 0;
	range->min = strtod(limits[0], NULL);
	if (errno)
		range->min = 0.0;
	if (nlim > 1) {
		errno = 0;
		range->max = strtod(limits[1], NULL);
		if (errno) {
			range->max = 0.0;
			return 1;
		}
		return 2;
	} else {
		range->max = range->min;
		return 1;
	}
	return 0;
}

DRangelist *
readdoublerangelist(char *string)
{
	char *ranges[RANGEMAX];
	int nranges;
	DRangelist *answer;
	int i;

	nranges = parsenargs(string, ranges, ",; ", RANGEMAX);
	answer = (DRangelist *)malloc(sizeof(DRangelist) + nranges*sizeof(DRange));
	if (!answer) return NULL;
	answer->nranges = nranges;
	for (i=0; i < nranges; ++i) {
		/* Probably should do error handling, heh, heh. */
		(void) readdoublerange(ranges[i], &answer->ranges[i]);
	}
	return answer;
}

/* Here, they can be either numeric or something from /etc/services.
 * Note we're not distinguishing by protocol; I figure that's not
 * going to be a big issue, as the protocols will presumably just
 * be tcp or udp, and the ports that overlap between the two almost
 * always have the same name.
 */
int
readportrange(char *string, Range *range)
{
	char *limits[2];
	int nports;
	struct servent *serv;

	/* Chop up */
	nports = parsenargs(string, limits, "-: ", 2);
	if (nports <= 0) return 0;
	/* First, see if service name */
	serv = getservbyname(limits[0], NULL);
	if (serv) {
		range->min = ntohs(serv->s_port);
	} else {
		errno = 0;
		range->min = strtol(limits[0], NULL, 0);
		if (errno) {
			/*perror(limits[0]);*/
			range->min = 0;
		}
	}
	if (nports <= 1) {
		range->max = range->min;
		return 1;
	}
	/* Again, see if service name */
	serv = getservbyname(limits[1], NULL);
	if (serv) {
		range->max = ntohs(serv->s_port);
		return 2;
	} else {
		errno = 0;
		range->max = strtol(limits[1], NULL, 0);
		if (errno) {
			/*perror(limits[1]);*/
			range->max = 0;
			return 1;
		} else {
			return 2;
		}
	}
}

/* Here's where polyvalent typing would be nice - wouldn't have to
 * rewrite this all the time.
 */
Rangelist *
readportrangelist(char *string)
{
	char *ranges[RANGEMAX];
	int nranges;
	Rangelist *answer;
	int i;

	nranges = parsenargs(string, ranges, ",; ", RANGEMAX);
	answer = (Rangelist *)malloc(sizeof(Rangelist) + nranges*sizeof(Range));
	if (!answer) return NULL;
	answer->nranges = nranges;
	for (i=0; i < nranges; ++i) {
		/* Probably should do error handling, heh, heh. */
		(void) readportrange(ranges[i], &answer->ranges[i]);
	}
	return answer;
}

/* Here, they can be either numeric or something from /etc/protocols */
int
readprotorange(char *string, Range *range)
{
	char *limits[2];
	int nprotos;
	struct protoent *proto;

	/* Chop up */
	nprotos = parsenargs(string, limits, "-: ", 2);
	if (nprotos <= 0) return 0;
	/* First, see if protocol name */
	proto = getprotobyname(limits[0]);
	if (proto) {
		range->min = proto->p_proto;
	} else {
		errno = 0;
		range->min = strtol(limits[0], NULL, 0);
		if (errno) {
			/*perror(limits[0]);*/
			range->min = 0;
		}
	}
	if (nprotos <= 1) {
		range->max = range->min;
		return 1;
	}
	/* Again, see if protocol name */
	proto = getprotobyname(limits[1]);
	if (proto) {
		range->max = proto->p_proto;
		return 2;
	} else {
		errno = 0;
		range->max = strtol(limits[1], NULL, 0);
		if (errno) {
			/*perror(limits[1]);*/
			range->max = 0;
			return 1;
		} else {
			return 2;
		}
	}
}

Rangelist *
readprotorangelist(char *string)
{
	char *ranges[RANGEMAX];
	int nranges;
	Rangelist *answer;
	int i;

	nranges = parsenargs(string, ranges, ",; ", RANGEMAX);
	answer = (Rangelist *)malloc(sizeof(Rangelist) + nranges*sizeof(Range));
	if (!answer) return NULL;
	answer->nranges = nranges;
	for (i=0; i < nranges; ++i) {
		/* Probably should do error handling, heh, heh. */
		(void) readprotorange(ranges[i], &answer->ranges[i]);
	}
	return answer;
}

int
inrange(int value, Range *range)
{
	/* Special case - empty range means no filtering (take all) */
	if (!range) return 1;
	if (value < range->min) return 0;
	if (value <= range->max || range->max == 0) return 1;
	return 0;
}

int
inrangelist(int value, Rangelist *rangelist)
{
	int i;


	/* Special case - empty list means no filtering (take all) */
	if (!rangelist) return 1;
	for (i=0; i < rangelist->nranges; ++i) {
		if (inrange(value, &rangelist->ranges[i]))
			return 1;
	}
	return 0;
}

int
indoublerange(double value, DRange *range)
{
	/* Special case - empty range means no filtering (take all) */
	if (!range) return 1;
	if (value < range->min) return 0;
	if (value <= range->max || range->max == 0.0) return 1;
	return 0;
}

int
indoublerangelist(double value, DRangelist *rangelist)
{
	int i;

	/* Special case - empty list means no filtering (take all) */
	if (!rangelist) return 1;
	for (i=0; i < rangelist->nranges; ++i) {
		if (indoublerange(value, &rangelist->ranges[i]))
			return 1;
	}
	return 0;
}

#ifdef TEST
#include <getopt.h>

void
main(int argc, char **argv)
{
	int c;
	DRangelist *drange;
	int dodouble=0;
	double dvalue;
	Rangelist *irange;
	int ivalue;
	char input[BUFSIZ];

	while ((c = getopt(argc, argv, "d:i:P:p:")) >= 0) {
		switch (c) {
		case 'd':
			drange = readdoublerangelist(optarg);
			dodouble=1;
			break;
		case 'i':
			irange = readintrangelist(optarg);
			break;
		case 'P':
			irange = readprotorangelist(optarg);
			break;
		case 'p':
			irange = readportrangelist(optarg);
			break;
		}
	}
	while (1) {
		printf("Value? ");
		gets(input);
		if (dodouble) {
			dvalue = atof(input);
			printf("%f inrange: %d\n",  dvalue,
				indoublerangelist(dvalue, drange));
		} else {
			ivalue = atoi(input);
			printf("%d inrange: %d\n",  ivalue,
				inrangelist(ivalue, irange));
		}
	}
}
#endif
			
