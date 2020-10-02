void arraymode(double *x, int length, double *mode, double *fuzz, double *fraction);
void arrayfuzzmode(double *x, int length, int infuzz, double *mode, double *fuzz, double *fraction);
void filemode(FILE *fp, double *mode, double *fuzz, double *fraction);
void arraystats(double *x, int limit, double *mu, double *sigma, double *rho);
void intarraystats(int *x, int limit, double *mu, double *sigma, double *rho);
void uintxarraystats(void *x, int intsize, size_t limit, double *mu, double *sigma, double *rho);
void bloomarraystats(void *x, int intsize, size_t limit, double *mu, double *sigma, u_int64_t *max, u_int64_t *min, double *chisquare);
void bytearraystats(unsigned char *x, int limit, double *mu, double *sigma, double *rho);
void intcountstats(int *x, int limit, double *mu, double *sigma);
void intentropy(int *x, int limit, double *entropy);
void filestats(FILE *fp, double *mu, double *sigma, double *rho);
void filestatsx(FILE *fp, double *mu, double *sigma, double *rho, double *dmin, double *dmax);

typedef struct _dist {
	int range;
	int count;
} Dist;

void arraydiststats(Dist *x, int limit, double *mu, double *sigma);
void filediststats(FILE *fp, double *mu, double *sigma);
void threshfilediststats(FILE *fp, int threshhold, double *mu, double *sigma);
