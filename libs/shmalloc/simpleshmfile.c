/*
 * simpleshmfile.c - a relatively simple wrapper around mmap which
 * attempts to create a shared memory region, backed up by a file,
 * at a fixed, durable address. This is extracted from a more
 * complicated version which included alloc/free routines; here,
 * a single, fixed region is created.
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/shm.h>
#include <malloc.h>
#include <memory.h>
#include <sys/stat.h>

#include "simpleshmfile.h"

#ifndef SHMMAX
#define SHMMAX	1024000
#endif

/* simpleshmfile_create - create a new shmfile
 * 	filename - pathname to shmfile
 * 	mode - mode for file creation
 *	where - desired base address for region, 0 for any
 *	static_size - for "private" info, mostly address
 *	total_size - desired total size in bytes
 *
 * returns ShmFile * with info about region, NULL on error
 *
 * creat = open(O_WRONLY|O_CREAT|O_TRUNC)
 *
 *
 */
ShmFile *
simpleshmfile_create(char *filename, int mode, caddr_t where,
	size_t static_size, size_t total_size)
{
	
	/* create = create and truncate */
	return simpleshmfile_open(filename, (mode|O_CREAT|O_TRUNC), where,
		static_size, total_size);
}

/* simpleshmfile_open - open (possibly create) a shmfile
 * 	filename - pathname to shmfile
 * 	mode - mode for file open
 *	where - desired base address for region, 0 for any
 *	static_size - for "private" info, mostly address, 0 for default
 *	total_size - desired total size in bytes, 0 for existing
 *
 * returns ShmFile * with info about region, NULL on error
 *
 */



ShmFile *
simpleshmfile_open(char *filename, int mode, caddr_t where,
	size_t static_size, size_t total_size)
{
	caddr_t region;
	ShmFile * answer;
	static size_t pagesize;
	int fd, prot=0, flags=0, need_remap=0;
	size_t offset=0;
	struct stat stats;
	
	/* Not sure why I cache this, but ... */
	if (!pagesize) pagesize = getpagesize();

	/* Make sense of the parameters - it's impossible to have "write only"
	 * shared memory. Well, on most architectures, at any rate.
	 */
	if (!(mode&O_RDWR)) {
		if (mode&O_WRONLY) {
			mode &= ~O_WRONLY;
			mode |= O_RDWR;
			prot = (PROT_READ|PROT_WRITE);
		} else {
			prot = PROT_READ;
		}
	} else {
		prot = (PROT_READ|PROT_WRITE);
	}
	if (mode&O_EXCL)
		flags |= MAP_PRIVATE;	/* ?? */
	else
		flags |= MAP_SHARED;
	/* Total size must be integral number of pages */
	if (total_size&(pagesize-1)) {	/* round up */
		total_size += pagesize - (total_size&(pagesize-1));
	}
	
	if (static_size&0xf) {	/* round up */
		static_size += 16 - (static_size&0xf);
	}
	if (total_size &&
	    total_size < static_size+sizeof(ShmDisk)) {
		errno = EINVAL;
		return NULL;
	}

	/* Open the associated file, and pull info from it. */
	fd = open(filename, mode, 0666);
	if (fd < 0)
		return NULL;
	if (fstat(fd, &stats) < 0) {	/* Huh? */
		close(fd);
		return NULL;
	}
fprintf(stderr, "%s: %ld\n", filename, stats.st_size);


	/* Allocate user header in normal memory (umm...) */
	answer = (ShmFile *)malloc(sizeof(ShmFile));
	if (!answer) {				/* ugh */
		close(fd);
		errno = ENOMEM;
		return NULL;
	}

	/* If there's info in the file, use it (unless overridden) */
	if (stats.st_size) {
		/* Existing one; try to read in what's there */
		read(fd, (void *)&answer->shfDisk, sizeof(ShmDisk));
fprintf(stderr, "magic %lx vs. %lx\n", answer->shfDisk.shdMagic, SHF_MAGIC);
		/* If it's correct ... */
		if (answer->shfDisk.shdMagic == SHF_MAGIC) {
			/* Unless we're remapping, take existing */
			if (!where)
				where = (caddr_t)answer->shfDisk.shdBase;
			if (!total_size)
				total_size = answer->shfDisk.shdSize;
			else if (!(mode&O_TRUNC) &&
					total_size < answer->shfDisk.shdSize) {
				/* @@ not sure why this? */
				free(answer);
				close(fd);
				errno = EINVAL;
				return NULL;
			}
			if (!static_size)
				static_size = answer->shfDisk.shdStatic;
			else if (!(mode&O_TRUNC) &&
				static_size != answer->shfDisk.shdStatic) {
				/* Oh, good grief */
				free(answer);
				close(fd);
				errno = EINVAL;
				return NULL;
			}
		}
	} else {
		/* New one; Linux seems to demand the file be "filled out"
		 * before mmap() */
		int zero=0;

		lseek(fd, (off_t)(total_size-sizeof(int)), SEEK_SET);
		write(fd, (char *)&zero, sizeof(int));
		lseek(fd, (off_t) 0, SEEK_SET);
	}
	
	/* Now prepare for mapping */
	if (where)
		flags |= MAP_FIXED;	/* we usually want to specify */
	
	/* go for the map */
fprintf(stderr, "mmap(%lx, %ld, %x, %x)\n", where, total_size, prot, flags);
	region = mmap(where, total_size, prot, flags, fd, 0);
	if (!region) {
		int juggle = errno;

		close(fd);
		free(answer);
		errno = juggle;
		return NULL;
	}
	/* What if it wasn't where we asked? */
	if (where && region != where) {
		/* @@ */
	}

	/* Copy down convenience information */
	answer->shfFD = fd;
	answer->shfName = filename;
	answer->shfProt = prot;
	answer->shfFlags = flags;

	/* Record region info */
	answer->shfDisk.shdBase = (ShmDisk *) region;
	answer->shfDisk.shdMagic = SHF_MAGIC;
	answer->shfDisk.shdSize = total_size;
	answer->shfDisk.shdStatic = static_size;

	if ((prot&PROT_WRITE)) {
fprintf(stderr, "write to %lx\n", region);
		/* write (possibly revised) ShmDisk header into region */
		*(ShmDisk *)region = answer->shfDisk;

		/* Sync, to make sure? */
		if (msync(region, total_size, MS_SYNC) < 0) {
			int juggle = errno;

perror("msync");
			close(fd);
			free(answer);
			(void) munmap(region, total_size);
			errno = juggle;
			return NULL;
		}
	}
	simpleshmfile_init(answer, mode&O_TRUNC);
	
	return answer;
}

