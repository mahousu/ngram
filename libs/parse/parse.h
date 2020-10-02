#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <linux/types.h>

int atofix(char *string, int factor);
int atoh(char *string);
short atohs(char *string);
char *fixtoa(int num, int factor, int places);
__u32 my_inet_addr(char *string);
int my_inet_pton(char *string, void *addr);
int parsealnum(char *string, char *args[]);
int parseargs(char *string, char *args[], char *seps);
int parsenargs(char *string, char *args[], char *seps, int n);
int parsequoteargs(char *string, char *args[], char *seps, char *quotes);
int parsexalnum(char *string, int arraycount, ...);
char * filearg(char *arg, char *space, int length);
