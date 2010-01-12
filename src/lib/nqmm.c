/********************************************************
 * Memory Management API of NDQI
 * Copyright (C) 2010 Liu Liu
 ********************************************************/

#include "nqmm.h"

typedef struct {
	int32_t refcount;
	NQMMBLOCK* block;
	uint32_t id;
	char ptr[0];
} NQZONE;

const uint32_t SIZEOF_NQMMP = sizeof(NQMMP);
const uint32_t SIZEOF_NQMMBLOCK = sizeof(NQMMBLOCK);
const uint32_t SIZEOF_NQZONE = sizeof(NQZONE);

static NQMMBLOCK* nqmmblocknew(uint32_t rsiz, uint32_t msiz)
{
	NQMMBLOCK* block = (NQMMBLOCK*)malloc(SIZEOF_NQMMBLOCK + sizeof(int32_t) * msiz + msiz * (SIZEOF_NQZONE + rsiz));
	block->msiz = msiz;
	block->next = NULL;
	block->offset = (int32_t*)(block + 1);
	block->zone = (char*)block + SIZEOF_NQMMBLOCK + sizeof(int32_t) * msiz;
	for (int i = 0; i < msiz; i++)
		block->offset[i] = i * (SIZEOF_NQZONE + rsiz);
	return block;
}

NQMMP* nqmmpnew(uint32_t rsiz)
{
	NQMMP* mmp = (NQMMP*)malloc(SIZEOF_NQMMP);
	mmp->rsiz = rsiz;
	mmp->msiz = 64;
	mmp->block = nqmmblocknew(rsiz, mmp->msiz);
	return mmp;
}

void* nqalloc(NQMMP* mmp)
{
	NQMMBLOCK* block = mmp->block;
	NQMMBLOCK* last_block = NULL;
	while (block != NULL && block->usage == block->msiz)
	{
		last_block = block;
		block = block->next;
	}
	if (block == NULL)
	{
		block = nqmmblocknew(mmp->rsiz, last_block->msiz * 2);
		last_block->next = block;
		mmp->block = last_block;
	}
	NQZONE* azone = (NQZONE*)(block->zone + block->offset[0]);
	azone->block = block;
	azone->id = block->offset[0];
	azone->refcount = 1;
	block->offset++;
	block->usage++;
	return azone->ptr;
}

void* nqalloc(uint32_t rsiz)
{
	NQZONE* azone = (NQZONE*)malloc(sizeof(NQZONE) + rsiz);
	azone->block = NULL;
	azone->id = 0;
	azone->refcount = 1;
	return azone->ptr;
}

void nqfree(void* zone)
{
	NQZONE* azone = ((NQZONE*)zone) - 1;
	if (--azone->refcount == 0)
	{
		if (azone->block)
		{
			azone->block->offset--;
			azone->block->offset[0] = azone->id;
		} else
			free(azone);
	}
}

void nqretain(void* zone)
{
	NQZONE* azone = ((NQZONE*)zone) - 1;
	++azone->refcount;
}

void nqmmpdel(NQMMP* mmp)
{
	NQMMBLOCK* block = mmp->block;
	NQMMBLOCK* last_block = NULL;
	while (block != NULL)
	{
		last_block = block;
		block = block->next;
		free(last_block);
	}
	free(mmp);
}
