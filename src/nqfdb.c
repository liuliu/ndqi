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
		frl_slab_pool_create(&db_pool, mtx_pool, 64, sizeof(NQFDB), FRL_LOCK_WITH);
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
	fdb->unidx = (NQFDBUNIDX*)frl_slab_palloc(unidx_pool);
	fdb->unidx->kstr = 0;
	fdb->unidx->datum = 0;
	fdb->unidx->prev = fdb->unidx->next = fdb->unidx;
#if APR_HAS_THREADS
	apr_thread_rwlock_create(&fdb->rwidxlock, mtx_pool);
	apr_thread_rwlock_create(&fdb->rwunidxlock, mtx_pool);
#endif
	return fdb;
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

typedef struct {
	char* kstr;
	float likeness;
} NQFPAIR;

typedef struct {
	uint32_t siz;
	CvMat* fm;
	NQFPAIR data[0];
} NQFUSERDATA;

static void nqfhpf(NQFPAIR* pr, uint32_t i, uint32_t siz)
{
	uint32_t l, r, largest = i;
	do {
		i = largest;
		r = (i+1)<<1;
		l = r-1;
		if (l < siz && pr[l].likeness < pr[i].likeness)
			largest = l;
		if (r < siz && pr[r].likeness < pr[largest].likeness)
			largest = r;
		if (largest != i)
		{
			NQFPAIR sw = pr[largest];
			pr[largest] = pr[i];
			pr[i] = sw;
		}
	} while (largest != i);
}

static void nqffwm(char* kstr, void* vbuf, void* ud)
{
	NQFUSERDATA* nqud = (NQFUSERDATA*)ud;
	NQFDBDATUM* datum = (NQFDBDATUM*)vbuf;
	double norm = cvNorm(nqud->fm, datum->f);
	float likeness = 1. / (1. + norm);
	if (likeness > nqud->data->likeness)
	{
		nqud->data->likeness = likeness;
		nqud->data->kstr = kstr;
		nqfhpf(nqud->data, 0, nqud->siz);
	}
}

int nqfdblike(NQFDB* fdb, CvMat* fm, char** kstr, int lmt, bool ordered, float* likeness)
{
	NQFUSERDATA* ud = (NQFUSERDATA*)malloc(sizeof(NQFUSERDATA) + lmt * sizeof(NQFPAIR));
	memset(ud, 0, sizeof(NQFUSERDATA) + lmt * sizeof(NQFPAIR));
	ud->siz = lmt;
	ud->fm = fm;
	ud->data->likeness = 0;
	nqrdbforeach(fdb->rdb, nqffwm, ud);

	int i;
	if (ordered)
	{
		for (i = lmt-1; i > 0; i--)
		{
			NQFPAIR sw = ud->data[i];
			ud->data[i] = ud->data[0];
			ud->data[0] = sw;
			nqfhpf(ud->data, 0, i);
		}
	}

	NQFPAIR* dptr = ud->data;
	int k = 0;
	if (likeness == 0)
	{
		for (i = 0; i < lmt; i++)
		{
			if (dptr->kstr != 0)
			{
				*kstr = dptr->kstr;
				kstr++;
				k++;
			}
			dptr++;
		}
	} else {
		for (i = 0; i < lmt; i++)
		{
			if (dptr->kstr != 0)
			{
				*kstr = dptr->kstr;
				*likeness = dptr->likeness;
				kstr++;
				likeness++;
				k++;
			}
			dptr++;
		}
	}

	free(ud);

	return k;
}

