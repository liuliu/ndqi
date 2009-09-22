#ifndef _GUARD_cvl1stomp_h_
#define _GUARD_cvl1stomp_h_

#include <cv.h>

typedef void (*CvSparseMatOps) ( CvMat* X, CvMat* Y, CvMat* I, void* );

int cvL1StOMPSolve( CvMat* A, CvMat* B, CvMat* X, double epsilon, CvTermCriteria so_term_crit = cvTermCriteria( CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 10, 1e-5 ), CvTermCriteria cg_term_crit = cvTermCriteria( CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 200, 1e-16 ) );

/* IMPORTANT: desired functionality of AOps and AtOps
 * AOps: Taken a Xj-length vector, output B-length vector, perform Y=AXj
 *       Xj and Y is not sparse vector. I is the vector denote 0 of unused space in A.
 *       you can see Xj as Xj=X(I), see example for more.
 * AtOps: Taken a B-length vector, output Xj-length vector, perform Yj=A'X
 *        X and Y is not sparse vector. I is the vector denote 0 of unused space in A.
 *        you can see Yj as Yj=Y(I), see example for more. */
int cvL1StOMPSolve( CvSparseMatOps AOps, CvSparseMatOps AtOps, void* userdata, CvMat* B, CvMat* X, double epsilon, CvTermCriteria so_term_crit = cvTermCriteria( CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 10, 1e-5 ), CvTermCriteria cg_term_crit = cvTermCriteria( CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 200, 1e-16 ) );

#endif
