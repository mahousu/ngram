CFLAGS = -O3 -I.

all:	simpleshmfile.o

simpletest:	simpletest.c simpleshmfile.o
	$(CC) $(CFLAGS) -o simpletest simpletest.c simpleshmfile.o

clean:
	rm -f *.o simpletest
