/********************************************************
 * The Distinctive Point Database API Header of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#ifndef _GUARD_NQDPDB_
#define _GUARD_NQDPDB_

#include <cv.h>

typedef struct {
	CvMat* dp;
	CvFeatureTree* dpft;
} NQDPDBDATUM;

typedef NQRDB NQDPDB;

NQDPDB* nqdpdbnew(void);
CvMat* nqdpnew(CvArr* img);
bool nqdpdbput(NQDPDB* dpdb, char* kstr, CvMat* dpm);
CvMat* nqdpdbget(NQDPDB* dpdb, char* kstr);
bool nqdpdblike(NQDPDB* dpdb, CvMat* dpm, char** kstr, int lmt, float** likeness = 0);
bool nqdpdbout(NQDPDB* dpdb, char* kstr);
void nqdpdel(CvMat* dp);
void nqdpdbdel(NQDPDB* dpdb);

#endif