int nqfdbsearch(NQFDB* fdb, CvMat* fm, char** kstr, int lmt, bool ordered, float* likeness)
{
	NQFUSERDATA* ud = (NQFUSERDATA*)malloc(sizeof(NQFUSERDATA) + lmt * sizeof(NQFPAIR));
	memset(ud, 0, sizeof(NQFUSERDATA) + lmt * sizeof(NQFPAIR));
	ud->siz = lmt;
	ud->fm = fm;
	ud->data->likeness = 0;
	int slmt = lmt * 2;
	CvMat* idx = cvCreateMat(fm->rows, slmt, CV_32SC1);
	CvMat* dist = cvCreateMat(fm->rows, slmt, CV_64FC1);
#if APR_HAS_THREADS
	apr_thread_rwlock_rdlock(fdb->rwidxlock);
#endif
	NQFDBIDX* idx_x;
	CvMat* fp = fm->cols > 32 ? cvCreateMat(1, 32, CV_32FC1) : 0;
	for (idx_x = fdb->idx->next; idx_x != fdb->idx; idx_x = idx_x->next)
	{
		if (fp != 0)
		{
			cvMatMul(fm, idx_x->p, fp);
			cvFindFeatures(idx_x->ft, fp, idx, dist, slmt);
		} else
			cvFindFeatures(idx_x->ft, fm, idx, dist, slmt);
		double* dptr = dist->data.db;
		int i, *iptr = idx->data.i;
		for (i = 0; i < slmt; i++, iptr++, dptr++)
			if (*iptr >= 0)
			{
				char* kstr = idx_x->kstr[*iptr];
				if (nqrdbget(fdb->rdb, kstr) != 0)
				{
					double norm = *dptr;
					if (fp != 0)
						norm = cvNorm(fm, idx_x->data[*iptr]->f);
					float likeness = 1. / (1. + norm);
					if (likeness > ud->data->likeness)
					{
						ud->data->likeness = likeness;
						ud->data->kstr = kstr;
						nqfhpf(ud->data, 0, ud->siz);
					}
				}
			}
	}
#if APR_HAS_THREADS
	apr_thread_rwlock_unlock(fdb->rwidxlock);
#endif
	cvReleaseMat(&idx);
	cvReleaseMat(&dist);
#if APR_HAS_THREADS
	apr_thread_rwlock_rdlock(fdb->rwunidxlock);
#endif
	NQFDBUNIDX* unidx_x;
	for (unidx_x = fdb->unidx->next; unidx_x != fdb->unidx; unidx_x = unidx_x->next)
		if (nqrdbget(fdb->rdb, unidx_x->kstr) != 0)
		{
			double norm = cvNorm(fm, unidx_x->datum->f);
			float likeness = 1. / (1. + norm);
			if (likeness > ud->data->likeness)
			{
				ud->data->likeness = likeness;
				ud->data->kstr = unidx_x->kstr;
				nqfhpf(ud->data, 0, ud->siz);
			}
		}
#if APR_HAS_THREADS
	apr_thread_rwlock_unlock(fdb->rwunidxlock);
#endif
	int i;
	if (ordered)
	{
		for (i = lmt-1; i > 0; i--)
		{
			NQFPAIR sw = ud->data[i];
			ud->data[i] = ud->data[0];
			ud->data[0] = sw;
			nqfhpf(ud->data, 0, i);
		}
	}

	NQFPAIR* dptr = ud->data;
	int k = 0;
	if (likeness == 0)
	{
		for (i = 0; i < lmt; i++)
		{
			if (dptr->kstr != 0)
			{
				*kstr = dptr->kstr;
				kstr++;
				k++;
			}
			dptr++;
		}
	} else {
		for (i = 0; i < lmt; i++)
		{
			if (dptr->kstr != 0)
			{
				*kstr = dptr->kstr;
				*likeness = dptr->likeness;
				kstr++;
				likeness++;
				k++;
			}
			dptr++;
		}
	}

	free(ud);

	return k;
}

bool nqfdbidx(NQFDB* fdb, int naive, double rho, double tau)
{
	NQFDBIDX* idx = (NQFDBIDX*)frl_slab_palloc(idx_pool);
#if APR_HAS_THREADS
	apr_thread_rwlock_wrlock(fdb->rwunidxlock);
#endif
	int unum = fdb->unum;
	int cols = fdb->unidx->next->datum->f->cols;
	idx->p = 0;
	NQFDBDATUM** data;
	if (cols > 32)
	{
		idx->p = cvCreateMat(cols, 32, CV_32FC1);
		CvRNG rng_state = cvRNG((int)(&fdb->unidx->next));
		cvRandArr(&rng_state, idx->p, CV_RAND_NORMAL, cvRealScalar(-1.), cvRealScalar(1.));
		cols = 32;
		data = idx->data = (NQFDBDATUM**)malloc(unum * sizeof(NQFDBDATUM*));
	}
	idx->f = cvCreateMat(unum, cols, CV_32FC1);
	char** kstr = idx->kstr = (char**)malloc(unum * sizeof(char*));
	float* fptr = idx->f->data.fl;
	NQFDBUNIDX* unidx_x = fdb->unidx->next;
	if (idx->p == 0)
	{
		while (unidx_x != fdb->unidx)
		{
			NQFDBUNIDX* next = unidx_x->next;
			memcpy(fptr, unidx_x->datum->f->data.fl, sizeof(float) * cols);
			*kstr = unidx_x->kstr;
			fptr += cols;
			kstr++;
			frl_slab_pfree(unidx_x);
			unidx_x = next;
		}
	} else {
		while (unidx_x != fdb->unidx)
		{
			NQFDBUNIDX* next = unidx_x->next;
			CvMat tmp = cvMat(1, cols, CV_32FC1, fptr);
			cvMatMul(unidx_x->datum->f, idx->p, &tmp);
			*kstr = unidx_x->kstr;
			*data = unidx_x->datum;
			fptr += cols;
			kstr++;
			data++;
			frl_slab_pfree(unidx_x);
			unidx_x = next;
		}
	}
	fdb->unidx->prev = fdb->unidx->next = fdb->unidx;
	fdb->unum = 0;
#if APR_HAS_THREADS
	apr_thread_rwlock_unlock(fdb->rwunidxlock);
#endif
	idx->inum = unum;
	idx->ft = cvCreateSpillTree(idx->f, naive, rho, tau);
#if APR_HAS_THREADS
	apr_thread_rwlock_wrlock(fdb->rwidxlock);
#endif
	fdb->inum += idx->inum;
	idx->prev = fdb->idx->prev;
	idx->next = fdb->idx;
	fdb->idx->prev->next = idx;
	fdb->idx->prev = idx;
#if APR_HAS_THREADS
	apr_thread_rwlock_unlock(fdb->rwidxlock);
#endif
}

