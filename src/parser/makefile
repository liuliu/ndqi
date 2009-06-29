LEX = flex -8
YACC = byacc -dv
OPENCV = /opt/opencv
LINKFLAG=-ldl -lpthread -lrt -lapr-1 -laprutil-1 -lcxcore -lcv -lhighgui -ltokyocabinet -ltokyodystopia -lexif
CXXFLAGS = -I"/opt/apr/include" -I"/opt/apr-util/include" -I"$(OPENCV)/include/opencv"

all: test

clean:
	rm *.o ../lib/*.o ../*.o gram.h gram.c gram.out scan.c test

test: test.o scan.o gram.o keywords.o ../nqpreqry.o ../nqrdb.o ../lib/frl_slab_pool.o ../lib/frl_managed_mem.o ../config/databases.o
	g++ -o $@ $^ $(LINKFLAG)

scan.c: scan.l gram.h
	$(LEX) scan.l
	mv lex.yy.c scan.c

gram.c gram.h: gram.y
	$(YACC) gram.y
	mv y.tab.h gram.h
	mv y.tab.c gram.c
	mv y.output gram.out

.c.o:
	g++ $(CXXFLAGS) $< -o $@ -g -c
