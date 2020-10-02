/* simpleshmfile.h - shared-memory-mapped-file system.
 *
 * shmfile is a set of routines which allow maintaining permanent,
 * shared data structures. They are permanent in the sense that
 * they are backed up in a memory-mapped file.  While resident, they
 * are stored in a shared memory region, so can be used by multiple
 * concurrent processes. This version simply allocates a single
 * fixed region (with optional static header), rather than the
 * more complicated one which includes a concurrent memory (sub)allocator.
 */
#ifndef _SIMPLESHMFILE_H
#define _SIMPLESHMFILE_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/shm.h>

/* Layout of a shared mapped region on disk or in memory:
 *
 *	ShmDisk header	(16 bytes)
 *
 *	static region	(shdStatic bytes)(optional, for user data headers)
 *
 *	allocated region (shdSize - (shdStatic+16) bytes)
 */
 
/* The ShmDisk structure is located (on disk and in memory) at the head of
 * the shared region
 */

typedef struct _shmDisk {
	struct _shmDisk	*shdBase;	/* base address of shared region */
	size_t		shdSize;	/* its size in bytes (inc header) */
	u_int32_t	shdMagic;	/* magic number identifier */
#define SHF_MAGIC	0xCABB1FFE
	size_t		shdStatic;	/* size of optional static area */
} ShmDisk;

/* The static area is located immediately after this header */

/* "User" version of the above, with a few additional useful pieces of
 * information
 */

typedef struct _shmFile {
	ShmDisk		shfDisk;	/* copy of header info */
	int		shfFD;		/* file descriptor for backing file */
	char *		shfName;	/* name of backing file */
	int		shfProt;	/* prot flags for mapping */
	int		shfFlags;	/* mode/mapping flags */
	caddr_t		shfStatic;	/* pointer to static area (or NULL) */
	caddr_t		shfAlloc;	/* pointer to allocatable region */
} ShmFile;


/* Routines */

/* File manipulation */
/* simpleshmfile_create makes a new region; simpleshmfile_open attaches to an existing one.
 * Parameters:
 *  filename is the name of a new or existing shared memory file
 *  mode is the usual open mode bits, interpreted as follows:
 *	O_RDONLY	map read-only (only valid for open)
 *	O_WRONLY, O_RDWR map read-write (for create or open)
 *	O_CREAT		for open, maps to create
 *	O_TRUNC		destroy and reinitialize memory region
 *   Other flags are simply passed to open() and have no special effect on
 *   the shared memory region.
 *  where is the desired start address, or NULL to allow mapping anywhere.
 *   It's generally advisable to leave this NULL, especially in opening an
 *   existing region.  If an address is supplied, it will attempt to allocate
 *   and remap to that address.  However, only internal-used addresses will
 *   be remapped. If there are addresses in the data structures place in the
 *   shared region, simpleshmfile_open will not touch them.  Altering the mapping
 *   address is only possible when the region is opened with write permission.
 *  static_size is the size of the optional static area left open at the
 *   start of the region.  It may be resized when reopened, with the usual
 *   caveats about remapping addresses.A 0 on reopen means keep current size.
 *  total_size is the total size of the region.  This may be safely increased
 *   when the region is reopened.Again, a 0 on reopen means keep current size.
 */
ShmFile * simpleshmfile_create(char *filename,	/* backing file */
		      int mode,		/* open mode */
		      caddr_t where,	/* desired start (0 for anywhere) */
		      size_t static_size, /* static region size */
		      size_t total_size); /* total region size */
ShmFile * simpleshmfile_open(char *filename,	/* backing file */
		    int mode,		/* open mode */
		    caddr_t where,	/* desired start (risky!) */
		    size_t static_size,	/* resizing is risky */
		    size_t total_size);	/* enlarging allowed */

/* This pulls the info (including static region) from a
 * previously-created shmfile.
 */
ShmFile * simpleshmfile_peek(char *filename);

/* sync contents to disk (usually a no-op) */
void simpleshmfile_sync(ShmFile * shf);
/* close file and unmap region */
void simpleshmfile_close(ShmFile * shf);
/* Free the info returned from simpleshmfile_peek */
void simpleshmfile_free(ShmFile * shf);

/* Memory allocation */
/* The first two are essentially internal routines called by
 * simpleshmfile_create and simpleshmfile_open as needed.  simpleshmfile_init corresponds
 * to O_TRUNC, while simpleshmfile_remap is used to adjust internal addresses
 * on resize.
 */
void simpleshmfile_init(ShmFile * shf, int trunc);	/* (re)initialize allocation region */
void simpleshmfile_remap(ShmFile * shf, caddr_t where); /* adjust to new base address */

#endif /* _SIMPLESHMFILE_H */
