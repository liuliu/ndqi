/********************************************************
 * Tag Database API Header of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#ifndef _GUARD_NQTDB_
#define _GUARD_NQTDB_

#include <tcutil.h>
#include <tchdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define NQTDBMAXSIZ (1024)

typedef struct {
	TCHDB* idx;
	TCMAP* tokens;
	TCMAP* dups;
} NQTDB;

NQTDB* nqtdbnew(void);
bool nqtdbput(NQTDB* tdb, char* kstr, char* val);
TCLIST* nqtdbget(NQTDB* tdb, char* kstr);
TCLIST* nqtdbsearch(NQTDB* tdb, char* val);
bool nqtdbout(NQTDB* tdb, char* kstr);
bool nqtdbout(NQTDB* tdb, char* kstr, char* val);
bool nqtdbopen(NQTDB* tdb, const char* path, int omode);
bool nqtdbsync(NQTDB* tdb);
bool nqtdbclose(NQTDB* tdb);
void nqtdbdel(NQTDB* tdb);

#endif
