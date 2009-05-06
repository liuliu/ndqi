#ifndef GUARD_cvlocalhist_h
#define GUARD_cvlocalhist_h

#include <cv.h>

float cvCompareLocalHist(int* hist_a, int* hist_b, int length);

void cvCalcLocalHist(CvArr* _img, int* hists, const int bins);

#endif
