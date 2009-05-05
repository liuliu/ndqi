/********************************************************
 * "Bags of Words" Database API of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#include "nqbwdb.h"
#include "lib/mlapcluster.h"

static apr_pool_t* mtx_pool = 0;
static frl_slab_pool_t* db_pool = 0;
static frl_slab_pool_t* dt_pool = 0;
static frl_slab_pool_t* idx_pool = 0;
static frl_slab_pool_t* unidx_pool = 0;
static frl_slab_pool_t* kstr_pool = 0;

NQBWDB* nqbwdbnew(void)
{
	if (mtx_pool == 0)
		apr_pool_create(&mtx_pool, NULL);
	if (db_pool == 0)
		frl_slab_pool_create(&db_pool, mtx_pool, 64, sizeof(NQBWDB), FRL_LOCK_WITH);
	if (dt_pool == 0)
		frl_slab_pool_create(&dt_pool, mtx_pool, 1024, sizeof(NQBWDBDATUM), FRL_LOCK_WITH);
	if (idx_pool == 0)
		frl_slab_pool_create(&idx_pool, mtx_pool, 128, sizeof(NQBWDBIDX), FRL_LOCK_WITH);
	if (unidx_pool == 0)
		frl_slab_pool_create(&unidx_pool, mtx_pool, 1024, sizeof(NQBWDBUNIDX), FRL_LOCK_WITH);
	if (kstr_pool == 0)
		frl_slab_pool_create(&kstr_pool, mtx_pool, 1024, 16, FRL_LOCK_WITH);
	NQBWDB* bwdb = (NQBWDB*)frl_slab_palloc(db_pool);
	bwdb->rdb = nqrdbnew();
	bwdb->emax = 20;
	bwdb->wnum = bwdb->inum = bwdb->unum = 0;
	bwdb->idx = (NQBWDBIDX*)frl_slab_palloc(idx_pool);
	bwdb->idx->rnum = 0;
	bwdb->idx->stem = 0;
	bwdb->idx->smmat = 0;
	bwdb->idx->smft = 0;
	bwdb->idx->kstr = 0;
	bwdb->idx->prev = bwdb->idx->next = bwdb->idx;
	bwdb->unidx = (NQBWDBUNIDX*)frl_slab_palloc(unidx_pool);
	bwdb->unidx->kstr = 0;
	bwdb->unidx->datum = 0;
	bwdb->unidx->prev = bwdb->unidx->next = bwdb->unidx;
#if APR_HAS_THREADS
	apr_thread_rwlock_create(&bwdb->rwidxlock, mtx_pool);
	apr_thread_mutex_create(&bwdb->unidxmutex, APR_THREAD_MUTEX_DEFAULT, mtx_pool);
#endif
	return bwdb;
}

CvMat* nqbweplr(CvMat* data, int e, int emax)
{
	CvAPCluster apcluster = CvAPCluster(CvAPCParams(2000, 200, 0.5));
	CvMat* idx = cvCreateMat(data->rows, e, CV_32SC1);
	CvMat* dist = cvCreateMat(data->rows, e, CV_64FC1);
	CvFeatureTree* ft = cvCreateKDTree(data);
	cvFindFeatures(ft, data, idx, dist, e, emax);
	int sizes[] = {data->rows, data->rows};
	CvSparseMat* sim = cvCreateSparseMat(2, sizes, CV_64FC1);
	int i, j, t = 0;
	double tpref = 0;
	int* idxptr = idx->data.i;
	double* distptr = dist->data.db;
	for (i = 0; i < data->rows; i++)
		for (j = 0; j < e; j++)
		{
			if (*idxptr != i && *idxptr >= 0)
			{
				cvSetReal2D(sim, i, *idxptr, -*distptr);
				tpref -= *distptr;
				t++;
			}
			idxptr++;
			distptr++;
		}
	double pref = tpref / t;
	for (i = 0; i < data->rows; i++)
		cvSetReal2D(sim, i, i, pref);
	CvMat* rsp = cvCreateMat(1, data->rows, CV_32SC1);
	apcluster.train(sim, rsp);
	cvZero(idx);
	idxptr = idx->data.i;
	int* rspptr = rsp->data.i;
	for (i = 0; i < data->rows; i++, rspptr++)
		idxptr[*rspptr] = 1;
	int r = 0;
	for (i = 0; i < data->rows; i++, idxptr++)
		if (*idxptr)
			r++;
	if (r <= 0)
	{
		cvReleaseMat(&rsp);
		cvReleaseFeatureTree(ft);
		cvReleaseMat(&dist);
		cvReleaseMat(&idx);
		cvReleaseSparseMat(&sim);
		return NULL;
	}
	CvMat* desc = cvCreateMat(r, data->cols, CV_32FC1);
	float* daptr = data->data.fl;
	float* dptr = desc->data.fl;
	idxptr = idx->data.i;
	for (i = 0; i < data->rows; i++, idxptr++, daptr += data->cols)
		if (*idxptr)
		{
			memcpy(dptr, daptr, sizeof(float)*data->cols);
			dptr += data->cols;
		}
	cvReleaseMat(&rsp);
	cvReleaseFeatureTree(ft);
	cvReleaseMat(&dist);
	cvReleaseMat(&idx);
	cvReleaseSparseMat(&sim);
	return desc;
}

bool nqbwdbput(NQBWDB* bwdb, char* kstr, CvMat* bwm)
{
	NQBWDBDATUM* dt = (NQBWDBDATUM*)frl_slab_palloc(dt_pool);
	NQBWDBUNIDX* unidx = (NQBWDBUNIDX*)frl_slab_palloc(unidx_pool);
	dt->bw = bwm;
	dt->bwft = 0;
	if (nqrdbput(bwdb->rdb, kstr, dt))
	{
		/* queue new object to unindex list */
		unidx->kstr = (char*)frl_slab_palloc(kstr_pool);
		memcpy(unidx->kstr, kstr, 16);
		unidx->datum = dt;
