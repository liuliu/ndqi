/********************************************************
 * Database Query API of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#include "nqqry.h"
#include "nqrdb.h"
#include "nqbwdb.h"
#include "nqfdb.h"

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
	NQQRYUSERDATA* ud = (NQQRYUSERDATA*)malloc(sizeof(NQQRYUSERDATA)+qry->lmt*sizeof(NQQRYPAIR));
	memset(ud, 0, sizeof(NQQRYUSERDATA)+qry->lmt*sizeof(NQQRYPAIR));
	ud->siz = qry->lmt;
	ud->data->likeness = 0;
	nqrdbforeach(qry->result, nqqrysort, ud);
	int i;
	if (qry->ordered)
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
	int i, maxlmt = 0;
	NQQRY* condptr = qry->conds;
	for (i = 0; i < qry->cnum; i++, condptr++)
	{
		if (condptr->lmt <= 0)
			condptr->lmt = qry->lmt * 2;
		if (condptr->lmt > QRY_MAX_LMT)
			condptr->lmt = QRY_MAX_LMT;
		if (condptr->lmt > maxlmt)
			maxlmt = condptr->lmt;
	}
	char** kstr = (char**)malloc(sizeof(kstr[0]) * maxlmt);
	float* likeness = (float*)malloc(sizeof(likeness[0]) * maxlmt);
	condptr = qry->conds;
	for (i = 0; i < qry->cnum; i++, condptr++)
	{
		memset(kstr, 0, sizeof(kstr[0]) * condptr->lmt);
		memset(likeness, 0, sizeof(likeness[0]) * condptr->lmt);
		switch (condptr->type)
		{
			case NQCTAND:
			case NQCTOR:
				if (qry->type == NQCTAND && i > 0)
					condptr->result = rdb;
				nqqrysearch(condptr);
				nqqryresult(condptr, kstr, likeness);
				nqrdbdel(condptr->result);
				break;
			case NQTBWDB:
				NQBWDB *ttbwdb, *tbwdb, *bwdb = (NQBWDB*)condptr->db;
				tbwdb = (scope != 0) ? nqbwdbjoin(bwdb, scope) : bwdb;
				ttbwdb = (qry->type == NQCTAND && i > 0) ? nqbwdbjoin(tbwdb, rdb) : tbwdb;
				switch (condptr->op)
				{
					case NQOPLIKE:
						nqbwdbsearch(ttbwdb, condptr->sbj.desc, kstr, condptr->lmt, false, likeness);
						break;
					case NQOPELIKE:
						nqbwdblike(ttbwdb, condptr->sbj.desc, kstr, condptr->lmt, condptr->ext, 0.6, false, likeness);
						break;
				}
				if (qry->type == NQCTAND && i > 0)
					nqbwdbdel(ttbwdb);
				if (scope != 0)
					nqbwdbdel(tbwdb);
				break;
			case NQTFDB:
				NQFDB *ttfdb, *tfdb, *fdb = (NQFDB*)condptr->db;
				tfdb = (scope != 0) ? nqfdbjoin(fdb, scope) : fdb;
				ttfdb = (qry->type == NQCTAND && i > 0) ? nqfdbjoin(tfdb, rdb) : tfdb;
				switch (condptr->op)
				{
					case NQOPLIKE:
						nqbwdbsearch(ttfdb, condptr->sbj.desc, kstr, condptr->lmt, false, likeness);
						break;
					case NQOPELIKE:
						nqfdblike(ttfdb, condptr->sbj.desc, kstr, condptr->lmt, false, likeness);
						break;
				}
				if (qry->type == NQCTAND && i > 0)
					nqfdbdel(ttfdb);
				if (scope != 0)
					nqfdbdel(tfdb);
				break;
		}
		int j;
		switch (qry->type)
		{
			case NQCTAND:
				if (i > 0)
				{
					nqrdbfilter(rdb, kstr);
					union { float fl; void* ptr; } it;
					char** ksptr = kstr;
					float* lkptr = likeness;
					for (j = 0; j < condptr->lmt; j++, ksptr++, lkptr++)
						if (*ksptr != 0 && (scope == 0 || nqrdbget(scope, *ksptr)))
						{
							it.ptr = nqrdbget(rdb, *ksptr);
							if (it.ptr != 0)
							{
								it.fl += condptr->cfd * *lkptr;
								nqrdbput(rdb, *ksptr, it.ptr);
							}
						}
					break;
				}
			case NQCTOR:
				union { float fl; void* ptr; } it;
				char** ksptr = kstr;
				float* lkptr = likeness;
				for (j = 0; j < condptr->lmt; j++, ksptr++, lkptr++)
					if (*ksptr != 0 && (scope == 0 || nqrdbget(scope, *ksptr)))
					{
						it.ptr = nqrdbget(rdb, *ksptr);
						it.fl += condptr->cfd * *lkptr;
						nqrdbput(rdb, *ksptr, it.ptr);
					}
				break;
		}
	}
	free(kstr);
	free(likeness);
	qry->result = rdb;
	return rdb;
}

NQQRY* nqqrynew(void* mem)
{
}

int nqqrydump(NQQRY* qry, void** mem)
{
	int t = sizeof(NQQRY) + sizeof(NQCOND) * qry->cnum;
	int i;
	NQCOND* condptr = qry->conds;
	for (i = 0; i < qry->cnum; i++, condptr++)
	{
		switch (condptr->type)
		{
			case NQTRDB:
			case NQTTCSDB:
			case NQTSPHINX:
				t += condptr->ext;
				break;
			case NQTBWDB:
			case NQTFDB:
				t += condptr->sbj.desc->rows * condptr->sbj.desc->step;
				t += sizeof(CvMat);
				break;
			case NQTTCNDB:
				t += sizeof(condptr->sbj);
				break;
		}
	}
	char* newmem = (char*)malloc(t);
	*mem = newmem;
	memcpy(newmem, qry, sizeof(NQQRY));
	newmem += sizeof(NQQRY);
	memcpy(newmem, qry->conds, sizeof(NQCOND) * qry->cnum);
	newmem += sizeof(NQCOND) * qry->cnum;
	condptr = qry->conds;
	for (i = 0; i < qry->cnum; i++, condptr++)
	{
		switch (condptr->type)
		{
			case NQTRDB:
			case NQTTCSDB:
			case NQTSPHINX:
				memcpy(newmem, condptr->sbj.str, condptr->ext);
				newmem += condptr->ext;
				break;
			case NQTBWDB:
			case NQTFDB:
				memcpy(newmem, condptr->sbj.desc->data.ptr, condptr->sbj.desc->rows * condptr->sbj.desc->step);
				newmem += condptr->sbj.desc->rows * condptr->sbj.desc->step;
				memcpy(newmem, condptr->sbj.desc);
				newmem += sizeof(CvMat);
				break;
			case NQTTCNDB:
				memcpy(newmem, &condptr->sbj, sizeof(condptr->sbj));
				newmem += sizeof(condptr->sbj);
				break;
		}
	}
	return t;
}
