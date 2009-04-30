#include "mlapcluster.h"
#include <limits>

CvAPCluster::CvAPCluster( CvAPCParams _params )
: params(_params)
{
}

CvAPCluster::~CvAPCluster()
{
	clear();
}

bool CvAPCluster::train( const CvMat* similarity,
			 const CvMat* labels )
{
	int stop = 0;

	CV_FUNCNAME( "CvAPCluster::train" );

	__BEGIN__;

	CvMat* rsp;
	CvMat* avl;
	int* rspmaxidx;
	double* rspmaxval;
	double* rspmaxval2;
	double* diagrsp;

	double& lam = params.lambda;
	double _lam = 1.-lam;
	int num;

	if ( !CV_IS_MAT(similarity) || similarity->rows != similarity->cols || CV_MAT_TYPE( similarity->type ) != CV_64FC1 )
		CV_ERROR( CV_StsBadArg,
		"similarity matrix is a double-point matrix with equal rows and cols" );

	num = similarity->rows;

	if ( !CV_IS_MAT(labels) || labels->cols != num || CV_MAT_TYPE( labels->type ) != CV_32SC1 )
		CV_ERROR( CV_StsBadArg,
		"labels array (when passed) must be a valid 1d integer vector of <sample_count> elements" );

	rsp = cvCreateMat( num, num, CV_64FC1 );
	avl = cvCreateMat( num, num, CV_64FC1 );

	cvZero( rsp );
	cvZero( avl );

	rspmaxidx = (int*)cvAlloc( num*sizeof(rspmaxidx[0]) );
	rspmaxval = (double*)cvAlloc( num*sizeof(rspmaxval[0]) );
	rspmaxval2 = (double*)cvAlloc( num*sizeof(rspmaxval2[0]) );
	diagrsp = (double*)cvAlloc( num*sizeof(diagrsp[0]) );

	for ( int k = 0; k < params.maxiteration; k++ )
	{
		double* rsp_vec = rsp->data.db;
		double* avl_vec = avl->data.db;
		double* sim_vec = similarity->data.db;
		double* sim_vec2 = sim_vec;

		for ( int i = 0; i < num; i++ )
		{
			int yk = -1;
			double y = -std::numeric_limits<double>::max(), y2 = -std::numeric_limits<double>::max();
			for ( int j = 0; j < num; j++ )
			{
				double t = *sim_vec2 + *avl_vec;
				if ( t > y )
				{
					y = t;
					yk = j;
				} else if ( t > y2 )
					y2 = t;
				sim_vec2++;
				avl_vec++;
			}
			for ( int j = 0; j < num; j++ )
			{
				if ( j != yk )
					*rsp_vec = *rsp_vec*lam+(*sim_vec-y)*_lam;
				else
					*rsp_vec = *rsp_vec*lam+(*sim_vec-y2)*_lam;
				rsp_vec++;
				sim_vec++;
			}
		}

		int* rspmaxidx_vec = rspmaxidx;
		double* rspmaxval_vec = rspmaxval;
		double* rspmaxval2_vec = rspmaxval2;

		for ( int i = 0; i < num; i++, rspmaxidx_vec++, rspmaxval_vec++, rspmaxval2_vec++ )
		{
			*rspmaxidx_vec = -1;
			*rspmaxval_vec = 0;
			*rspmaxval2_vec = 0;
		}

		rsp_vec = rsp->data.db;
		double* diagrsp_vec = diagrsp;

		for ( int i = 0; i < num; i++, diagrsp_vec++ )
		{
			rspmaxidx_vec = rspmaxidx;
			rspmaxval_vec = rspmaxval;
			rspmaxval2_vec = rspmaxval2;
			for ( int j = 0; j < num; j++, rspmaxidx_vec++, rspmaxval_vec++, rspmaxval2_vec++ )
			{
				if ( i != j )
				{
					if ( *rsp_vec > *rspmaxval_vec )
					{
						*rspmaxidx_vec = i;
						*rspmaxval_vec = *rsp_vec;
					} else if ( *rsp_vec > *rspmaxval2_vec )
						*rspmaxval2_vec = *rsp_vec;
				} else
					*diagrsp_vec = *rsp_vec;
				rsp_vec++;
			}
		}

		avl_vec = avl->data.db;
		diagrsp_vec = diagrsp;

		for ( int i = 0; i < num; i++, diagrsp_vec++ )
		{
			rspmaxidx_vec = rspmaxidx;
			rspmaxval_vec = rspmaxval;
			rspmaxval2_vec = rspmaxval2;
			for ( int j = 0; j < num; j++, rspmaxidx_vec++, rspmaxval_vec++, rspmaxval2_vec++ )
			{
				double tmp;
				if ( i != j )
				{
					if ( i != *rspmaxidx_vec )
						tmp = *diagrsp_vec + *rspmaxval_vec;
					else
						tmp = *diagrsp_vec + *rspmaxval2_vec;
					if ( tmp > 0 )
						tmp = 0;
				} else
					tmp = *rspmaxval_vec;
				*avl_vec = *avl_vec*lam+tmp*_lam;
				avl_vec++;
			}
		}

		stop++;
		int* cls_vec = labels->data.i;
		rsp_vec = rsp->data.db;
		avl_vec = avl->data.db;
		for ( int i = 0; i < num; i++, cls_vec++ )
		{
			int maxidx = i;
			double maxval = -std::numeric_limits<double>::max();
			for ( int j = 0; j < num; j++ )
			{
				double t = *rsp_vec + *avl_vec;
				if ( t > maxval )
				{
					maxval = t;
					maxidx = j;
				}
				rsp_vec++;
				avl_vec++;
			}
			if ( *cls_vec != maxidx )
				stop = 0;
			*cls_vec = maxidx;
		}
		if ( stop > params.stopcriterion )
			break;
	}

	cvFree( &diagrsp );
	cvFree( &rspmaxval2 );
	cvFree( &rspmaxval );
	cvFree( &rspmaxidx );

	cvReleaseMat( &avl );
	cvReleaseMat( &rsp );

	__END__;

	return stop > params.stopcriterion;
}

