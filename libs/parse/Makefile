#CFLAGS = -O3 -pg
CFLAGS = -O3

PROGS= inetaddr inetaddr6
LIBS= libparse.a
TESTS= parseargs parsenargs parsequoteargs atofix fixtoa

all: $(PROGS) $(LIBS)

install: $(LIBS)
	cp $(LIBS) $(HOME)/lib64/

clean:
	rm -f $(PROGS) $(LIBS) $(TESTS) *.o

inetaddr: inetaddr.c $(LIBS)
	$(CC) -DTEST -o inetaddr inetaddr.c $(LIBS)

inetaddr6: inetaddr6.c $(LIBS)
	$(CC) -DTEST -o inetaddr6 inetaddr6.c $(LIBS)

parseargs: parseargs.c
	$(CC) -DTEST -o parseargs parseargs.c

parsenargs: parsenargs.c
	$(CC) -DTEST -o parsenargs parsenargs.c

parsequoteargs: parsequoteargs.c
	$(CC) -DTEST -o parsequoteargs parsequoteargs.c

libparse.a: atofix.o atoh.o fixtoa.o inetaddr.o parsealnum.o parseargs.o parsequoteargs.o parsexalnum.o inetaddr6.o parsenargs.o filearg.o
	ar vr $@ $?
