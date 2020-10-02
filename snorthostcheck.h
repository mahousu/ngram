#ifndef _SNORTHOSTCHECK_H
#define _SNORTHOSTCHECK_H
int readsnorthostlist(char *list, int startymd, int endymd);
int checksnorthosts(u_int32_t source, u_int32_t dest);
int addsnortokhosts(char *list);

#ifdef _SNORTHOSTCHECKINTERNAL
/* Just be simple-minded */

#define HOSTSMAX	100000

int findsnorthost(struct in_addr host);
int addsnorthost(struct in_addr host);
int addmiscreanthost(struct in_addr source, struct in_addr dest, cidr_t *safe);
int parsehosts(char *line, struct in_addr *hosta, struct in_addr *hostb);
int readsnorthosts(char *file, int startymd, int endymd);
#endif

#endif
