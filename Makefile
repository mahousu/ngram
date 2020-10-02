INCS= -I./libs/math/ -I./libs/parse/ -I./libs/cidr/ -I./libs/fnv/ -I./libs/shmalloc
#CFLAGS= -O3 -pg $(INCS)
#CFLAGS= -O3 $(INCS) -DSHMALLOC
#CFLAGS= -g $(INCS) -DSHMALLOC
CFLAGS= -O3 $(INCS)
#CFLAGS= -g $(INCS)
MYLIBS= ./libs/libmark.a
LIBS= -lpcap -lpthread $(MYLIBS) -lm
#OFILES= ngram.o arrayngram.o bloom.o entropy.o range.o readapplication.o readinternet.o readlink.o readtransport.o snortcheck.o
OFILES= arrayngram.o entropy.o ngramcommon.o range.o readapplication.o readinternet.o readlink.o readtransport.o snortcheck.o snorthostcheck.o readtree.o ymd.o taggedhostcheck.o
SMALLOFILES= ngramsmall.o smallbloom.o $(OFILES)
BIGOFILES= ngram.o bloom.o  $(OFILES)
ALLOFILES= ngram.o ngramsmall.o smallbloom.o bloom.o $(OFILES)

EXES= ngram ngramsmall #datepcap #ngramwalk ngramcmp
TESTS= bloomtest snortcheck range ngramtest snorthostcheck arraytest entropytest datepcaptest

all:	$(MYLIBS) $(EXES)

install: all
	cp -p $(EXES) ~carson/bin/x86_64/

test:	$(TESTS)

ngram:	$(BIGOFILES)
	$(CC) $(CFLAGS) -o ngram $(BIGOFILES) $(LIBS)

smallbloom.o: bloom.c
	$(CC) $(CFLAGS) -DSMALLBLOOM -c bloom.c -o smallbloom.o

ngramsmall.o: ngram.c
	$(CC) $(CFLAGS) -DSMALLBLOOM -c ngram.c -o ngramsmall.o

ngramsmall:	$(SMALLOFILES)
	$(CC) $(CFLAGS) -o ngramsmall $(SMALLOFILES) $(LIBS)

libs/libmark.a:
	cd libs; make libmark.a

clean:
	-rm -f $(ALLOFILES) $(EXES) $(TESTS)
	cd libs; make clean

snortcheck:	snortcheck.c readtree.o ymd.o
	$(CC) $(CFLAGS) -DTEST -o snortcheck snortcheck.c readtree.o ymd.o $(LIBS)

snorthostcheck:	snorthostcheck.c readtree.o ymd.o
	$(CC) $(CFLAGS) -DTEST -o snorthostcheck snorthostcheck.c readtree.o  ymd.o $(LIBS)

datepcap:	datepcap.c ymd.o
	$(CC) $(CFLAGS) -o datepcap datepcap.c ymd.o $(LIBS)

bloomtest:	bloom.c
	$(CC) $(CFLAGS) -DTEST -o bloomtest bloom.c $(LIBS)

arraytest:	arrayngram.c
	$(CC) $(CFLAGS) -DTEST -o arraytest arrayngram.c $(LIBS)

range:	range.c
	$(CC) $(CFLAGS) -DTEST -o range range.c $(LIBS)

ngramtest:	ngram.c $(OFILES)
	$(CC) $(CFLAGS) -DTEST -o ngramtest ngram.c $(OFILES) $(LIBS)

entropytest:	entropy.c
	$(CC) $(CFLAGS) -DTEST -o entropytest entropy.c $(LIBS)

datepcaptest:	datepcap.c ymd.o
	$(CC) $(CFLAGS) -DTEST -o datepcaptest datepcap.c ymd.o $(LIBS)

ngramwalk:	ngramwalk.c bloom.o arrayngram.o ngramcommon.o $(LIBS)
	$(CC) $(CFLAGS) -DTEST -o ngramwalk ngramwalk.c bloom.o arrayngram.o ngramcommon.o $(LIBS)

ngramcmp:	ngramcmp.c bloom.o arrayngram.o ngramcommon.o $(LIBS)
	$(CC) $(CFLAGS) -DTEST -o ngramcmp ngramcmp.c bloom.o arrayngram.o ngramcommon.o $(LIBS)
