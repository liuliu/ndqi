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

typedef struct NQFDBIDX {
	CvMat* f;
	CvMat* p;
	CvFeatureTree* ft;
	uint32_t inum;
	char** kstr;
	NQFDBDATUM** data;
	struct NQFDBIDX* prev;
	struct NQFDBIDX* next;
} NQFDBIDX;

typedef struct NQFDBUNIDX {
	char* kstr;
	NQFDBDATUM* datum;
	struct NQFDBUNIDX* prev;
	struct NQFDBUNIDX* next;
} NQFDBUNIDX;

typedef struct {
	NQRDB* rdb;
	uint32_t inum;
	uint32_t unum;
	NQFDBIDX* idx;
	NQFDBUNIDX* unidx;
#if APR_HAS_THREADS
	apr_thread_rwlock_t* rwunidxlock;
	apr_thread_rwlock_t* rwidxlock;
#endif
} NQFDB;

NQFDB* nqfdbnew(void);
bool nqfdbput(NQFDB* fdb, char* kstr, CvMat* fm);
CvMat* nqfdbget(NQFDB* fdb, char* kstr);
int nqfdblike(NQFDB* fdb, CvMat* fm, char** kstr, int lmt, bool ordered = false, float* likeness = 0);
int nqfdbsearch(NQFDB* fdb, CvMat* fm, char** kstr, int lmt, bool ordered = false, float* likeness = 0);
bool nqfdbidx(NQFDB* fdb, int naive = 2, double rho = 0.75, double tau = 0.2);
bool nqfdbreidx(NQFDB* fdb, int naive = 2, double rho = 0.75, double tau = 0.2);
bool nqfdbout(NQFDB* fdb, char* kstr);
void nqfdbdel(NQFDB* fdb);

#endif
