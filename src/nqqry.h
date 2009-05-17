/********************************************************
 * Database Query API Header of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#ifndef _GUARD_NQQRY_
#define _GUARD_NQQRY_

#include <cv.h>
#include "nqrdb.h"

#define QRY_MAX_LMT (2000)

typedef struct NQQRY {
	void* db;
	void* col;
	union {
		float fl;
		int i;
		char* str;
		CvMat* desc;
	} sbj;
	float cfd;
	int type;
	int op;
	int ext;
	int cnum;
	int ordered;
	int lmt;
	NQQRY** conds;
	NQRDB* result;
} NQQRY;

enum {
	NQCTAND,		/* and conjunction         */
	NQCTOR,			/* or conjunction          */
	NQTRDB,			/* rdb type                */
	NQTBWDB,		/* bwdb type               */
	NQTFDB,			/* fdb type                */
	NQTTCDB,		/* tokyo-cabinet table db  */
	NQTSPHINX		/* sphinx full-text search */
};

enum {
	NQOPSTREQ,		/* string is equal to                      */
	NQOPNUMEQ,		/* number is equal to                      */
	NQOPNUMGT,		/* number is greater than                  */
	NQOPNUMGE,		/* number is greater than or equal to      */
	NQOPNUMLT,		/* number is less than                     */
	NQOPNUMLE,		/* number is less than or equal to         */
	NQOPLIKE,		/* object is like (index search)           */
	NQOPELIKE		/* object is exact like (exhausted search) */
};

int nqqryresult(NQQRY* qry, char** kstr, float* likeness = 0);
NQRDB* nqqrysearch(NQQRY* qry);
bool nqqrydump(NQQRY* qry, void** mem, int* len);
NQQRY* nqqrynew(void);
NQQRY* nqqrynew(void* mem);
void nqqrydel(NQQRY* qry);

#endif
