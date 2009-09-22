OPENCV=/opt/opencv
OUTDIR=../../build
COMPFLAG=-O3 --fast-math -msse2
LINKFLAG=-ldl -lpthread -lrt -lapr-1 -laprutil-1 -lcxcore -lcv -lhighgui -ltokyocabinet -ltokyodystopia -lexif
LINKLIB=-L"$(OPENCV)/lib"
CXXFLAGS=-I"/opt/apr/include" -I"/opt/apr-util/include" -I"$(OPENCV)/include/opencv"

# all: $(OUTDIR)/rdb_test $(OUTDIR)/dpdb_test $(OUTDIR)/irdb_test $(OUTDIR)/idx_test $(OUTDIR)/lh_test $(OUTDIR)/gs_test $(OUTDIR)/cx_test $(OUTDIR)/qry_test $(OUTDIR)/mm_test $(OUTDIR)/file_dpdb_test $(OUTDIR)/file_lh_test $(OUTDIR)/tdb_test

$(OUTDIR)/tdb_test: tdb_test.o ../nqtdb.o
	g++ $(CXXFLAGS) $(LINKLIB) -o $@ $^ $(LINKFLAG)

$(OUTDIR)/rdb_test: rdb_test.o ../nqrdb.o ../lib/frl_slab_pool.o ../lib/frl_util_md5.o
	g++ $(CXXFLAGS) $(LINKLIB) -o $@ $^ $(LINKFLAG)

$(OUTDIR)/dpdb_test: dpdb_test.o ../nqbwdb.o ../nqdp.o ../nqrdb.o ../lib/mlapcluster.o ../lib/frl_slab_pool.o ../lib/frl_util_md5.o $(OPENCV)/src/ml/ml_inner_functions.o
	g++ $(CXXFLAGS) $(LINKLIB) -o $@ $^ $(LINKFLAG)

$(OUTDIR)/idx_test: idx_test.o ../nqbwdb.o ../nqdp.o ../nqrdb.o ../lib/mlapcluster.o ../lib/frl_slab_pool.o ../lib/frl_util_md5.o $(OPENCV)/src/ml/ml_inner_functions.o
	g++ $(CXXFLAGS) $(LINKLIB) -o $@ $^ $(LINKFLAG)

$(OUTDIR)/irdb_test: irdb_test.o ../nqbwdb.o ../nqir.o ../nqrdb.o ../lib/mlapcluster.o ../lib/frl_slab_pool.o ../lib/frl_util_md5.o $(OPENCV)/src/ml/ml_inner_functions.o
	g++ $(CXXFLAGS) $(LINKLIB) -o $@ $^ $(LINKFLAG)

$(OUTDIR)/eplr_test: eplr_test.o ../nqutil.o ../nqdp.o ../lib/mlapcluster.o ../nqrdb.o ../lib/frl_slab_pool.o ../lib/frl_util_md5.o $(OPENCV)/src/ml/ml_inner_functions.o
	g++ $(CXXFLAGS) $(LINKLIB) -o $@ $^ $(LINKFLAG)

$(OUTDIR)/lh_test: lh_test.o ../nqlh.o ../lib/cvlocalhist.o ../nqfdb.o ../nqrdb.o ../lib/frl_slab_pool.o ../lib/frl_util_md5.o $(OPENCV)/src/ml/ml_inner_functions.o
	g++ $(CXXFLAGS) $(LINKLIB) -o $@ $^ $(LINKFLAG)

$(OUTDIR)/gs_test: gs_test.o ../nqgs.o ../lib/cvgist.o ../nqfdb.o ../nqrdb.o ../lib/frl_slab_pool.o ../lib/frl_util_md5.o $(OPENCV)/src/ml/ml_inner_functions.o
	g++ $(CXXFLAGS) $(LINKLIB) -o $@ $^ $(LINKFLAG)

$(OUTDIR)/cx_test: cx_test.o ../nqlh.o ../lib/cvlocalhist.o ../nqfdb.o ../nqbwdb.o ../nqdp.o ../nqrdb.o ../lib/mlapcluster.o ../lib/frl_slab_pool.o ../lib/frl_util_md5.o $(OPENCV)/src/ml/ml_inner_functions.o
	g++ $(CXXFLAGS) $(LINKLIB) -o $@ $^ $(LINKFLAG)

$(OUTDIR)/qry_test: qry_test.o ../nqqry.o ../nqlh.o ../nqmeta.o ../lib/cvlocalhist.o ../nqfdb.o ../nqbwdb.o ../nqdp.o ../nqrdb.o ../lib/mlapcluster.o ../lib/frl_slab_pool.o ../lib/frl_util_md5.o $(OPENCV)/src/ml/ml_inner_functions.o
	g++ $(CXXFLAGS) $(LINKLIB) -o $@ $^ $(LINKFLAG)

$(OUTDIR)/mm_test: mm_test.o ../lib/frl_managed_mem.o
	g++ $(CXXFLAGS) $(LINKLIB) -o $@ $^ $(LINKFLAG)

$(OUTDIR)/file_dpdb_test: file_dpdb_test.o ../nqbwdb.o ../nqdp.o ../nqrdb.o ../lib/mlapcluster.o ../lib/frl_slab_pool.o ../lib/frl_util_md5.o $(OPENCV)/src/ml/ml_inner_functions.o
	g++ $(CXXFLAGS) $(LINKLIB) -o $@ $^ $(LINKFLAG)

$(OUTDIR)/file_lh_test: file_lh_test.o ../nqlh.o ../lib/cvlocalhist.o ../nqfdb.o ../nqrdb.o ../lib/frl_slab_pool.o ../lib/frl_util_md5.o $(OPENCV)/src/ml/ml_inner_functions.o
	g++ $(CXXFLAGS) $(LINKLIB) -o $@ $^ $(LINKFLAG)

clean:
	rm *.o ../lib/*.o ../*.o

.c.o:
	g++ $(CXXFLAGS) $< -o $@ -c $(COMPFLAG)

.cpp.o:
	g++ $(CXXFLAGS) $< -o $@ -c $(COMPFLAG)

.cc.o:
	g++ $(CXXFLAGS) $< -o $@ -c $(COMPFLAG)
