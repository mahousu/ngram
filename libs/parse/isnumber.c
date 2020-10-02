#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

int
isnumber(char *string)
{
	while (*string) {
		if (!isdigit(*string)) return 0;
		++string;
	}
	return 1;
}


main(int argc, char **argv)
{
	int i;

	for (i=1; i < argc; ++i) {
		if (!isnumber(argv[i]))
			exit(-1);
	}
	exit(0);
}
