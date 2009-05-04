/********************************************************
 * Fixed-Length Vector Database API of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#include "nqfdb.h"

static apr_pool_t* mtx_pool = 0;
static frl_slab_pool_t* db_pool = 0;
static frl_slab_pool_t* dt_pool = 0;
static frl_slab_pool_t* idx_pool = 0;
static frl_slab_pool_t* unidx_pool = 0;
static frl_slab_pool_t* kstr_pool = 0;

NQFDB* nqfdbnew(void)
{
	if (mtx_pool == 0)
		apr_pool_create(&mtx_pool, NULL);
	if (db_pool == 0)
		flr_slab_pool_create(&db_pool, mtx_pool, 64, sizeof(NQFDB), FRL_LOCK_WITH);
	if (dt_pool == 0)
		frl_slab_pool_create(&dt_pool, mtx_pool, 1024, sizeof(NQFDBDATUM), FRL_LOCK_WITH);
	if (idx_pool == 0)
		frl_slab_pool_create(&idx_pool, mtx_pool, 128, sizeof(NQFDBIDX), FRL_LOCK_WITH);
	if (unidx_pool == 0)
		frl_slab_pool_create(&unidx_pool, mtx_pool, 1024, sizeof(NQFDBUNIDX), FRL_LOCK_WITH);
	if (kstr_pool == 0)
		frl_slab_pool_create(&kstr_pool, mtx_pool, 1024, 16, FRL_LOCK_WITH);
}

bool nqfdbput(NQFDB* fdb, char* kstr, CvMat* fm)
{
}

CvMat* nqfdbget(NQFDB* fdb, char* kstr)
{
}

int nqfdblike(NQFDB* fdb, CvMat* fm, char** kstr, int lmt, bool ordered = false, float* likeness = 0)
{
}

int nqfdbsearch(NQFDB* fdb, CvMat* fm, char** kstr, int lmt, bool ordered = false, float* likeness = 0)
{
}

bool nqfdbidx(NQFDB* fdb)
{
}

bool nqfdbreidx(NQFDB* fdb)
{
}

bool nqfdbout(NQFDB* fdb, char* kstr)
{
}

void nqfdbdel(NQFDB* fdb)
{
}
