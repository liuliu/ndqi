/********************************************************
 * Database Query API of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#include "nqqry.h"
#include "nqrdb.h"
#include "nqbwdb.h"
#include "nqfdb.h"
#include "nqtdb.h"

/* 3rd party library */
#include <tcutil.h>
#include <tctdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "lib/frl_managed_mem.h"

typedef struct {
	char* kstr;
	float likeness;
} NQQRYPAIR;

typedef struct {
	uint32_t siz;
	NQQRYPAIR data[0];
} NQQRYUSERDATA;

static void nqqryhpf(NQQRYPAIR* pr, uint32_t i, uint32_t siz)
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
			NQQRYPAIR sw = pr[largest];
			pr[largest] = pr[i];
			pr[i] = sw;
		}
	} while (largest != i);
}

static void nqqrysort(char* kstr, void* vbuf, void* ud)
{
	NQQRYUSERDATA* nqud = (NQQRYUSERDATA*)ud;
	float likeness;
	memcpy(&likeness, &vbuf, sizeof(float));
	if (likeness > nqud->data->likeness)
	{
		nqud->data->likeness = likeness;
		nqud->data->kstr = kstr;
		nqqryhpf(nqud->data, 0, nqud->siz);
	}
}

int nqqryresult(NQQRY* qry, char** kstr, float* likeness)
{
	if (qry->result == 0)
		return 0;
	NQQRYUSERDATA* ud = (NQQRYUSERDATA*)malloc(sizeof(NQQRYUSERDATA)+qry->lmt*sizeof(NQQRYPAIR));
	memset(ud, 0, sizeof(NQQRYUSERDATA)+qry->lmt*sizeof(NQQRYPAIR));
	ud->siz = qry->lmt;
	ud->data->likeness = 0;
	nqrdbforeach(qry->result, nqqrysort, ud);
	int i;
	if (qry->order)
	{
		for (i = qry->lmt-1; i > 0; i--)
		{
			NQQRYPAIR sw = ud->data[i];
			ud->data[i] = ud->data[0];
			ud->data[0] = sw;
			nqqryhpf(ud->data, 0, i);
		}
	}
	NQQRYPAIR* dptr = ud->data;
	int k = 0;
	if (likeness == 0)
	{
		for (i = 0; i < qry->lmt; i++)
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
		for (i = 0; i < qry->lmt; i++)
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

NQRDB* nqqrysearch(NQQRY* qry)
{
	NQRDB* scope = qry->result;
	NQRDB* rdb = nqrdbnew();
	if (qry->lmt <= 0 || qry->lmt > QRY_MAX_LMT)
		qry->lmt = QRY_MAX_LMT;
	bool empty = true;
	int i, maxlmt = 0;
	NQQRY** condptr = qry->conds;
	for (i = 0; i < qry->cnum; i++, condptr++)
	{
		if ((*condptr)->lmt <= 0)
			(*condptr)->lmt = qry->lmt * 2;
		if ((*condptr)->lmt > QRY_MAX_LMT)
			(*condptr)->lmt = QRY_MAX_LMT;
		if ((*condptr)->lmt > maxlmt)
			maxlmt = (*condptr)->lmt;
	}
	char** kstr = (char**)malloc(sizeof(kstr[0]) * maxlmt);
	float* likeness = (float*)malloc(sizeof(likeness[0]) * maxlmt);
	condptr = qry->conds;
	for (i = 0; i < qry->cnum; i++, condptr++)
	{
		if (*condptr == 0)
			continue;
		memset(kstr, 0, sizeof(kstr[0]) * (*condptr)->lmt);
		memset(likeness, 0, sizeof(likeness[0]) * (*condptr)->lmt);
		NQBWDB *ttbwdb, *tbwdb, *bwdb;
		NQFDB *ttfdb, *tfdb, *fdb;
		TCTDB *tdb;
		NQTDB *ntdb;
		TCLIST *tclist;
		char *colname;
		TDBQRY *tdbqry;
		int j, k, TDBQCNOT;
		bool negate = (*condptr)->op & NQOPNOT;
		switch ((*condptr)->type)
		{
			case NQCTAND:
			case NQCTOR:
				(*condptr)->order = true;
				if (qry->type == NQCTAND && i > 0)
					(*condptr)->result = rdb;
				nqqrysearch(*condptr);
				k = nqqryresult(*condptr, kstr, likeness);
				break;
			case NQTBWDB:
				if ((*condptr)->sbj.desc == 0)
					continue;
				bwdb = (NQBWDB*)(*condptr)->db;
				tbwdb = (scope != 0) ? nqbwdbjoin(bwdb, scope) : bwdb;
				ttbwdb = (qry->type == NQCTAND && i > 0) ? nqbwdbjoin(tbwdb, rdb) : tbwdb;
				k = 0;
				switch ((*condptr)->op & ~NQOPNOT)
				{
					case NQOPLIKE:
						k = nqbwdbsearch(ttbwdb, (*condptr)->sbj.desc, kstr, (*condptr)->lmt, true, likeness);
						break;
					case NQOPELIKE:
						k = nqbwdblike(ttbwdb, (*condptr)->sbj.desc, kstr, (*condptr)->lmt, (*condptr)->mode, 0.6, true, likeness);
						break;
				}
				for (j = 0; j < k; j++)
					if (log(1. + likeness[j] / 50.f) <= (*condptr)->thr)
					{
						k = j;
						break;
					}
				if (qry->type == NQCTAND && i > 0)
					nqbwdbdel(ttbwdb);
				if (scope != 0)
					nqbwdbdel(tbwdb);
				break;
			case NQTFDB:
				if ((*condptr)->sbj.desc == 0)
					continue;
				fdb = (NQFDB*)(*condptr)->db;
				tfdb = (scope != 0) ? nqfdbjoin(fdb, scope) : fdb;
				ttfdb = (qry->type == NQCTAND && i > 0) ? nqfdbjoin(tfdb, rdb) : tfdb;
				k = 0;
				switch ((*condptr)->op & ~NQOPNOT)
				{
					case NQOPLIKE:
						k = nqfdbsearch(ttfdb, (*condptr)->sbj.desc, kstr, (*condptr)->lmt, true, likeness);
						break;
					case NQOPELIKE:
						k = nqfdblike(ttfdb, (*condptr)->sbj.desc, kstr, (*condptr)->lmt, true, likeness);
						break;
				}
				for (j = 0; j < k; j++)
					if (likeness[j] <= (*condptr)->thr)
					{
						k = j;
						break;
					}
				if (qry->type == NQCTAND && i > 0)
					nqfdbdel(ttfdb);
				if (scope != 0)
					nqfdbdel(tfdb);
				break;
			case NQTTCTDB:
				if ((*condptr)->sbj.str == 0 || (*condptr)->col == 0)
					continue;
				negate = false;
				tdb = (TCTDB*)(*condptr)->db;
				colname = (char*)(*condptr)->col;
				tdbqry = tctdbqrynew(tdb);
				TDBQCNOT = ((*condptr)->op & NQOPNOT) ? TDBQCNEGATE : 0;
				switch ((*condptr)->op & ~NQOPNOT)
				{
					case NQOPNUMEQ:
						tctdbqryaddcond(tdbqry, colname, TDBQCNOT | TDBQCNUMEQ, (*condptr)->sbj.str);
						break;
					case NQOPNUMGT:
						tctdbqryaddcond(tdbqry, colname, TDBQCNOT | TDBQCNUMGT, (*condptr)->sbj.str);
						break;
					case NQOPNUMLT:
						tctdbqryaddcond(tdbqry, colname, TDBQCNOT | TDBQCNUMLT, (*condptr)->sbj.str);
						break;
					case NQOPNUMGE:
						tctdbqryaddcond(tdbqry, colname, TDBQCNOT | TDBQCNUMGE, (*condptr)->sbj.str);
						break;
					case NQOPNUMLE:
						tctdbqryaddcond(tdbqry, colname, TDBQCNOT | TDBQCNUMLE, (*condptr)->sbj.str);
						break;
					case NQOPNUMBT:
						tctdbqryaddcond(tdbqry, colname, TDBQCNOT | TDBQCNUMBT, (*condptr)->sbj.str);
						break;
					case NQOPSTREQ:
					case NQOPLIKE:
					case NQOPELIKE:
						tctdbqryaddcond(tdbqry, colname, TDBQCNOT | TDBQCSTREQ, (*condptr)->sbj.str);
						break;
					case NQOPNULL:
						tctdbqryaddcond(tdbqry, colname, ~TDBQCNOT & TDBQCSTRBW, "");
						break;
				}
				{
					tclist = tctdbqrysearch(tdbqry);
					k = tclistnum(tclist);
					if (k > maxlmt)
					{
						maxlmt = k;
						free(kstr);
						free(likeness);
						kstr = (char**)malloc(sizeof(kstr[0]) * maxlmt);
						likeness = (float*)malloc(sizeof(likeness[0]) * maxlmt);
					}
					memset(kstr, 0, sizeof(kstr[0]) * k);
					memset(likeness, 0, sizeof(likeness[0]) * k);
					char** ksptr = kstr;
					float* lkptr = likeness;
					for (j = 0; j < k; j++, ksptr++, lkptr++)
					{
						int ksiz;
						*ksptr = (char*)tclistval(tclist, j, &ksiz);
						*lkptr = 1.;
					}
					tctdbqrydel(tdbqry);
				}
				break;
			case NQTTDB:
				if ((*condptr)->sbj.str == 0)
					continue;
				switch ((*condptr)->op & ~NQOPNOT)
				{
					case NQOPSTREQ:
					case NQOPLIKE:
					case NQOPELIKE:
						ntdb = (NQTDB*)(*condptr)->db;
						tclist = nqtdbsearch(ntdb, (*condptr)->sbj.str);
						k = tclistnum(tclist);
						if (k > maxlmt)
						{
							maxlmt = k;
							free(kstr);
							free(likeness);
							kstr = (char**)malloc(sizeof(kstr[0]) * maxlmt);
							likeness = (float*)malloc(sizeof(likeness[0]) * maxlmt);
						}
						memset(kstr, 0, sizeof(kstr[0]) * k);
						memset(likeness, 0, sizeof(likeness[0]) * k);
						char** ksptr = kstr;
						float* lkptr = likeness;
						for (j = 0; j < k; j++, ksptr++, lkptr++)
						{
							int ksiz;
							*ksptr = (char*)tclistval(tclist, j, &ksiz);
							*lkptr = 1.;
						}
						break;
				}
				break;
		}
		switch (qry->type)
		{
			case NQCTAND:
				if (!empty)
				{
					if (negate)
					{
						char** ksptr = kstr;
						for (j = 0; j < k; j++, ksptr++)
							nqrdbout(rdb, *ksptr);
					} else {
						nqrdbfilter(rdb, kstr, k);
						union { float fl; void* ptr; } it;
						char** ksptr = kstr;
						float* lkptr = likeness;
						for (j = 0; j < k; j++, ksptr++, lkptr++)
							if (*ksptr != 0 && (scope == 0 || nqrdbget(scope, *ksptr)))
							{
								it.ptr = nqrdbget(rdb, *ksptr);
								if (it.ptr != 0)
								{
									it.fl += (*condptr)->cfd * *lkptr;
									nqrdbput(rdb, *ksptr, it.ptr);
								}
							}
					}
					break;
				}
			case NQCTOR:
				union { float fl; void* ptr; } it;
				char** ksptr = kstr;
				float* lkptr = likeness;
				for (j = 0; j < k; j++, ksptr++, lkptr++)
					if (*ksptr != 0 && (scope == 0 || nqrdbget(scope, *ksptr)))
					{
						it.ptr = nqrdbget(rdb, *ksptr);
						it.fl += (*condptr)->cfd * *lkptr;
						nqrdbput(rdb, *ksptr, it.ptr);
					}
				empty = false;
				break;
		}
		/* conditionally free resources */
		if ((*condptr)->type == NQCTAND || (*condptr)->type == NQCTOR)
			nqrdbdel((*condptr)->result);
		if ((*condptr)->type == NQTTCTDB || (*condptr)->type == NQTTDB)
		{
			tclistdel(tclist);
		}
	}
	free(kstr);
	free(likeness);
	qry->result = rdb;
	return rdb;
}

static apr_pool_t* mtx_pool = 0;
static frl_slab_pool_t* qry_pool = 0;

NQQRY* nqqrynew(void)
{
	if (mtx_pool == 0)
		apr_pool_create(&mtx_pool, NULL);
	if (qry_pool == 0)
		frl_slab_pool_create(&qry_pool, mtx_pool, 1024, sizeof(NQQRY), FRL_LOCK_WITH);
	NQQRY* qry = (NQQRY*)frl_slab_palloc(qry_pool);
	memset(qry, 0, sizeof(NQQRY));
	qry->order = true;
	qry->cfd = 1.;
	return qry;
}

void nqqrydel(NQQRY* qry)
{
	if (qry->result != 0)
		nqrdbdel(qry->result);
	NQQRY** condptr = qry->conds;
	int i;
	for (i = 0; i < qry->cnum; i++, condptr++)
		nqqrydel(*condptr);
	if (qry->conds != 0)
		free(qry->conds);
	if (qry->col != 0)
		frl_managed_unref(qry->col);
	switch (qry->type)
	{
		case NQTTDB:
		case NQTTCTDB:
		case NQTSPHINX:
			if (qry->sbj.str != 0)
				frl_managed_unref(qry->sbj.str);
			break;
		case NQTBWDB:
		case NQTFDB:
			if (qry->sbj.desc != 0)
				cvReleaseMat(&qry->sbj.desc); // cvDecRefData(qry->sbj.desc);
			break;
	}
	frl_slab_pfree(qry);
}

static int nqqrydumpcount(NQQRY* qry)
{
	int t = sizeof(NQQRY);
	int i;
	NQQRY** condptr = qry->conds;
	for (i = 0; i < qry->cnum; i++, condptr++)
	{
		switch ((*condptr)->type)
		{
			case NQCTAND:
			case NQCTOR:
				t += nqqrydumpcount(*condptr);
				break;
			case NQTTDB:
			case NQTTCTDB:
			case NQTSPHINX:
				t += sizeof(NQQRY) + strlen((*condptr)->sbj.str) + 1;
				break;
			case NQTBWDB:
			case NQTFDB:
				t += (*condptr)->sbj.desc->rows * (*condptr)->sbj.desc->step;
				t += sizeof(NQQRY) + sizeof(CvMat);
				break;
		}
	}
	return t;
}

static bool nqqrydumpcpy(NQQRY* qry, char* mem)
{
	memcpy(mem, qry, sizeof(NQQRY));
	mem += sizeof(NQQRY);
	int i;
	NQQRY** condptr = qry->conds;
	for (i = 0; i < qry->cnum; i++, condptr++)
	{
		switch ((*condptr)->type)
		{
			case NQCTAND:
			case NQCTOR:
				nqqrydumpcpy(*condptr, mem);
				mem += nqqrydumpcount(*condptr);
				break;
			case NQTTDB:
			case NQTTCTDB:
			case NQTSPHINX:
				memcpy(mem, *condptr, sizeof(NQQRY));
				mem += sizeof(NQQRY);
				memcpy(mem, (*condptr)->sbj.str, strlen((*condptr)->sbj.str) + 1);
				mem += strlen((*condptr)->sbj.str) + 1;
				break;
			case NQTBWDB:
			case NQTFDB:
				memcpy(mem, *condptr, sizeof(NQQRY));
				mem += sizeof(NQQRY);
				memcpy(mem, (*condptr)->sbj.desc, sizeof(CvMat));
				mem += sizeof(CvMat);
				memcpy(mem, (*condptr)->sbj.desc->data.ptr, (*condptr)->sbj.desc->rows * (*condptr)->sbj.desc->cols * sizeof(float));
				mem += (*condptr)->sbj.desc->rows * (*condptr)->sbj.desc->cols * sizeof(float);
				break;
		}
	}
	return true;
}

NQQRY* nqqrynew(void* mem)
{
	char* newmem = (char*)mem;
	NQQRY* qry = (NQQRY*)newmem;
	newmem += sizeof(NQQRY);
	NQQRY** condptr = qry->conds = (NQQRY**)malloc(qry->cnum * sizeof(NQQRY*));
	int i;
	for (i = 0; i < qry->cnum; i++, condptr++)
	{
		*condptr = (NQQRY*)newmem;
		newmem += sizeof(NQQRY);
		switch ((*condptr)->type)
		{
			case NQCTAND:
			case NQCTOR:
				*condptr = nqqrynew(newmem - sizeof(NQQRY));
				newmem += nqqrydumpcount(*condptr);
				break;
			case NQTTDB:
			case NQTTCTDB:
			case NQTSPHINX:
				(*condptr)->sbj.str = newmem;
				newmem += strlen((*condptr)->sbj.str) + 1;
				break;
			case NQTBWDB:
			case NQTFDB:
				(*condptr)->sbj.desc = (CvMat*)newmem;
				newmem += sizeof(CvMat);
				(*condptr)->sbj.desc->data.ptr = (uchar*)newmem;
				newmem += (*condptr)->sbj.desc->rows * (*condptr)->sbj.desc->cols * sizeof(float);
				break;
		}
	}
	return qry;
}

void* nqqrydump(NQQRY* qry, int* sp)
{
	int t = nqqrydumpcount(qry);
	char* newmem = (char*)malloc(t);
	nqqrydumpcpy(qry, newmem);
	*sp = t;
	return newmem;
}
