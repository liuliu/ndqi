/*
 * An OpenCV Implementation of gist descriptor
 * the implementation of gist in matlab code is a kind of gabor filter.
 * refer to: http://people.csail.mit.edu/torralba/code/spatialenvelope/
 * Author: Liu Liu
 * liuliu.1987+opencv@gmail.com
 */
#ifndef GUARD_cvgist_h
#define GUARD_cvgist_h

#include <cv.h>

struct CvGaborFilter
{
	int orientation;
	int scale;
	int size;
	CvMat* conv_real;
	CvMat* conv_img;
};

CvGaborFilter* cvCreateGaborFilters( int orientations, int scales, int size );

void cvReleaseGaborFilters( CvGaborFilter** gabors, int gabor_n );

void cvCalcGist( float* gist, const CvArr* _img, CvGaborFilter* gabors, int gabor_n );

float cvCompareGist( float* gist_a, float* gist_b, int length );

#endif
