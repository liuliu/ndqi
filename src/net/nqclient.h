#ifndef _GUARD_NQCLIENT_
#define _GUARD_NQCLIENT_

#include "../nqqry.h"

void ncinit(void);
void ncsnap(void);
void ncsync(void);
void ncterm(void);
void ncmgidx(const char* db, int max);
void ncidx(const char* db);
void ncreidx(const char* db);
void ncoutany(char* uuid);
void ncputany(char* uuid);

CvMat* ncbwdbget(const char* db, char* uuid);
CvMat* ncfdbget(const char* db, char* uuid);
char* nctdbget(const char* db, char* col, char* uuid);
bool ncwdbput(const char* db, char* uuid, char* word);
int ncqrysearch(NQQRY* qry, char** kstr, float* likeness = 0);

#endif
