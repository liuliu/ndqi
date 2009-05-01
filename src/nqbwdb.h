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
	CvMat* desc;
	uint32_t rnum;
	char** kstr;
} NQBWDBSTEM;

typedef struct NQBWDBIDX {
	NQBWDBSTEM* stem;
	CvFeatureTree* smft;
	uint32_t rnum;
	struct NQBWDBIDX* prev;
	struct NQBWDBIDX* next;
} NQBWDBIDX;

typedef struct {
	CvMat* bw;
	CvFeatureTree* bwft;
} NQBWDBDATUM;

typedef struct {
	NQRDB* rdb;
	uint32_t emax;
	NQBWDBIDX* idx;
} NQBWDB;

NQBWDB* nqbwdbnew(void);
CvMat* nqbweplr(CvMat* data, int e = 5, int emax = 50);
bool nqbwdbput(NQBWDB* bwdb, char* kstr, CvMat* bwm);
CvMat* nqbwdbget(NQBWDB* bwdb, char* kstr);
int nqbwdblike(NQBWDB* bwdb, CvMat* bwm, char** kstr, int lmt, int mode = NQBW_LIKE_BEST_MATCH_COUNT, double match = 0.6, bool ordered = 0, float* likeness = 0);
bool nqbwdbreidx(NQBWDB* bwdb);
bool nqbwdbout(NQBWDB* bwdb, char* kstr);
void nqbwdbdel(NQBWDB* bwdb);

#endif
