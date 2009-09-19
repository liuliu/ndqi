#ifndef _GUARD_cvcgsolve_h_
#define _GUARD_cvcgsolve_h_

#include <cxcore.h>

typedef void (*CvMatOps) ( CvMat*, CvMat*, void* );

/* conjugate gradient method to solve AX = B, it has time complexity of O(n^2*m), should be faster than gaussian elimination (O(n^3)) */
double cvCGSolve( CvMat* A, CvMat* B, CvMat* X, CvTermCriteria term_crit = cvTermCriteria( CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 200, 1e-16 ) );

/* taken AOps rather than raw matrix A, AOps is for AX */
double cvCGSolve( CvMatOps AOps, void* userdata, CvMat* B, CvMat* X, CvTermCriteria term_crit = cvTermCriteria( CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 200, 1e-16 ) );

#endif
