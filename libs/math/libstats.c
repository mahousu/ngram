#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <strings.h>
#include "libstats.h"

#define min(a, b) ((a) <= (b) ? (a) : (b))
#define max(a, b) ((a) >= (b) ? (a) : (b))

int
plusreduction(int *array, int from, int to)
{
	int total=0, i;

	for (i=from; i<=to; ++i)
		total += array[i];
	return total;
}

void
filemode(FILE *fp, double *mode, double *fuzz, double *fraction)
{
	int *table;
	int i, index, limit=1000, last=0;
	double input;
	int count=0;
	int maxdex;
	int frac;
	int j;

	table = malloc(limit*sizeof(int));
	bzero(table, limit*sizeof(int));
	while (!feof(fp)) {
		fscanf(fp, "%lf", &input);
		index = (int)rint(input);
		if (index >= limit) {
			int oldlimit = limit;
			limit = index + 100;
			table = realloc(table, limit*sizeof(int));
			bzero(table+oldlimit, (limit-oldlimit)*sizeof(int));
		}
		++table[index];
		++count;
		if (index > last)
			last = index +1;
	}
	maxdex = 0;
	for (i=0; i < last; ++i)
		if (table[i] > table[maxdex])
			maxdex = i;
tryagain:
	*mode = (double)maxdex;
	for (j=0; j < min(maxdex, last-maxdex); ++j) {
		frac = plusreduction(table, maxdex-j, maxdex+j);
		if (frac > count/2)
			break;
	}
	if (j == min(maxdex, last-maxdex)) { /* oops, suspect mode... */
		/*fprintf(stderr, "mode %d hits wall...\n", maxdex);*/
		for (i=maxdex+1; i < last; ++i) {
			if (table[i] >= table[maxdex]) {
				maxdex = i;
				goto tryagain;
			}
		}
	}
	*fuzz = (double)j;
	*fraction = (double)frac/(double)count;
	free((void *)table);
}

void
arraymode(double *x, int length, double *mode, double *fuzz, double *fraction)
{
	int *table;
	int i, j, index, limit=1000, last=0;
	double input;
	int count=0;
	int frac;
	int maxdex;

	table = malloc(limit*sizeof(int));
	bzero(table, limit*sizeof(int));
	for (i=0; i < length; ++i) {
		input = x[i];
		index = (int)rint(input);
		if (index >= limit) {
			int oldlimit = limit;
			limit = index + 100;
			table = realloc(table, limit*sizeof(int));
			bzero(table+oldlimit, (limit-oldlimit)*sizeof(int));
		}
		++table[index];
		++count;
		if (index > last)
			last = index +1;
	}
	maxdex = 0;
	for (i=0; i < last; ++i)
		if (table[i] > table[maxdex])
			maxdex = i;
tryagain2:
	*mode = (double)maxdex;
	for (j=0; j < min(maxdex, last-maxdex); ++j) {
		frac = plusreduction(table, maxdex-j, maxdex+j);
		if (frac > count/2)
			break;
	}
	if (j == min(maxdex, last-maxdex)) { /* oops, suspect mode... */
		/*fprintf(stderr, "mode %d hits wall...\n", maxdex);*/
		for (i=maxdex+1; i < last; ++i) {
			if (table[i] >= table[maxdex]) {
				maxdex = i;
				goto tryagain2;
			}
		}
	}
	*fuzz = (double)j;
	*fraction = (double)frac/(double)count;
	free((void *)table);
}

int
minirange(int *table, int last, int i, int fuzz)
{
	int low = max(0, i-fuzz);
	int high = min(last-1, i+fuzz);

	return plusreduction(table, low, high);
}

void
arrayfuzzmode(double *x, int length, int infuzz, double *mode, double *fuzz, double *fraction)
{
	int *table;
	int i, j, index, limit=1000, last=0;
	double input;
	int count=0;
	int frac;
	int maxdex;

	table = malloc(limit*sizeof(int));
	bzero(table, limit*sizeof(int));
	for (i=0; i < length; ++i) {
		input = x[i];
		index = (int)rint(input);
		if (index >= limit) {
			int oldlimit = limit;
			limit = index + 100;
			table = realloc(table, limit*sizeof(int));
			bzero(table+oldlimit, (limit-oldlimit)*sizeof(int));
		}
		++table[index];
		++count;
		if (index > last)
			last = index +1;
	}
	maxdex = 0;
	
	for (i=0; i < last; ++i)
		if (minirange(table, last, i, infuzz) >
			minirange(table, last, maxdex, infuzz))
			maxdex = i;
tryagain3:
	*mode = (double)maxdex;
	for (j=0; j < min(maxdex, last-maxdex); ++j) {
		frac = plusreduction(table, maxdex-j, maxdex+j);
		if (frac > count/2)
			break;
	}
	if (j == min(maxdex, last-maxdex)) { /* oops, suspect mode... */
		/*fprintf(stderr, "mode %d hits wall...\n", maxdex);*/
		for (i=maxdex+1; i < last; ++i) {
			if (minirange(table, last, i, infuzz) >=
				minirange(table, last, maxdex, infuzz)) {
				maxdex = i;
				goto tryagain3;
			}
		}
	}
	*fuzz = (double)j;
	*fraction = (double)frac/(double)count;
	free((void *)table);
}

