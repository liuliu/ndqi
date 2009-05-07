#include "../nqfdb.h"
#include "../nqlh.h"

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
			CvMat* fm = nqlhnew(small, 512);
			cvReleaseImage(&image);
			cvReleaseImage(&small);
			nqfdbput(fdb, entry->d_name, fm);
			if (entry->d_name[5] == '2' && entry->d_name[6] == '4' && entry->d_name[7] == '4')
				memcpy(todel, entry->d_name, 32);
			printf("file: %s loaded\n", entry->d_name);
			i++;
			if (i % 50 == 0)
			{
				printf("start indexing...\n");
				nqfdbidx(fdb);
			}
		}
	}

	nqfdbreidx(fdb);

	IplImage* timage = cvLoadImage("../dpdb.jpg", CV_LOAD_IMAGE_COLOR);
	IplImage* tsmall = cvCreateImage(cvSize(640, 480), 8, 3);
	cvResize(timage, tsmall, CV_INTER_AREA);
	CvMat* fm = nqlhnew(tsmall, 512);
	cvReleaseImage(&timage);
	cvReleaseImage(&tsmall);

	printf("test to del one record: %s\n", todel);
	nqfdbout(fdb, todel);
	printf("find likeness pair to dpdb.jpg:\n");
	char** kstr = (char**)malloc(sizeof(kstr[0])*10);
	float* likeness = (float*)malloc(sizeof(likeness[0])*10);
	double tt = (double)cvGetTickCount();
	int k = nqfdbsearch(fdb, fm, kstr, 10, 1, likeness);
	tt = (double)cvGetTickCount()-tt;
	printf( "search time = %gms\n", tt/(cvGetTickFrequency()*1000.));
	for (int i = 0; i < k; i++)
		printf("%s : %f\n", kstr[i], likeness[i]);

	nqfdbdel(fdb);

	/* Finally close the directory stream */
	closedir(dir);
	return 0;
}
