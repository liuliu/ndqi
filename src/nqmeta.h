/********************************************************
 * Metadata API Header of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#ifndef _GUARD_NQMETA_
#define _GUARD_NQMETA_

#include <tcutil.h>
#include <tctdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

TCMAP* nqmetanew(const char* file);
bool nqmetasetindex(TCTDB* tdb);

#endif
