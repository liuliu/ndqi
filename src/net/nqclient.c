#include "nqclient.h"
#include "../config/databases.h"
#include "../nqbwdb.h"
#include "../nqfdb.h"

/* 3rd party library */
#include <tcutil.h>
#include <tcadb.h>
#include <tctdb.h>
#include <tcwdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

CvMat* ncbwdbget(const char* db, char* uuid)
{
	NQBWDB* bwdb = (NQBWDB*)ScanDatabaseLookup(db)->ref;
	CvMat *desc = nqbwdbget(bwdb, uuid);
	if (desc == NULL)
		return NULL;
	return cvCloneMat(desc);
}

CvMat* ncfdbget(const char* db, char* uuid)
{
	NQFDB* fdb = (NQFDB*)ScanDatabaseLookup(db)->ref;
	CvMat *desc = nqfdbget(fdb, uuid);
	if (desc == NULL)
		return NULL;
	return cvCloneMat(desc);
}

char* nctdbget(const char* db, char* col, char* uuid)
{
	TCTDB* tdb = (TCTDB*)ScanDatabaseLookup(db)->ref;
	TCMAP* row = tctdbget(tdb, uuid, 16);
	if (row == NULL)
		return NULL;
	int siz;
	char* str = (char*)tcmapget(row, col, strlen(col), &siz);
	if (str == NULL)
		return NULL;
	unsigned int tlen = strlen(str);
	int len = (tlen > siz) ? siz : tlen;
	char* result = (char*)malloc(len + 1);
	memcpy(result, str, len);
	result[len] = '\0';
	return result;
}

int ncqrysearch(NQQRY* qry, char** kstr, float* likeness)
{
	nqqrysearch(qry);
	return nqqryresult(qry, kstr, likeness);
}
