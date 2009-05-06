/********************************************************
 * Gist API of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#include "nqgs.h"
#include "lib/cvgist.h"

static CvGaborFilter* gabors = cvCreateGaborFilters(6, 4, 48);

CvMat* nqgsnew(CvArr* image, int n)
{
	CvMat imghdr, *img = cvGetMat(image, &imghdr);
	int new_width = img->cols, new_height = img->rows;
	if (new_width > new_height)
	{
		new_width = new_width * 128 / new_height;
		new_height = 128;
	} else {
		new_height = new_height * 128 / new_width;
		new_width = 128;
	}
	IplImage* small = cvCreateImage(cvSize(new_width, new_height), IPL_DEPTH_8U, 1);
	cvResize(image, small, CV_INTER_AREA);
	CvMat* gist = cvCreateMat(1, n * 16, CV_32FC1);
	cvCalcGist(gist->data.fl, small, gabors, n);
	cvReleaseImage(&small);
	cvNormalize(gist, gist);
	return gist;
}
