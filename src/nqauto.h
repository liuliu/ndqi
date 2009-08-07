/********************************************************
 * Automatic Tagging API Header of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#ifndef _GUARD_NQAUTOTAG_
#define _GUARD_NQAUTOTAG_

#include <tcutil.h>
#include <tctdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <cv.h>

/* OCR (optical character recognition) */
TCLIST* nqocrnew(CvArr* image);
/* KOD (known object detection) */
TCMAP* nqkodnew(CvArr* image);

#endif
