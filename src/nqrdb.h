/********************************************************
 * The Radix Tree Database API Header of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#ifndef _GUARD_NQRDB_
#define _GUARD_NQRDB_

#include "nqutil.h"

typedef struct NQRDBDATUM {
	uint8_t ht;
	uint32_t rnum;
	uint32_t max;
	uint32_t kint[4];
	void* vbuf;
	NQRDBDATUM* pr;
	NQRDBDATUM** chd;
	NQRDBDATUM* prev;
	NQRDBDATUM* next;
} NQRDBDATUM;

typedef struct {
	uint64_t rnum;
	NQRDBDATUM* head;
#if APR_HAS_THREADS
	apr_thread_rwlock_t* rwlock;
#endif
} NQRDB;

NQRDB* nqrdbnew(void);
bool nqrdbput(NQRDB* rdb, char* kstr, void* vbuf);
int nqrdbput(NQRDB* rdb, char** kstr, void** vbuf, int len);
void* nqrdbget(NQRDB* rdb, char* kstr);
bool nqrdbout(NQRDB* rdb, char* kstr);
bool nqrdbforeach(NQRDB* rdb, NQFOREACH op, void* ud);
void nqrdbdel(NQRDB* rdb);

#endif
