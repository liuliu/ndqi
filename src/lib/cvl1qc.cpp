#include "cvl1qc.h"

static int icvL1QCNewton( CvMat* A, CvMat* B, CvMat* X, CvMat* U, double epsilon, double tau, CvTermCriteria nt_term_crit, CvTermCriteria cg_term_crit )
{
	const double alpha = .01;
	const double beta = .5;

	CvMat* R = cvCreateMat( B->rows, B->cols, CV_MAT_TYPE(B->type) );
	cvGEMM( A, X, 1, B, -1, R );
	CvMat* fu1 = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* fu2 = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* lfu1 = cvCreateMat( fu1->rows, fu1->cols, CV_MAT_TYPE(fu1->type) );
	CvMat* lfu2 = cvCreateMat( fu2->rows, fu2->cols, CV_MAT_TYPE(fu2->type) );
	cvSub( U, X, lfu1 );
	cvAdd( X, U, lfu2 );
	cvSubRS( lfu1, cvScalar(0), fu1 );
	cvSubRS( lfu2, cvScalar(0), fu2 );
	double epsilon2 = epsilon * epsilon;
	double tau_inv = 1. / tau;
	double fe = .5 * (cvDotProduct( R, R ) - epsilon2);
	double fe_inv = 1. / fe;
	cvLog( lfu1, lfu1 );
	cvLog( lfu2, lfu2 );
	CvScalar sumU = cvSum( U );
	CvScalar sumfu1 = cvSum( lfu1 );
	CvScalar sumfu2 = cvSum( lfu2 );
	double f = sumU.val[0] - tau_inv * (sumfu1.val[0] + sumfu2.val[0] + log(-fe));

	CvMat* atr = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* ntgx = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* ntgu = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* sig1211 = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* sigx = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* w1 = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* AtA = cvCreateMat( A->cols, A->cols, CV_MAT_TYPE(A->type) );
	CvMat* H11 = cvCreateMat( A->cols, A->cols, CV_MAT_TYPE(A->type) );
	CvMat* du = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* pX = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* pU = cvCreateMat( U->rows, U->cols, CV_MAT_TYPE(U->type) );
	CvMat* pR = cvCreateMat( R->rows, R->cols, CV_MAT_TYPE(R->type) );
	CvMat* pfu1 = cvCreateMat( fu1->rows, fu1->cols, CV_MAT_TYPE(fu1->type) );
	CvMat* pfu2 = cvCreateMat( fu2->rows, fu2->cols, CV_MAT_TYPE(fu2->type) );
	CvMat* Adx = cvCreateMat( B->rows, B->cols, CV_MAT_TYPE(B->type) );
	CvMat* dx = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );

	int result = nt_term_crit.max_iter;

	int t, i;

	for ( t = 0; t < nt_term_crit.max_iter; ++t )
	{
		cvGEMM( A, R, 1, NULL, 0, atr, CV_GEMM_A_T );
		cvGEMM( A, A, 1, NULL, 0, AtA, CV_GEMM_A_T );
		cvGEMM( atr, atr, 1, NULL, 0, H11, CV_GEMM_B_T );
		double* atrp = atr->data.db;
		double* fu1p = fu1->data.db;
		double* fu2p = fu2->data.db;
		double* ntgxp = ntgx->data.db;
		double* ntgup = ntgu->data.db;
		double* sig1211p = sig1211->data.db;
		double* sigxp = sigx->data.db;
		double* w1p = w1->data.db;
		double* dup = du->data.db;
		for ( i = 0; i < X->rows; ++i, ++atrp, ++fu1p, ++fu2p, ++ntgxp, ++ntgup, ++sig1211p, ++sigxp, ++w1p, ++dup )
		{
			double fu1_inv = 1. / (*fu1p);
			double fu2_inv = 1. / (*fu2p);
			double ntgxv = fu1_inv - fu2_inv + fe_inv * (*atrp);
			double ntguv = -tau - fu1_inv - fu2_inv;
			double sig11 = fu1_inv * fu1_inv + fu2_inv * fu2_inv;
			double sig12 = -fu1_inv * fu1_inv + fu2_inv * fu2_inv;
			*sig1211p = sig12 / sig11;
			*sigxp = sig11 - sig12 * (*sig1211p);
			*w1p = ntgxv - (*sig1211p) * ntguv;
			*ntgxp = -tau_inv * ntgxv;
			*ntgup = -tau_inv * ntguv;
			*dup = ntguv / sig11;
		}
		cvAddWeighted( AtA, -fe_inv, H11, -fe_inv * fe_inv, 0, H11 );
		sigxp = sigx->data.db;
		double* H11p = H11->data.db;
		for ( i = 0; i < A->cols; ++i, ++sigxp, H11p += A->cols + 1 )
			*H11p += *sigxp;
		if ( cvCGSolve( H11, w1, dx, cg_term_crit ) > .5 )
		{
			result = t;
			goto __clean_up__;
		}
		cvMatMul( A, dx, Adx );
		dup = du->data.db;
		sig1211p = sig1211->data.db;
		double* dxp = dx->data.db;
		for ( i = 0; i < X->rows; ++i, ++dup, ++sig1211p, ++dxp )
			*dup -= (*sig1211p) * (*dxp);

		/* minimum step size that stays in the interior */
		double aqe = cvDotProduct( Adx, Adx );
		double bqe = 2. * cvDotProduct( R, Adx );
		double cqe = cvDotProduct( R, R ) - epsilon2;
		double smax = MIN( 1, -bqe + sqrt( bqe * bqe - 4 * aqe * cqe ) / (2 * aqe) );
		dup = du->data.db;
		dxp = dx->data.db;
		fu1p = fu1->data.db;
		fu2p = fu2->data.db;
		for ( i = 0; i < X->rows; ++i, ++dup, ++dxp, ++fu1p, ++fu2p )
		{
			if ( (*dxp) - (*dup) > 0 )
				smax = MIN( smax, -(*fu1p) / ((*dxp) - (*dup)) );
			if ( (*dxp) + (*dup) < 0 )
				smax = MIN( smax, (*fu2p) / ((*dxp) + (*dup)) );
		}
		smax *= .99;

		/* backtracking line search */
		bool suffdec = 0;
		int backiter = 0;
		double fep = fe;
		double fp = f;
		double lambda2;
		while (!suffdec)
		{
			cvAddWeighted( X, 1, dx, smax, 0, pX );
			cvAddWeighted( U, 1, du, smax, 0, pU );
			cvAddWeighted( R, 1, Adx, smax, 0, pR );
			cvSub( pU, pX, lfu1 );
			cvAdd( pX, pU, lfu2 );
			cvSubRS( lfu1, cvScalar(0), pfu1 );
			cvSubRS( lfu2, cvScalar(0), pfu2 );
			fep = .5 * (cvDotProduct( pR, pR ) - epsilon2);
			cvLog( lfu1, lfu1 );
			cvLog( lfu2, lfu2 );
			CvScalar sumpU = cvSum( pU );
			CvScalar sumpfu1 = cvSum( pfu1 );
			CvScalar sumpfu2 = cvSum( pfu2 );
			fp = sumpU.val[0] - tau_inv * (sumpfu1.val[0] + sumpfu2.val[0] + log(-fep));
			lambda2 = cvDotProduct( ntgx, dx ) + cvDotProduct( ntgu, du );
			double flin = f + alpha * smax * lambda2;
			suffdec = (fp <= flin);
			smax = beta * smax;
			++backiter;
			if ( backiter > 32 )
			{
				result = t;
				goto __clean_up__;
			}
		}

		/* set up for next iteration */
		cvCopy( pX, X );
		cvCopy( pU, U );
		cvCopy( pR, R );
		cvCopy( pfu1, fu1 );
		cvCopy( pfu2, fu2 );
		fe = fep;
		fe_inv = 1. / fe;
		f = fp;
		lambda2 = -lambda2 * .5;
		if ( lambda2 < nt_term_crit.epsilon )
		{
			result = t + 1;
			break;
		}
	}

