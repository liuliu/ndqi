#include "../nqutil.h"
#include "../nqdpdb.h"
#include <highgui.h>

int main()
{
	IplImage* timage = cvLoadImage("dpdb.jpg", CV_LOAD_IMAGE_GRAYSCALE);
	IplImage* tsmall = cvCreateImage(cvSize(640, 480), 8, 1);
	cvResize(timage, tsmall, CV_INTER_AREA);
	CvMat* dpm = nqdpnew(tsmall, cvSURFParams(1200, 0));
	printf("before examplar op: %d\n", dpm->rows);
	int* ki = (int*)malloc(sizeof(int)*dpm->rows);
	double tt = (double)cvGetTickCount();
	int r = nqeplr(dpm, ki);
	tt = (double)cvGetTickCount()-tt;
	printf( "examplar time = %gms\n", tt/(cvGetTickFrequency()*1000.));
	printf("after examplar op: %d\n", r);
	return 0;
}
