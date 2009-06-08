/********************************************************
 * Database Pre-Query Structure API Header of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#ifndef _GUARD_NQPREQRY_
#define _GUARD_NQPREQRY_

#include "nqqry.h"

typedef struct NQPREQRY {
	int db;
	char* col;
	union {
		char* str;
		struct NQPREQRY* subqry;
	} sbj;
	int type;
	int op;
	int cnum;
	int mode;
	int ordered;
	int lmt;
	struct NQPREQRY** conds;
	NQRDB* result;
} NQPREQRY;

enum {
	NQDBLFD  = 0x01,	/* local feature descriptor database */
	NQDBLH   = 0x02,	/* local histogram database          */
	NQDBGIST = 0x03,	/* gist feature database             */
	NQDBTAG  = 0x04,	/* tag database                      */
	NQDBEXIF = 0x05		/* exif database                     */
};

NQPREQRY* nqpreqrynew(void);
void nqpreqrydel(NQPREQRY* qry);

#endif
