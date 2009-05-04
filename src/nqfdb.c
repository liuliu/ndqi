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
