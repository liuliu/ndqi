/********************************************************
 * Automatic Tagging API of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#include "nqauto.h"

TCLIST* nqocrnew(CvArr* image)
{
	return NULL;
}
TCMAP* nqkodnew(CvArr* image)
{
	static CvMemStorage* storage = cvCreateMemStorage(NULL);
	static CvHaarClassifierCascade* face = (CvHaarClassifierCascade*)cvLoad("data/haarcascades/haarcascade_frontalface_alt.xml", 0, 0, 0);

	TCMAP* cols = tcmapnew();
	char buf[1024];
	CvSeq* faces = cvHaarDetectObjects(image, face, storage, 1.1, 2, CV_HAAR_DO_CANNY_PRUNING, cvSize(30, 30));
	snprintf(buf, 1024, "%i", faces ? faces->total : 0);
	tcmapput(cols, "face.num", 8, buf, strlen(buf));
	cvClearMemStorage(storage);
	return cols;
}
