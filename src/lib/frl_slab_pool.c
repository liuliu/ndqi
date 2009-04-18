/*************************
 * Fotas Runtime Library *
 * ***********************
 * a slab pool, reallcable, recollectable
 * Author: Liu Liu
 */

#include "frl_slab_pool.h"

APR_DECLARE(apr_status_t) frl_slab_block_create(frl_slab_block_t** newblock, frl_slab_pool_t* pool, apr_size_t capacity)
{
	frl_slab_block_t* block;
	block = (frl_slab_block_t*)malloc(SIZEOF_FRL_SLAB_BLOCK_T);
	if ( block == NULL )
		return APR_ENOMEM;
	block->next = 0;
	block->pool = pool;
	block->capacity = capacity;

	block->usage_stack = (apr_byte_t**)malloc((capacity+1)*SIZEOF_POINTER);
	if (block->usage_stack == NULL)
		return APR_ENOMEM;

	block->arena = (apr_byte_t*)malloc(capacity*(pool->per_size+SIZEOF_FRL_MEM_T+SIZEOF_FRL_MEM_SAFE_T));
	if (block->arena == NULL)
	{
		free(block->usage_stack);
		return APR_ENOMEM;
	}
	
	int i;
	for (i = 0; i < capacity; i++)
	{
		block->usage_stack[i+1] = block->arena+i*(pool->per_size+SIZEOF_FRL_MEM_T+SIZEOF_FRL_MEM_SAFE_T);
		frl_mem_safe_t* safe = (frl_mem_safe_t*)(block->usage_stack[i+1]+pool->per_size+SIZEOF_FRL_MEM_T);
		safe->flag = i;
	}
	block->stack_pointer = block->usage_stack+capacity;

	*newblock = block;

	return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) frl_slab_block_destory(frl_slab_block_t* block)
{
	free(block->usage_stack);
	free(block->arena);
	free(block);
	return APR_SUCCESS;
}

//both implementations are thread safe
#define DCAS_BUSY_VAL (0)

APR_DECLARE(void*) frl_slab_palloc_lock_free(frl_slab_pool_t* pool)
{
	frl_slab_block_t* pool_block = 0;
	frl_slab_block_t* block = 0;
	frl_slab_block_t* last_block = 0;
	apr_byte_t* val = 0;
	apr_byte_t** attempt = 0;
	apr_byte_t** new_pointer = 0;
	apr_byte_t** pop = 0;
	frl_slab_block_t* tail = 0;
	apr_size_t max_capacity = 0;
	for (;;)
	{
		pool_block = pool->block;
		block = pool_block;
		last_block = pool_block;
		do {
			for (;;)
			{
				attempt = block->stack_pointer;
				if (DCAS_BUSY_VAL == attempt)
					continue;
				if (attempt <= block->usage_stack)
					break;
				new_pointer = attempt-1;
				pop = (apr_byte_t**)apr_atomic_casptr((volatile void**)&block->stack_pointer, DCAS_BUSY_VAL, attempt);
				if (pop == attempt)
				{
					val = *attempt;
					block->stack_pointer = new_pointer;
					break;
				}
			}
			if (attempt > block->usage_stack)
			{
				frl_mem_t* mem = (frl_mem_t*)val;
				mem->block = block;
				mem->pointer = (void*)(val+SIZEOF_FRL_MEM_T);
				apr_atomic_casptr((volatile void**)&pool->block, block, pool_block);
				return mem->pointer;
			}
			if (block->next == pool_block)
				last_block = block;
			block = block->next;
		} while (block != pool_block);
		max_capacity = pool->max_capacity;
		if (APR_SUCCESS != frl_slab_block_create(&block, pool, max_capacity*2))
			return NULL;
		frl_mem_t* mem = (frl_mem_t*)(*block->stack_pointer);
		mem->block = block;
		mem->pointer = (void*)(*block->stack_pointer+SIZEOF_FRL_MEM_T);
		block->stack_pointer--;
		block->next = pool_block;
		tail = (frl_slab_block_t*)apr_atomic_casptr((volatile void**)&last_block->next, block, pool_block);
		if (tail == pool_block)
		{
			apr_atomic_cas32(&pool->max_capacity, max_capacity*2, max_capacity);
			apr_atomic_casptr((volatile void**)&pool->block, block, pool_block);
			return mem->pointer;
		}
		frl_slab_block_destory(block);
	}
}

