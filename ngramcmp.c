#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ngram.h"

/* ngramcmp.c - compare the ngrams in two files a and b. This is an
 * all-to-all comparison, which extracts some statistics about how the
 * ngrams in file b compare to those in file a. The main intent is to
 * see how faithful an imitation b is of a, where b has been generated
 * by a random walk over ngrams (generally of shorter length) from a.
 *
 * It works in a straightforward fashion: Create a counting Bloom filter
 * based on a, then try looking up all the ngrams from b in it.
 */

#ifdef SHMALLOC
char *shmfilename;
int shmmode = (O_CREAT);
#endif

int showstats=0;
int dumpdist=0;


void
reporttotals(NGram *ngram, void *filter)
{
	double mu, sigma;
	u_int64_t max, min;

	(*ngram->diststats)(&mu, &sigma, &max, &min, filter);
	printf("distribution mu %lf sigma %lf max %ld min %ld\n",
		mu, sigma, max, min);
}


/* Read dest data and find its frequency in the source. We do the
 * same overlapping reads as when making the filter.
 *
 * Again, we attempt to handle the case where we can only read in the
 * data a chunk at a time.
 */
void
comparengrams(FILE *fp, int nsteps, NGram *ngram, void *filter, int verbose)
{
	u_int8_t *buffer;
	int nread, retain, i, ret, j;
	int zeros=0, nzeros=0;
	long int total=0;
	int count;
	struct stat statbuf;
#define COUNTSIZE	1024
	static u_int32_t countbuffer[COUNTSIZE];
	u_int64_t scale;
	double mu, sigma;
	u_int64_t max, min;

	(*ngram->diststats)(&mu, &sigma, &max, &min, filter);
	scale = (max+COUNTSIZE-1)/COUNTSIZE;

	if (fstat(fileno(fp), &statbuf) < 0 || statbuf.st_size <= 0) {
		buffer = (u_int8_t *)malloc(nsteps*BUFSIZ);
		nread = nsteps*BUFSIZ;
	} else {
		buffer = (u_int8_t *)malloc(statbuf.st_size);
		nread = statbuf.st_size;
		if (!buffer) {
			buffer = (u_int8_t *)malloc(nsteps*BUFSIZ);
			nread = nsteps*BUFSIZ;
		}
	}

	if (!buffer) {
		perror("comparengrams malloc");
		return;
	}
	/* Prime the buffer */
	nread = fread(buffer, 1, nread, fp);
	if (nread <= 0) {
		perror("comparengrams read");
		free((void *)buffer);
		return;
	}
	do {
	    for (i=0; i+nsteps <= nread; ++i) {
		    count = (*ngram->findngram)(buffer+i, nsteps, filter);
		    if (count < 0) perror("findngram");
		    else {
			    total += count;
			    ++countbuffer[count/scale];
			    if (count == 0) {
				    if (verbose>1) {
					    printf("%d:", i);
					    for (j=0; j < nsteps; ++j) {
						printf(" %c(%d)",
						    buffer[i+j], buffer[i+j]);
					    }
					    printf("\n");

					    /*printf("%s\n", buffer);*/
				    }
				    ++zeros;
			    } else {
				    ++nzeros;
			    }
		    }
	    }
	    memmove(buffer, buffer + (nread-(nsteps-1)), nsteps-1);
	    nread = fread(buffer+(nsteps-1), 1, nread-(nsteps-1), fp);
	    nread += nsteps-1;
	} while (nread > (nsteps-1));
	free((void *)buffer);
	if (verbose) {
		printf("%d found, %d not found, total %ld\n",
				nzeros, zeros, total);
		for (i=0; i < COUNTSIZE; ++i)
			printf(" %d", countbuffer[i]);
		printf("\n");
	}
	printf("%8.6lf fraction found\n",
		(double)nzeros/((double)nzeros+(double)zeros));
	return;
}

FILE *dumpfile;
int dumplevel = 1;


void
main(int argc, char **argv)
{
	int ngramtype=0;
	int nsteps=2;
	int c;
	FILE *afp, *bfp;
	int i;
	extern NGram *ngram;
	int verbose=0;
	void *filter;

	while ((c = getopt(argc, argv, "N:n:vA:a:sd")) >= 0) {
		switch (c) {
		case 'N':
			if (!strncasecmp(optarg, "bloom", 5)) {
				ngramtype = NGRAM_BLOOM;
			} else if (!strncasecmp(optarg, "array", 5)) {
				ngramtype = NGRAM_ARRAY;
			} else {
				fprintf(stderr, "Unknown filter %s\n",
					optarg);
				exit(1);
			}
                        setngramlabel(ngramtype, NULL, nsteps, 0, 0, 0);
                        break;
		case 'n':
			nsteps = atoi(optarg);
                        setngramlabel(ngramtype, NULL, nsteps, 0, 0, 0);
			break;
		case 'v':
			++verbose;
			break;
		case 's':
			++showstats;
			break;
		case 'd':
			++dumpdist;
			break;
#ifdef SHMALLOC
		case 'a':
			shmfilename = optarg;
			ngram_peek(shmfilename);
			shmmode = (O_CREAT);
			break;
		case 'A':
			shmfilename = optarg;
			shmmode = (O_CREAT|O_TRUNC);
			break;
#endif
		}
	}
#ifdef SHMALLOC
	filter = (*ngram->newfilter)(nsteps, shmfilename, shmmode);
#else
	filter = (*ngram->newfilter)(nsteps);
#endif
	if (!filter) {
		perror("malloc");
		exit(1);
	}
	if (optind >= argc) {
		fprintf(stderr, "No input file!\n");
		exit(2);
	}
	afp = fopen(argv[optind], "r");
	if (!afp) {
		perror(argv[optind]);
		exit(3);
	}
	if (optind+1 < argc) {
		bfp = fopen(argv[optind+1], "r");
		if (!bfp) {
			perror(argv[optind+1]);
			exit(3);
		}
	} else {
		bfp = stdin;
	}
	ngramreadfile(afp, nsteps, ngram, filter);
	fclose(afp);
	comparengrams(bfp, nsteps, ngram, filter, verbose);
	if (dumpdist)
		reporttotals(ngram, filter);
	exit(0);
}
