int readsnort(char *file, int startymd, int endymd);
int readsnortlist(char *list, int startymd, int endymd);
int checksnort(struct timeval *thistime, u_int32_t size, int startymd, int endymd);
int ymdrange(FILE *fp, int *startymd, int *endymd);
void dumptimes(void);
void sortsnorttimes(void);