__clean_up__:

	cvReleaseMat( &pfu2 );
	cvReleaseMat( &pfu1 );
	cvReleaseMat( &pR );
	cvReleaseMat( &pU );
	cvReleaseMat( &pX );
	cvReleaseMat( &dx );
	cvReleaseMat( &Adx );
	cvReleaseMat( &du );
	cvReleaseMat( &H11 );
	cvReleaseMat( &AtA );
	cvReleaseMat( &w1 );
	cvReleaseMat( &sigx );
	cvReleaseMat( &sig1211 );
	cvReleaseMat( &ntgu );
	cvReleaseMat( &ntgx );
	cvReleaseMat( &lfu2 );
	cvReleaseMat( &lfu1 );
	cvReleaseMat( &fu2 );
	cvReleaseMat( &fu1 );
	cvReleaseMat( &R );
	return result;
}

int cvL1QCSolve( CvMat* A, CvMat* B, CvMat* X, double epsilon, double mu, CvTermCriteria lb_term_crit, CvTermCriteria cg_term_crit )
{
	CvMat* AAt = cvCreateMat( A->rows, A->rows, CV_MAT_TYPE(A->type) );
	cvGEMM( A, A, 1, NULL, 0, AAt, CV_GEMM_B_T );
	CvMat* W = cvCreateMat( A->rows, 1, CV_MAT_TYPE(X->type) );
	if ( cvCGSolve( AAt, B, W, cg_term_crit ) > .5 )
	{
		cvReleaseMat( &W );
		cvReleaseMat( &AAt );
		return -1;
	}
	cvGEMM( A, W, 1, NULL, 0, X, CV_GEMM_A_T );
	cvReleaseMat( &W );
	cvReleaseMat( &AAt );

	CvMat* U = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	cvAbsDiffS( X, U, cvScalar(0) );
	CvScalar sumAbsX = cvSum( U );
	double minAbsX, maxAbsX;
	cvMinMaxLoc( U, &minAbsX, &maxAbsX );
	cvConvertScale( U, U, .95, maxAbsX * .1 );

	double tau = MAX( (2 * X->rows + 1) / sumAbsX.val[0], 1 );

	if ( !(lb_term_crit.type & CV_TERMCRIT_ITER) )
		lb_term_crit.max_iter = ceil( (log(2 * X->rows + 1) - log(lb_term_crit.epsilon) - log(tau)) / log(mu) );

	CvTermCriteria nt_term_crit = cvTermCriteria( CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 50, lb_term_crit.epsilon );

	for ( int i = 0; i < lb_term_crit.max_iter; ++i )
	{
		icvL1QCNewton( A, B, X, U, epsilon, tau, nt_term_crit, cg_term_crit );
		tau *= mu;
	}

	cvReleaseMat( &U );

	return 0;
}

