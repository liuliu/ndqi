#ifndef _GUARD_cvl1qc_h_
#define _GUARD_cvl1qc_h_

#include <cv.h>
#include "cvcgsolve.h"

/* taken A, B, X, minimize ||X||_{L1} with constraint: ||AX - B|| < \epsilon */
int cvL1QCSolve( CvMat* A, CvMat* B, CvMat* X, double epsilon, double mu = 10., CvTermCriteria lb_term_crit = cvTermCriteria( CV_TERMCRIT_EPS, 0, 1e-3 ), CvTermCriteria cg_term_crit = cvTermCriteria( CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 200, 1e-16 ) );

/* taken AOps, AtOps, it specially designed for large scale, AOps is for AX, AtOps is for A'X */
int cvL1QCSolve( CvMatOps AOps, CvMatOps AtOps, void* userdata, CvMat* B, CvMat* X, double epsilon, double mu = 10., CvTermCriteria lb_term_crit = cvTermCriteria( CV_TERMCRIT_EPS, 0, 1e-3 ), CvTermCriteria cg_term_crit = cvTermCriteria( CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 200, 1e-16 ) );

#endif
