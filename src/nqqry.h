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
	void* orderby;
	union {
		char* str;
		CvMat* desc;
	} sbj;
	float cfd;
	int type;
	int op;
	int cnum;
	int mode;
	int order;
	int lmt;
	struct NQQRY** conds;
	NQRDB* result;
} NQQRY;

enum {
	NQCTAND   = 0x01,	/* and conjunction         */
	NQCTOR    = 0x02,	/* or conjunction          */
	NQTRDB    = 0x03,	/* rdb type                */
	NQTBWDB   = 0x04,	/* bwdb type               */
	NQTFDB    = 0x05,	/* fdb type                */
	NQTTCTDB  = 0x06,	/* tokyo-cabinet table db  */
	NQTTCWDB  = 0x07,	/* tokyo-cabinet word db   */
	NQTSPHINX = 0x08,	/* sphinx full-text search */
	NQSUBQRY  = 0x10	/* sub-query               */
};

enum {
	NQOPSTREQ = 0x01,	/* string is equal to                      */
	NQOPNUMEQ = 0x02,	/* number is equal to                      */
	NQOPNUMGT = 0x03,	/* number is greater than                  */
	NQOPNUMGE = 0x04,	/* number is greater than or equal to      */
	NQOPNUMLT = 0x05,	/* number is less than                     */
	NQOPNUMLE = 0x06,	/* number is less than or equal to         */
	NQOPNUMBT = 0x07,	/* between two numbers                     */
	NQOPLIKE  = 0x08,	/* object is like (index search)           */
	NQOPELIKE = 0x09,	/* object is exact like (exhausted search) */
	NQOPNOT   = 0x10	/* not operator                            */
};

int nqqryresult(NQQRY* qry, char** kstr, float* likeness = 0);
NQRDB* nqqrysearch(NQQRY* qry);
void* nqqrydump(NQQRY* qry, int* sp);
NQQRY* nqqrynew(void);
NQQRY* nqqrynew(void* mem);
void nqqrydel(NQQRY* qry);

#endif
