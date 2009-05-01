/********************************************************
 * "Bags of Words" Database API of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#include "nqbwdb.h"
#include "lib/mlapcluster.h"

static apr_pool_t* mtx_pool = 0;
static frl_slab_pool_t* db_pool = 0;
static frl_slab_pool_t* dt_pool = 0;
static frl_slab_pool_t* unidx_pool = 0;

NQBWDB* nqbwdbnew(void)
{
	if (mtx_pool == 0)
		apr_pool_create(&mtx_pool, NULL);
	if (db_pool == 0)
		frl_slab_pool_create(&db_pool, mtx_pool, 64, sizeof(NQBWDB), FRL_LOCK_WITH);
	if (dt_pool == 0)
		frl_slab_pool_create(&dt_pool, mtx_pool, 1024, sizeof(NQBWDBDATUM), FRL_LOCK_WITH);
	if (unidx_pool == 0)
		frl_slab_pool_create(&unidx_pool, mtx_pool, 1024, sizeof(NQBWDBUNIDX), FRL_LOCK_WITH);
	NQBWDB* bwdb = (NQBWDB*)frl_slab_palloc(db_pool);
	bwdb->rdb = nqrdbnew();
	bwdb->emax = 20;
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
	double pref = tpref/t;
	for (i = 0; i < data->rows; i++)
		cvSetReal2D(sim, i, i, pref);
	CvMat* rsp = cvCreateMat(1, data->rows, CV_32SC1);
	apcluster.train(sim, rsp);
	idxptr = idx->data.i;
	for (i = 0; i < data->rows; i++, idxptr++)
		*idxptr = 0;
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
	unidx->kstr = kstr;
	unidx->datum = dt;
	bwdb->unidx->next = unidx;
	unidx->prev = bwdb->unidx;
	dt->bw = bwm;
	dt->bwft = 0;
	nqrdbput(bwdb->rdb, kstr, dt);
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
	NQBWUSERDATA* ud = (NQBWUSERDATA*)malloc(sizeof(NQBWUSERDATA)+lmt*sizeof(NQBWPAIR));
	memset(ud, 0, sizeof(NQBWUSERDATA)+lmt*sizeof(NQBWPAIR));
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

bool nqbwdbidx(NQBWDB* bwdb)
{

}

bool nqbwdbout(NQBWDB* bwdb, char* kstr)
{
	NQBWDBDATUM* dt = (NQBWDBDATUM*)nqrdbget(bwdb->rdb, kstr);
	if (dt != NULL)
	{
		if (dt->bw != 0) cvReleaseMat(&dt->bw);
		if (dt->bwft != 0) cvReleaseFeatureTree(dt->bwft);
		return nqrdbout(bwdb->rdb, kstr);
	}
	return false;
}
static void nqbwnuk(char* kstr, void* vbuf, void* ud)
{
	NQBWDBDATUM* dt = (NQBWDBDATUM*)vbuf;
	if (dt->bwft != 0) cvReleaseFeatureTree(dt->bwft);
	if (dt->bw != 0) cvReleaseMat(&dt->bw);
}

void nqbwdbdel(NQBWDB* bwdb)
{
	nqrdbforeach(bwdb->rdb, nqbwnuk, 0);
	nqrdbdel(bwdb->rdb);
	frl_slab_pfree(bwdb);
}
