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

void ncinit()
{
	ScanDatabase* database = ScanDatabases;
	ScanDatabase* LastScanDatabase = ScanDatabases + sizeof(ScanDatabases) / sizeof(ScanDatabase);
	for (; database < LastScanDatabase; database++)
	{
		switch (database->type)
		{
			case NQTBWDB:
				database->ref = nqbwdbnew();
				break;
			case NQTFDB:
				database->ref = nqfdbnew();
				break;
			case NQTTCTDB:
				database->ref = tctdbnew();
				tctdbopen((TCTDB*)database->ref, database->location, TDBOWRITER | TDBOCREAT);
				nqmetasetindex((TCTDB*)database->ref);
				break;
			case NQTTCWDB:
				database->ref = tcwdbnew();
				tcwdbopen((TCWDB*)database->ref, database->location, WDBOWRITER | WDBOCREAT);
				database->helper = tcadbnew();
				tcadbopen((TCADB*)database->helper, );
				break;
		}
	}
}

void ncsnap()
{
}

void ncsync()
{
}

void ncterm()
{
}

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

bool ncwdbput(const char* db, char* uuid, char* word)
{
	ScanDatabase* sdb = ScanDatabaseLookup(db);
	TCWDB* wdb = (TCWDB*)sdb->ref;
	TCADB* adb = (TCADB*)sdb->helper;
	int sp;
	uint32_t uid64, *uid64_b = (uint32_t*)tcadbget(adb, uuid, 16, &sp);
	if (uid64_b == NULL)
	{
		tcadbtranbegin(adb);
		uint32_t counter, *counter_b = (uint32_t*)tcadbget(adb, "counter", 7, &sp);
		if (counter_b == NULL)
			counter = 0;
		else {
			counter = *counter_b + 1;
			free(counter_b);
		}
		tcadbput(adb, "counter", 7, &counter, sizeof(counter));
		tcadbtrancommit(adb);
	} else {
		uid64 = *uid64_b;
		free(uid64_b);
	}
	return tcwdbput2(wdb, uid64, word, NULL);
}

int ncqrysearch(NQQRY* qry, char** kstr, float* likeness)
{
	nqqrysearch(qry);
	return nqqryresult(qry, kstr, likeness);
}