#if APR_HAS_THREADS
		apr_thread_mutex_lock(bwdb->unidxmutex);
#endif
		unidx->prev = bwdb->unidx->prev;
		unidx->next = bwdb->unidx;
		bwdb->unidx->prev->next = unidx;
		bwdb->unidx->prev = unidx;
		bwdb->unum++;
#if APR_HAS_THREADS
		apr_thread_mutex_unlock(bwdb->unidxmutex);
#endif
		return true;
	} else
		return false;
}

CvMat* nqbwdbget(NQBWDB* bwdb, char* kstr)
{
	NQBWDBDATUM* dt = (NQBWDBDATUM*)nqrdbget(bwdb->rdb, kstr);
	return dt->bw;
}

typedef struct {
	char* kstr;
	float likeness;
} NQBWPAIR;

typedef struct {
	uint32_t siz;
	uint32_t emax;
	double match;
	CvMat* bwm;
	CvMat* idx;
	CvMat* dist;
	NQBWPAIR data[0];
} NQBWUSERDATA;

static void nqbwhpf(NQBWPAIR* pr, uint32_t i, uint32_t siz)
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
			NQBWPAIR sw = pr[largest];
			pr[largest] = pr[i];
			pr[i] = sw;
		}
	} while (largest != i);
}

static void nqbwfwmc(char* kstr, void* vbuf, void* ud)
{
	NQBWUSERDATA* nqud = (NQBWUSERDATA*)ud;
	NQBWDBDATUM* datum = (NQBWDBDATUM*)vbuf;
	if (datum->bwft == 0)
		datum->bwft = cvCreateKDTree(datum->bw);
	cvFindFeatures(datum->bwft, nqud->bwm, nqud->idx, nqud->dist, 2, nqud->emax);
	int* iptr = nqud->idx->data.i;
	double* dptr = nqud->dist->data.db;
	int co = 0;
	int i;
	for (i = 0; i < nqud->dist->rows; i++, dptr += 2, iptr += 2)
		if ((iptr[0] >= 0 && iptr[1] >= 0)&&
			((dptr[1] < dptr[0] && dptr[1] < dptr[0] * nqud->match)||
			 (dptr[0] < dptr[1] && dptr[0] < dptr[1] * nqud->match)))
			co++;
	float likeness = (float)co / (float)(nqud->bwm->rows);
	if (likeness > nqud->data->likeness)
	{
		nqud->data->likeness = likeness;
		nqud->data->kstr = kstr;
		nqbwhpf(nqud->data, 0, nqud->siz);
	}
}

