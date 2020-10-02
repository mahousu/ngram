#CFLAGS= -O3 -pg
CFLAGS= -O3

all:	lookup3.o tests

tests:	lookup3 tryhashword2

lookup3:	lookup3.c
	$(CC) $(CFLAGS) -DSELF_TEST -o lookup3 lookup3.c

lookup3.o:	lookup3.c
	$(CC) $(CFLAGS) -c lookup3.c

tryhashword2:	tryhashword2.c lookup3.o
	$(CC) $(CFLAGS) -o tryhashword2 tryhashword2.c lookup3.o

clean:
	-rm -f lookup3 tryhashword2 *.o