/* simpleshmfile_peek - peek into an on-disk shmfile
 * 	filename - pathname to shmfile
 *
 * returns ShmFile * with info about the file, including any static area
 * or NULL on error
 *
 */
ShmFile *
simpleshmfile_peek(char *filename)
{
	ShmFile * answer;
	int fd;
	struct stat stats;
	size_t static_size;

	fd = open(filename, O_RDONLY);
	if (fd < 0) return NULL;
	if (fstat(fd, &stats) < 0) {	/* Huh? */
		close(fd);
		return NULL;
	}
	if (stats.st_size < sizeof(ShmDisk)) {	/* Just junk ... */
		close(fd);
		return NULL;
	}
	/* Give it a try */
	answer = (ShmFile *)malloc(sizeof(ShmFile));
	if (!answer) {				/* ugh */
		close(fd);
		errno = ENOMEM;
		return NULL;
	}
	memset((void *)answer, 0, sizeof(ShmFile));
	/* Try to read in what's there */
	read(fd, (void *)&answer->shfDisk, sizeof(ShmDisk));
	/* See if it's real */
	if (answer->shfDisk.shdMagic != SHF_MAGIC) {	/* nope */
		close(fd);
		free((void *)answer);
		return NULL;
	}
	/* OK, it's real; now look for a static area */
	static_size = answer->shfDisk.shdStatic;
	if (static_size > 0) {
		answer->shfStatic = malloc(static_size);
		read(fd, answer->shfStatic, static_size);
	}
	close(fd);
	return answer;
}


/* (re)initialize allocation region */
void
simpleshmfile_init(ShmFile * shf, int trunc)
{
	int static_size;

	/* set up pointers and initialize static region */
	if (shf->shfDisk.shdStatic) {
		shf->shfStatic = (caddr_t) (shf->shfDisk.shdBase + 1);
		shf->shfAlloc = (shf->shfStatic+shf->shfDisk.shdStatic);
		if (trunc) {
			bzero(shf->shfStatic, shf->shfDisk.shdStatic);
		}
	} else {
		shf->shfStatic = NULL;
		shf->shfAlloc = (caddr_t)(shf->shfDisk.shdBase + 1);
	}
}


void
simpleshmfile_sync(ShmFile * shf)
{
	msync(shf->shfDisk.shdBase, shf->shfDisk.shdSize, MS_ASYNC);
	/*fsync(shf->shfFD);*/
}

/* Clean up and close an allocated shared memory region */
void
simpleshmfile_close(ShmFile * shf)
{
	msync(shf->shfDisk.shdBase, shf->shfDisk.shdSize, MS_ASYNC);
	munmap(shf->shfDisk.shdBase, shf->shfDisk.shdSize);
	close(shf->shfFD);
	free(shf);
}

/* Clean up after peeking at a shmfile */
void
simpleshmfile_free(ShmFile * shf)
{
	if (shf->shfStatic)
		free(shf->shfStatic);
	free(shf);
}
