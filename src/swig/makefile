OPENCV=/opt/opencv
LINKLIB=-L"$(OPENCV)/lib"
LINKFLAG=-ldl -lpthread -lrt -lapr-1 -laprutil-1 -lcv -ltokyocabinet -ltokyodystopia -lexif
CXXFLAGS=-I"/opt/apr/include" -I"/opt/apr-util/include" -I"$(OPENCV)/include/opencv" -I"/usr/include/python2.6" -fPIC

DIST_PACKAGE = /usr/lib/python2.6/dist-packages
OWNER=bin
GROUP=bin
BINDIR=/usr/local/bin
INSTALL=/usr/bin/install

all: swig _ndqi.so

clean:
	-rm *.o *.so

install:
	mkdir $(DIST_PACKAGE)/ndqi
	cp _ndqi.so $(DIST_PACKAGE)/ndqi/$*
	cp ndqi.py $(DIST_PACKAGE)/ndqi/core.py
	cp client.py $(DIST_PACKAGE)/ndqi/$*

swig:
	swig -c++ -python ndqi.i

_ndqi.so: ../nqmeta.o ndqi_wrap.o
	g++ -shared $(CXXFLAGS) $(LINKLIB) -o $@ $^ $(LINKFLAG) -O3

.c.o:
	g++ $(CXXFLAGS) $< -o $@ -g -c -O3

%.o: %.cxx
	g++ $(CXXFLAGS) $< -o $@ -g -c -O3

