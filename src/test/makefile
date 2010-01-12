LEX = flex -8
YACC = byacc -dv
OPENCV = /opt/opencv
CXX = g++
LINKFLAG = -pthread
INCFLAGS = -I"$(OPENCV)/include/opencv"
CXXFLAGS = -O2 --fast-math -msse2

all: mm_test

clean:
	rm *.o ../lib/*.o

mm_test: mm_test.o ../lib/nqmm.o
	$(CXX) -o $@ $^ $(LINKFLAG)

.c.o:
	$(CXX) $(INCFLAGS) $< -o $@ -c $(CXXFLAGS)

.cpp.o:
	$(CXX) $(INCFLAGS) $< -o $@ -c $(CXXFLAGS)

.cc.o:
	$(CXX) $(INCFLAGS) $< -o $@ -c $(CXXFLAGS)