static void nqbwfwms(char* kstr, void* vbuf, void* ud)
{
	NQBWUSERDATA* nqud = (NQBWUSERDATA*)ud;
	NQBWDBDATUM* datum = (NQBWDBDATUM*)vbuf;
	if (datum->bwft == 0)
		datum->bwft = cvCreateKDTree(datum->bw);
	cvFindFeatures(datum->bwft, nqud->bwm, nqud->idx, nqud->dist, 1, nqud->emax);
	int* iptr = nqud->idx->data.i;
	double* dptr = nqud->dist->data.db;
	double co = 0;
	int i;
	for (i = 0; i < nqud->dist->rows; i++, dptr++, iptr++)
		if (iptr[0] >= 0 && dptr[0] < nqud->match)
			co += 1. / (1e-4 + dptr[0]);
	float likeness = (float)co / (float)(nqud->bwm->rows);
	if (likeness > nqud->data->likeness)
	{
		nqud->data->likeness = likeness;
		nqud->data->kstr = kstr;
		nqbwhpf(nqud->data, 0, nqud->siz);
	}
}

int nqbwdblike(NQBWDB* bwdb, CvMat* bwm, char** kstr, int lmt, int mode, double match, bool ordered, float* likeness)
{
	NQBWUSERDATA* ud = (NQBWUSERDATA*)malloc(sizeof(NQBWUSERDATA) + lmt * sizeof(NQBWPAIR));
	memset(ud, 0, sizeof(NQBWUSERDATA) + lmt * sizeof(NQBWPAIR));
	ud->emax = bwdb->emax;
	ud->match = match;
	ud->siz = lmt;
	ud->bwm = bwm;
	ud->data->likeness = 0;

	switch (mode)
	{
		case NQBW_LIKE_BEST_MATCH_COUNT:
			ud->idx = cvCreateMat(bwm->rows, 2, CV_32SC1);
			ud->dist = cvCreateMat(bwm->rows, 2, CV_64FC1);
			nqrdbforeach(bwdb->rdb, nqbwfwmc, ud);
			break;
		case NQBW_LIKE_BEST_MATCH_SCORE:
			ud->idx = cvCreateMat(bwm->rows, 1, CV_32SC1);
			ud->dist = cvCreateMat(bwm->rows, 1, CV_64FC1);
			nqrdbforeach(bwdb->rdb, nqbwfwms, ud);
			break;
	}
	int i;
	if (ordered)
	{
		for (i = lmt-1; i > 0; i--)
		{
			NQBWPAIR sw = ud->data[i];
			ud->data[i] = ud->data[0];
			ud->data[0] = sw;
			nqbwhpf(ud->data, 0, i);
		}
	}

	NQBWPAIR* dptr = ud->data;
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

	cvReleaseMat(&ud->idx);
	cvReleaseMat(&ud->dist);
	free(ud);

	return k;
}

static void nqbwsort(char* kstr, void* vbuf, void* ud)
{
	NQBWUSERDATA* nqud = (NQBWUSERDATA*)ud;
	float likeness;
	memcpy(&likeness, &vbuf, sizeof(float));
	if (likeness > nqud->data->likeness)
	{
		nqud->data->likeness = likeness;
		nqud->data->kstr = kstr;
		nqbwhpf(nqud->data, 0, nqud->siz);
	}
}

