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
	NQFDB* fdb = (NQFDB*)frl_slab_palloc(db_pool);
	fdb->rdb = nqrdbnew();
	fdb->inum = fdb->unum = 0;
	fdb->idx = (NQFDBIDX*)frl_slab_palloc(idx_pool);
	fdb->idx->inum = 0;
	fdb->idx->ft = 0;
	fdb->idx->kstr = 0;
	fdb->idx->prev = fdb->idx->next = fdb->idx;
	fdb->unidx = (NQFDBUNIDX*)frl_slab_palloc(idx_pool);
	fdb->unidx->kstr = 0;
	fdb->unidx->datum = 0;
	fdb->unidx->prev = fdb->unidx->next = fdb->unidx;
#if APR_HAS_THREADS
	apr_thread_rwlock_create(&fdb->rwidxlock, mtx_pool);
	apr_thread_rwlock_create(&fdb->rwunidxlock, mtx_pool);
#endif
}

bool nqfdbput(NQFDB* fdb, char* kstr, CvMat* fm)
{
	NQFDBDATUM* dt = (NQFDBDATUM*)frl_slab_palloc(dt_pool);
	NQFDBUNIDX* unidx = (NQFDBUNIDX*)frl_slab_palloc(unidx_pool);
	dt->f = fm;
	if (nqrdbput(fdb->rdb, kstr, dt))
	{
		unidx->kstr = (char*)frl_slab_palloc(kstr_pool);
		memcpy(unidx->kstr, kstr, 16);
		unidx->datum = dt;
#if APR_HAS_THREADS
		apr_thread_rwlock_wrlock(fdb->rwunidxlock);
#endif
		unidx->prev = fdb->unidx->prev;
		unidx->next = fdb->unidx;
		fdb->unidx->prev->next = unidx;
		fdb->unidx->prev = unidx;
		fdb->unum++;
#if APR_HAS_THREADS
		apr_thread_rwlock_unlock(fdb->rwunidxlock);
#endif
		return true;
	}
	return false;
}

CvMat* nqfdbget(NQFDB* fdb, char* kstr)
{
	NQFDBDATUM* dt = (NQFDBDATUM*)nqrdbget(fdb->rdb, kstr);
	return dt->f;
}

static void nqffwm(char* kstr, void* vbuf, void* ud)
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
	NQFDBDATUM* dt = (NQFDBDATUM*)nqrdbget(fdb, kstr);
	if (dt != NULL)
	{
		if (dt->f != 0) cvReleaseMat(&dt->f);
		frl_slab_pfree(dt);
		return nqrdbout(fdb, kstr);
	}
	return false;
}

void nqfdbdel(NQFDB* fdb)
{
}
