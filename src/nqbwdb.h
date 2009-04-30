/********************************************************
 * "Bags of Words" Database API Header of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#ifndef _GUARD_NQBWDB_
#define _GUARD_NQBWDB_

#include <cv.h>
#include "nqrdb.h"

typedef struct {
	CvMat* bw;
	CvFeatureTree* bwft;
} NQBWDBDATUM;

typedef struct {
	NQRDB* rdb;
	uint32_t emax;
	float match;
} NQBWDB;

NQBWDB* nqbwdbnew(void);
CvMat* nqbweplr(CvMat* data, int e = 5, int emax = 50);
bool nqbwdbput(NQBWDB* bwdb, char* kstr, CvMat* bwm);
CvMat* nqbwdbget(NQBWDB* bwdb, char* kstr);
int nqbwdblike(NQBWDB* bwdb, CvMat* bwm, char** kstr, int lmt, bool ordered, float* likeness = 0);
bool nqbwdbout(NQBWDB* bwdb, char* kstr);
void nqbwdbdel(NQBWDB* bwdb);

#endif
