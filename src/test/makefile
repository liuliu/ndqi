OPENCV=/opt/opencv
COMPFLAG=-O3
LINKFLAG=-ldl -lpthread -lrt -lapr-1 -laprutil-1 -lcxcore -lcv -lhighgui
LINKLIB=-L"$(OPENCV)/lib"
CXXFLAGS=-I"/opt/apr/include" -I"/opt/apr-util/include" -I"$(OPENCV)/include/opencv"

all: rdb_test dpdb_test

rdb_test: rdb_test.o ../nqrdb.o ../lib/frl_slab_pool.o ../lib/frl_util_md5.o
	g++ $(CXXFLAGS) $(LINKLIB) -o $@ $^ $(LINKFLAG)

dpdb_test: dpdb_test.o ../nqdpdb.o ../nqrdb.o ../lib/frl_slab_pool.o ../lib/frl_util_md5.o
	g++ $(CXXFLAGS) $(LINKLIB) -o $@ $^ $(LINKFLAG)

clean:
	rm *.o

.c.o:
	g++ $(CXXFLAGS) $< -o $@ -g -c $(COMPFLAG)
