#CFLAGS = -O3 -pg
CFLAGS = -O3
PROGS = stats
OURLIBS = libstats.a
LIBS = $(OURLIBS) -lm

all: $(OURLIBS) $(PROGS)

clean:
	rm -f $(PROGS) *.o $(OURLIBS)

install: all
	cp $(PROGS) $(HOME)/bin/x86_64/
	cp $(OURLIBS) $(HOME)/lib64/

libstats.a:	libstats.o
	ar vr $@ $?

stats: stats.o
	$(CC) -o stats stats.o $(LIBS)
