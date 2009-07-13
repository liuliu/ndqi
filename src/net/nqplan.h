#ifndef _GUARD_NQPLAN_
#define _GUARD_NQPLAN_

#include "../nqqry.h"
#include "../nqpreqry.h"

typedef struct NQPLANITER {
	NQQRY* qry;
	NQPLANITER* prev;
	NQQRY* postqry;
	int type;
} NQPLANITER;

typedef struct {
	NQPLANITER* head;
	NQPLANITER* tail;
	int cnum;
} NQPLAN;

NQPLAN* nqplannew(NQPREQRY* preqry);
int nqplanrun(NQPLAN* plan, char** kstr, float* likeness = 0);
void nqplandel(NQPLAN* plan);

#endif
