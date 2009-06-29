#include "frl_managed_mem.h"

void* frl_managed_malloc(apr_uint32_t size)
{
	apr_uint32_t* mem = (apr_uint32_t*)malloc(size+SIZEOF_APR_UINT32_T);
	mem[0] = 1;
	return mem+1;
}

void frl_managed_ref(void* pointer)
{
	volatile apr_uint32_t* mem = (volatile apr_uint32_t*)pointer-1;
	apr_atomic_inc32(mem);
}

void frl_managed_unref(void* pointer)
{
	volatile apr_uint32_t* mem = (volatile apr_uint32_t*)pointer-1;
	apr_uint32_t refcount = apr_atomic_dec32(mem);
	if (refcount == 0)
	{
		mem[1] = 0;
		free((void*)mem);
	}
}

void frl_managed_free(void* pointer)
{
	apr_uint32_t* mem = (apr_uint32_t*)pointer-1;
	free(mem);
}
