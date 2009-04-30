#include "nqutil.h"
#include "lib/mlapcluster.h"

#define _dispatch_mat_ptr(x, step) (CV_MAT_DEPTH((x)->type) == CV_32F ? (void*)((x)->data.fl+(step)) : (CV_MAT_DEPTH((x)->type) == CV_64F ? (void*)((x)->data.db+(step)) : (void*)(0)))

int nqeplr(CvMat* data, int* ki, int k)
{
	CvAPCluster apcluster = CvAPCluster(CvAPCParams(2000, 200, 0.5));
	CvMat* sim = cvCreateMat(data->rows, data->rows, CV_64FC1);
	int i;
	int j;
	double total = 0;
	for (i = 0; i < data->rows-1; i++)
		for (j = i+1; j < data->rows; j++)
		{
			CvMat a = cvMat(data->cols, 1, data->type, _dispatch_mat_ptr(data, i*data->cols));
			CvMat b = cvMat(data->cols, 1, data->type, _dispatch_mat_ptr(data, j*data->cols));
			double dist = cvNorm(&a, &b);
			total -= dist;
			cvmSet(sim, i, j, -dist);
			cvmSet(sim, j, i, -dist);
		}
	double pref = total*2/(data->rows*(data->rows-1));
	for (i = 0; i < data->rows; i++)
		cvmSet(sim, i, i, pref);
	CvMat* rsp = cvCreateMat(1, data->rows, CV_32SC1);
	apcluster.train(sim, rsp);
	for (i = 0; i < data->rows; i++)
		ki[i] = 0;
	for (i = 0; i < data->rows; i++)
		ki[rsp->data.i[i]] = 1;
	int r = 0;
	for (i = 0; i < data->rows; i++)
		if (ki[i])
		{
			ki[r] = i;
			r++;
		}
	cvReleaseMat(&rsp);
	cvReleaseMat(&sim);
	return r;
}

int nqeplr(CvSparseMat* sim, int* ki, int t, int k)
{
}
