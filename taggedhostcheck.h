#ifndef _TAGGEDHOSTCHECK_H
#define _TAGGEDHOSTCHECK_H
int readtaggedhosts(char *list);
int readtaggedhostsfile(char *file);
int addtaggedhost(char *line);
int checktaggedhosts(u_int32_t source, u_int32_t dest);
extern int taggedhostflag;

#ifdef _TAGGEDHOSTCHECKINTERNAL
/* Just be simple-minded */

#define HOSTSMAX	100000

int findtaggedhost(struct in_addr host);
#endif

#endif
