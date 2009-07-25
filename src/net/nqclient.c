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

void ncmgidx(const char* db, int max)
{
	const ScanDatabase* database = ScanDatabaseLookup(db);
	switch (database->type)
	{
		case NQTBWDB:
			nqbwdbmgidx((NQBWDB*)database->ref, max);
			break;
		case NQTFDB:
			nqfdbmgidx((NQFDB*)database->ref, max);
			break;
	}
}

void ncidx(const char* db)
{
	const ScanDatabase* database = ScanDatabaseLookup(db);
	switch (database->type)
	{
		case NQTBWDB:
			nqbwdbidx((NQBWDB*)database->ref);
			break;
		case NQTFDB:
			nqfdbidx((NQFDB*)database->ref);
			break;
	}
}

void ncreidx(const char* db)
{
	const ScanDatabase* database = ScanDatabaseLookup(db);
	switch (database->type)
	{
		case NQTBWDB:
			nqbwdbreidx((NQBWDB*)database->ref);
			break;
		case NQTFDB:
			nqfdbreidx((NQFDB*)database->ref);
			break;
	}
}

void ncoutany(char* uuid)
{
}

void ncputany(char* uuid)
{
	char filename[] = "/home/liu/store/lossless/######################.png";
	char* pch = strchr(filename, '#');
	char filename_o[] = "/home/liu/store/raw/######################.jpg";
	char* pch_o = strchr(filename_o, '#');
	if (pch == NULL || pch_o == NULL)
		return;
	char* digest = (char*)frl_md5((apr_byte_t*)uuid).digest;
	strncpy(pch, uuid, 22);
	strncpy(pch_o, uuid, 22);

	NQBWDB* lfd = (NQBWDB*)ScanDatabaseLookup("lfd")->ref;
	NQFDB* lh = (NQFDB*)ScanDatabaseLookup("lh")->ref;
	NQFDB* gist = (NQFDB*)ScanDatabaseLookup("gist")->ref;
	TCTDB* exif = (TCTDB*)ScanDatabaseLookup("exif")->ref;

	IplImage* image = cvLoadImage(filename, CV_LOAD_IMAGE_COLOR);
	IplImage* gray = cvCreateImage(cvGetSize(image), 8, 1);
	cvCvtColor(image, gray, CV_BGR2GRAY);
	CvMat* lfdrmat = nqdpnew(gray, cvSURFParams(1200, 0));
	CvMat* lfdmat = nqbweplr(lfdrmat);
	cvReleaseMat(&lfdrmat);
	nqbwdbput(lfd, digest, lfdmat);
	CvMat* lhmat = nqlhnew(image, 512);
	nqfdbput(lh, digest, lhmat);
	CvMat* gsmat = nqgsnew(gray, 24);
	nqfdbput(gist, digest, gsmat);
	cvReleaseImage(&gray);
	cvReleaseImage(&image);

	TCMAP* meta = nqmetanew(filename_o);
	tctdbput(exif, digest, 16, meta);
	tcmapdel(meta);
}

CvMat* ncbwdbget(const char* db, char* uuid)
{
	NQBWDB* bwdb = (NQBWDB*)ScanDatabaseLookup(db)->ref;
	CvMat *desc = nqbwdbget(bwdb, uuid);
	if (desc == NULL)
		return NULL;
	cvIncRefData(desc);
	return desc;
}

CvMat* ncfdbget(const char* db, char* uuid)
{
	NQFDB* fdb = (NQFDB*)ScanDatabaseLookup(db)->ref;
	CvMat *desc = nqfdbget(fdb, uuid);
	if (desc == NULL)
		return NULL;
	cvIncRefData(desc);
	return desc;
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
	if (qry->type == NQCTOR || qry->type == NQCTAND)
	{
		nqqrysearch(qry);
		return nqqryresult(qry, kstr, likeness);
	} else {
		NQQRY _qry;
		memset(&_qry, 0, sizeof(NQQRY));
		_qry.type = NQCTOR;
		_qry.cnum = 1;
		_qry.lmt = QRY_MAX_LMT;
		_qry.cfd = 1.;
		_qry.conds = &qry;
		nqqrysearch(&_qry);
		return nqqryresult(&_qry, kstr, likeness);
	}
}
