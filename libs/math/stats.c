#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include "libstats.h"

void
stats(FILE *fp)
{
	double mu=0.0, sigma=0.0, rho=0.0;

	filestats(fp, &mu, &sigma, &rho);
	printf("mu =    %12.6f\t", mu);
	printf("sigma = %12.6f\t", sigma);
	printf("rho =   %12.6f\n", rho);
}


int
main(int argc, char **argv)
{
	FILE *fp;

	if (argc > 2) {
		while (--argc > 0) {
			fp = fopen(*++argv, "r");
			if (!fp) {
				perror(*argv);
				continue;
			}
			printf("%s:\t", *argv);
			stats(fp);
			fclose(fp);
		}
	} else if (argc > 1) {
		fp = fopen(argv[1], "r");
		if (!fp) {
			perror(argv[1]);
			exit(1);
		}
		stats(fp);
	} else {
		stats(stdin);
	}
	return 0;
}