int nqbwdbsearch(NQBWDB* bwdb, CvMat* bwm, char** kstr, int lmt, bool ordered, float* likeness)
{
#if APR_HAS_THREADS
	apr_thread_rwlock_rdlock(bwdb->rwidxlock);
#endif
	CvMat* idx = cvCreateMat(bwm->rows, 1, CV_32SC1);
	CvMat* dist = cvCreateMat(bwm->rows, 1, CV_64FC1);
	double* odist = (double*)malloc(bwm->rows * sizeof(double));
	int i;
	for (i = 0; i < bwm->rows; i++)
		odist[i] = 1e30;
	NQBWDBSTEM** selected = (NQBWDBSTEM**)malloc(bwm->rows * sizeof(NQBWDBSTEM*));
	memset(selected, 0, bwm->rows * sizeof(NQBWDBSTEM*));
	NQBWDBIDX* idx_x;
	NQRDB* tdb = nqrdbnew();
	for (idx_x = bwdb->idx->next; idx_x != bwdb->idx; idx_x = idx_x->next)
	{
		cvFindFeatures(idx_x->smft, bwm, idx, dist, 1, bwdb->emax);
		double* dptr = dist->data.db;
		double* odptr = odist;
		NQBWDBSTEM** sptr = selected;
		int i, j, *iptr = idx->data.i;
		for (i = 0; i < bwm->rows; i++, iptr++, dptr++, odptr++, sptr++)
			if (*dptr < *odptr)
			{
				NQBWDBSTEM* stem;
				char** kstr;
				if (*sptr != 0)
				{
					stem = *sptr;
					kstr = stem->kstr;
					for (j = 0; j < stem->rnum; j++, kstr++)
					{
						void* void_cast = nqrdbget(tdb, *kstr);
						float float_cast;
						memcpy(&float_cast, &void_cast, sizeof(float));
						float_cast -= stem->idf;
						memcpy(&void_cast, &float_cast, sizeof(float));
						nqrdbput(tdb, *kstr, void_cast);
					}
				}
				stem = idx_x->stem + *iptr;
				kstr = stem->kstr;
				for (j = 0; j < stem->rnum; j++, kstr++)
				{
					void* void_cast = nqrdbget(tdb, *kstr);
					float float_cast;
					memcpy(&float_cast, &void_cast, sizeof(float));
					float_cast += stem->idf;
					memcpy(&void_cast, &float_cast, sizeof(float));
					nqrdbput(tdb, *kstr, void_cast);
				}
				*odptr = *dptr;
				*sptr = stem;
			}
	}
	free(odist);
	free(selected);
	cvReleaseMat(&idx);
	cvReleaseMat(&dist);
	NQBWUSERDATA* ud = (NQBWUSERDATA*)malloc(sizeof(NQBWUSERDATA)+lmt*sizeof(NQBWPAIR));
	memset(ud, 0, sizeof(NQBWUSERDATA)+lmt*sizeof(NQBWPAIR));
	ud->siz = lmt;
	ud->data->likeness = 0;
	nqrdbforeach(tdb, nqbwsort, ud);
	if (ordered)
	{
		for (i = lmt-1; i > 0; i--)
		{
			NQBWPAIR sw = ud->data[i];
			ud->data[i] = ud->data[0];
			ud->data[0] = sw;
			nqbwhpf(ud->data, 0, i);
		}
	}

	NQBWPAIR* dptr = ud->data;
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
	nqrdbdel(tdb);
#if APR_HAS_THREADS
	apr_thread_rwlock_unlock(bwdb->rwidxlock);
#endif
	return k;
}

