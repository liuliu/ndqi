/********************************************************
 * The Distinctive Point Database API Header of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#ifndef _GUARD_NQDPDB_
#define _GUARD_NQDPDB_

#include <cv.h>
#include "nqrdb.h"

typedef struct {
	CvMat* dp;
	CvFeatureTree* dpft;
} NQDPDBDATUM;

typedef struct {
	NQRDB* rdb;
	uint32_t emax;
	float match;
	uint32_t kmax;
} NQDPDB;

NQDPDB* nqdpdbnew(void);
CvMat* nqdpnew(CvArr* image, CvSURFParams params);
bool nqdpdbput(NQDPDB* dpdb, char* kstr, CvMat* dpm);
CvMat* nqdpdbget(NQDPDB* dpdb, char* kstr);
int nqdpdblike(NQDPDB* dpdb, CvMat* dpm, char** kstr, int lmt, bool ordered, float* likeness = 0);
bool nqdpdbout(NQDPDB* dpdb, char* kstr);
void nqdpdbdel(NQDPDB* dpdb);

#endif
