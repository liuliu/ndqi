/*
 * An OpenCV Implementation of gist descriptor
 * the implementation of gist in matlab code is a kind of gabor filter.
 * refer to: http://people.csail.mit.edu/torralba/code/spatialenvelope/
 * Author: Liu Liu
 * liuliu.1987+opencv@gmail.com
 */
#include "cvgist.h"

CvGaborFilter*
cvCreateGaborFilters( int orientations,
		      int scales,
		      int size )
{
	CvGaborFilter* gabors = (CvGaborFilter*)malloc( orientations*scales*sizeof(CvGaborFilter) );
	int k = 0;
	for ( int scale = 0; scale < scales; scale++ )
		for ( int orientation = 0; orientation < orientations; orientation++ )
		{
			gabors[k].orientation = orientation;
			gabors[k].scale = scale;
			gabors[k].size = size;
			CvMat* conv_real = gabors[k].conv_real = cvCreateMat( size, size, CV_32FC1 );
			CvMat* conv_img = gabors[k].conv_img = cvCreateMat( size, size, CV_32FC1 );
			double x, y;
			double kv = 3.141592654*exp(-0.34657359*(scale+2));
			double qu = orientation*3.141592654/orientations;
			double cos_qu = cos(qu);
			double sin_qu = sin(qu);
			double kv_kv_si = kv*kv*0.050660592;
			for ( int i = 0; i < size; i++ )
				for ( int j = 0; j < size; j++ )
				{
					x = i-(size-1)/2.;
					y = j-(size-1)/2.;
					double exp_kv_x_y = exp(-kv*kv*(x*x+y*y)*0.025330296);
					double kv_qu_x_y = kv*(cos_qu*x+sin_qu*y);
					cvmSet( conv_real, j, i, exp_kv_x_y*(cos(kv_qu_x_y)-0.000051723)*kv_kv_si );
					cvmSet( conv_img, j, i, exp_kv_x_y*(sin(kv_qu_x_y)-0.000051723)*kv_kv_si );
				}
			k++;
		}
	return gabors;
}

void
cvReleaseGaborFilters( CvGaborFilter** gabors,
		       int gabor_n )
{
	CvGaborFilter* old_gabors = *gabors;
	for (int i = 0; i < gabor_n; i++)
	{
		cvReleaseMat( &old_gabors[i].conv_real );
		cvReleaseMat( &old_gabors[i].conv_img );
	}
	free( old_gabors );
	*gabors = NULL;
}

void
cvCalcGist( float* gist,
	    const CvArr* _img,
	    CvGaborFilter* gabors,
	    int gabor_n )
{
	IplImage* img = (IplImage*)_img;
	CvMat* mat = cvCreateMat( img->height, img->width, CV_32FC1 );
	cvConvert( img, mat );
	CvMat* mat_real = cvCreateMat( img->height, img->width, CV_32FC1 );
	CvMat* mat_img = cvCreateMat( img->height, img->width, CV_32FC1 );
	float* iter_gist = gist;
	//cvNamedWindow("real", CV_WINDOW_AUTOSIZE);
	//IplImage* dis = cvCreateImage( cvSize( img->width, img->height ), IPL_DEPTH_8U, 1 );
	for ( int k = 0; k < gabor_n; k++ )
	{
		CvPoint center = cvPoint( gabors[k].size/2, gabors[k].size/2 );
		cvFilter2D( mat, mat_real, gabors[k].conv_real, center );
		cvFilter2D( mat, mat_img, gabors[k].conv_img, center );
		for ( int i = 0; i < 4; i++ )
			for ( int j = 0; j < 4; j++ )
			{
				int n = 0;
				float sum = 0, real_part, img_part;
				for ( int x = mat->cols*i/4; x < mat->cols*(i+1)/4; x++ )
					for ( int y = mat->rows*j/4; y < mat->rows*(j+1)/4; y++ )
					{
						real_part = cvGetReal2D( mat_real, y, x );
						img_part = cvGetReal2D( mat_img, y, x );
						sum+=cvSqrt( real_part*real_part+img_part*img_part );
						n++;
					}
				*iter_gist = sum/n;
				iter_gist++;
			}
		/*
		for (int i = 0; i < img->height; i++)
			for (int j = 0; j < img->width; j++)
			{
				float ve = cvGetReal2D(mat_real, i, j);

				cvSetReal2D( (IplImage*)dis, i, j, (int)ve+128 );
			}
		cvShowImage("real", dis);
		cvWaitKey(0);
		*/
	}
	cvReleaseMat( &mat_img );
	cvReleaseMat( &mat_real );
	cvReleaseMat( &mat );
}

float
cvCompareGist( float* gist_a,
	       float* gist_b,
	       int length )
{
	double s1 = 0, s11 = 0, s2 = 0, s22 = 0, s12 = 0, a = 0, b = 0;
	float *iter_gist_a = gist_a, *iter_gist_b = gist_b;
	for( int i = 0; i < length; i++ )
	{
		a = *iter_gist_a;
		b = *iter_gist_b;

		s12+=a*b;
		s1+=a;
		s11+=a*a;
		s2+=b;
		s22+=b*b;

		iter_gist_a++;
		iter_gist_b++;
	}

	double scale = 1./length,
	       s11_s1 = s11-s1*s1*scale,
	       s22_s2 = s22-s2*s2*scale,
	       num = s12-s1*s2*scale;
	if (( fabs(num) <= DBL_EPSILON )&&( fabs(s11_s1) <= DBL_EPSILON )&&( fabs(s22_s2) <= DBL_EPSILON ))
		return 1;

	if ( fabs(s11_s1) <= DBL_EPSILON )
		s11_s1 = 1;
	if ( fabs(s22_s2) <= DBL_EPSILON )
		s22_s2 = 1;
	double denom2 = sqrt(s11_s1*s22_s2);

	return ((fabs(denom2) > DBL_EPSILON) ? num/denom2 : 1);
	/*
	float sum = 0;
	float *iter_gist_a = gist_a, *iter_gist_b = gist_b;
	for ( int i = 0; i < length; i++ )
	{
		sum+=(*iter_gist_a-*iter_gist_b)*(*iter_gist_a-*iter_gist_b);

		iter_gist_a++;
		iter_gist_b++;
	}
	return cvSqrt(sum);
	*/
}
