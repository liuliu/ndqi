#include "cvl1stomp.h"
#include "cvcgsolve.h"
#include "cxmisc.h"

struct CvFDR {
	double val;
	int i;
};

#define cmp_fdr(fdr1, fdr2) \
	((fdr1).val < (fdr2).val)

static CV_IMPLEMENT_QSORT( icvQSortFDR, CvFDR, cmp_fdr )

int cvL1StOMPSolve( CvMat* A, CvMat* B, CvMat* X, double epsilon, CvTermCriteria so_term_crit, CvTermCriteria cg_term_crit )
{
	int result = 0;

	cvZero( X );
	CvMat* corr = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* res = cvCloneMat( B );
	CvMat* I = cvCreateMat( X->rows, X->cols, CV_32SC1 );
	cvZero( I );
	CvMat* Aj = cvCreateMat( A->rows, A->cols, CV_MAT_TYPE(A->type) );
	CvMat* AtA = cvCreateMat( A->cols, A->cols, CV_MAT_TYPE(A->type) );
	CvMat* AtB = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	cvGEMM( A, B, 1, NULL, 0, AtB, CV_GEMM_A_T );
	CvMat* AjB = corr;
	CvMat* Xj = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	double normB = cvNorm( B );
	double normres = normB;
	double n2 = sqrt(B->rows);
	int ActiveSetSize = 0;
	CvFDR* fdr = (CvFDR*)cvAlloc( X->rows * sizeof(fdr[0]) );
	double sqrt2_inv = 1. / sqrt(2.);
	double epsilon_inv = X->rows / epsilon;

	for ( int i = 0; i < so_term_crit.max_iter; ++i )
	{
		corr->rows = X->rows;
		/* Compute residual correlations */
		cvGEMM( A, res, 1, NULL, 0, corr, CV_GEMM_A_T );
		cvAbs( corr, corr );

		/* FDR search for threshold */
		double* corrp = corr->data.db;
		CvFDR* fdrp = fdr;
		normres = n2 / normres;
		for ( int k = 0; k < X->rows; ++k, ++corrp, ++fdrp )
		{
			fdrp->val = *corrp;
			*corrp *= normres;
			/* precision problem in erfc function, formula:
			 * |c_{j}| > t*\sigma
			 * become |c_{j}| / \sigma *after* t selected */
			fdrp->i = k;
		}
		icvQSortFDR( fdr, X->rows, 0 );
		fdrp = fdr;
		bool good = false;
		double thres;
		for ( int k = X->rows - 1; k >= 0; --k, ++fdrp )
		{
			double efc = erfc( fdrp->val * sqrt2_inv ) * epsilon_inv;
			if ( efc <= k + 1 )
			{
				good = true;
				thres = fdr[k].val * normres;
				/* adjust threshold with \sigma */
				break;
			}
		}
		if ( !good )
			break;

		/* Apply hard thresholding */
		int J = 0;
		corrp = corr->data.db;
		int* Ip = I->data.i;
		for ( int k = 0; k < X->rows; ++k, ++corrp, ++Ip )
			if ( *corrp > thres && !(*Ip) )
			{
				++J;
				*Ip = 1;
			}
		if ( J == 0 )
			break;

		/* Generate active set params */
		ActiveSetSize += J;
		AjB->rows = Xj->rows = AtA->rows = AtA->cols = Aj->cols = ActiveSetSize;
		AtA->step = Aj->step = ActiveSetSize * sizeof(double);
		double* Ajp = Aj->data.db;
		double* Ap = A->data.db;
		for ( int k = 0; k < A->rows; ++k )
		{
			Ip = I->data.i;
			for ( int l = 0; l < A->cols; ++l, ++Ip )
			{
				if ( *Ip )
				{
					*Ajp = *Ap;
					++Ajp;
				}
				++Ap;
			}
		}
		double* AjBp = AjB->data.db;
		double* AtBp = AtB->data.db;
		Ip = I->data.i;
		for ( int k = 0; k < X->rows; ++k, ++Ip, ++AtBp )
			if ( *Ip )
			{
				*AjBp = *AtBp;
				++AjBp;
			}

		/* Compute current estimate and residual */
		cvGEMM( Aj, Aj, 1, NULL, 0, AtA, CV_GEMM_A_T );
		if ( cvCGSolve( AtA, AjB, Xj, cg_term_crit ) > .5 )
		{
			result = -1;
			goto __clean_up__;
		}

		/* Compute residual */
		cvGEMM( Aj, Xj, -1, B, 1, res );
		normres = cvNorm(res);
		if ( normres < normB * so_term_crit.epsilon )
			break;
	}

	/* Get the final X */
	{
		double* Xp = X->data.db;
		double* Xjp = Xj->data.db;
		int* Ip = I->data.i;
		for ( int k = 0; k < X->rows; ++k, ++Ip, ++Xp )
			if ( *Ip )
			{
				*Xp = *Xjp;
				++Xjp;
			}
	}

__clean_up__:
	cvFree( &fdr );
	cvReleaseMat( &Xj );
	cvReleaseMat( &AtB );
	cvReleaseMat( &AtA );
	cvReleaseMat( &Aj );
	cvReleaseMat( &I );
	cvReleaseMat( &res );
	cvReleaseMat( &corr );

	return result;
}

