/********************************
 * Fotas Runtime Vision Library *
 ********************************
 * Author: Liu Liu
 */
#include "cvlocalhist.h"
#include <iostream>
#define GOLDEN (0.61803398)

int tabid[256];

void
icvCalcHist( IplImage* img,
	     int* hist,
	     const int length,
	     const CvRect rect )
{
	uchar *iter_data = 0;
	CvSize size;
	int step;
	memset( hist, 0, length*sizeof(int) );
	cvSetImageROI( img, rect );
	cvGetImageRawData( img, &iter_data, &step, &size );
	for ( int i = 0; i < size.height; i++ )
	{
		for (int j = 0; j < size.width; j++)
		{
			hist[tabid[*iter_data]]++;
			iter_data++;
		}
		iter_data+=step-size.width;
	}
}

void
icvHistProjection( CvArr* _img,
		    int bins,
		    int* v_hist = NULL,
		    int* h_hist = NULL )
{
	CvSize size;
	int step;
	IplImage* img = (IplImage*)_img;
	uchar *iter_data = NULL;
	cvGetImageRawData( img, &iter_data, &step, &size );
	int *iter_v_hist = v_hist,
	    *iter_h_hist = h_hist;
	for ( int i = 0; i < size.height; i++ )
	{
		iter_v_hist = v_hist;
		for ( int j = 0; j < size.width; j++ )
		{
			if ( v_hist != NULL )
			{
				iter_v_hist[tabid[*iter_data]]++;
				iter_v_hist+=bins;
			}
			if ( h_hist != NULL ) iter_h_hist[tabid[*iter_data]]++;
			iter_data++;
		}
		if ( h_hist != NULL ) iter_h_hist+=bins;
		iter_data+=step-size.width;
	}
}

inline double
icvCompareHist( int *hist1,
		int *hist2,
		const int length )
{
	double s1 = 0,
	       s11 = 0,
	       s2 = 0,
	       s22 = 0,
	       s12 = 0,
	       a = 0,
	       b = 0;
	int *iter_hist1 = hist1,
	    *iter_hist2 = hist2;

	for( int i = 0; i < length; i++, iter_hist1++, iter_hist2++ )
	{
		a = *iter_hist1;
		b = *iter_hist2;

		s12+=a*b;
		s1+=a;
		s11+=a*a;
		s2+=b;
		s22+=b*b;
	}

	double scale = 1./length;
	double num = s12-s1*s2*scale,
	       denom2 = sqrt((s11-s1*s1*scale)*(s22-s2*s2*scale));

	return ((fabs(denom2) > DBL_EPSILON) ? num/denom2 : 1);
}

int
icvSplitScan( int *scanline,
	      const int length,
	      const int width,
	      const int start,
	      const int end,
	      const int lock_position,
	      const double lock )
{
	int *l_hist = (int*)malloc( length*sizeof(int) );
	memset( l_hist, 0, length*sizeof(int) );
	int *r_hist = (int*)malloc( length*sizeof(int) );
	memset( r_hist, 0, length*sizeof(int) );
	int *iter_scanline = scanline,
	    *iter_l_hist = l_hist,
	    *iter_r_hist = r_hist;
	for ( int i = 0; i < start; i++ )
	{
		iter_l_hist = l_hist;
		for ( int j = 0; j < length; j++, iter_l_hist++ )
		{
			*iter_l_hist+=*iter_scanline;
			iter_scanline++;
		}
	}
	for ( int i = start; i < width; i++ )
	{
		iter_r_hist = r_hist;
		for ( int j = 0; j < length; j++, iter_r_hist++ )
		{
			*iter_r_hist+=*iter_scanline;
			iter_scanline++;
		}
	}
	int best_id = start;
	double best_value = 1.,
	       temp = 0;
	iter_scanline = scanline+start*length;
	for (int i = start; i < end; i++ )
	{
		iter_l_hist = l_hist;
		iter_r_hist = r_hist;
		for ( int j = 0; j < length; j++, iter_l_hist++, iter_r_hist++ )
		{
			*iter_l_hist+=*iter_scanline;
			*iter_r_hist-=*iter_scanline;
			iter_scanline++;
		}
		temp = icvCompareHist( l_hist, r_hist, length );
		if ( temp < best_value )
		{
			best_id = i;
			best_value = temp;
		}
		if ( (lock_position == i)&&(temp-lock <= best_value) )
		{
			best_value = temp-lock;
			best_id = lock_position;
		}
	}
	free( l_hist );
	free( r_hist );
	return best_id;
}

float
cvCompareLocalHist( int* hist_a,
		    int* hist_b,
		    int length )
{
	return (icvCompareHist( hist_a, hist_b, length )+
		icvCompareHist( hist_a+length*1, hist_b+length*1, length )+
		icvCompareHist( hist_a+length*2, hist_b+length*2, length )+
		icvCompareHist( hist_a+length*3, hist_b+length*3, length ))*0.25;
}

