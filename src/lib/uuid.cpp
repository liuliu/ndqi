#include "uuid.h"

char* cvUUIDCreate( CvMat* img )
{
	IplImage *hsv = cvCreateImage( cvGetSize( img ), IPL_DEPTH_8U, 3 );
	IplImage *small = cvCreateImage( cvSize( 64, 64 ), IPL_DEPTH_8U, 3 );
	IplImage *blur = cvCreateImage( cvSize( 64, 64 ), IPL_DEPTH_8U, 3 );
	cvCvtColor( img, hsv, CV_BGR2HSV );
	cvResize( hsv, small, CV_INTER_AREA );
	cvSmooth( small, blur, CV_GAUSSIAN, 3 );
	CvSize size;
	int step;
	unsigned char* blur_vec = 0;
	cvGetImageRawData( blur, &blur_vec, &step, &size );
	frl_md5 digest = frl_md5();
	digest.hash( (char*)blur_vec, step*size.height );
	cvReleaseImage( &hsv );
	cvReleaseImage( &small );
	cvReleaseImage( &blur );
	apr_byte_t* uuid = (apr_byte_t*)malloc( 23 );
	digest.base64_encode( uuid );
	return (char*)uuid;
}
