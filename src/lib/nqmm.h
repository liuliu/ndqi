/********************************************************
 * Memory Management API Header of NDQI
 * Copyright (C) 2010 Liu Liu
 ********************************************************/

#ifndef _GUARD_NQMM_
#define _GUARD_NQMM_

#include <stdlib.h>
#include <stdint.h>

typedef struct NQMMBLOCK {
	uint32_t msiz;
	uint32_t usage;
	struct NQMMBLOCK* next;
	int32_t* offset;
	char* zone;
} NQMMBLOCK;

typedef struct {
	NQMMBLOCK* block;
	uint32_t rsiz;
	uint32_t msiz;
} NQMMP;

NQMMP* nqmmpnew(uint32_t rsiz);
void* nqalloc(NQMMP* mmp);
void* nqalloc(uint32_t rsiz);
void nqfree(void*);
void nqretain(void*);
void nqmmpdel(NQMMP*);

#endif