APR_DECLARE(void) frl_slab_pfree_lock_free(void* pointer)
{
	frl_mem_t* mem = (frl_mem_t*)((apr_byte_t*)pointer-SIZEOF_FRL_MEM_T);
	frl_slab_block_t* block = mem->block;
	apr_byte_t* cmpval = 0;
	apr_byte_t* oldval = 0;
	apr_byte_t** attempt = 0;
	apr_byte_t** new_pointer = 0;
	apr_byte_t** push = 0;
	for (;;)
	{
		attempt = block->stack_pointer;
		if (DCAS_BUSY_VAL == attempt)
			continue;
		new_pointer = attempt+1;
		cmpval = *new_pointer;
		push = (apr_byte_t**)apr_atomic_casptr((volatile void**)&block->stack_pointer, DCAS_BUSY_VAL, attempt);
		if (push == attempt)
		{
			oldval = (apr_byte_t*)apr_atomic_casptr((volatile void**)new_pointer, mem, cmpval);
			if (oldval == cmpval)
				break;
			block->stack_pointer = attempt;
		}
	}
	block->stack_pointer = new_pointer;
}

APR_DECLARE(void*) frl_slab_palloc_lock_with(frl_slab_pool_t* pool)
{
#if APR_HAS_THREADS
	apr_thread_mutex_lock(pool->mutex);
#endif
	frl_slab_block_t* block = pool->block;
	frl_slab_block_t* last_block = pool->block;
	do {
		if (block->stack_pointer > block->usage_stack)
		{
			apr_byte_t* pop = *block->stack_pointer;
			block->stack_pointer--;
			frl_mem_t* mem = (frl_mem_t*)pop;
			mem->block = block;
			mem->pointer = (void*)(pop+SIZEOF_FRL_MEM_T);
			pool->block = block;
#if APR_HAS_THREADS
			apr_thread_mutex_unlock(pool->mutex);
#endif
			return mem->pointer;
		}
		if (block->next == pool->block)
			last_block = block;
		block = block->next;
	} while (block != pool->block);
	if (APR_SUCCESS != frl_slab_block_create(&block, pool, pool->max_capacity*2))
		return NULL;
	frl_mem_t* mem = (frl_mem_t*)(*block->stack_pointer);
	mem->block = block;
	mem->pointer = (void*)(*block->stack_pointer+SIZEOF_FRL_MEM_T);
	block->stack_pointer--;
	block->next = last_block->next;
	last_block->next = block;
	pool->max_capacity*=2;
	pool->block = block;
#if APR_HAS_THREADS
	apr_thread_mutex_unlock(pool->mutex);
#endif
	return mem->pointer;
}

APR_DECLARE(void) frl_slab_pfree_lock_with(void* pointer)
{
	frl_mem_t* mem = (frl_mem_t*)((apr_byte_t*)pointer-SIZEOF_FRL_MEM_T);
	frl_slab_block_t* block = mem->block;
	frl_slab_pool_t* pool = block->pool;
#if APR_HAS_THREADS
	apr_thread_mutex_lock(pool->mutex);
#endif
	block->stack_pointer++;
	*block->stack_pointer = (apr_byte_t*)mem;
#if APR_HAS_THREADS
	apr_thread_mutex_unlock(pool->mutex);
#endif
}

//switch between lock/lock-free version
APR_DECLARE(void*) frl_slab_palloc(frl_slab_pool_t* pool)
{
	if (FRL_LOCK_FREE == pool->lock)
		return frl_slab_palloc_lock_free(pool);
	else if (FRL_LOCK_WITH == pool->lock)
		return frl_slab_palloc_lock_with(pool);
}

APR_DECLARE(void) frl_slab_pfree(void* pointer)
{
	frl_mem_t* mem = (frl_mem_t*)((apr_byte_t*)pointer-SIZEOF_FRL_MEM_T);
	frl_slab_pool_t* pool = mem->block->pool;
	if (FRL_LOCK_FREE == pool->lock)
		frl_slab_pfree_lock_free(pointer);
	else if (FRL_LOCK_WITH == pool->lock)
		frl_slab_pfree_lock_with(pointer);
}

APR_DECLARE(void*) frl_slab_pcalloc(frl_slab_pool_t* pool)
{
	void* pointer = frl_slab_palloc(pool);
	if (pointer != NULL)
		memset(pointer, 0, pool->per_size);
	return pointer;
}

