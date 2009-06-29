/********************************************************
 * Database Pre-Query Structure API of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#include "nqpreqry.h"
#include "lib/frl_managed_mem.h"

static apr_pool_t* mtx_pool = 0;
static frl_slab_pool_t* preqry_pool = 0;

NQPREQRY* nqpreqrynew(void)
{
	if (mtx_pool == 0)
		apr_pool_create(&mtx_pool, NULL);
	if (preqry_pool == 0)
		frl_slab_pool_create(&preqry_pool, mtx_pool, 1024, sizeof(NQPREQRY), FRL_LOCK_WITH);
	NQPREQRY* preqry = (NQPREQRY*)frl_slab_palloc(preqry_pool);
	memset(preqry, 0, sizeof(NQPREQRY));
	preqry->cfd = 1.;
	return preqry;
}

NQPREQRY* nqpreqrynew(apr_pool_t* pool)
{
	NQPREQRY* preqry = (NQPREQRY*)apr_pcalloc(pool, sizeof(NQPREQRY));
	preqry->cfd = 1.;
	return preqry;
}

static char* nqstrdup(const char* str, unsigned int maxlen)
{
	unsigned int tlen = strlen(str);
	unsigned int slen = (tlen > maxlen) ? maxlen : tlen;
	char* ret = (char*)frl_managed_malloc(slen + 1);
	memcpy(ret, str, slen);
	ret[slen] = '\0';
	return ret;
}

NQPREQRY* nqpreqrydup(NQPREQRY* qry)
{
	NQPREQRY* newqry = nqpreqrynew();
	memcpy(newqry, qry, sizeof(NQPREQRY));
	newqry->result = 0;
	NQPREQRY** condptr1 = qry->conds;
	NQPREQRY** condptr2 = newqry->conds = (NQPREQRY**)malloc(qry->cnum * sizeof(NQPREQRY*));
	int i;
	for (i = 0; i < qry->cnum; i++, condptr1++, condptr2++)
		*condptr2 = nqpreqrydup(*condptr1);
	if (qry->type & NQSUBQRY)
		newqry->sbj.subqry = nqpreqrydup(qry->sbj.subqry);
	else if (qry->sbj.str != 0) {
		newqry->sbj.str = nqstrdup(qry->sbj.str, 1024);
	}
	if (qry->col != 0)
		newqry->col = nqstrdup(qry->col, 1024);
	return newqry;
}

void nqpreqrydel(NQPREQRY* qry)
{
	if (qry->result != 0)
		nqrdbdel(qry->result);
	NQPREQRY** condptr = qry->conds;
	int i;
	for (i = 0; i < qry->cnum; i++, condptr++)
		nqpreqrydel(*condptr);
	if (qry->conds != 0)
		free(qry->conds);
	if (qry->type & NQSUBQRY)
		nqpreqrydel(qry->sbj.subqry);
	else if (qry->sbj.str != 0)
		frl_managed_unref(qry->sbj.str);
	if (qry->col != 0)
		frl_managed_unref(qry->col);
	frl_slab_pfree(qry);
}

