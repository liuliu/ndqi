#ifndef _GUARD_NQUTIL_
#define _GUARD_NQUTIL_

#include "lib/frl_slab_pool.h"
#include <cv.h>

typedef void (*NQFOREACH) (char*, void*, void*);

int nqeplr(CvMat* data, int* ki, int k = 0);
int nqeplr(CvSparseMat* sim, int* ki, int t, int k);

#endif