APR_DECLARE(void) frl_slab_pool_clear(frl_slab_pool_t* pool)
{
#if APR_HAS_THREADS
	apr_thread_mutex_lock(pool->mutex);
#endif
	frl_slab_block_t* pool_block = pool->block;
	frl_slab_block_t* block = pool_block;
	do {
		int i;
		for (i = 0; i < block->capacity; i++)
		{
			block->usage_stack[i+1] = block->arena+i*(pool->per_size+SIZEOF_FRL_MEM_T+SIZEOF_FRL_MEM_SAFE_T);
			frl_mem_safe_t* safe = (frl_mem_safe_t*)(block->usage_stack[i+1]+pool->per_size+SIZEOF_FRL_MEM_T);
			safe->flag = i;
		}
		block->stack_pointer = block->usage_stack+block->capacity;
		block = block->next;
	} while (block != pool->block);
#if APR_HAS_THREADS
	apr_thread_mutex_unlock(pool->mutex);
#endif
}

APR_DECLARE(apr_status_t) frl_slab_pool_destroy(frl_slab_pool_t* pool)
{
#if APR_HAS_THREADS
	apr_thread_mutex_destroy(pool->mutex);
#endif
	frl_slab_block_t* block = pool->block;
	frl_slab_block_t* last_block = 0;
	do {
		last_block = block;
		block = block->next;
		frl_slab_block_destory(last_block);
	} while (block != pool->block);
	free(pool);
	return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) frl_slab_pool_safe(frl_slab_pool_t* pool)
{
	frl_slab_block_t* pool_block = pool->block;
	frl_slab_block_t* block = pool_block;
	do {
		int i;
		for (i = 0; i < block->capacity; i++)
		{
			frl_mem_safe_t* safe = (frl_mem_safe_t*)(block->arena+i*(pool->per_size+SIZEOF_FRL_MEM_T+SIZEOF_FRL_MEM_SAFE_T)+pool->per_size+SIZEOF_FRL_MEM_T);
			if (safe->flag != i)
				return FRL_MEMLEAK;
		}
		if (block->stack_pointer == DCAS_BUSY_VAL)
			return FRL_STACKBUSY;
		apr_byte_t* lower_boundary = block->arena;
		apr_byte_t* upper_boundary = block->arena+block->capacity*(pool->per_size+SIZEOF_FRL_MEM_T+SIZEOF_FRL_MEM_SAFE_T);
		for (i = 0; i < block->capacity; i++)
			if ((block->usage_stack[i+1] < lower_boundary)||(block->usage_stack[i+1] >= upper_boundary))
				return FRL_STACKERR;
		block = block->next;
	} while (block != pool_block);
	return APR_SUCCESS;
}

APR_DECLARE(frl_mem_stat_t) frl_slab_pool_stat(frl_slab_pool_t* pool)
{
	frl_mem_stat_t stat;
	stat.block_size = 0;
	stat.capacity = 0;
	stat.per_size = pool->per_size;
	stat.usage = 0;
	frl_slab_block_t* pool_block = pool->block;
	frl_slab_block_t* block = pool_block;
	do {
		stat.block_size++;
		stat.capacity+=block->capacity;
		stat.usage+=block->capacity-(long long)(block->stack_pointer-block->usage_stack);
		block = block->next;
	} while (block != pool_block);
	return stat;
}

APR_DECLARE(apr_status_t) frl_slab_pool_create(frl_slab_pool_t** newpool, apr_pool_t *mempool, apr_uint32_t capacity, apr_uint32_t per_size, frl_lock_u lock)
{
	frl_slab_pool_t* pool;
	pool = (frl_slab_pool_t*)malloc(SIZEOF_FRL_SLAB_POOL_T);
	if (pool == NULL)
		return APR_ENOMEM;

	pool->max_capacity = capacity;
	pool->per_size = per_size;
	pool->lock = lock;

	if (APR_SUCCESS != frl_slab_block_create(&pool->block, pool, capacity))
		return APR_ENOMEM;

	pool->block->next = pool->block;

#if APR_HAS_THREADS
	apr_thread_mutex_create(&pool->mutex, APR_THREAD_MUTEX_UNNESTED, mempool);
#endif

	*newpool = pool;

	return APR_SUCCESS;
}

