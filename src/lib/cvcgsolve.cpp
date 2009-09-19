#include "cvcgsolve.h"

double cvCGSolve( CvMat* A, CvMat* B, CvMat* X, CvTermCriteria term_crit )
{
	cvZero( X );
	CvMat* R = cvCloneMat( B );
	double delta = cvDotProduct( R, R );
	CvMat* D = cvCloneMat( R );
	double delta0 = delta;
	double bestres = 1.;
	CvMat* BX = cvCloneMat( X );
	CvMat* Q = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	for ( int i = 0; i < term_crit.max_iter; i++ )
	{
		cvMatMul( A, D, Q );
		double alpha = delta / cvDotProduct( D, Q );
		cvScaleAdd( D, cvScalar(alpha), X, X );
		if ( (i + 1) % 50 == 0 )
		{
			cvMatMul( A, X, R );
			cvSub( B, R, R );
		} else {
			cvScaleAdd( Q, cvScalar(-alpha), R, R );
		}
		double deltaold = delta;
		delta = cvDotProduct( R, R );
		double beta = delta / deltaold;
		cvScaleAdd( D, cvScalar(beta), R, D );
		double res = delta / delta0;
		if ( res < bestres )
		{
			cvCopy( X, BX );
			bestres = res;
		}
		if ( delta < delta0 * term_crit.epsilon )
			break;
	}
	cvCopy( BX, X );
	cvReleaseMat( &R );
	cvReleaseMat( &D );
	cvReleaseMat( &BX );
	cvReleaseMat( &Q );
	return sqrt( bestres );
}

double cvCGSolve( CvMatOps AOps, void* userdata, CvMat* B, CvMat* X, CvTermCriteria term_crit )
{
	cvZero( X );
	CvMat* R = cvCloneMat( B );
	double delta = cvDotProduct( R, R );
	CvMat* D = cvCloneMat( R );
	double delta0 = delta;
	double bestres = 1.;
	CvMat* BX = cvCloneMat( X );
	CvMat* Q = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	for ( int i = 0; i < term_crit.max_iter; i++ )
	{
		AOps( D, Q, userdata );
		double alpha = delta / cvDotProduct( D, Q );
		cvScaleAdd( D, cvScalar(alpha), X, X );
		if ( (i + 1) % 50 == 0 )
		{
			AOps( X, R, userdata );
			cvSub( B, R, R );
		} else {
			cvScaleAdd( Q, cvScalar(-alpha), R, R );
		}
		double deltaold = delta;
		delta = cvDotProduct( R, R );
		double beta = delta / deltaold;
		cvScaleAdd( D, cvScalar(beta), R, D );
		double res = delta / delta0;
		if ( res < bestres )
		{
			cvCopy( X, BX );
			bestres = res;
		}
		if ( delta < delta0 * term_crit.epsilon )
			break;
	}
	cvCopy( BX, X );
	cvReleaseMat( &R );
	cvReleaseMat( &D );
	cvReleaseMat( &BX );
	cvReleaseMat( &Q );
	return sqrt( bestres );
}
