#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "parse.h"
#include "cidr.h"
#define _TAGGEDHOSTCHECKINTERNAL
#include "taggedhostcheck.h"

/* This is a simplified version, which just reads a premade set
 * of miscreant hosts. It assumes all filtering of said list has
 * already been done.
 */

/* This should of course be made protocol-independent */

/* These are the bad hosts */
struct in_addr taggedhosts[HOSTSMAX];
int maxhosts;


/* find the first location with value >= host. If none, return maxhosts.
 * This is a sloppy implementation of binary search.
 */
int
findtaggedhost(struct in_addr host)
{
	int here, bottom, top;

	if (!maxhosts) return 0;

	bottom = 0;
	top = maxhosts;
	for (bottom=0, top=maxhosts; bottom != top; ) {
		here = (bottom+top)/2;
		if (taggedhosts[here].s_addr == host.s_addr) {
			return here;
		} else if (taggedhosts[here].s_addr < host.s_addr) {
			if (bottom != here) {
				bottom = here;
			} else {
				bottom = here+1;
			}
		} else /* taggedhosts[here].s_addr > host.s_addr */ {
			if (top != here) {
				top = here;
			} else {
				top = here-1;
			}
		}
	}
	return bottom;
}

/* add a host address into the array. Returns 1 if newly added, 0 if
 * already present, and -1 if error.
 */
int
addtaggedhost(char *line)
{
        int where;
	struct in_addr host;

	if (inet_aton(line, &host) <=0)
		return -1;

        where = findtaggedhost(host);
        if (where == maxhosts) {
                if (maxhosts >= HOSTSMAX) return -1;
                taggedhosts[where] = host;
        } else if (taggedhosts[where].s_addr == host.s_addr) {
                return 0;
        } else {
                /* Shove array down */
                if (maxhosts >= HOSTSMAX) return -1;
                memmove(taggedhosts+where+1, taggedhosts+where,
                        (maxhosts-where)*sizeof(struct in_addr));
                taggedhosts[where] = host;
        }
        ++maxhosts;
        return 1;
}

/* Read in a (space or semicolon separated) list of hosts.
 */
int
readtaggedhosts(char *list)
{
	char *args[HOSTSMAX];
	int i, n;
	int total=0;

	n = parsenargs(list, args, ",;", HOSTSMAX);
	for (i=0; i < n; ++i) {
		total += addtaggedhost(args[i]);
	}
	return total;
}



/* Not actually using this one right now - going with
 * generic list argument business.
 */
int
readtaggedhostsfile(char *file)
{
	int total=0;
	char line[BUFSIZ];
	FILE *fp;

	fp = fopen(file, "r");
	if (!fp) {
		perror(file);
		return 0;
	}

	/* Read through the file line by line. */
	while (fgets(line, BUFSIZ, fp)) {
		total += addtaggedhost(line);
	}
	fclose(fp);
	return total;
}

int
checktaggedhosts(u_int32_t source, u_int32_t dest)
{
	struct in_addr src, dst;
	int where;

	src.s_addr = source;
	dst.s_addr = dest;
	where = findtaggedhost(src);
        if (where < maxhosts) {
		if (taggedhosts[where].s_addr == src.s_addr)
			return 1;
	}
	where = findtaggedhost(dst);
        if (where < maxhosts) {
		if (taggedhosts[where].s_addr == dst.s_addr)
			return 1;
	}
	return 0;
}


#ifdef TEST
void
dumptaggedhosts(void)
{
	int i;

	for (i=0; i < maxhosts; ++i) {
		printf("%s\n", inet_ntoa(taggedhosts[i]));
	}
}

main(int argc, char **argv)
{
	int total;
	char trystring[BUFSIZ];
	struct in_addr tryhost;

	total = listarg(argv[1], addtaggedhost);

	printf("Read %d, maxhosts %d\n", total, maxhosts);
	dumptaggedhosts();
	while (1) {
		printf("Give me a host: ");
		gets(trystring);
		(void) inet_aton(trystring, &tryhost);

		printf("Check returns %d\n",
			checktaggedhosts(tryhost.s_addr, tryhost.s_addr));
		fflush(stdout);
	}
}
#endif

