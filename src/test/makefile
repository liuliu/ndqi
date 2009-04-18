COMPFLAG=-O2
LINKFLAG=-ldl -lpthread -lrt
LINKLIB=-lapr-1 -laprutil-1
CXXFLAGS=-I"/opt/apr/include" -I"/opt/apr-util/include"

all: rdb_test

rdb_test: rdb_test.o ../nqrdb.o ../lib/frl_slab_pool.o ../lib/frl_util_md5.o
	g++ $(CXXFLAGS) $(LINKLIB) -o $@ $^ $(LINKFLAG)

clean:
	rm *.o

.c.o:
	g++ $(CXXFLAGS) $< -o $@ -g -c $(COMPFLAG)
