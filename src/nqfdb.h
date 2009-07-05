/********************************************************
 * Fixed-Length Vector Database API Header of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#ifndef _GUARD_NQFDB_
#define _GUARD_NQFDB_

#include <cv.h>
#include "nqrdb.h"

typedef struct {
	CvMat* f;
} NQFDBDATUM;

const short int NQFDBDATUM_MAGIC_VAL = 0x2487;

typedef struct NQFDBIDX {
	CvMat* f;
	CvMat* p;
	CvFeatureTree* ft;
	uint32_t inum;
	int naive;
	double rho;
	double tau;
	char** kstr;
	NQFDBDATUM** data;
	struct NQFDBIDX* prev;
	struct NQFDBIDX* next;
} NQFDBIDX;

const short int NQFDBIDX_MAGIC_VAL = 0x9837;

typedef struct NQFDBUNIDX {
	char* kstr;
	NQFDBDATUM* datum;
	struct NQFDBUNIDX* prev;
	struct NQFDBUNIDX* next;
} NQFDBUNIDX;

const short int NQFDBUNIDX_MAGIC_VAL = 0x8393;

typedef struct {
	NQRDB* rdb;
	bool shallow;
	uint32_t inum;
	uint32_t unum;
	NQFDBIDX* idx;
	NQFDBUNIDX* unidx;
#if APR_HAS_THREADS
	apr_thread_rwlock_t* rwunidxlock;
	apr_thread_rwlock_t* rwidxlock;
#endif
} NQFDB;

const short int NQFDB_MAGIC_VAL = 0x2F28;

NQFDB* nqfdbnew(void);
NQFDB* nqfdbjoin(NQFDB* fdb, char** kstr, int len);
NQFDB* nqfdbjoin(NQFDB* fdb, NQRDB* rdb);
bool nqfdbput(NQFDB* fdb, char* kstr, CvMat* fm);
CvMat* nqfdbget(NQFDB* fdb, char* kstr);
int nqfdblike(NQFDB* fdb, CvMat* fm, char** kstr, int lmt, bool ordered = false, float* likeness = 0);
int nqfdbsearch(NQFDB* fdb, CvMat* fm, char** kstr, int lmt, bool ordered = false, float* likeness = 0);
bool nqfdbidx(NQFDB* fdb, int naive = 2, double rho = 0.75, double tau = 0.2);
bool nqfdbreidx(NQFDB* fdb, int naive = 2, double rho = 0.75, double tau = 0.2);
bool nqfdbout(NQFDB* fdb, char* kstr);
bool nqfdbsnap(NQFDB* fdb, char* filename);
bool nqfdbsync(NQFDB* fdb, char* filename);
void nqfdbdel(NQFDB* fdb);

#endif
