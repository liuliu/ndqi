#include "nqclient.h"
#include "../config/databases.h"
#include "../lib/frl_util_md5.h"
#include "../nqbwdb.h"
#include "../nqfdb.h"
#include "../nqdp.h"
#include "../nqlh.h"
#include "../nqgs.h"
#include "../nqmeta.h"

/* 3rd party library */
#include <tcutil.h>
#include <tcadb.h>
#include <tctdb.h>
#include <tcwdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <cv.h>
#include <highgui.h>

void ncinit()
{
	ScanDatabase* database = (ScanDatabase*)ScanDatabases;
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
				tctdbopen((TCTDB*)database->ref, database->refloc, TDBOWRITER | TDBOCREAT);
				nqmetasetindex((TCTDB*)database->ref);
				break;
			case NQTTCWDB:
				database->ref = tcwdbnew();
				tcwdbopen((TCWDB*)database->ref, database->refloc, WDBOWRITER | WDBOCREAT);
				database->hpr = tcadbnew();
				tcadbopen((TCADB*)database->hpr, database->hprloc);
				break;
		}
	}
}

void ncsnap()
{
	ScanDatabase* database = ScanDatabases;
	for (; database < LastScanDatabase; database++)
	{
		switch (database->type)
		{
			case NQTBWDB:
				nqbwdbsnap((NQBWDB*)database->ref, database->refloc);
				break;
			case NQTFDB:
				nqfdbsnap((NQFDB*)database->ref, database->refloc);
				break;
			case NQTTCTDB:
				tctdbsync((TCTDB*)database->ref);
				break;
			case NQTTCWDB:
				tcwdbsync((TCWDB*)database->ref);
				tcadbsync((TCADB*)database->hpr);
				break;
		}
	}
}

void ncsync()
{
	ScanDatabase* database = ScanDatabases;
	for (; database < LastScanDatabase; database++)
	{
		switch (database->type)
		{
			case NQTBWDB:
				nqbwdbsync((NQBWDB*)database->ref, database->refloc);
				break;
			case NQTFDB:
				nqfdbsync((NQFDB*)database->ref, database->refloc);
				break;
			case NQTTCTDB:
				tctdbsync((TCTDB*)database->ref);
				break;
			case NQTTCWDB:
				tcwdbsync((TCWDB*)database->ref);
				tcadbsync((TCADB*)database->hpr);
				break;
		}
	}
}

void ncterm()
{
	ScanDatabase* database = ScanDatabases;
	for (; database < LastScanDatabase; database++)
	{
		switch (database->type)
		{
			case NQTBWDB:
				nqbwdbdel((NQBWDB*)database->ref);
				break;
			case NQTFDB:
				nqfdbdel((NQFDB*)database->ref);
				break;
			case NQTTCTDB:
				tctdbclose((TCTDB*)database->ref);
				tctdbdel((TCTDB*)database->ref);
				break;
			case NQTTCWDB:
				tcwdbclose((TCWDB*)database->ref);
				tcwdbdel((TCWDB*)database->ref);
				tcadbclose((TCADB*)database->hpr);
				tcadbdel((TCADB*)database->hpr);
				break;
		}
	}
}

void ncputany(char* uuid)
{
	char filename[] = "~/store/lossless/#####################.png";
	char* pch = strchr(filename, '#');
	if (pch == NULL)
		return;
	frl_md5 b64;
	memcpy(b64.digest, uuid, 16);
	char name[23];
	b64.base64_encode((apr_byte_t*)name);
	strncpy(pch, name, 22);

	NQBWDB* lfd = (NQBWDB*)ScanDatabaseLookup("lfd")->ref;
	NQFDB* lh = (NQFDB*)ScanDatabaseLookup("lh")->ref;
	NQFDB* gs = (NQFDB*)ScanDatabaseLookup("gs")->ref;
	TCTDB* exif = (TCTDB*)ScanDatabaseLookup("exif")->ref;

	IplImage* image = cvLoadImage(filename, CV_LOAD_IMAGE_GRAYSCALE);
	CvMat* lfdrmat = nqdpnew(image, cvSURFParams(1200, 0));
	CvMat* lfdmat = nqbweplr(lfdrmat);
	cvReleaseMat(&lfdrmat);
	nqbwdbput(lfd, uuid, lfdmat);
	CvMat* lhmat = nqlhnew(image, 512);
	nqfdbput(lh, uuid, lhmat);
	CvMat* gsmat = nqgsnew(image, 24);
	nqfdbput(gs, uuid, gsmat);
	cvReleaseImage(&image);
	TCMAP* meta = nqmetanew(filename);
	tctdbput(exif, uuid, 16, meta);
	tcmapdel(meta);
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
	const ScanDatabase* sdb = ScanDatabaseLookup(db);
	TCWDB* wdb = (TCWDB*)sdb->ref;
	TCADB* adb = (TCADB*)sdb->hpr;
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