typedef struct {
	CvMatOps AOps;
	CvMatOps AtOps;
	CvMat* AR;
	CvMat* AtR;
	void* userdata;
} CvAAtOpsData;

static void icvAAtOps( CvMat* X, CvMat* Y, void* userdata )
{
	CvAAtOpsData* data = (CvAAtOpsData*)userdata;
	data->AtOps( X, data->AtR, data->userdata );
	data->AOps( data->AtR, Y, data->userdata );
}

typedef struct {
	CvMatOps AOps;
	CvMatOps AtOps;
	CvMat* AR;
	CvMat* AtR;
	CvMat* tX;
	CvMat* sigx;
	CvMat* atr;
	double fe_inv;
	double fe_inv_2;
	void* userdata;
} CvH11OpsData;

static void icvH11Ops( CvMat* X, CvMat* Y, void* userdata )
{
	CvH11OpsData* h11 = (CvH11OpsData*)userdata;
	h11->AOps( X, h11->AR, h11->userdata );
	h11->AtOps( h11->AR, h11->AtR, h11->userdata );
	double rc = h11->fe_inv_2 * cvDotProduct( h11->atr, X );
	cvAddWeighted( h11->AtR, -h11->fe_inv, h11->atr, rc, 0, h11->AtR );
	cvMul( h11->sigx, X, h11->tX );
	cvAdd( h11->tX, h11->AtR, Y );
}

