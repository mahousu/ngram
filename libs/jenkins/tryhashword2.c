#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>

extern void hashword2(const uint32_t *k, size_t length, uint32_t *pc, uint32_t *pb);
extern void hashlittle2(const void *key, size_t length, uint32_t *pc, uint32_t *pb);

void
hashlines(FILE *fp)
{
	static char buffer[BUFSIZ];
	int len;
	uint32_t pc, pb;

	while (fgets(buffer, BUFSIZ, fp)) {
		len = strlen(buffer);
		pc = pb = 0;
		hashlittle2((void *)buffer, len, &pc, &pb);
		printf("%08x %08x (%d) %s", pc, pb, len, buffer);
	}
}

main(int argc, char **argv)
{
	int i;
	FILE *fp;

	if (argc > 1) {
		for (i=1; i < argc; ++i) {
			fp = fopen(argv[i], "r");
			if (!fp) {
				perror(argv[i]);
				continue;
			}
			hashlines(fp);
			fclose(fp);
		}
	} else {
		hashlines(stdin);
	}
}
