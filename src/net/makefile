LEX = flex -8
YACC = byacc -dv
OPENCV = /opt/opencv
CXX = g++
LINKFLAG = -ldl -lpthread -lrt -lapr-1 -laprutil-1 -lcv -lhighgui -ltokyocabinet -lexif -levent
INCFLAGS = -I"/opt/apr/include" -I"/opt/apr-util/include" -I"$(OPENCV)/include/opencv"
CXXFLAGS = -O3 --fast-math -msse2

all: nqserver

clean:
	rm *.o ../parser/*.o ../config/*.o ../lib/*.o ../*.o ../parser/gram.h ../parser/gram.c ../parser/gram.out ../parser/scan.c nqserver

nqserver: nqserver.o nqplan.o nqclient.o ../parser/scan.o ../parser/gram.o ../parser/keywords.o ../nqbwdb.o ../nqfdb.o ../nqtdb.o ../nqqry.o ../nqpreqry.o ../nqrdb.o ../lib/frl_slab_pool.o ../lib/frl_managed_mem.o ../lib/frl_util_md5.o ../lib/mlapcluster.o $(OPENCV)/src/ml/ml_inner_functions.o ../lib/cvgist.o ../lib/cvlocalhist.o ../config/databases.o ../nqdp.o ../nqlh.o ../nqgs.o ../nqmeta.o ../nqauto.o
	$(CXX) -o $@ $^ $(LINKFLAG)

../parser/scan.c: ../parser/scan.l ../parser/gram.h
	cd ../parser/ && \
	$(LEX) scan.l && \
	mv lex.yy.c scan.c

../parser/gram.c ../parser/gram.h: ../parser/gram.y
	cd ../parser/ && \
	$(YACC) gram.y && \
	mv y.tab.h gram.h && \
	mv y.tab.c gram.c && \
	mv y.output gram.out

.c.o:
	$(CXX) $(INCFLAGS) $< -o $@ -c $(CXXFLAGS)

.cpp.o:
	$(CXX) $(INCFLAGS) $< -o $@ -c $(CXXFLAGS)

.cc.o:
	$(CXX) $(INCFLAGS) $< -o $@ -c $(CXXFLAGS)