static int icvL1QCNewton( CvAAtOpsData& AAtData, CvMat* B, CvMat* X, CvMat* U, double epsilon, double tau, CvTermCriteria nt_term_crit, CvTermCriteria cg_term_crit )
{
	const double alpha = .01;
	const double beta = .5;

	CvMat* R = cvCreateMat( B->rows, B->cols, CV_MAT_TYPE(B->type) );
	AAtData.AOps( X, AAtData.AR, AAtData.userdata );
	cvSub( AAtData.AR, B, R );
	CvMat* fu1 = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* fu2 = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* lfu1 = cvCreateMat( fu1->rows, fu1->cols, CV_MAT_TYPE(fu1->type) );
	CvMat* lfu2 = cvCreateMat( fu2->rows, fu2->cols, CV_MAT_TYPE(fu2->type) );
	cvSub( U, X, lfu1 );
	cvAdd( X, U, lfu2 );
	cvSubRS( lfu1, cvScalar(0), fu1 );
	cvSubRS( lfu2, cvScalar(0), fu2 );
	double epsilon2 = epsilon * epsilon;
	double tau_inv = 1. / tau;
	double fe = .5 * (cvDotProduct( R, R ) - epsilon2);
	double fe_inv = 1. / fe;
	cvLog( lfu1, lfu1 );
	cvLog( lfu2, lfu2 );
	CvScalar sumU = cvSum( U );
	CvScalar sumfu1 = cvSum( lfu1 );
	CvScalar sumfu2 = cvSum( lfu2 );
	double f = sumU.val[0] - tau_inv * (sumfu1.val[0] + sumfu2.val[0] + log(-fe));

	CvMat* atr = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* ntgx = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* ntgu = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* sig1211 = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* sigx = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* w1 = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* du = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* pX = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* pU = cvCreateMat( U->rows, U->cols, CV_MAT_TYPE(U->type) );
	CvMat* pR = cvCreateMat( R->rows, R->cols, CV_MAT_TYPE(R->type) );
	CvMat* pfu1 = cvCreateMat( fu1->rows, fu1->cols, CV_MAT_TYPE(fu1->type) );
	CvMat* pfu2 = cvCreateMat( fu2->rows, fu2->cols, CV_MAT_TYPE(fu2->type) );
	CvMat* Adx = cvCreateMat( B->rows, B->cols, CV_MAT_TYPE(B->type) );
	CvMat* dx = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* tX = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );

	int result = nt_term_crit.max_iter;

	CvH11OpsData H11OpsData;
	H11OpsData.AOps = AAtData.AOps;
	H11OpsData.AtOps = AAtData.AtOps;
	H11OpsData.AR = AAtData.AR;
	H11OpsData.AtR = AAtData.AtR;
	H11OpsData.userdata = AAtData.userdata;
	H11OpsData.tX = tX;
	H11OpsData.atr = atr;
	H11OpsData.sigx = sigx;

	int t, i;

	for ( t = 0; t < nt_term_crit.max_iter; ++t )
	{
		AAtData.AtOps( R, atr, AAtData.userdata );
		double* atrp = atr->data.db;
		double* fu1p = fu1->data.db;
		double* fu2p = fu2->data.db;
		double* ntgxp = ntgx->data.db;
		double* ntgup = ntgu->data.db;
		double* sig1211p = sig1211->data.db;
		double* sigxp = sigx->data.db;
		double* w1p = w1->data.db;
		double* dup = du->data.db;
		for ( i = 0; i < X->rows; ++i, ++atrp, ++fu1p, ++fu2p, ++ntgxp, ++ntgup, ++sig1211p, ++sigxp, ++w1p, ++dup )
		{
			double fu1_inv = 1. / (*fu1p);
			double fu2_inv = 1. / (*fu2p);
			double ntgxv = fu1_inv - fu2_inv + fe_inv * (*atrp);
			double ntguv = -tau - fu1_inv - fu2_inv;
			double sig11 = fu1_inv * fu1_inv + fu2_inv * fu2_inv;
			double sig12 = -fu1_inv * fu1_inv + fu2_inv * fu2_inv;
			*sig1211p = sig12 / sig11;
			*sigxp = sig11 - sig12 * (*sig1211p);
			*w1p = ntgxv - (*sig1211p) * ntguv;
			*ntgxp = -tau_inv * ntgxv;
			*ntgup = -tau_inv * ntguv;
			*dup = ntguv / sig11;
		}
		H11OpsData.fe_inv = fe_inv;
		H11OpsData.fe_inv_2 = fe_inv * fe_inv;
		if ( cvCGSolve( icvH11Ops, &H11OpsData, w1, dx, cg_term_crit ) > .5 )
		{
			result = t;
			goto __clean_up__;
		}
		AAtData.AOps( dx, Adx, AAtData.userdata );
		dup = du->data.db;
		sig1211p = sig1211->data.db;
		double* dxp = dx->data.db;
		for ( i = 0; i < X->rows; ++i, ++dup, ++sig1211p, ++dxp )
			*dup -= (*sig1211p) * (*dxp);

		/* minimum step size that stays in the interior */
		double aqe = cvDotProduct( Adx, Adx );
		double bqe = 2. * cvDotProduct( R, Adx );
		double cqe = cvDotProduct( R, R ) - epsilon2;
		double smax = MIN( 1, -bqe + sqrt( bqe * bqe - 4 * aqe * cqe ) / (2 * aqe) );
		dup = du->data.db;
		dxp = dx->data.db;
		fu1p = fu1->data.db;
		fu2p = fu2->data.db;
		for ( i = 0; i < X->rows; ++i, ++dup, ++dxp, ++fu1p, ++fu2p )
		{
			if ( (*dxp) - (*dup) > 0 )
				smax = MIN( smax, -(*fu1p) / ((*dxp) - (*dup)) );
			if ( (*dxp) + (*dup) < 0 )
				smax = MIN( smax, (*fu2p) / ((*dxp) + (*dup)) );
		}
		smax *= .99;

		/* backtracking line search */
		bool suffdec = 0;
		int backiter = 0;
		double fep = fe;
		double fp = f;
		double lambda2;
		while (!suffdec)
		{
			cvAddWeighted( X, 1, dx, smax, 0, pX );
			cvAddWeighted( U, 1, du, smax, 0, pU );
			cvAddWeighted( R, 1, Adx, smax, 0, pR );
			cvSub( pU, pX, lfu1 );
			cvAdd( pX, pU, lfu2 );
			cvSubRS( lfu1, cvScalar(0), pfu1 );
			cvSubRS( lfu2, cvScalar(0), pfu2 );
			fep = .5 * (cvDotProduct( pR, pR ) - epsilon2);
			cvLog( lfu1, lfu1 );
			cvLog( lfu2, lfu2 );
			CvScalar sumpU = cvSum( pU );
			CvScalar sumpfu1 = cvSum( pfu1 );
			CvScalar sumpfu2 = cvSum( pfu2 );
			fp = sumpU.val[0] - tau_inv * (sumpfu1.val[0] + sumpfu2.val[0] + log(-fep));
			lambda2 = cvDotProduct( ntgx, dx ) + cvDotProduct( ntgu, du );
			double flin = f + alpha * smax * lambda2;
			suffdec = (fp <= flin);
			smax = beta * smax;
			++backiter;
			if ( backiter > 32 )
			{
				result = t;
				goto __clean_up__;
			}
		}

		/* set up for next iteration */
		cvCopy( pX, X );
		cvCopy( pU, U );
		cvCopy( pR, R );
		cvCopy( pfu1, fu1 );
		cvCopy( pfu2, fu2 );
		fe = fep;
		fe_inv = 1. / fe;
		f = fp;
		lambda2 = -lambda2 * .5;
		if ( lambda2 < nt_term_crit.epsilon )
		{
			result = t + 1;
			break;
		}
	}

