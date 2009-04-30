/********************************************************
 * The Distinctive Point API of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#include "nqdp.h"

static CvMemStorage* storage = 0;

CvMat* nqdpnew(CvArr* image, CvSURFParams params)
{
	CvSeq *keypoints = 0, *descriptors = 0;
	if (storage == 0)
		storage = cvCreateMemStorage();
	CvMemStorage* chstorage = cvCreateChildMemStorage(storage);
	cvExtractSURF(image, 0, &keypoints, &descriptors, chstorage, params);
	if (keypoints == 0 || descriptors == 0)
		return NULL;
	int cols = params.extended ? 128 : 64;
	CvMat* desc = cvCreateMat(descriptors->total, cols, CV_32FC1);
	float* dptr = desc->data.fl;
	CvSeqReader reader;
    cvStartReadSeq(descriptors, &reader);
	int i;
	for (i = 0; i < descriptors->total; i++)
	{
		float* descriptor = (float*)reader.ptr;
		memcpy(dptr, descriptor, sizeof(float)*cols);
		CV_NEXT_SEQ_ELEM(reader.seq->elem_size, reader);
		dptr += cols;
	}
	cvReleaseMemStorage(&chstorage);
	return desc;
}
