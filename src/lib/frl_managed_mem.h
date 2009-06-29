#ifndef GUARD_frl_managed_mem_h
#define GUARD_frl_managed_mem_h

#include "frl_base.h"
#include <stdlib.h>     /* for malloc, free and abort */

void* frl_managed_malloc(apr_uint32_t size);
void frl_managed_ref(void* pointer);
void frl_managed_unref(void* pointer);
void frl_managed_free(void* pointer);

#endif
