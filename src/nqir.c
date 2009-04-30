/********************************************************
 * The Interest Region API of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#include "nqir.h"

static CvMemStorage* storage = 0;

CvMat* nqirnew(CvArr* image, CvMSERParams params)
{
	CvSeq* contours = 0;
	if (storage == 0)
		storage = cvCreateMemStorage();
	CvMemStorage* chstorage = cvCreateChildMemStorage(storage);
	cvExtractMSER(image, 0, &contours, chstorage, params);
	CvMat* desc = cvCreateMat(contours->total, 7, CV_32FC1);
	float* dptr = desc->data.fl;
	CvSeqReader reader;
    cvStartReadSeq(contours, &reader);
	int i;
	for (i = 0; i < contours->total; i++)
	{
		CvContour* contour = *(CvContour**)reader.ptr;
		CvMoments moments;
		contour->flags |= CV_SEQ_POLYGON;
		cvMoments(contour, &moments);
		CvHuMoments hu_moments;
		cvGetHuMoments(&moments, &hu_moments);
		double am, m[] = {hu_moments.hu1, hu_moments.hu2, hu_moments.hu3, hu_moments.hu4, hu_moments.hu5, hu_moments.hu6, hu_moments.hu7};
		int j, sm;
		for (j = 0; j < 7; j++)
		{
			am = fabs(m[j]);
			sm = (m[j] > 0) ? 1 : ((m[j] < 0) ? -1 : 0);
			*dptr = sm * log10(am);
			dptr++;
		}
		CV_NEXT_SEQ_ELEM(reader.seq->elem_size, reader);
	}
	cvReleaseMemStorage(&chstorage);
	return desc;
}
