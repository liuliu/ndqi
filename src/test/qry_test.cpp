#include "../nqqry.h"
#include "../nqlh.h"
#include "../nqdp.h"
#include "../nqfdb.h"
#include "../nqbwdb.h"
#include "../nqmeta.h"

#include "highgui.h"
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#include <tcadb.h>
#include <tcwdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

apr_pool_t* mempool;

int main()
{
	apr_initialize();
	apr_pool_create(&mempool, NULL);
	apr_atomic_init(mempool);

	NQFDB* fdb = nqfdbnew();
	NQBWDB* bwdb = nqbwdbnew();

	TCTDB* tdb = tctdbnew();
	tctdbopen(tdb, "casket.tct", TDBOWRITER | TDBOCREAT);
	nqmetasetindex(tdb);
	TCADB* adb = tcadbnew();
	tcadbopen(adb, "*");
	TCWDB* wdb = tcwdbnew();
	tcwdbopen(wdb, "casket", WDBOWRITER | WDBOCREAT);
	/* string to hold directory name */
	char *dirname = "./../testdata/dpdb/";

	/* Pointers needed for functions to iterating directory entries. */
	DIR *dir;
	struct dirent *entry;

	/* Struct needed for determining info about an entry with stat(). */
	struct stat statinfo;

	/* Open the directory */
	if ((dir = opendir(dirname)) == NULL)
	{
		fprintf(stderr, "ERROR: Could not open directory %s\n", dirname);
		return 1;
	}

	/* change working dir to be able to read file information */
	chdir(dirname);
	char todel[32];
	uint64_t i = 0;
	while ((entry = readdir(dir)) != NULL)
	{
		/* only list regular files... */
		stat(entry->d_name, &statinfo);
		/* ...so test for this */
		if (S_ISREG(statinfo.st_mode))
		{
			printf("loading file: %s\n", entry->d_name);
			char kstr[16];
			memcpy(kstr, entry->d_name, 16);
			tcadbput(adb, &i, sizeof(uint64_t), kstr, 16);
			TCMAP* meta = nqmetanew(entry->d_name);
			tctdbput(tdb, kstr, 16, meta);
			tcmapdel(meta);
			char prefix[] = "dsc007";
			memcpy(prefix, entry->d_name, 6);
			tcwdbput2(wdb, i, prefix, NULL);
			IplImage* image = cvLoadImage(entry->d_name, CV_LOAD_IMAGE_COLOR);
			IplImage* small = cvCreateImage(cvSize(640, 480), 8, 3);
			cvResize(image, small, CV_INTER_AREA);
			IplImage* dimage = cvLoadImage(entry->d_name, CV_LOAD_IMAGE_GRAYSCALE);
			IplImage* dsmall = cvCreateImage(cvSize(640, 480), 8, 1);
			cvResize(dimage, dsmall, CV_INTER_AREA);
			CvMat* dpm = nqdpnew(dsmall, cvSURFParams(1200, 0));
			if (dpm != 0)
			{
				CvMat* dpe = nqbweplr(dpm);
				cvReleaseMat(&dpm);
				nqbwdbput(bwdb, entry->d_name, dpe);
			}
			CvMat* fm = nqlhnew(small, 512);
			cvReleaseImage(&image);
			cvReleaseImage(&small);
			cvReleaseImage(&dimage);
			cvReleaseImage(&dsmall);
			nqfdbput(fdb, entry->d_name, fm);
			if (entry->d_name[5] == '2' && entry->d_name[6] == '4' && entry->d_name[7] == '4')
				memcpy(todel, entry->d_name, 32);
			printf("file: %s loaded\n", entry->d_name);
			i++;
			if (i % 50 == 0)
			{
				printf("start indexing FDB...\n");
				nqfdbidx(fdb);
			}
			if (i % 5 == 0)
			{
				printf("start indexing BWDB...\n");
				nqbwdbidx(bwdb);
			}
		}
	}
	printf("start indexing FDB...\n");
	nqfdbidx(fdb);
	printf("start merging index of BWDB...\n");
	nqbwdbmgidx(bwdb, 25);

	printf("extract features from two samples\n");
	IplImage* timage1 = cvLoadImage("../dpdb.jpg", CV_LOAD_IMAGE_COLOR);
	IplImage* tsmall1 = cvCreateImage(cvSize(640, 480), 8, 3);
	IplImage* dtimage1 = cvLoadImage("../dpdb.jpg", CV_LOAD_IMAGE_GRAYSCALE);
	IplImage* dtsmall1 = cvCreateImage(cvSize(640, 480), 8, 1);
	cvResize(timage1, tsmall1, CV_INTER_AREA);
	cvResize(dtimage1, dtsmall1, CV_INTER_AREA);
	CvMat* dpm1 = nqdpnew(dtsmall1, cvSURFParams(1200, 0));
	CvMat* dpe1;
	if (dpm1 != 0)
	{
		dpe1 = nqbweplr(dpm1);
		cvReleaseMat(&dpm1);
	}
	CvMat* fm1 = nqlhnew(tsmall1, 512);
	cvReleaseImage(&timage1);
	cvReleaseImage(&tsmall1);
	cvReleaseImage(&dtimage1);
	cvReleaseImage(&dtsmall1);

	IplImage* timage2 = cvLoadImage("../fdb.jpg", CV_LOAD_IMAGE_COLOR);
	IplImage* tsmall2 = cvCreateImage(cvSize(640, 480), 8, 3);
	IplImage* dtimage2 = cvLoadImage("../fdb.jpg", CV_LOAD_IMAGE_GRAYSCALE);
	IplImage* dtsmall2 = cvCreateImage(cvSize(640, 480), 8, 1);
	cvResize(timage2, tsmall2, CV_INTER_AREA);
	cvResize(dtimage2, dtsmall2, CV_INTER_AREA);
	CvMat* dpm2 = nqdpnew(dtsmall2, cvSURFParams(1200, 0));
	CvMat* dpe2;
	if (dpm2 != 0)
	{
		dpe2 = nqbweplr(dpm2);
		cvReleaseMat(&dpm2);
	}
	CvMat* fm2 = nqlhnew(tsmall2, 512);
	cvReleaseImage(&timage2);
	cvReleaseImage(&tsmall2);
	cvReleaseImage(&dtimage2);
	cvReleaseImage(&dtsmall2);

	printf("test to del one record: %s\n", todel);
	nqfdbout(fdb, todel);
	printf("find likeness pair to dpdb.jpg and fdb.jpg:\n");
	char** kstr = (char**)malloc(sizeof(kstr[0])*20);
	float* likeness = (float*)malloc(sizeof(likeness[0])*20);
	double tt = (double)cvGetTickCount();
	NQQRY* qry = nqqrynew();
	qry->type = NQCTAND;
	qry->lmt = 20;
	qry->cnum = 3;
	qry->conds = (NQQRY**)malloc(sizeof(NQQRY*) * 3);
	NQQRY* fqry = qry->conds[0] = nqqrynew();
	fqry->type = NQTFDB;
	fqry->op = NQOPLIKE;
	fqry->db = fdb;
	fqry->sbj.desc = fm1;
	NQQRY* bwqry = qry->conds[1] = nqqrynew();
	bwqry->type = NQTBWDB;
	bwqry->op = NQOPLIKE;
	bwqry->db = bwdb;
	bwqry->sbj.desc = dpe1;
	NQQRY* tblqry = qry->conds[2] = nqqrynew();
	tblqry->db = tdb;
	tblqry->col = (void*)"model";
	tblqry->type = NQTTCTDB;
	tblqry->op = NQOPSTREQ;
	tblqry->sbj.str = "dsc-w1";
	/*
	NQQRY* tagqry = qry->conds[2] = nqqrynew();
	tagqry->db = wdb;
	tagqry->col = adb;
	tagqry->type = NQTTCWDB;
	tagqry->op = NQOPSTREQ;
	tagqry->sbj.str = "dsc007";
	*/
	/*
	NQQRY* subqry = qry->conds[2] = nqqrynew();
	subqry->type = NQCTOR;
	subqry->lmt = 20;
	subqry->cnum = 2;
	subqry->conds = (NQQRY**)malloc(sizeof(NQQRY*) * 2);
	NQQRY* subfqry = subqry->conds[0] = nqqrynew();
	subfqry->type = NQTFDB;
	subfqry->op = NQOPLIKE;
	subfqry->db = fdb;
	subfqry->sbj.desc = fm2;
	NQQRY* subbwqry = subqry->conds[1] = nqqrynew();
	subbwqry->type = NQTBWDB;
	subbwqry->op = NQOPLIKE;
	subbwqry->db = bwdb;
	subbwqry->sbj.desc = dpe2;
	*/
	int len;
	char* mem = (char*)nqqrydump(qry, &len);
	printf("query is encoded to %d long\n", len);
	NQQRY* newqry = nqqrynew(mem);
	printf("query is decoded\n");
	nqqrysearch(newqry);
	printf("retrieve query result\n");
	int k = nqqryresult(newqry, kstr, likeness);
	tt = (double)cvGetTickCount()-tt;
	printf( "search time = %gms\n", tt/(cvGetTickFrequency()*1000.));
	for (int i = 0; i < k; i++)
		printf("%s : %f\n", kstr[i], likeness[i]);

	nqfdbdel(fdb);

	/* Finally close the directory stream */
	closedir(dir);
	return 0;
}
