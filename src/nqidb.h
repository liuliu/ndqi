#ifndef _GUARD_NQIDB_
#define _GUARD_NQIDB_

#include "nqutil.h"

typedef struct NQIDBDATUM {
	uint32_t i[5];
	struct NQIDBDATUM* con[0];
} NQIDBDATUM;

const int NQIDBDATUMSIZ = sizeof(NQIDBDATUM) + sizeof(NQIDBDATUM*);

typedef struct {
	uint64_t rnum;
	NQIDBDATUM* con;
} NQIDB;

NQIDB* nqidbnew(void);
bool nqidbput(NQIDB* idb, char* kstr);
bool nqidbget(NQIDB* idb, char* kstr);
bool nqidbout(NQIDB* idb, char* kstr);
bool nqidbforeach(NQIDB* idb, NQFOREACH op, void* ud);
void nqidbdel(NQIDB* idb);

#endif