void
filestats(FILE *fp, double *mu, double *sigma, double *rho)
{
	struct stat info;
	double *x;
	int limit;
	int n=0, i;
	double sumsquare=0.0, sum=0.0, top=0.0;
	double sigma2=0.0;

	fstat(fileno(fp), &info);
	if (info.st_size > 0) {
		limit = info.st_size/2;	/* @@ approximate */
	} else {
		limit = 300000;
	}
	x = (double *)malloc(limit*sizeof(double));

	for (i=0; i<limit; ++i){
		fscanf(fp, "%lf", &x[i]);
		if (feof(fp))
			break;
		sumsquare += x[i]*x[i];
		sum += x[i];
		++n;
	}
	*mu = sum/(double)n;
	*sigma = sqrt((sumsquare - (double)n*(*mu)*(*mu))/(double)(n-1));

	for (i=1; i < n; ++i){
		top += ((double)x[i]- *mu)*((double)x[i-1]- *mu);
		sigma2 += ((double)x[i-1] - *mu)*((double)x[i-1] - *mu);

	}
	*rho = top/sigma2;
	free(x);
}

void
filestatsx(FILE *fp, double *mu, double *sigma, double *rho, double *dmin, double *dmax)
{
	struct stat info;
	double *x;
	int limit;
	int n=0, i;
	double sumsquare=0.0, sum=0.0, top=0.0;
	double sigma2=0.0;

	fstat(fileno(fp), &info);
	if (info.st_size > 0) {
		limit = info.st_size/2;	/* @@ approximate */
	} else {
		limit = 300000;
	}
	x = (double *)malloc(limit*sizeof(double));

	*dmin = HUGE_VAL;
	*dmax = -HUGE_VAL;

	for (i=0; i<limit; ++i){
		fscanf(fp, "%lf", &x[i]);
		if (feof(fp))
			break;
		sumsquare += x[i]*x[i];
		sum += x[i];
		if (x[i] < *dmin) *dmin = x[i];
		if (x[i] > *dmax) *dmax = x[i];
		++n;
	}
	*mu = sum/(double)n;
	*sigma = sqrt((sumsquare - (double)n*(*mu)*(*mu))/(double)(n-1));

	for (i=1; i < n; ++i){
		top += ((double)x[i]- *mu)*((double)x[i-1]- *mu);
		sigma2 += ((double)x[i-1] - *mu)*((double)x[i-1] - *mu);

	}
	*rho = top/sigma2;
	free(x);
}

void
arraystats(double *x, int limit, double *mu, double *sigma, double *rho)
{
	int n=0, i;
	double sumsquare=0.0, sum=0.0, top=0.0;
	double sigma2=0.0;

	for (i=0; i<limit; ++i){
		sumsquare += x[i]*x[i];
		sum += x[i];
		++n;
	}
	*mu = sum/(double)n;
	*sigma = sqrt((sumsquare - (double)n*(*mu)*(*mu))/(double)(n-1));

	for (i=1; i < n; ++i){
		top += ((double)x[i]- *mu)*((double)x[i-1]- *mu);
		sigma2 += ((double)x[i-1] - *mu)*((double)x[i-1] - *mu);

	}
	*rho = top/sigma2;
}

void
intarraystats(int *x, int limit, double *mu, double *sigma, double *rho)
{
	int n=0, i;
	double sumsquare=0.0, sum=0.0, top=0.0;
	double sigma2=0.0;

	for (i=0; i<limit; ++i){
		sumsquare += (double)x[i]*x[i];
		sum += (double)x[i];
		++n;
	}
	*mu = sum/(double)n;
	*sigma = sqrt((sumsquare - (double)n*(*mu)*(*mu))/(double)(n-1));

	for (i=1; i < n; ++i){
		top += ((double)x[i]- *mu)*((double)x[i-1]- *mu);
		sigma2 += ((double)x[i-1] - *mu)*((double)x[i-1] - *mu);

	}
	*rho = top/sigma2;
}

