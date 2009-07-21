LEX = flex -8
YACC = byacc -dv
OPENCV = /opt/opencv
LINKFLAG=-ldl -lpthread -lrt -lapr-1 -laprutil-1 -lcxcore -lcv -lhighgui
CXXFLAGS = -I"/opt/apr/include" -I"/opt/apr-util/include" -I"$(OPENCV)/include/opencv"

all: processor

clean:
	rm *.o ../lib/*.o processor

processor: processor.o ../lib/uuid.o ../lib/frl_util_md5.o
	g++ -o $@ $^ $(LINKFLAG)

.c.o:
	g++ $(CXXFLAGS) $< -o $@ -g -c


