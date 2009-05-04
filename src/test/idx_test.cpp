#include "../nqbwdb.h"
#include "../nqdp.h"
#include "../lib/frl_util_md5.h"

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
			IplImage* image = cvLoadImage(entry->d_name, CV_LOAD_IMAGE_GRAYSCALE);
			IplImage* small = cvCreateImage(cvSize(640, 480), 8, 1);
			cvResize(image, small, CV_INTER_AREA);
			CvMat* dpm = nqdpnew(small, cvSURFParams(1200, 0));
			if (dpm != NULL)
			{
				CvMat* dpe = nqbweplr(dpm);
		printf("%d => %d\n", dpm->rows, dpe->rows);
				cvReleaseMat(&dpm);
				if (dpe != NULL)
				{
					nqbwdbput(bwdb, entry->d_name, dpe);
					if (entry->d_name[5] == '7' && entry->d_name[6] == '8' && entry->d_name[7] == '6')
						memcpy(todel, entry->d_name, 32);
					printf("file: %s loaded\n", entry->d_name);
					i++;
					if (i % 50 == 0)
						nqbwdbidx(bwdb);
				}
			}
		}
	}

	nqbwdbreidx(bwdb, 2);

	IplImage* timage = cvLoadImage("../dpdb.jpg", CV_LOAD_IMAGE_GRAYSCALE);
	IplImage* tsmall = cvCreateImage(cvSize(640, 480), 8, 1);
	cvResize(timage, tsmall, CV_INTER_AREA);
	CvMat* dpm = nqdpnew(tsmall, cvSURFParams(1200, 0));
	CvMat* dpe = nqbweplr(dpm);

	printf("test to del one record: %s\n", todel);
	nqbwdbout(bwdb, todel);
	printf("find likeness pair to dpdb.jpg:\n");
	char** kstr = (char**)malloc(sizeof(kstr[0])*10);
	float* likeness = (float*)malloc(sizeof(likeness[0])*10);
	double tt = (double)cvGetTickCount();
	int k = nqbwdbsearch(bwdb, dpe, kstr, 10, 1, likeness);
	tt = (double)cvGetTickCount()-tt;
	printf( "search time = %gms\n", tt/(cvGetTickFrequency()*1000.));
	for (int i = 0; i < k; i++)
		printf("%s : %f\n", kstr[i], likeness[i]);

	nqbwdbdel(bwdb);

	/* Finally close the directory stream */
	closedir(dir);
	return 0;
}

