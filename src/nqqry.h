/********************************************************
 * Database Query API Header of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#ifndef _GUARD_NQQRY_
#define _GUARD_NQQRY_

#define QRY_MAX_LMT (2000)

typedef struct NQQRY {
	void* db;
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
	NQQRY* conds;
	NQRDB* result;
} NQQRY;

enum {
	NQTRDB,			/* rdb type                    */
	NQTBWDB,		/* bwdb type                   */
	NQTFDB,			/* fdb type                    */
	NQTTCNDB,		/* tokyo-cabinet db for number */
	NQTTCSDB,		/* tokyo-cabinet db for string */
	NQTSPHINX		/* sphinx full-text search     */
};

enum {
	NQCTAND,		/* and conjunction */
	NQCTOR			/* or conjunction  */
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

NQRDB* nqqrysearch(NQQRY* qry);
int nqqrydump(NQQRY* qry, void** mem);
NQQRY* nqqrynew(void);
NQQRY* nqqrynew(void* mem);

#endif
