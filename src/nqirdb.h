/********************************************************
 * The Interest Region Database API Header of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#ifndef _GUARD_NQIRDB_
#define _GUARD_NQIRDB_

#include <cv.h>
#include "nqrdb.h"

typedef struct {
	CvMat* ir;
	CvFeatureTree* irft;
} NQIRDBDATUM;

typedef struct {
	NQRDB* rdb;
	uint32_t kmax;
} NQIRDB;

#endif