struct CvSparseNode2D
{
	int i;
	int k;
	double val;
};

bool
CvAPCluster::train( const CvSparseMat* similarity,
		    const CvMat* labels )
{
	int stop = 0;

	CV_FUNCNAME( "CvAPCluster::train" );

	__BEGIN__;

	int* psize;
	int* psize_vec;
	int num;
	double* rsp;
	double* avl;
	double* rsp_vec;
	double* avl_vec;
	CvSparseNode2D* nodes;
	CvSparseNode2D** node_entries;
	int* rspmaxidx;
	double* rspmaxval;
	double* rspmaxval2;
	double* diagrsp;

	int total = 0;
	double& lam = params.lambda;
	double _lam = 1.-lam;

	if ( !CV_IS_SPARSE_MAT(similarity) || similarity->dims != 2 || similarity->size[0] != similarity->size[1] || CV_MAT_TYPE( similarity->type ) != CV_64FC1 )
		CV_ERROR( CV_StsBadArg,
		"similarity matrix is a double-point sparse matrix with equal rows and cols" );

	num = similarity->size[0];

	if ( !CV_IS_MAT(labels) || labels->cols != num || CV_MAT_TYPE( labels->type ) != CV_32SC1 )
		CV_ERROR( CV_StsBadArg,
		"labels array (when passed) must be a valid 1d integer vector of <sample_count> elements" );

	psize = (int*)cvAlloc( num*sizeof(psize[0]) );
	psize_vec = psize;
	for ( int i = 0; i < num; i++, psize_vec++ )
		*psize_vec = 0;

	CvSparseMatIterator mat_iterator;
	CvSparseNode* node;
	node = cvInitSparseMatIterator( similarity, &mat_iterator );
	for ( ; node != 0; node = cvGetNextSparseNode( &mat_iterator ) )
	{
		const int* idx = CV_NODE_IDX( similarity, node );
		psize[idx[0]]++;
		total++;
	}

	rsp = (double*)cvAlloc( total*sizeof(rsp[0]) );
	avl = (double*)cvAlloc( total*sizeof(avl[0]) );
	rsp_vec = rsp;
	avl_vec = avl;

	nodes = (CvSparseNode2D*)cvAlloc( total*sizeof(nodes[0]) );
	node_entries = (CvSparseNode2D**)cvAlloc( num*sizeof(node_entries[0]) );
	psize_vec = psize;
	node_entries[0] = nodes;
	for ( int i = 1; i < num; i++, psize_vec++ )
		node_entries[i] = node_entries[i-1] + *psize_vec;
	node = cvInitSparseMatIterator( similarity, &mat_iterator );
	for ( ; node != 0; node = cvGetNextSparseNode( &mat_iterator ), rsp_vec++, avl_vec++ )
	{
		*rsp_vec = *avl_vec = 0;
		const int* idx = CV_NODE_IDX( similarity, node );
		CvSparseNode2D*& node_entry = node_entries[idx[0]];
		node_entry->i = idx[0];
		node_entry->k = idx[1];
		node_entry->val = *(double*)CV_NODE_VAL( similarity, node );
		node_entry++;
	}
	cvFree( &node_entries );

	rspmaxidx = (int*)cvAlloc( num*sizeof(rspmaxidx[0]) );
	rspmaxval = (double*)cvAlloc( num*sizeof(rspmaxval[0]) );
	rspmaxval2 = (double*)cvAlloc( num*sizeof(rspmaxval2[0]) );
	diagrsp = (double*)cvAlloc( num*sizeof(diagrsp[0]) );

	for ( int k = 0; k < params.maxiteration; k++ )
	{
		CvSparseNode2D* nodes_vec = nodes;
		psize_vec = psize;
		CvSparseNode2D* nodes_vec2 = nodes;
		avl_vec = avl;
		rsp_vec = rsp;
		for ( int i = 0; i < num; i++, psize_vec++ )
		{
			int yk = -1;
			double y = -std::numeric_limits<double>::max(), y2 = -std::numeric_limits<double>::max();
			for ( int j = 0; j < *psize_vec; j++ )
			{
				double t = nodes_vec2->val + *avl_vec;
				if ( t > y )
				{
					y = t;
					yk = nodes_vec2->k;
				} else if ( t > y2 )
					y2 = t;
				nodes_vec2++;
				avl_vec++;
			}
			for ( int j = 0; j < *psize_vec; j++ )
			{
				if ( nodes_vec->k != yk )
					*rsp_vec = *rsp_vec*lam+(nodes_vec->val-y)*_lam;
				else
					*rsp_vec = *rsp_vec*lam+(nodes_vec->val-y2)*_lam;
				nodes_vec++;
				rsp_vec++;
			}
		}

		int* rspmaxidx_vec = rspmaxidx;
		double* rspmaxval_vec = rspmaxval;
		double* rspmaxval2_vec = rspmaxval2;

		for ( int i = 0; i < num; i++, rspmaxidx_vec++, rspmaxval_vec++, rspmaxval2_vec++ )
		{
			*rspmaxidx_vec = -1;
			*rspmaxval_vec = 0;
			*rspmaxval2_vec = 0;
		}

		rsp_vec = rsp;
		psize_vec = psize;
		nodes_vec = nodes;
		double* diagrsp_vec = diagrsp;
		for ( int i = 0; i < num; i++, psize_vec++, diagrsp_vec++ )
			for ( int j = 0; j < *psize_vec; j++ )
			{
				if ( i != nodes_vec->k )
				{
					if ( *rsp_vec > rspmaxval[nodes_vec->k] )
					{
						rspmaxidx[nodes_vec->k] = i;
						rspmaxval[nodes_vec->k] = *rsp_vec;
					} else if ( *rsp_vec > rspmaxval2[nodes_vec->k] )
						rspmaxval2[nodes_vec->k] = *rsp_vec;
				} else
					*diagrsp_vec = *rsp_vec;
				nodes_vec++;
				rsp_vec++;
			}

		avl_vec = avl;
		diagrsp_vec = diagrsp;
		psize_vec = psize;
		nodes_vec = nodes;
		for ( int i = 0; i < num; i++, psize_vec++, diagrsp_vec++ )
			for ( int j = 0; j < *psize_vec; j++ )
			{
				double tmp;
				if ( i != nodes_vec->k )
				{
					if ( i != rspmaxidx[nodes_vec->k] )
						tmp = *diagrsp_vec + rspmaxval[nodes_vec->k];
					else
						tmp = *diagrsp_vec + rspmaxval2[nodes_vec->k];
					if ( tmp > 0 )
						tmp = 0;
				} else
					tmp = rspmaxval[nodes_vec->k];
				*avl_vec = *avl_vec*lam+tmp*_lam;
				nodes_vec++;
				avl_vec++;
			}

		stop++;
		int* cls_vec = labels->data.i;
		rsp_vec = rsp;
		avl_vec = avl;
		psize_vec = psize;
		nodes_vec = nodes;
		for ( int i = 0; i < num; i++, psize_vec++, cls_vec++ )
		{
			int maxidx = i;
			double maxval = -std::numeric_limits<double>::max();
			for ( int j = 0; j < *psize_vec; j++ )
			{
				double t = *rsp_vec + *avl_vec;
				if ( t > maxval )
				{
					maxval = t;
					maxidx = nodes_vec->k;
				}
				nodes_vec++;
				rsp_vec++;
				avl_vec++;
			}
			if ( *cls_vec != maxidx )
				stop = 0;
			*cls_vec = maxidx;
		}
		if ( stop > params.stopcriterion )
			break;
	}

	cvFree( &diagrsp );
	cvFree( &rspmaxval2 );
	cvFree( &rspmaxval );
	cvFree( &rspmaxidx );

	cvFree( &rsp );
	cvFree( &avl );
	cvFree( &psize );
	cvFree( &nodes );

	__END__;

	return stop > params.stopcriterion;
}


void
CvAPCluster::clear()
{
}

void
CvAPCluster::write( CvFileStorage* fs,
		    const char* name )
{
}

void
CvAPCluster::read( CvFileStorage* fs,
		   CvFileNode* root_node )
{
}
