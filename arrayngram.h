inline u_int32_t tongram(u_int8_t *p, int size);
u_int8_t *tonarray(u_int32_t ngram, int size);

#ifdef SHMALLOC
NgramFilterSet * allocngramarrayrange(Range ngram, char *shmfilename, int mode);
NgramCounter * allocngramarray(int ngram, char *shmfilename, int mode);
#else
NgramFilterSet * allocngramarrayrange(Range ngram);
NgramCounter * allocngramarray(int ngram);
#endif

int readngramsrange(void *item, size_t length, NgramFilterSet *vfilter);
int readngrams(void *item, size_t length, int ngram, void *filter);
int delngramarrayrange(void *item, size_t length, NgramFilterSet *filter);
int delngramarray(void *item, size_t length, int ngram, void *filter);

int reportngram(FILE *file, u_int16_t count, int size, long int i);
void reportngrams(FILE *file, u_int16_t *ngrams, int size);

#ifdef FILETEST
freadngrams(FILE *fp, int size, u_int16_t *ngrams);
#endif

int findarray(void *item, int length, void *filter);
int findngramarray(void *item, size_t length, int ngram, void *filter);
int findngramarrayrange(void *item, size_t length, NgramFilterSet *filter);
int finddistarray(void *item, size_t length, int ngram, double *mu,
 double *sigma, double *rho, NgramFilterSet *filter);
void distarray(int ngram, double *mu, double *sigma, u_int64_t *max, u_int64_t *min, void *filter);
void distarrayrange(int ngram, double *mu, double *sigma, u_int64_t *max, u_int64_t *min, NgramFilterSet *filter);
void dumparray(FILE *file, int ngram, void *filter);
void dumparrayrange(FILE *file, NgramFilterSet *filter);
void closearray(void *filter);
void closearrayrange(NgramFilterSet *filter);