__clean_up__:

	cvReleaseMat( &pfu2 );
	cvReleaseMat( &pfu1 );
	cvReleaseMat( &pR );
	cvReleaseMat( &pU );
	cvReleaseMat( &pX );
	cvReleaseMat( &tX );
	cvReleaseMat( &dx );
	cvReleaseMat( &Adx );
	cvReleaseMat( &du );
	cvReleaseMat( &w1 );
	cvReleaseMat( &sigx );
	cvReleaseMat( &sig1211 );
	cvReleaseMat( &ntgu );
	cvReleaseMat( &ntgx );
	cvReleaseMat( &lfu2 );
	cvReleaseMat( &lfu1 );
	cvReleaseMat( &fu2 );
	cvReleaseMat( &fu1 );
	cvReleaseMat( &R );
	return result;
}

int cvL1QCSolve( CvMatOps AOps, CvMatOps AtOps, void* userdata, CvMat* B, CvMat* X, double epsilon, double mu, CvTermCriteria lb_term_crit, CvTermCriteria cg_term_crit )
{
	CvMat* Z = cvCreateMat( X->rows, 1, CV_MAT_TYPE(X->type) );
	CvMat* W = cvCreateMat( B->rows, 1, CV_MAT_TYPE(B->type) );
	CvAAtOpsData AAtData;
	AAtData.AOps = AOps;
	AAtData.AtOps = AtOps;
	AAtData.AtR = Z;
	AAtData.userdata = userdata;
	if ( cvCGSolve( icvAAtOps, &AAtData, B, W, cg_term_crit ) > .5 )
	{
		cvReleaseMat( &W );
		cvReleaseMat( &Z );
		return -1;
	}
	AtOps( W, X, userdata );
	AAtData.AR = W;

	CvMat* U = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	cvAbsDiffS( X, U, cvScalar(0) );
	CvScalar sumAbsX = cvSum( U );
	double minAbsX, maxAbsX;
	cvMinMaxLoc( U, &minAbsX, &maxAbsX );
	cvConvertScale( U, U, .95, maxAbsX * .1 );

	double tau = MAX( (2 * X->rows + 1) / sumAbsX.val[0], 1 );

	if ( !(lb_term_crit.type & CV_TERMCRIT_ITER) )
		lb_term_crit.max_iter = ceil( (log(2 * X->rows + 1) - log(lb_term_crit.epsilon) - log(tau)) / log(mu) );

	CvTermCriteria nt_term_crit = cvTermCriteria( CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 50, lb_term_crit.epsilon );

	int totaliter = 0;
	for ( int i = 0; i < lb_term_crit.max_iter; ++i )
	{
		totaliter += icvL1QCNewton( AAtData, B, X, U, epsilon, tau, nt_term_crit, cg_term_crit );
		tau *= mu;
	}

	cvReleaseMat( &U );
	cvReleaseMat( &W );
	cvReleaseMat( &Z );

	return 0;
}
