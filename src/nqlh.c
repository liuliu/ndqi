/********************************************************
 * The Local Histogram API of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#include "nqlh.h"
#include "lib/cvlocalhist.h"

CvMat* nqlhnew(CvArr* image, int bins)
{
	IplImage* h_plane = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);
	IplImage* s_plane = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);
	IplImage* hsv = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 3);
	cvCvtColor(image, hsv, CV_BGR2HSV);
	cvCvtPixToPlane(hsv, h_plane, s_plane, 0, 0);
	cvReleaseImage(&hsv);
	CvMat* desc = cvCreateMat(1, bins, CV_32FC1);
	int i, step;
	CvSize size;
	uchar* ptr;
	cvGetImageRawData(h_plane, &ptr, &step, &size);
	step = step * size.height;
	for (i = 0; i < step; i++, ptr++)
		*ptr = *ptr * 255 / 180;
	CvMat hd = cvMat(1, bins >> 1, CV_32FC1, desc->data.fl);
	CvMat sd = cvMat(1, bins >> 1, CV_32FC1, desc->data.fl + (bins >> 1));
	cvCalcLocalHist(h_plane, hd.data.i, bins >> 3);
	cvCalcLocalHist(s_plane, sd.data.i, bins >> 3);
	for (i = 0; i < bins >> 1; i++)
	{
		hd.data.fl[i] = (float)hd.data.i[i];
		sd.data.fl[i] = (float)sd.data.i[i];
	}
	cvNormalize(&hd, &hd);
	cvNormalize(&sd, &sd);
	cvReleaseImage(&h_plane);
	cvReleaseImage(&s_plane);
	return desc;
}