/* Like the above, but handles different (unsigned) integer sizes and much
 * larger arrays.
 */
void
uintxarraystats(void *x, int intsize, size_t limit, double *mu, double *sigma, double *rho)
{
	u_int8_t *xrun = (int8_t *)x;
	u_int64_t value, lastvalue;
	size_t n=0, i;
	double sumsquare=0.0, sum=0.0, top=0.0;
	double sigma2=0.0;

	xrun = (int8_t *)x;
	for (i=0; i<limit; ++i){
		switch (intsize) {
		case 1:
			value = *xrun;
			break;
		case 2:
			value = *(u_int16_t *)xrun;
			break;
		case 4:
			value = *(u_int32_t *)xrun;
			break;
		case 8:
			value = *(u_int64_t *)xrun;
			break;
		}
		sumsquare += (double)value*value;
		sum += (double)value;
		xrun += intsize;

		++n;
	}
	*mu = sum/(double)n;
	*sigma = sqrt((sumsquare - (double)n*(*mu)*(*mu))/(double)(n-1));

	xrun = (u_int8_t *)x;
	switch (intsize) {
	case 1:
		lastvalue = *xrun;
		break;
	case 2:
		lastvalue = *(u_int16_t *)xrun;
		break;
	case 4:
		lastvalue = *(u_int32_t *)xrun;
		break;
	case 8:
		lastvalue = *(u_int64_t *)xrun;
		break;
	}
	xrun += intsize;

	for (i=1; i < n; ++i){
		switch (intsize) {
		case 1:
			value = *xrun;
			break;
		case 2:
			value = *(u_int16_t *)xrun;
			break;
		case 4:
			value = *(u_int32_t *)xrun;
			break;
		case 8:
			value = *(u_int64_t *)xrun;
			break;
		}
		top += ((double)value- *mu)*((double)lastvalue- *mu);
		sigma2 += ((double)lastvalue - *mu)*((double)lastvalue - *mu);
		lastvalue = value;
	}
	*rho = top/sigma2;
}

/* Like the above, but ignores zero entries. I also leave out rho, as it's
 * not meaningful here.
 */
void
nonzerouintxarraystats(void *x, int intsize, size_t limit, double *mu, double *sigma, size_t *nonzero)
{
	u_int8_t *xrun = (int8_t *)x;
	u_int64_t value;
	size_t n=0, i, ournonzero=0;
	double sumsquare=0.0, sum=0.0, top=0.0;

	xrun = (int8_t *)x;
	for (i=0; i<limit; ++i){
		switch (intsize) {
		case 1:
			value = *xrun;
			break;
		case 2:
			value = *(u_int16_t *)xrun;
			break;
		case 4:
			value = *(u_int32_t *)xrun;
			break;
		case 8:
			value = *(u_int64_t *)xrun;
			break;
		}
		if (value != 0)
			++ournonzero;
		sumsquare += (double)value*value;
		sum += (double)value;
		xrun += intsize;

		++n;
	}
	*mu = sum/(double)ournonzero;
	*sigma = sqrt((sumsquare - (double)ournonzero*(*mu)*(*mu))/(double)(ournonzero-1));
	*nonzero = ournonzero;
}

/* Special version for extracting statistics from a Bloom filter */
void
bloomarraystats(void *x, int intsize, size_t limit,
	double *mu, double *sigma, u_int64_t *max, u_int64_t *min, double *chisquare)
{
	u_int8_t *xrun = (int8_t *)x;
	u_int64_t value;
	size_t n=0, i;
	double sumsquare=0.0, sum=0.0;

	*max = 0;
#ifndef INFINITY64
#define INFINITY64	0xffffffff
#endif
	*min = INFINITY64;
	xrun = (int8_t *)x;
	for (i=0; i<limit; ++i) {
		switch (intsize) {
		case 1:
			value = *xrun;
			break;
		case 2:
			value = *(u_int16_t *)xrun;
			break;
		case 4:
			value = *(u_int32_t *)xrun;
			break;
		case 8:
			value = *(u_int64_t *)xrun;
			break;
		}
		sumsquare += (double)value*value;
		sum += (double)value;
		xrun += intsize;
		if (value > *max) *max = value;
		if (value < *min) *min = value;

		++n;
	}
	*mu = sum/(double)n;
	*sigma = sqrt((sumsquare - (double)n*(*mu)*(*mu))/(double)(n-1));

	xrun = (int8_t *)x;
	*chisquare = 0.0;
	for (i=0; i<limit; ++i) {
		switch (intsize) {
		case 1:
			value = *xrun;
			break;
		case 2:
			value = *(u_int16_t *)xrun;
			break;
		case 4:
			value = *(u_int32_t *)xrun;
			break;
		case 8:
			value = *(u_int64_t *)xrun;
			break;
		}
		*chisquare += ((double)value - *mu)*((double)value - *mu)/(*mu);
		xrun += intsize;
	}
}

