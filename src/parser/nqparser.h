#ifndef _GUARD_NQPARSER_
#define _GUARD_NQPARSER_

#include "../nqpreqry.h"

typedef struct {
	int type;
	void* result;
} NQPARSERESULT;

typedef struct {
	int type;
	int dbtype;
	const char* db;
	char* col;
	union {
		char* str;
		struct NQPREQRY* qry;
	} sbj;
	char* val;
} NQMANIPULATE;

enum {
	NQRTSELECT = 0x1,
	NQRTINSERT = 0x2,
	NQRTUPDATE = 0x3,
	NQRTDELETE = 0x4,
};

enum {
	NQMPSIMPLE  = 0x1,
	NQMPUUIDENT = 0x2,
	NQMPWHERE   = 0x3,
};

NQPARSERESULT* nqparse(char* str, int siz);
void nqparsedel(void);

#endif
