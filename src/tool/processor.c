#include "../lib/uuid.h"
#include "../lib/frl_base.h"

#include <highgui.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "ERROR: you must run with an argument.\n");
		return 1;
	}

	apr_initialize();
	apr_pool_t* mempool;
	apr_pool_create(&mempool, NULL);

	/* string to hold directory name */
	const char* dirname = argv[1];

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
	while ((entry = readdir(dir)) != NULL)
	{
		/* only list regular files... */
		stat(entry->d_name, &statinfo);
		/* ...so test for this */
		if (S_ISREG(statinfo.st_mode))
		{
			IplImage* image = cvLoadImage(entry->d_name, CV_LOAD_IMAGE_COLOR);
			CvMat imghdr, *img = cvGetMat(image, &imghdr);
			char* uuid = cvUUIDCreate(img);
			int width = (image->width > image->height) ? 800 : image->width * 800 / image->height;
			int height = (image->height > image->width) ? 800 : image->height * 800 / image->width;
			IplImage* small = cvCreateImage(cvSize(width, height), 8, 3);
			cvResize(image, small, CV_INTER_AREA);
			char filename[] = "/home/liu/store/lossless/######################.png";
			char* pch = strchr(filename, '#');
			strncpy(pch, uuid, 22);
			printf("%s\n", filename);
			cvSaveImage(filename, small);
			char filename_o[] = "/home/liu/store/raw/######################.jpg";
			char* pch_o = strchr(filename_o, '#');
			strncpy(pch_o, uuid, 22);
			apr_file_copy(entry->d_name, filename_o, APR_FILE_SOURCE_PERMS, mempool);
		}
	}

	return 0;
}
