/********************************************************
 * "Bags of Words" Database API Header of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#ifndef _GUARD_NQBWDB_
#define _GUARD_NQBWDB_

#include <cv.h>
#include "nqrdb.h"

#define NQBW_LIKE_BEST_MATCH_COUNT (0x1)
#define NQBW_LIKE_BEST_MATCH_SCORE (0x2)

typedef struct {
	CvMat desc;
	uint32_t rnum;
	char** kstr;
	float idf;
} NQBWDBSTEM;

typedef struct NQBWDBIDX {
	NQBWDBSTEM* stem;
	CvMat* smmat;
	CvFeatureTree* smft;
	uint32_t rnum;
	char** kstr;
	uint32_t inum;
	double match;
	struct NQBWDBIDX* prev;
	struct NQBWDBIDX* next;
} NQBWDBIDX;

typedef struct {
	CvMat* bw;
	CvFeatureTree* bwft;
} NQBWDBDATUM;

typedef struct NQBWDBUNIDX {
	char* kstr;
	NQBWDBDATUM* datum;
	struct NQBWDBUNIDX* prev;
	struct NQBWDBUNIDX* next;
} NQBWDBUNIDX;

typedef struct {
	NQRDB* rdb;
	uint32_t emax;
	uint32_t wnum;
	uint32_t inum;
	uint32_t unum;
	NQBWDBIDX* idx;
	NQBWDBUNIDX* unidx;
#if APR_HAS_THREADS
	apr_thread_rwlock_t* rwidxlock;
	apr_thread_rwlock_t* rwunidxlock;
#endif
} NQBWDB;

NQBWDB* nqbwdbnew(void);
CvMat* nqbweplr(CvMat* data, int e = 5, int emax = 50);
bool nqbwdbput(NQBWDB* bwdb, char* kstr, CvMat* bwm);
CvMat* nqbwdbget(NQBWDB* bwdb, char* kstr);
int nqbwdblike(NQBWDB* bwdb, CvMat* bwm, char** kstr, int lmt, int mode = NQBW_LIKE_BEST_MATCH_COUNT, double match = 0.6, bool ordered = false, float* likeness = 0);
int nqbwdbsearch(NQBWDB* bwdb, CvMat* bwm, char** kstr, int lmt, bool ordered = false, float* likeness = 0);
bool nqbwdbidx(NQBWDB* bwdb, int min = 1, double match = 0.6);
bool nqbwdbreidx(NQBWDB* bwdb, int min = 1, double match = 0.6);
bool nqbwdbout(NQBWDB* bwdb, char* kstr);
void nqbwdbdel(NQBWDB* bwdb);

#endif
