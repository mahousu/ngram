MYOBJS= jenkins/lookup3.o parse/parsenargs.o parse/filearg.o math/libstats.o cidr/cidr.o shmalloc/simpleshmfile.o fnv/hash_64_range.o fnv/hash_64.o

MYLIB= libmark.a

all:	$(MYLIB)

libmark.a:	$(MYOBJS)
	ar vr $@ $?

clean:
	cd jenkins; make clean
	cd parse; make clean
	cd math; make clean
	cd cidr; make clean
	cd shmalloc; make clean
	cd fnv; make clean
	-rm -f $(MYLIB)

jenkins/lookup3.o: jenkins/lookup3.c
	cd jenkins; make lookup3.o

parse/parsenargs.o:
	cd parse; make parsenargs.o

parse/filearg.o:
	cd parse; make filearg.o

math/libstats.o:
	cd math; make libstats.o

cidr/cidr.o:
	cd cidr; make cidr.o

shmalloc/simpleshmfile.o:
	cd shmalloc; make simpleshmfile.o

fnv/longlong.h: fnv/fnv.h
	cd fnv; make longlong.h

fnv/hash_64_range.o: fnv/hash_64_range.c fnv/longlong.h
	cd fnv; make hash_64_range.o

fnv/hash_64.o: fnv/hash_64.c fnv/longlong.h
	cd fnv; make hash_64.o