bool nqbwdbidx(NQBWDB* bwdb, int min, double match)
{
#if APR_HAS_THREADS
	apr_thread_mutex_lock(bwdb->unidxmutex);
#endif
	int unum = bwdb->unum;
	int i, t = 0, tt = 0, kmax = 0, cols = 0;
	NQBWDBUNIDX* unidx_array = (NQBWDBUNIDX*)malloc(unum * sizeof(NQBWDBUNIDX));
	NQBWDBUNIDX* unidx_array_end = unidx_array + unum;
	NQBWDBUNIDX* unidx_y = unidx_array;
	NQBWDBUNIDX* unidx_x = bwdb->unidx->next;
	while (unidx_x != bwdb->unidx)
	{
		NQBWDBUNIDX* next = unidx_x->next;
		if (unidx_x->datum->bw->rows > kmax)
			kmax = unidx_x->datum->bw->rows;
		if (unidx_x->datum->bw->cols > cols)
			cols = unidx_x->datum->bw->cols;
		*unidx_y = *unidx_x;
		t += unidx_x->datum->bw->rows;
		frl_slab_pfree(unidx_x);
		unidx_x = next;
		unidx_y++;
	}
	bwdb->unum = 0;
	bwdb->unidx->next = bwdb->unidx->prev = bwdb->unidx;
#if APR_HAS_THREADS
	apr_thread_mutex_unlock(bwdb->unidxmutex);
#endif
	if (t <= 0)
		return false;
	int sizes[] = {t, t};
	CvSparseMat* sim = cvCreateSparseMat(2, sizes, CV_64FC1);
	CvMat* idx = cvCreateMat(kmax, 2, CV_32SC1);
	CvMat* dist = cvCreateMat(kmax, 2, CV_64FC1);
	double tpref = 0;
	int x_offset = 0;
	for (unidx_x = unidx_array; unidx_x != unidx_array_end; unidx_x++)
	{
		int y_offset = 0;
		idx->rows = dist->rows = unidx_x->datum->bw->rows;
		for (unidx_y = unidx_array; unidx_y != unidx_array_end; unidx_y++)
		{
			if (unidx_x != unidx_y)
			{
				if (unidx_y->datum->bwft == 0)
					unidx_y->datum->bwft = cvCreateKDTree(unidx_y->datum->bw);
				cvFindFeatures(unidx_y->datum->bwft, unidx_x->datum->bw, idx, dist, 2, bwdb->emax);
				int* iptr = idx->data.i;
				double* dptr = dist->data.db;
				for (i = 0; i < dist->rows; i++, dptr += 2, iptr += 2)
					if ((iptr[0] >= 0 && iptr[1] >= 0)&&
						((dptr[1] < dptr[0] && dptr[1] < dptr[0] * match)||
						 (dptr[0] < dptr[1] && dptr[0] < dptr[1] * match)))
					{
						cvSetReal2D(sim, x_offset + i, y_offset + (dptr[1] < dptr[0] ? iptr[1] : iptr[0]), -(dptr[1] < dptr[0] ? dptr[1] : dptr[0]));
						tpref -= dptr[1] < dptr[0] ? dptr[1] : dptr[0];
						tt++;
					}
			}
			y_offset += unidx_y->datum->bw->rows;
		}
		x_offset += unidx_x->datum->bw->rows;
	}
	idx->rows = dist->rows = kmax;
	cvReleaseMat(&idx);
	cvReleaseMat(&dist);
	double pref = tpref / tt;
	for (i = 0; i < t; i++)
		cvSetReal2D(sim, i, i, pref);
	CvAPCluster apcluster = CvAPCluster(CvAPCParams(2000, 200, 0.5));
	CvMat* rsp = cvCreateMat(1, t, CV_32SC1);
	apcluster.train(sim, rsp);
	int* bookmark = (int*)malloc(t * sizeof(int));
	int* count = (int*)malloc(t * sizeof(int));
	memset(count, 0, t * sizeof(int));
	int* rspptr = rsp->data.i;
	for (i = 0; i < t; i++, rspptr++)
		count[*rspptr]++;
	int* bptr = bookmark;
	int* cptr = count;
	int r = 0;
	for (i = 0; i < t; i++, bptr++, cptr++)
		if (*cptr >= min)
		{
			*bptr = r;
			r++;
		} else
			*bptr = -1;
	if (r <= 0)
	{
		cvReleaseMat(&rsp);
		cvReleaseSparseMat(&sim);
		free(unidx_array);
		free(bookmark);
		free(count);
		return false;
	}
	NQBWDBIDX* dbidx = (NQBWDBIDX*)frl_slab_palloc(idx_pool);
	dbidx->rnum = r;
	dbidx->inum = bwdb->unum;
	dbidx->stem = (NQBWDBSTEM*)malloc(r * sizeof(NQBWDBSTEM));
	dbidx->smmat = cvCreateMat(r, cols, CV_32FC1);
	cptr = count;
	NQBWDBSTEM* stemptr = dbidx->stem;
	for (i = 0; i < t; i++, cptr++)
		if (*cptr >= min)
		{
			stemptr->rnum = *cptr;
			stemptr->kstr = (char**)malloc(stemptr->rnum * sizeof(char*));
			stemptr->kstr += stemptr->rnum;
			stemptr++;
		}
	x_offset = 0;
	cptr = count;
	stemptr = dbidx->stem;
	float* smmptr = dbidx->smmat->data.fl;
	rspptr = rsp->data.i;
	char** dbkstr = dbidx->kstr = (char**)malloc(dbidx->inum * sizeof(char*));
	for (unidx_x = unidx_array; unidx_x != unidx_array_end; unidx_x++, dbkstr++)
	{
		for (i = 0; i < unidx_x->datum->bw->rows; i++)
		{
			if (*cptr >= min)
			{
				stemptr->desc = cvMat(1, cols, CV_32FC1, smmptr);
				memcpy(smmptr, unidx_x->datum->bw->data.fl + i * cols, sizeof(float) * cols);
				smmptr += cols;
				stemptr++;
			}
			int ptr = bookmark[*rspptr];
			if (ptr >= 0)
			{
				dbidx->stem[ptr].kstr--;
				*dbidx->stem[ptr].kstr = unidx_x->kstr;
			}
			rspptr++;
			cptr++;
		}
		x_offset += unidx_x->datum->bw->rows;
		*dbkstr = unidx_x->kstr;
	}
	cvReleaseMat(&rsp);
	cvReleaseSparseMat(&sim);
	free(bookmark);
	free(count);
	dbidx->smft = cvCreateKDTree(dbidx->smmat);
	free(unidx_array);
#if APR_HAS_THREADS
	apr_thread_rwlock_wrlock(bwdb->rwidxlock);
#endif
	bwdb->wnum += dbidx->rnum;
	bwdb->inum += dbidx->inum;
	dbidx->prev = bwdb->idx->prev;
	dbidx->next = bwdb->idx;
	bwdb->idx->prev->next = dbidx;
	bwdb->idx->prev = dbidx;
	dbidx = bwdb->idx->next;
	for (dbidx = bwdb->idx->next; dbidx != bwdb->idx; dbidx = dbidx->next)
	{
		NQBWDBSTEM* stem = dbidx->stem;
		int j;
		for (j = 0; j < dbidx->rnum; j++, stem++)
			stem->idf = log10((float)bwdb->wnum / (float)stem->rnum);
	}
#if APR_HAS_THREADS
	apr_thread_rwlock_unlock(bwdb->rwidxlock);
#endif
	return true;
}