/* In this one, the array indices are the possible values, while
 * the entries in each index are the counts. We then mu and sigma
 * those. (Here, rho has no meaning, as the values aren't ordered.)
 */
void
intcountstats(int *x, int limit, double *mu, double *sigma)
{
	int n=0, i;
	double sumsquare=0.0, sum=0.0, top=0.0;
	double sigma2=0.0;

	for (i=0; i<limit; ++i){
		sumsquare += (double)x[i]*i*i;
		sum += (double)x[i]*i;
		n += x[i];
	}
	*mu = sum/(double)n;
	*sigma = sqrt((sumsquare - (double)n*(*mu)*(*mu))/(double)(n-1));
}

/* This is the same setup as above - that is, the array shows the
 * distribution. Here, we calculate the statistical entropy.
 */
void
intentropy(int *x, int limit, double *entropy)
{
	int n=0, i;
	double sum=0.0;

	/* Shall we be stupid about it? Yes, we shall. */

	for (i=0; i<limit; ++i) {
		sum += (double)x[i];
	}
	*entropy = 0;
	for (i=0; i<limit; ++i) {
		if (x[i])
			*entropy += ((double)x[i]/sum)*log2(sum/((double)x[i]));
	}
}

void
bytearraystats(unsigned char *x, int limit, double *mu, double *sigma, double *rho)
{
	int n=0, i;
	double sumsquare=0.0, sum=0.0, top=0.0;
	double sigma2=0.0;

	for (i=0; i<limit; ++i){
		sumsquare += (double)x[i]*x[i];
		sum += (double)x[i];
		++n;
	}
	*mu = sum/(double)n;
	*sigma = sqrt((sumsquare - (double)n*(*mu)*(*mu))/(double)(n-1));

	for (i=1; i < n; ++i){
		top += ((double)x[i]- *mu)*((double)x[i-1]- *mu);
		sigma2 += ((double)x[i-1] - *mu)*((double)x[i-1] - *mu);

	}
	*rho = top/sigma2;
}

/* diststats - get stat info from a distribution table:
 * 	range (x.00 - x.99)	count (# in that range)
 *
 * Obviously, only (approx) mu and sigma info is available - without
 * sequencing, rho is meaningless.
 */

void
filediststats(FILE *fp, double *mu, double *sigma)
{
	double range;
	int count;
	int n=0, i;
	double sumsquare=0.0, sum=0.0, top=0.0;
	double sigma2=0.0;

	for (i=0; !feof(fp); ++i){
		fscanf(fp, "%lf %d", &range, &count);
		if (feof(fp))
			break;
		sumsquare += count*range*range;
		sum += count*range;
		n += count;
	}
	*mu = sum/(double)n;
	*sigma = sqrt((sumsquare - (double)n*(*mu)*(*mu))/(double)(n-1));
}

void
arraydiststats(Dist *x, int limit, double *mu, double *sigma)
{
	int n=0, i;
	double sumsquare=0.0, sum=0.0, top=0.0;
	double sigma2=0.0;

	for (i=0; i<limit; ++i){
		sumsquare += (double)x[i].count*x[i].range*x[i].range;
		sum += (double)x[i].count*x[i].range;
		n += x[i].count;
	}
	*mu = sum/(double)n;
	*sigma = sqrt((sumsquare - (double)n*(*mu)*(*mu))/(double)(n-1));
}

/* threshdiststats - get stat info from a distribution table:
 * 	range (x.00 - x.99)	count (# in that range)
 *
 *  threshhold - ignore any entries with count < threshhold
 */

void
threshfilediststats(FILE *fp, int threshhold, double *mu, double *sigma)
{
	double range;
	int count;
	int n=0, i;
	double sumsquare=0.0, sum=0.0, top=0.0;
	double sigma2=0.0;

	for (i=0; !feof(fp); ++i){
		fscanf(fp, "%lf %d", &range, &count);
		if (feof(fp))
			break;
		if (count < threshhold) continue;
		sumsquare += count*range*range;
		sum += count*range;
		n += count;
	}
	*mu = sum/(double)n;
	*sigma = sqrt((sumsquare - (double)n*(*mu)*(*mu))/(double)(n-1));
}