void
cvCalcLocalHist( CvArr* _img,
		 int* hists,
		 const int bins )
{
	IplImage* img = (IplImage*)_img;
	uchar *iter_data = NULL;
	CvSize size = cvGetSize(img);

	for (int i = 0; i < 256; i++)
		tabid[i] = i*bins/256;

	int *v_hist = (int*)malloc( size.width*bins*sizeof(int) ),
	    *h_hist = (int*)malloc( size.height*bins*sizeof(int) );
	memset( v_hist, 0, size.width*bins*sizeof(int) );
	memset( h_hist, 0, size.height*bins*sizeof(int) );
	icvHistProjection( img, bins, v_hist, h_hist );
	int width_t = icvSplitScan( v_hist, bins, size.width, (int)(size.width*(.9-GOLDEN)), (int)(size.width*(GOLDEN+.1)), size.width/2, .01 ),
	    height_t = icvSplitScan( h_hist, bins, size.height, (int)(size.height*(.9-GOLDEN)), (int)(size.height*(GOLDEN+.1)), size.height/2, .01 );
	free( v_hist );
	free( h_hist );

	int *l_hist = (int*)malloc( bins*sizeof(int) ),
	    *r_hist = (int*)malloc( bins*sizeof(int) );
	double w_h = 0,
	       h_h = 0;

        icvCalcHist( img, l_hist, bins, cvRect( 0, 0, width_t, size.height ) );
	icvCalcHist( img, r_hist, bins, cvRect( width_t, 0, size.width-width_t, size.height ) );
	w_h = icvCompareHist( l_hist, r_hist, bins );

	icvCalcHist( img, l_hist, bins, cvRect( 0, 0, size.width, height_t ) );
	icvCalcHist( img, r_hist, bins, cvRect( 0, height_t, size.width, size.height-height_t ) );
	h_h = icvCompareHist( l_hist, r_hist, bins );

	free( l_hist );
	free( r_hist );

	if ( w_h > h_h )
	{
		int *u_hist = (int*)malloc( size.width*bins*sizeof(int) ),
		    *b_hist = (int*)malloc( size.width*bins*sizeof(int) );
		memset( u_hist, 0, size.width*bins*sizeof(int) );
		memset( b_hist, 0, size.width*bins*sizeof(int) );

		cvSetImageROI( img, cvRect( 0, 0, size.width, height_t ) );
		icvHistProjection( img, bins, u_hist );

		cvSetImageROI( img, cvRect( 0, height_t, size.width, size.height-height_t ) );
		icvHistProjection( img, bins, b_hist );

		int u_t = icvSplitScan( u_hist, bins, size.width, (int)(size.width*(.9-GOLDEN)), (int)(size.width*(GOLDEN+.1)), size.width/2, .01 ),
		    b_t = icvSplitScan( b_hist, bins, size.width, (int)(size.width*(.9-GOLDEN)), (int)(size.width*(GOLDEN+.1)), size.width/2, .01 );
		free( u_hist );
		free( b_hist );

		icvCalcHist( img, hists, bins, cvRect( 0, 0, u_t, height_t ) );
		icvCalcHist( img, hists+bins*1, bins, cvRect( u_t, 0, size.width-u_t, height_t ) );
		icvCalcHist( img, hists+bins*2, bins, cvRect( 0, height_t, b_t, size.height-height_t ) );
		icvCalcHist( img, hists+bins*3, bins, cvRect( b_t, height_t, size.width-b_t, size.height-height_t ) );
	} else {
		int *l_hist = (int*)malloc( size.height*bins*sizeof(int) ),
		    *r_hist = (int*)malloc( size.height*bins*sizeof(int) );
		memset( l_hist, 0,  size.height*bins*sizeof(int) );
		memset( r_hist, 0,  size.height*bins*sizeof(int) );

		cvSetImageROI( img, cvRect( 0, 0, width_t, size.height ) );
		icvHistProjection( img, bins, NULL, l_hist );

		cvSetImageROI( img, cvRect( width_t, 0, size.width-width_t, size.height ) );
		icvHistProjection( img, bins, NULL, r_hist );

		int l_t = icvSplitScan( l_hist, bins, size.height, (int)(size.height*(.9-GOLDEN)), (int)(size.height*(GOLDEN+.1)), size.height/2, .01 ),
		    r_t = icvSplitScan( r_hist, bins, size.height, (int)(size.height*(0.9-GOLDEN)), (int)(size.height*(GOLDEN+.1)), size.height/2, .01 );
		free( l_hist );
		free( r_hist );

		icvCalcHist( img, hists, bins, cvRect( 0, 0, width_t, l_t ) );
		icvCalcHist( img, hists+bins*1, bins, cvRect( width_t, 0, size.width-width_t, r_t ) );
		icvCalcHist( img, hists+bins*2, bins, cvRect( 0, l_t, width_t, size.height-l_t ) );
		icvCalcHist( img, hists+bins*3, bins, cvRect( width_t, r_t, size.width-width_t, size.height-r_t ) );
	}
}

