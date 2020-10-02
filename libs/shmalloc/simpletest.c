#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <getopt.h>

#include "simpleshmfile.h"

#define CHUNK	256

caddr_t
quickhash(char *string)
{
	unsigned int answer=0;
	int i;

	for (i=0; i < strlen(string); ++i) {
		answer += string[i];
		answer *= 69069;
		answer <<= 16;
	}
	return (caddr_t)answer;
}


int
main (int argc, char **argv)
{
	int *palloc;
	ShmFile * shmuck;
	caddr_t where=0;
	size_t totalsize=0, staticsize=0;
	int i;
	int flags=(O_RDWR|O_CREAT);
	int doread=0;
	int c;
	int *base;

	/* Choose the starting address from the filename */
	/*where = quickhash(argv[1]);*/
	/* Allocate based on number of children */
	totalsize = getpagesize()*50;
	while ((c=getopt(argc, argv, "rpxa:t:"))>=0) {
	switch (c) {
	case 'r':	/* map read-only */
		flags=O_RDONLY;
		doread=1;
		break;
	case 'p':	/* private map */
	case 'x':	/* private map */
		flags |= O_EXCL;
		break;
		
	case 'a':	/* map at given address */
		where = strtol(optarg, NULL, 16);
		break;
	case 't':
		totalsize = strtol(optarg, NULL, 0);
		break;
	case 's':
		staticsize = strtol(optarg, NULL, 0);
		break;
	}
	}
	printf("Ask for: at %lx static %ld total of %ld\n",
		where, staticsize, totalsize);

	shmuck = simpleshmfile_open(argv[optind], flags, where, staticsize, totalsize);
	if (!shmuck) {
		perror(argv[optind]);
		exit(1);
	}
	printf("Received: at %lx static %ld total of %ld\n",
			shmuck->shfDisk.shdBase,
			shmuck->shfDisk.shdStatic, shmuck->shfDisk.shdSize);
	base = (int *)shmuck->shfAlloc;
	if (doread) {
		for (i=0; i < 10; ++i) {
			printf("%d: %d ", i, base[i]);
		}
		printf("\n");
	} else {
		for (i=0; i < 10; ++i) {
			base[i] = getpid() + i;
		}
	}
	simpleshmfile_close(shmuck);
	return 0;
}
