#ifndef _CIDR_H
#define _CIDR_H

typedef struct _cidr {
	struct in_addr subnet;
	struct in_addr mask;
} cidr_t;

void cidr_to_mask(char *cidr, struct in_addr *spart,
		struct in_addr *mpart, struct in_addr *hpart);
int incidr(struct in_addr host, cidr_t *cidr);
int incidrlist(struct in_addr host, cidr_t *cidrlist);
void strtocidr(char *str, cidr_t *cidr);
cidr_t * strtocidrlist(char *str);

#endif /* _CIDR_H */
