#CFLAGS= -O3 -pg -I../parse/
CFLAGS= -O3 -I../parse/

all: cidr.o

test: cidrtomask cidrlist

cidrtomask:	cidr.c
	 $(CC) $(CFLAGS) -DTEST1 -o cidrtomask cidr.c ../parse/libparse.a

cidrlist:	cidr.c
	 $(CC) $(CFLAGS) -DTEST2 -o cidrlist cidr.c ../parse/libparse.a


clean:
	rm -f cidrtomask cidrlist *.o
