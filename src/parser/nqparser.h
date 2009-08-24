#ifndef _GUARD_NQPARSER_
#define _GUARD_NQPARSER_

#include "../nqpreqry.h"

typedef struct {
	int type;
	void* result;
} NQPARSERESULT;

typedef struct NQMPLIST {
	int type;
	const char* db;
	char* col;
	char* val;
	struct NQMPLIST* prev;
	struct NQMPLIST* next;
} NQMPLIST;

typedef struct {
	int type;
	NQMPLIST* assign;
	union {
		char* str;
		struct NQPREQRY* qry;
	} sbj;
} NQMANIPULATE;

typedef union {
    int i;
    float fl;
    void* ptr;
    const char* str;
} NQCOMMANDPARAM;

typedef struct {
    int cmd;
    NQCOMMANDPARAM params[3];
} NQCOMMAND;

enum {
	NQRTSELECT  = 0x1,
	NQRTINSERT  = 0x2,
	NQRTUPDATE  = 0x3,
	NQRTDELETE  = 0x4,
    NQRTCOMMAND = 0x5,
};

enum {
	NQMPSIMPLE  = 0x1,
	NQMPUUIDENT = 0x2,
	NQMPWHERE   = 0x3,
};

enum {
    NQCMDSYNCDISK = 0x1,
    NQCMDSYNCMEM  = 0x2,
    NQCMDMGIDX    = 0x3,
    NQCMDIDX      = 0x4,
    NQCMDREIDX    = 0x5,
};

void nqparseresultdel(NQPARSERESULT* result);
NQPARSERESULT* nqparse(char* str, int siz);
void nqparsedel(void);

#endif