static void nqbwrefr(char* kstr, void* vbuf, void* ud)
{
	NQBWDB* bwdb = (NQBWDB*)ud;
	NQBWDBUNIDX* unidx = (NQBWDBUNIDX*)frl_slab_palloc(unidx_pool);
	unidx->kstr = (char*)frl_slab_palloc(kstr_pool);
	memcpy(unidx->kstr, kstr, 16);
	unidx->datum = (NQBWDBDATUM*)vbuf;
	unidx->prev = bwdb->unidx->prev;
	unidx->next = bwdb->unidx;
	bwdb->unidx->prev->next = unidx;
	bwdb->unidx->prev = unidx;
}

static void nqbwunidxclr(NQBWDB* bwdb)
{
	NQBWDBUNIDX* unidx = bwdb->unidx->next;
	while (unidx != bwdb->unidx)
	{
		NQBWDBUNIDX* next = unidx->next;
		frl_slab_pfree(unidx->kstr);
		frl_slab_pfree(unidx);
		unidx = next;
	}
}

static void nqbwidxclr(NQBWDB* bwdb)
{
	NQBWDBIDX* idx = bwdb->idx->next;
	while (idx != bwdb->idx)
	{
		NQBWDBIDX* next = idx->next;
		NQBWDBSTEM* stem = idx->stem;
		int i;
		for (i = 0; i < idx->rnum; i++, stem++)
			free(stem->kstr);
		free(idx->stem);
		char** kstr = idx->kstr;
		for (i = 0; i < idx->inum; i++, kstr++)
			frl_slab_pfree(*kstr);
		free(idx->kstr);
		cvReleaseMat(&idx->smmat);
		cvReleaseFeatureTree(idx->smft);
		frl_slab_pfree(idx);
		idx = next;
	}
}

