/* range.h - ranges of integer or float values */

#ifndef _RANGE_H
#define _RANGE_H

/* We'll allow multiple ranges and/or individual values. */
typedef struct _range {
	int min, max;
} Range;

typedef struct _rangelist {
	int nranges;
	Range ranges[0];
} Rangelist;

/* For the sake of definiteness, have some sort of limit */
#define RANGEMAX	32000

extern Rangelist *protocols, *ports;

typedef struct _drange {
	double min, max;
} DRange;

typedef struct _drangelist {
	int nranges;
	DRange ranges[0];
} DRangelist;

int readdoublerange(char *string, DRange *range);
DRangelist *readdoublerangelist(char *string);
int readintrange(char *string, Range *range);
Rangelist * readintrangelist(char *string);
int readportrange(char *string, Range *range);
Rangelist * readportrangelist(char *string);
int readprotorange(char *string, Range *range);
Rangelist * readprotorangelist(char *string);

int inrange(int value, Range *range);
int inrangelist(int value, Rangelist *rangelist);
int indoublerange(double value, DRange *range);
int indoublerangelist(double value, DRangelist *rangelist);

#endif
