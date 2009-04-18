/*************************
 * Fotas Runtime Library *
 * ***********************
 * a slab pool, reallcable, recollectable
 * Author: Liu Liu
 */
#ifndef GUARD_frl_slab_pool_h
#define GUARD_frl_slab_pool_h

#include "frl_base.h"
#include <stdlib.h>     /* for malloc, free and abort */

#define FRL_MEMLEAK (APR_OS_START_USERERR + 7)
#define FRL_STACKERR (APR_OS_START_USERERR + 8)
#define FRL_STACKBUSY (APR_OS_START_USERERR + 9)

struct frl_slab_pool_t;

struct frl_slab_block_t
{
	apr_uint32_t capacity;
	apr_byte_t** usage_stack;
	apr_byte_t** stack_pointer;
	apr_byte_t* arena;
	frl_slab_block_t* next;
	frl_slab_pool_t* pool;
};

struct frl_slab_pool_t
{
#if APR_HAS_THREADS
	apr_thread_mutex_t* mutex;
#endif
	apr_uint32_t per_size;
	apr_uint32_t max_capacity;
	frl_slab_block_t* block;
	frl_lock_u lock;
};

struct frl_mem_t
{
	apr_uint32_t id;
	void* pointer;
	frl_slab_block_t* block;
};

struct frl_mem_safe_t
{
	apr_uint32_t flag;
};

struct frl_mem_stat_t
{
	apr_uint32_t per_size;
	apr_uint32_t capacity;
	apr_size_t usage;
	apr_uint32_t block_size;
};

const apr_uint32_t SIZEOF_FRL_SLAB_POOL_T = sizeof(frl_slab_pool_t);
const apr_uint32_t SIZEOF_FRL_SLAB_BLOCK_T = sizeof(frl_slab_block_t);
const apr_uint32_t SIZEOF_FRL_MEM_T = sizeof(frl_mem_t);
const apr_uint32_t SIZEOF_FRL_MEM_SAFE_T = sizeof(frl_mem_safe_t);
const apr_uint32_t SIZEOF_FRL_MEM_STAT_T = sizeof(frl_mem_stat_t);

APR_DECLARE(void*) frl_slab_palloc(frl_slab_pool_t* pool);
APR_DECLARE(void*) frl_slab_pcalloc(frl_slab_pool_t* pool);
APR_DECLARE(void) frl_slab_pfree(void* pointer);
APR_DECLARE(void) frl_slab_pool_clear(frl_slab_pool_t* pool);
APR_DECLARE(apr_status_t) frl_slab_pool_destroy(frl_slab_pool_t* pool);
APR_DECLARE(apr_status_t) frl_slab_pool_create(frl_slab_pool_t** newpool, apr_pool_t *mempool, apr_uint32_t capacity, apr_uint32_t per_size, frl_lock_u lock);
APR_DECLARE(apr_status_t) frl_slab_pool_safe(frl_slab_pool_t* pool);
APR_DECLARE(frl_mem_stat_t) frl_slab_pool_stat(frl_slab_pool_t* pool);

#endif

