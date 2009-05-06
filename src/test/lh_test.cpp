#include "../nqlh.h"

#include "highgui.h"
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

int main()
{
	IplImage* timage = cvLoadImage("../testdata/dpdb.jpg", CV_LOAD_IMAGE_COLOR);
	CvMat* desc = nqlhnew(timage, 512);
	for (int i = 0; i < 512; i++)
		printf("%f ", desc->data.fl[i]);
	printf("\n");
	return 0;
}
