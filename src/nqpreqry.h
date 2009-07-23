/********************************************************
 * Database Pre-Query Structure API Header of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#ifndef _GUARD_NQPREQRY_
#define _GUARD_NQPREQRY_

#include "nqqry.h"

typedef struct NQPREQRY {
	const char* db;
	char* col;
	char* orderby;
	union {
		char* str;
		struct NQPREQRY* subqry;
	} sbj;
	float cfd;
	int type;
	int op;
	int cnum;
	int order;
	int lmt;
	struct NQPREQRY** conds;
	NQRDB* result;
} NQPREQRY;

NQPREQRY* nqpreqrynew(void);
NQPREQRY* nqpreqrynew(apr_pool_t* pool);
NQPREQRY* nqpreqrydup(NQPREQRY* qry);
void nqpreqrydel(NQPREQRY* qry);

#endif