bool nqbwdbreidx(NQBWDB* bwdb, int min, double match)
{
#if APR_HAS_THREADS
	apr_thread_mutex_lock(bwdb->unidxmutex);
#endif
	nqbwunidxclr(bwdb);
	bwdb->unidx->prev = bwdb->unidx->next = bwdb->unidx;
	nqrdbforeach(bwdb->rdb, nqbwrefr, bwdb);
	bwdb->unum = bwdb->rdb->rnum;
#if APR_HAS_THREADS
	apr_thread_mutex_unlock(bwdb->unidxmutex);
	apr_thread_rwlock_wrlock(bwdb->rwidxlock);
#endif
	nqbwidxclr(bwdb);
	bwdb->inum = 0;
	bwdb->idx->prev = bwdb->idx->next = bwdb->idx;
#if APR_HAS_THREADS
	apr_thread_rwlock_unlock(bwdb->rwidxlock);
#endif
	return nqbwdbidx(bwdb, min, match);
}

bool nqbwdbout(NQBWDB* bwdb, char* kstr)
{
	NQBWDBDATUM* dt = (NQBWDBDATUM*)nqrdbget(bwdb->rdb, kstr);
	if (dt != NULL)
	{
		if (dt->bw != 0) cvReleaseMat(&dt->bw);
		if (dt->bwft != 0) cvReleaseFeatureTree(dt->bwft);
		frl_slab_pfree(dt);
		return nqrdbout(bwdb->rdb, kstr);
	}
	return false;
}

static void nqbwnuk(char* kstr, void* vbuf, void* ud)
{
	NQBWDBDATUM* dt = (NQBWDBDATUM*)vbuf;
	if (dt->bwft != 0) cvReleaseFeatureTree(dt->bwft);
	if (dt->bw != 0) cvReleaseMat(&dt->bw);
	frl_slab_pfree(dt);
}

void nqbwdbdel(NQBWDB* bwdb)
{
	nqrdbforeach(bwdb->rdb, nqbwnuk, 0);
	nqbwunidxclr(bwdb);
	nqbwidxclr(bwdb);
	frl_slab_pfree(bwdb->unidx);
	frl_slab_pfree(bwdb->idx);
#if APR_HAS_THREADS
	apr_thread_rwlock_destroy(bwdb->rwidxlock);
	apr_thread_mutex_destroy(bwdb->unidxmutex);
#endif
	nqrdbdel(bwdb->rdb);
	frl_slab_pfree(bwdb);
}
