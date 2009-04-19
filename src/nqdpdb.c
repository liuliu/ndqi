/********************************************************
 * The Distinctive Point Database API of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#include "nqdpdb.h"

static apr_pool_t* mtx_pool = 0;
static frl_slab_pool_t* dt_pool = 0;

NQDPDB* nqdpdbnew(void)
{
	if (mtx_pool == 0)
		apr_pool_create(&mtx_pool, NULL);
	if (dt_pool == 0)
		frl_slab_pool_create(&dt_pool, 1024, sizeof(NQDPDBDATUM), FRL_LOCK_WITH);
	return nqrdbnew();
}

CvMat* nqdpnew(CvArr* img)
{
}

bool nqdpdbput(NQDPDB* dpdb, char* kstr, CvMat* dpm)
{
	NQDPDBDATUM* dt = (NQDPDBDATUM*)frl_slab_palloc(dt_pool);
	dt->dp = dpm;
	nqrdbput(dpdb, kstr, dt);
}

CvMat* nqdpdbget(NQDPDB* dpdb, char* kstr)
{
	NQDPDBDATUM* dt = (NQDPDBDATUM*)nqrdbget(dpdb, kstr);
	return dt->dp;
}

bool nqdpdblike(NQDPDB* dpdb, CvMat* dpm, char** kstr, int lmt, float** likeness = 0)
{
}

bool nqdpdbout(NQDPDB* dpdb, char* kstr)
{
	return nqrdbout(dpdb, kstr);
}

void nqdpdel(CvMat* dp)
{
	cvReleaseMat(&dp);
}

void nqdpdbdel(NQDPDB* dpdb)
{
	nqrdbdel(dpdb);
}
