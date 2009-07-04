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

const short int NQBWDBIDX_MAGIC_VAL = 0x8239;

typedef struct {
	CvMat* bw;
	CvFeatureTree* bwft;
} NQBWDBDATUM;

const short int NQBWDBDATUM_MAGIC_VAL = 0x7643;

typedef struct NQBWDBUNIDX {
	char* kstr;
	NQBWDBDATUM* datum;
	struct NQBWDBUNIDX* prev;
	struct NQBWDBUNIDX* next;
} NQBWDBUNIDX;

const short int NQBWDBUNIDX_MAGIC_VAL = 0x1839;

typedef struct {
	NQRDB* rdb;
	bool shallow;
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

const short int NQBWDB_MAGIC_VAL = 0x9304;

NQBWDB* nqbwdbnew(void);
NQBWDB* nqbwdbjoin(NQBWDB* bwdb, char** kstr, int len);
NQBWDB* nqbwdbjoin(NQBWDB* bwdb, NQRDB* rdb);
CvMat* nqbweplr(CvMat* data, int e = 5, int emax = 50);
bool nqbwdbput(NQBWDB* bwdb, char* kstr, CvMat* bwm);
CvMat* nqbwdbget(NQBWDB* bwdb, char* kstr);
int nqbwdblike(NQBWDB* bwdb, CvMat* bwm, char** kstr, int lmt, int mode = NQBW_LIKE_BEST_MATCH_COUNT, double match = 0.6, bool ordered = false, float* likeness = 0);
int nqbwdbsearch(NQBWDB* bwdb, CvMat* bwm, char** kstr, int lmt, bool ordered = false, float* likeness = 0);
bool nqbwdbidx(NQBWDB* bwdb, int min = 1, double match = 0.6);
bool nqbwdbmgidx(NQBWDB* bwdb, int max, int min = 1, double match = 0.6);
bool nqbwdbreidx(NQBWDB* bwdb, int min = 1, double match = 0.6);
bool nqbwdbout(NQBWDB* bwdb, char* kstr);
bool nqbwdbsnap(NQBWDB* bwdb, char* filename);
bool nqbwdbsync(NQBWDB* bwdb, char* filename);
void nqbwdbdel(NQBWDB* bwdb);

#endif
