/********************************************************
 * The Distinctive Point Database API of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#include "nqdpdb.h"

static apr_pool_t* mtx_pool = 0;
static frl_slab_pool_t* db_pool = 0;
static frl_slab_pool_t* dt_pool = 0;
static CvMemStorage* storage = 0;

NQDPDB* nqdpdbnew(void)
{
	if (mtx_pool == 0)
		apr_pool_create(&mtx_pool, NULL);
	if (db_pool == 0)
		frl_slab_pool_create(&db_pool, mtx_pool, 64, sizeof(NQDPDB), FRL_LOCK_WITH);
	if (dt_pool == 0)
		frl_slab_pool_create(&dt_pool, mtx_pool, 1024, sizeof(NQDPDBDATUM), FRL_LOCK_WITH);
	if (storage == 0)
		storage = cvCreateMemStorage();
	NQDPDB* dpdb = (NQDPDB*)frl_slab_palloc(db_pool);
	dpdb->rdb = nqrdbnew();
	dpdb->emax = 20;
	dpdb->match = .75;
	dpdb->kmax = 0;
	return dpdb;
}

CvMat* nqdpnew(CvArr* image, CvSURFParams params)
{
	CvSeq *keypoints = 0, *descriptors = 0;
	CvMemStorage* chstorage = cvCreateChildMemStorage(storage);
	cvExtractSURF(image, 0, &keypoints, &descriptors, chstorage, params);
	if (keypoints == 0 || descriptors == 0)
		return NULL;
	int cols = params.extended ? 128 : 64;
	CvMat* desc = cvCreateMat(descriptors->total, cols, CV_32FC1);
	float* dptr = desc->data.fl;
	CvSeqReader reader;
    cvStartReadSeq(descriptors, &reader);
	int i;
	for (i = 0; i < descriptors->total; i++)
	{
		float* descriptor = (float*)reader.ptr;
		memcpy(dptr, descriptor, sizeof(float)*cols);
		CV_NEXT_SEQ_ELEM(reader.seq->elem_size, reader);
		dptr += cols;
	}
	cvReleaseMemStorage(&chstorage);
	return desc;
}

bool nqdpdbput(NQDPDB* dpdb, char* kstr, CvMat* dpm)
{
	NQDPDBDATUM* dt = (NQDPDBDATUM*)frl_slab_palloc(dt_pool);
	dt->dp = dpm;
	dt->dpft = 0;
	if (dpm->rows > dpdb->kmax) dpdb->kmax = dpm->rows;
	nqrdbput(dpdb->rdb, kstr, dt);
}

CvMat* nqdpdbget(NQDPDB* dpdb, char* kstr)
{
	NQDPDBDATUM* dt = (NQDPDBDATUM*)nqrdbget(dpdb->rdb, kstr);
	return dt->dp;
}

typedef struct {
	char* kstr;
	float likeness;
} NQDPPAIR;

typedef struct {
	uint32_t siz;
	uint32_t emax;
	float match;
	CvMat* dpm;
	CvMat* idx;
	CvMat* dist;
	NQDPPAIR data[0];
} NQDPUSERDATA;

static void nqdphpf(NQDPPAIR* pr, uint32_t i, uint32_t siz)
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
			NQDPPAIR sw = pr[largest];
			pr[largest] = pr[i];
			pr[i] = sw;
		}
	} while (largest != i);
}

static void nqdpfwm(char* kstr, void* vbuf, void* ud)
{
	NQDPUSERDATA* nqud = (NQDPUSERDATA*)ud;
	NQDPDBDATUM* datum = (NQDPDBDATUM*)vbuf;
	if (datum->dpft == 0)
		datum->dpft = cvCreateKDTree(datum->dp);
	cvFindFeatures(datum->dpft, nqud->dpm, nqud->idx, nqud->dist, 2, nqud->emax);
	double* dptr = nqud->dist->data.db;
	int co = 0;
	int i;
	for (i = 0; i < nqud->dist->rows; i++, dptr += 2)
		if ((dptr[0] < dptr[1] && dptr[0] < dptr[1] * nqud->match)||
			(dptr[1] < dptr[0] && dptr[1] < dptr[0] * nqud->match))
			co++;
	float likeness = (float)(co) / (float)(nqud->dpm->rows);
	if (likeness > nqud->data->likeness)
	{
		nqud->data->likeness = likeness;
		nqud->data->kstr = kstr;
		nqdphpf(nqud->data, 0, nqud->siz);
	}
}

int nqdpdblike(NQDPDB* dpdb, CvMat* dpm, char** kstr, int lmt, bool ordered, float* likeness)
{
	NQDPUSERDATA* ud = (NQDPUSERDATA*)malloc(sizeof(NQDPUSERDATA)+lmt*sizeof(NQDPPAIR));
	memset(ud, 0, sizeof(NQDPUSERDATA)+lmt*sizeof(NQDPPAIR));
	ud->emax = dpdb->emax;
	ud->match = dpdb->match;
	ud->siz = lmt;
	ud->dpm = dpm;
	ud->idx = cvCreateMat(dpdb->kmax, 2, CV_32SC1);
	ud->dist = cvCreateMat(dpdb->kmax, 2, CV_64FC1);
	ud->data->likeness = 0;

	nqrdbforeach(dpdb->rdb, nqdpfwm, ud);

	int i;
	if (ordered)
	{
		for (i = lmt-1; i > 0; i--)
		{
			NQDPPAIR sw = ud->data[i];
			ud->data[i] = ud->data[0];
			ud->data[0] = sw;
			nqdphpf(ud->data, 0, i);
		}
	}

	NQDPPAIR* dptr = ud->data;
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

bool nqdpdbout(NQDPDB* dpdb, char* kstr) 
{
	NQDPDBDATUM* dt = (NQDPDBDATUM*)nqrdbget(dpdb->rdb, kstr);
	if (dt != NULL)
	{
		if (dt->dp != 0) cvReleaseMat(&dt->dp);
		if (dt->dpft != 0) cvReleaseFeatureTree(dt->dpft);
		return nqrdbout(dpdb->rdb, kstr);
	}
	return false;
}
static void nqdpnuk(char* kstr, void* vbuf, void* ud)
{
	NQDPDBDATUM* dt = (NQDPDBDATUM*)vbuf;
	if (dt->dp != 0) cvReleaseMat(&dt->dp);
	if (dt->dpft != 0) cvReleaseFeatureTree(dt->dpft);
}

void nqdpdbdel(NQDPDB* dpdb)
{
	nqrdbforeach(dpdb->rdb, nqdpnuk, 0);
	nqrdbdel(dpdb->rdb);
	frl_slab_pfree(dpdb);
}
