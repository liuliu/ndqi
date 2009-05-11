/********************************************************
 * Database Query API of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#include "nqqry.h"
#include "nqrdb.h"
#include "nqbwdb.h"
#include "nqfdb.h"

NQRDB* nqqrysearch(NQQRY* qry)
{
	NQRDB* rdb = nqrdbnew();
	NQCOND* condptr = qry->conds;
	if (qry->lmt <= 0 || qry->lmt > QRY_MAX_LMT)
		qry->lmt = QRY_MAX_LMT;
	int i;
	for (i = 0; i < qry->cnum; i++, condptr++)
	{
		switch (condptr->type)
		{
			if (condptr->lmt <= 0)
				condptr->lmt = qry->lmt * 2;
			if (condptr->lmt > QRY_MAX_LMT * 2)
				condptr->lmt = QRY_MAX_LMT * 2;
			char** kstr = (char**)malloc(sizeof(kstr[0]) * condptr->lmt);
			float* likeness = (float*)malloc(sizeof(likeness[0]) * condptr->lmt);
			case NQTBWDB:
				NQBWDB *tbwdb, *bwdb = (NQBWDB*)condptr->column;
				if (qry->ct == NQCTAND && i > 0)
					tbwdb = nqbwdbjoin(bwdb, rdb);
				switch (condptr->op)
				{
					case NQOPLIKE:
						nqbwdbsearch(bwdb, condptr->sbj.desc, kstr, condptr->lmt, false, likeness);
						break;
					case NQOPELIKE:
						nqbwdblike(bwdb, condptr->sbj.desc, kstr, condptr->lmt, condptr->ext, 0.6, false, likeness);
						break;
				}
				if (qry->ct == NQCTAND && i > 0)
					nqbwdbdel(tbwdb);
				break;
			case NQTFDB:
				NQFDB *tfdb, *fdb = (NQFDB*)condptr->column;
				if (qry->ct == NQCTAND && i > 0)
					tfdb = nqfdbjoin(fdb, rdb);
				switch (condptr->op)
				{
					case NQOPLIKE:
						nqbwdbsearch(fdb, condptr->sbj.desc, kstr, condptr->lmt, false, likeness);
						break;
					case NQOPELIKE:
						nqfdblike(fdb, condptr->sbj.desc, kstr, condptr->lmt, false, likeness);
						break;
				}
				if (qry->ct == NQCTAND && i > 0)
					nqfdbdel(tfdb);
				break;
		}
		int j;
		switch (qry->ct)
		{
			case NQCTAND:
				if (i > 0)
				{
					union { float fl; void* ptr; } it;
					char** ksptr = kstr;
					float* lkptr = likeness;
					for (j = 0; j < condptr->lmt; j++, ksptr++, lkptr++)
						if (*ksptr != 0)
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
					if (*ksptr != 0)
					{
						it.ptr = nqrdbget(rdb, *ksptr);
						it.fl += condptr->cfd * *lkptr;
						nqrdbput(rdb, *ksptr, it.ptr);
					}
				break;
		}
	}
	if (qry->result != 0)
		nqrdbdel(qry->result);
	qry->result = rdb;
	return rdb;
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
