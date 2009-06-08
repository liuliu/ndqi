/********************************************************
 * Database Pre-Query Structure API of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#include "nqpreqry.h"

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
	return preqry;
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
	if (qry->type & NQSUBQ)
		nqpreqrydel(qry->sbj.subqry);
	else if (qry->sbj.str != 0)
		free(qry->sbj.str);
	if (qry->col != 0)
		free(qry->col);
	frl_slab_pfree(qry);
}