static void nqfrefr(char* kstr, void* vbuf, void* ud)
{
	NQFDB* fdb = (NQFDB*)ud;
	NQFDBUNIDX* unidx = (NQFDBUNIDX*)frl_slab_palloc(unidx_pool);
	unidx->kstr = (char*)frl_slab_palloc(kstr_pool);
	memcpy(unidx->kstr, kstr, 16);
	unidx->datum = (NQFDBDATUM*)vbuf;
	unidx->prev = fdb->unidx->prev;
	unidx->next = fdb->unidx;
	fdb->unidx->prev->next = unidx;
	fdb->unidx->prev = unidx;
}

static void nqfunidxclr(NQFDB* fdb)
{
	NQFDBUNIDX* unidx = fdb->unidx->next;
	while (unidx != fdb->unidx)
	{
		NQFDBUNIDX* next = unidx->next;
		frl_slab_pfree(unidx->kstr);
		frl_slab_pfree(unidx);
		unidx = next;
	}
	fdb->unum = 0;
	fdb->unidx->prev = fdb->unidx->next = fdb->unidx;
}

static void nqfidxclr(NQFDB* fdb)
{
	NQFDBIDX* idx = fdb->idx->next;
	while (idx != fdb->idx)
	{
		NQFDBIDX* next = idx->next;
		char** kstr = idx->kstr;
		int i;
		for (i = 0; i < idx->inum; i++, kstr++)
			frl_slab_pfree(*kstr);
		if (idx->p != 0)
			cvReleaseMat(&idx->p);
		cvReleaseMat(&idx->f);
		cvReleaseFeatureTree(idx->ft);
		free(idx->kstr);
		free(idx->data);
		idx = next;
	}
	fdb->inum = 0;
	fdb->idx->prev = fdb->idx->next = fdb->idx;
}

bool nqfdbreidx(NQFDB* fdb, int naive, double rho, double tau)
{
#if APR_HAS_THREADS
	apr_thread_rwlock_wrlock(fdb->rwunidxlock);
#endif
	nqfunidxclr(fdb);
	nqrdbforeach(fdb->rdb, nqfrefr, fdb);
	fdb->unum = fdb->rdb->rnum;
#if APR_HAS_THREADS
	apr_thread_rwlock_unlock(fdb->rwunidxlock);
	apr_thread_rwlock_wrlock(fdb->rwidxlock);
#endif
	nqfidxclr(fdb);
#if APR_HAS_THREADS
	apr_thread_rwlock_unlock(fdb->rwidxlock);
#endif
	return nqfdbidx(fdb, naive, rho, tau);
}

bool nqfdbout(NQFDB* fdb, char* kstr)
{
	NQFDBDATUM* dt = (NQFDBDATUM*)nqrdbget(fdb->rdb, kstr);
	if (dt != NULL)
	{
		if (dt->f != 0) cvReleaseMat(&dt->f);
		frl_slab_pfree(dt);
		return nqrdbout(fdb->rdb, kstr);
	}
	return false;
}

static void nqfnuk(char* kstr, void* vbuf, void* ud)
{
	NQFDBDATUM* dt = (NQFDBDATUM*)vbuf;
	if (dt->f != 0) cvReleaseMat(&dt->f);
	frl_slab_pfree(dt);
}

void nqfdbdel(NQFDB* fdb)
{
	nqrdbforeach(fdb->rdb, nqfnuk, 0);
	nqfunidxclr(fdb);
	nqfidxclr(fdb);
	frl_slab_pfree(fdb->unidx);
	frl_slab_pfree(fdb->idx);
#if APR_HAS_THREADS
	apr_thread_rwlock_destroy(fdb->rwidxlock);
	apr_thread_rwlock_destroy(fdb->rwunidxlock);
#endif
	nqrdbdel(fdb->rdb);
	frl_slab_pfree(fdb);
}
