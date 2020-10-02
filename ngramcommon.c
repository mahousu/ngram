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
#include <sys/stat.h>
#include "ngram.h"
#include "snortcheck.h"
#include "snorthostcheck.h"
#include "taggedhostcheck.h"
#include "parse.h"
#include "ymd.h"


/* Default types and sizes */
Ngram *ngram=&bloom;            /* which counter we're using */
int ngramsize=4;

Ngram quotient, trie;		/* stub types until code is written */


NgramLabel ngramlabel = {
	NGRAM_BLOOM,
	{4,4},
	0
};


#ifdef SHMALLOC
ShmFile *shmfile;
#endif

#ifdef SHMALLOC
int
mmap_fault(void *base, size_t length)
{
	u_int8_t *prun = (u_int8_t *)base;
	size_t i;
	/* Make this a returned value to fool the optimizer */
	int preread=0;
	int jump = getpagesize();

	for (i=0; i < length; i += jump)
		preread += prun[i];
	return preread;
}

int prereadvalue;

void *
ngram_shmalloc(size_t length, char *filename, int mode)
{
	extern ShmFile *shmfile;

	if (!filename) {
		shmfile = NULL;
		return malloc(length);
	}
	mode |= O_RDWR;

	shmfile = simpleshmfile_open(filename, mode, 0, sizeof(NgramLabel),
			length+sizeof(NgramLabel));
	if (!shmfile)
		return NULL;
	/* Write the filter info into the static area */
	memcpy((void *)shmfile->shfStatic,
		(void *)&ngramlabel, sizeof(NgramLabel));

	prereadvalue = mmap_fault(shmfile->shfAlloc, length);
	/*fprintf(stderr, "preread %d\n", prereadvalue);*/
	return shmfile->shfAlloc;
}

void
ngram_shmfree(void *buffer)
{
	extern ShmFile *shmfile;

	if (!shmfile) {
		free(buffer);
		return;
	}
	if (shmfile->shfAlloc != buffer) {
		fprintf(stderr, "Where did %lx come from?\n", buffer);
		return;
	}
	simpleshmfile_close(shmfile);
	shmfile = NULL;
}

void
ngram_peek(char *filename)
{
	ShmFile *shf;
	NgramLabel disklabel;
	int n;

	shf = simpleshmfile_peek(filename);
	if (!shf) return;
	/* If we have an ngram label, grab it */
	if (shf->shfDisk.shdStatic >= sizeof(NgramLabel)) {
		memcpy(&disklabel, shf->shfStatic, sizeof(NgramLabel));
		/* Import any valid information from it */
		for (n = disklabel.ngramsize.min;
				n <= disklabel.ngramsize.max; ++n) {
			setngramlabel(disklabel.type, &disklabel.ngramsize, n,
				disklabel.length[n], disklabel.total[n],
				disklabel.distinct[n]);
		}
	}
	simpleshmfile_free(shf);
}

#endif

/* Set whatever info we have about the filter being used */
void
setngramlabel(int type, Range *ngramsize, int n, size_t length, size_t total, size_t distinct)
{

	switch (type) {
	case 0:
		/* no value */
		break;
	case NGRAM_ARRAY:
		ngramlabel.type = type;
		ngram = &array;
		break;
	case NGRAM_BLOOM:
		ngramlabel.type = type;
		ngram = &bloom;
		break;
	case NGRAM_QUOTIENT:
		ngramlabel.type = type;
		ngram = &quotient;
		break;
	case NGRAM_TRIE:
		ngramlabel.type = type;
		ngram = &trie;
		break;
	default:
		fprintf(stderr, "Uhh... what's %d?\n", type);
		break;
	}
	if (ngramsize && ngramsize->max > 0) {
		ngramlabel.ngramsize = *ngramsize;
		if (ngram)
			ngram->f->ngramsize = *ngramsize;
	}
	if (n) {
		if (length)
			ngramlabel.length[n] = length;
		if (total)
			ngramlabel.total[n] = total;
		if (distinct)
			ngramlabel.distinct[n] = distinct;
	}
}

/* Note: unlike the case with packet data, here what we are reading in is
 * continuous data. So with overlapping ngrams, we have to retain some
 * of the previous buffer to check its overlaps with the new buffer.
 *
 * For now, we're running this on systems with lots of memory, so we
 * try to read everything in at once if we can. Only if this fails
 * (or we can't determine how much is everything) do we resort to
 * doing things chunkwise. If memory is at a premium, this can be
 * adjusted.
 */
void
ngramreadfile(FILE *fp, Ngram *ngram)
{
	u_int8_t *buffer;
	int nread, retain, ret;
	struct stat statbuf;
	int nsteps;

	if (!ngram || !ngram->f) return;
	nsteps = ngram->f->ngramsize.max;
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
		perror("ngramreadfile malloc");
		return;
	}
	/* Prime the buffer */
	nread = fread(buffer, 1, nread, fp);
	if (nread <= 0) {
			perror("ngramreadfile read");
			free((void *)buffer);
			return;
	}
	do {
		ret = (*ngram->op->additemset)(buffer, nread, ngram->f);
		if (ret < 0) {
			perror("ngramreadfile additem");
		}
		memmove(buffer, buffer + (nread-(nsteps-1)), nsteps-1);
		nread = fread(buffer+(nsteps-1), 1, nread-(nsteps-1), fp);
		nread += nsteps-1;
	} while (nread > (nsteps-1));
	free((void *)buffer);
	return;
}
