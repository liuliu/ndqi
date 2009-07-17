#ifndef _GUARD_NQCLIENT_
#define _GUARD_NQCLIENT_

#include "../nqqry.h"

void ncinit(void);
CvMat* ncbwdbget(const char* db, char* uuid);
CvMat* ncfdbget(const char* db, char* uuid);
char* nctdbget(const char* db, char* col, char* uuid);
bool ncwdbput(const char* db, char* uuid, char* word);
int ncqrysearch(NQQRY* qry, char** kstr, float* likeness = 0);

#endif
