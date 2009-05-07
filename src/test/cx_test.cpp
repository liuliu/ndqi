#include "../nqfdb.h"
#include "../nqlh.h"
#include "../nqbwdb.h"
#include "../nqdp.h"

#include "highgui.h"
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

apr_pool_t* mempool;

int main()
{
	apr_initialize();
	apr_pool_create(&mempool, NULL);
	apr_atomic_init(mempool);

	NQFDB* fdb = nqfdbnew();
	NQBWDB* bwdb = nqbwdbnew();

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
	int i = 0;
	while ((entry = readdir(dir)) != NULL)
	{
		/* only list regular files... */
		stat(entry->d_name, &statinfo);
		/* ...so test for this */
		if (S_ISREG(statinfo.st_mode))
		{
			printf("loading file: %s\n", entry->d_name);
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

	nqfdbidx(fdb);
	nqbwdbmgidx(bwdb, 25);

	IplImage* timage = cvLoadImage("../dpdb.jpg", CV_LOAD_IMAGE_COLOR);
	IplImage* tsmall = cvCreateImage(cvSize(640, 480), 8, 3);
	IplImage* dtimage = cvLoadImage("../dpdb.jpg", CV_LOAD_IMAGE_GRAYSCALE);
	IplImage* dtsmall = cvCreateImage(cvSize(640, 480), 8, 1);
	cvResize(timage, tsmall, CV_INTER_AREA);
	cvResize(dtimage, dtsmall, CV_INTER_AREA);
	CvMat* dpm = nqdpnew(dtsmall, cvSURFParams(1200, 0));
	CvMat* dpe;
	if (dpm != 0)
	{
		dpe = nqbweplr(dpm);
		cvReleaseMat(&dpm);
	}
	CvMat* fm = nqlhnew(tsmall, 512);
	cvReleaseImage(&timage);
	cvReleaseImage(&tsmall);
	cvReleaseImage(&dtimage);
	cvReleaseImage(&dtsmall);

	printf("test to del one record: %s\n", todel);
	nqfdbout(fdb, todel);
	printf("find likeness pair to dpdb.jpg:\n");
	char** kstr = (char**)malloc(sizeof(kstr[0])*20);
	float* likeness = (float*)malloc(sizeof(likeness[0])*20);
	double tt = (double)cvGetTickCount();
	/*
	int k = nqbwdbsearch(bwdb, dpe, kstr, 20, 1, likeness);
	printf("finish the first search %d\n", k);
	NQFDB* nfdb = nqfdbjoin(fdb, kstr, 20);
	printf("finish join operation\n");
	k = nqfdbsearch(nfdb, fm, kstr, 10, 1, likeness);
	nqfdbdel(nfdb);
	*/
	/*
	int k = nqfdbsearch(fdb, fm, kstr, 20, 1, likeness);
	printf("finish the first search %d\n", k);
	NQBWDB* nbwdb = nqbwdbjoin(bwdb, kstr, 20);
	printf("finish join operation\n");
	k = nqbwdbsearch(nbwdb, dpe, kstr, 10, 1, likeness);
	nqbwdbdel(nbwdb);
	*/
	/*
	int k = nqbwdbsearch(bwdb, dpe, kstr, 20, 1, likeness);
	printf("finish the first search %d\n", k);
	NQRDB* tdb = nqrdbnew();
	nqrdbput(tdb, kstr, 0, k);
	NQFDB* nfdb = nqfdbjoin(fdb, tdb);
	printf("finish join operation\n");
	k = nqfdbsearch(nfdb, fm, kstr, 10, 1, likeness);
	nqfdbdel(nfdb);
	*/
	int k = nqfdbsearch(fdb, fm, kstr, 20, 1, likeness);
	printf("finish the first search %d\n", k);
	NQRDB* tdb = nqrdbnew();
	nqrdbput(tdb, kstr, 0, k);
	NQBWDB* nbwdb = nqbwdbjoin(bwdb, tdb);
	printf("finish join operation\n");
	k = nqbwdbsearch(nbwdb, dpe, kstr, 10, 1, likeness);
	nqbwdbdel(nbwdb);
	tt = (double)cvGetTickCount()-tt;
	printf( "search time = %gms\n", tt/(cvGetTickFrequency()*1000.));
	for (int i = 0; i < k; i++)
		printf("%s : %f\n", kstr[i], likeness[i]);

	nqfdbdel(fdb);

	/* Finally close the directory stream */
	closedir(dir);
	return 0;
}