struct CvSparseAtAOpsData {
	CvSparseMatOps AOps;
	CvSparseMatOps AtOps;
	CvMat* Z;
	CvMat* I;
	void* userdata;
};

void icvSparseAtAOps( CvMat* X, CvMat* Y, void* userdata )
{
	CvSparseAtAOpsData* data = (CvSparseAtAOpsData*)userdata;
	data->AOps( X, data->Z, data->I, data->userdata );
	data->AtOps( data->Z, Y, data->I, data->userdata );
}

int cvL1StOMPSolve( CvSparseMatOps AOps, CvSparseMatOps AtOps, void* userdata, CvMat* B, CvMat* X, double epsilon, CvTermCriteria so_term_crit, CvTermCriteria cg_term_crit )
{
	int result = 0;

	cvZero( X );
	CvMat* corr = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	CvMat* res = cvCloneMat( B );
	CvMat* I = cvCreateMat( X->rows, X->cols, CV_32SC1 );
	cvZero( I );
	CvMat* Ifull = cvCreateMat( X->rows, X->cols, CV_32SC1 );
	cvSet( Ifull, cvScalar(1) );
	CvMat* AtB = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	AtOps( B, AtB, Ifull, userdata );
	CvMat* AjB = corr;
	CvMat* Xj = cvCreateMat( X->rows, X->cols, CV_MAT_TYPE(X->type) );
	double normB = cvNorm( B );
	double normres = normB;
	double n2 = sqrt(B->rows);
	int ActiveSetSize = 0;
	CvFDR* fdr = (CvFDR*)cvAlloc( X->rows * sizeof(fdr[0]) );
	double sqrt2_inv = 1. / sqrt(2.);
	double epsilon_inv = X->rows / epsilon;
	CvSparseAtAOpsData data;
	data.AOps = AOps;
	data.AtOps = AtOps;
	data.Z = res;
	data.I = I;
	data.userdata = userdata;

	for ( int i = 0; i < so_term_crit.max_iter; ++i )
	{
		corr->rows = X->rows;
		/* Compute residual correlations */
		AtOps( res, corr, Ifull, userdata );
		cvAbs( corr, corr );

		/* FDR search for threshold */
		double* corrp = corr->data.db;
		CvFDR* fdrp = fdr;
		normres = n2 / normres;
		for ( int k = 0; k < X->rows; ++k, ++corrp, ++fdrp )
		{
			fdrp->val = *corrp;
			*corrp *= normres;
			/* precision problem in erfc function, formula:
			 * |c_{j}| > t*\sigma
			 * become |c_{j}| / \sigma *after* t selected */
			fdrp->i = k;
		}
		icvQSortFDR( fdr, X->rows, 0 );
		fdrp = fdr;
		bool good = false;
		double thres;
		for ( int k = X->rows - 1; k >= 0; --k, ++fdrp )
		{
			double efc = erfc( fdrp->val * sqrt2_inv ) * epsilon_inv;
			if ( efc <= k + 1 )
			{
				good = true;
				thres = fdr[k].val * normres;
				/* adjust threshold with \sigma */
				break;
			}
		}
		if ( !good )
			break;

		/* Apply hard thresholding */
		int J = 0;
		corrp = corr->data.db;
		int* Ip = I->data.i;
		for ( int k = 0; k < X->rows; ++k, ++corrp, ++Ip )
			if ( *corrp > thres && !(*Ip) )
			{
				++J;
				*Ip = 1;
			}
		if ( J == 0 )
			break;

		/* Generate active set params */
		ActiveSetSize += J;
		AjB->rows = Xj->rows = ActiveSetSize;
		double* AjBp = AjB->data.db;
		double* AtBp = AtB->data.db;
		Ip = I->data.i;
		for ( int k = 0; k < X->rows; ++k, ++Ip, ++AtBp )
			if ( *Ip )
			{
				*AjBp = *AtBp;
				++AjBp;
			}

		/* Compute current estimate and residual */
		if ( cvCGSolve( icvSparseAtAOps, &data, AjB, Xj, cg_term_crit ) > .5 )
		{
			result = -1;
			goto __clean_up__;
		}

		/* Compute residual */
		AOps( Xj, res, I, userdata );
		cvSub( B, res, res );
		normres = cvNorm(res);
		if ( normres < normB * so_term_crit.epsilon )
			break;
	}

	/* Get the final X */
	{
		double* Xp = X->data.db;
		double* Xjp = Xj->data.db;
		int* Ip = I->data.i;
		for ( int k = 0; k < X->rows; ++k, ++Ip, ++Xp )
			if ( *Ip )
			{
				*Xp = *Xjp;
				++Xjp;
			}
	}

__clean_up__:
	cvFree( &fdr );
	cvReleaseMat( &Xj );
	cvReleaseMat( &AtB );
	cvReleaseMat( &Ifull );
	cvReleaseMat( &I );
	cvReleaseMat( &res );
	cvReleaseMat( &corr );

	return result;
}
