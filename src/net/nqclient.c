#include "nqclient.h"
#include "../config/databases.h"
#include "../lib/frl_util_md5.h"
#include "../nqbwdb.h"
#include "../nqfdb.h"
#include "../nqtdb.h"
#include "../nqdp.h"
#include "../nqlh.h"
#include "../nqgs.h"
#include "../nqmeta.h"
#include "../nqauto.h"

/* 3rd party library */
#include <tcutil.h>
#include <tctdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <cv.h>
#include <highgui.h>

static void nctdbsetindex(TCTDB* tdb, const char* set)
{
	if (set == NULL)
		return;
	char setbuf[1024];
	strncpy(setbuf, set, 1024);
	char* pch = strtok(setbuf, " ");
	while (pch != NULL)
	{
		int len = strlen(pch);
		char buf[128];
		if (len < 128 + 2)
		{
			strncpy(buf, pch, len - 2);
			buf[len - 2] = '\0';
			switch (pch[len - 1])
			{
				case 'n':
					tctdbsetindex(tdb, buf, TDBITLEXICAL);
					break;
				case 's':
					tctdbsetindex(tdb, buf, TDBITLEXICAL);
					break;
			}
		}
		pch = strtok(NULL, " ");
	}
}

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
				nctdbsetindex((TCTDB*)database->ref, database->hprloc);
				break;
			case NQTTDB:
				database->ref = nqtdbnew();
				nqtdbopen((NQTDB*)database->ref, database->refloc, HDBOWRITER | HDBOCREAT);
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
			case NQTTDB:
				nqtdbsync((NQTDB*)database->ref);
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
			case NQTTDB:
				nqtdbsync((NQTDB*)database->ref);
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
			case NQTTDB:
				nqtdbclose((NQTDB*)database->ref);
				nqtdbdel((NQTDB*)database->ref);
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
	ScanDatabase* database = ScanDatabases;
	for (; database < LastScanDatabase; database++)
	{
		switch (database->type)
		{
			case NQTBWDB:
				nqbwdbout((NQBWDB*)database->ref, uuid);
				break;
			case NQTFDB:
				nqfdbout((NQFDB*)database->ref, uuid);
				break;
			case NQTTCTDB:
				tctdbout((TCTDB*)database->ref, uuid, 16);
				break;
			case NQTTDB:
				nqtdbout((NQTDB*)database->ref, uuid);
				break;
		}
	}
}

void ncputany(char* uuid)
{
#define NQ_DIRECTORY(r,o) char r[] = o;
#include "../config/directories.i"
#undef NQ_DIRECTORY
	char* pch = strchr(LossLessImageDirectory, '#');
	char* pch_o = strchr(RawImageDirectory, '#');
	if (pch == NULL || pch_o == NULL)
		return;
	frl_md5 b64;
	memcpy(b64.digest, uuid, 16);
	char name[23];
	b64.base64_encode((apr_byte_t*)name);
	strncpy(pch, name, 22);
	strncpy(pch_o, name, 22);

	NQBWDB* lfd = (NQBWDB*)ScanDatabaseLookup("lfd")->ref;
	NQFDB* lh = (NQFDB*)ScanDatabaseLookup("lh")->ref;
	NQFDB* gist = (NQFDB*)ScanDatabaseLookup("gist")->ref;
	TCTDB* exif = (TCTDB*)ScanDatabaseLookup("exif")->ref;
	TCTDB* kod = (TCTDB*)ScanDatabaseLookup("kod")->ref;

	IplImage* image = cvLoadImage(LossLessImageDirectory, CV_LOAD_IMAGE_COLOR);
	IplImage* gray = cvCreateImage(cvGetSize(image), 8, 1);
	cvCvtColor(image, gray, CV_BGR2GRAY);
	CvMat* lfdrmat = nqdpnew(gray, cvSURFParams(1200, 0));
	CvMat* lfdmat = nqbweplr(lfdrmat);
	cvReleaseMat(&lfdrmat);
	nqbwdbput(lfd, uuid, lfdmat);
	CvMat* lhmat = nqlhnew(image, 512);
	nqfdbput(lh, uuid, lhmat);
	CvMat* gsmat = nqgsnew(gray);
	nqfdbput(gist, uuid, gsmat);
	TCMAP* kodcol = nqkodnew(image);
	tctdbput(kod, uuid, 16, kodcol);
	tcmapdel(kodcol);
	cvReleaseImage(&gray);
	cvReleaseImage(&image);

	TCMAP* meta = nqmetanew(RawImageDirectory);
	tctdbput(exif, uuid, 16, meta);
	tcmapdel(meta);
}

CvMat* ncbwdbget(const char* db, char* uuid)
{
	NQBWDB* bwdb = (NQBWDB*)ScanDatabaseLookup(db)->ref;
	CvMat *desc = nqbwdbget(bwdb, uuid);
	if (desc == NULL)
		return NULL;
	// cvIncRefData(desc);
	return cvCloneMat(desc);
}

CvMat* ncfdbget(const char* db, char* uuid)
{
	NQFDB* fdb = (NQFDB*)ScanDatabaseLookup(db)->ref;
	CvMat *desc = nqfdbget(fdb, uuid);
	if (desc == NULL)
		return NULL;
	// cvIncRefData(desc);
	return cvCloneMat(desc);
}

char* nctctdbget(const char* db, char* col, char* uuid)
{
	TCTDB* tdb = (TCTDB*)ScanDatabaseLookup(db)->ref;
	TCMAP* row = tctdbget(tdb, uuid, 16);
	if (row == NULL)
		return NULL;
	int siz;
	char* str = (char*)tcmapget(row, col, strlen(col), &siz);
	tcmapdel(row);
	if (str == NULL)
		return NULL;
	unsigned int tlen = strlen(str);
	int len = (tlen > siz) ? siz : tlen;
	char* result = (char*)malloc(len + 1);
	memcpy(result, str, len);
	result[len] = '\0';
	return result;
}

bool nctctdbput(const char* db, char* col, char* uuid, char* val)
{
	TCTDB* tdb = (TCTDB*)ScanDatabaseLookup(db)->ref;
	TCMAP* row = tctdbget(tdb, uuid, 16);
	if (row == NULL)
		return false;
	tcmapput(row, col, strlen(col), val, strlen(val));
	bool result = tctdbput(tdb, uuid, 16, row);
	tcmapdel(row);
	return result;
}

TCLIST* nctdbget(const char* db, char* uuid)
{
	const ScanDatabase* sdb = ScanDatabaseLookup(db);
	NQTDB* tdb = (NQTDB*)sdb->ref;
	return nqtdbget(tdb, uuid);
}

bool nctdbput(const char* db, char* uuid, char* word)
{
	const ScanDatabase* sdb = ScanDatabaseLookup(db);
	NQTDB* tdb = (NQTDB*)sdb->ref;
	return nqtdbput(tdb, uuid, word);
}

bool nctdbout(const char* db, char* uuid, char* word)
{
	const ScanDatabase* sdb = ScanDatabaseLookup(db);
	NQTDB* tdb = (NQTDB*)sdb->ref;
	return nqtdbout(tdb, uuid, word);
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
