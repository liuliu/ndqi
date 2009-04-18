/********************************************************
 * The Distinctive Point Database API Header of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#ifndef _GUARD_NQDPDB_
#define _GUARD_NQDPDB_

#include <cv.h>

typedef struct {
	char* name;
	uint64_t rnum;
	CvMat** rdp;
	CvFeatureTree** rdpc;
	uint64_t basiz;
	uint32_t* ba32;
} NQDPDB;

NQDPDB* nqdpdbnew(void);

bool nqdpdbput(NQDPDB* dpdb, const void* kbuf, int ksiz, CvArr* img);

bool nqdpdblike(NQDPDB* dpdb, );

#endif
