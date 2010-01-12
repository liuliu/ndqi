#include <stdio.h>
#include <sys/time.h>
#include "../lib/nqmm.h"

uint64_t nq_time_now()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

int main(int argc, char** argv)
{
	uint64_t old_time;
	NQMMP* mmp = nqmmpnew(73);
	char* a[10000];
	old_time = nq_time_now();
	for (int i = 0; i < 10000; i++)
		a[i] = (char*)nqalloc(mmp);
	for (int j = 1; j < 10000; j++)
		for (int i = 0; i < 1000; i++)
		{
			nqfree(a[i * 10]);
			a[i * 10] = (char*)nqalloc(mmp);
		}
	printf("nqmm elapsed time: %d\n", (int)(nq_time_now() - old_time));
	nqmmpdel(mmp);
	char* b[10000];
	old_time = nq_time_now();
	for (int i = 0; i < 10000; i++)
		b[i] = (char*)nqalloc(73);
	for (int j = 0; j < 10000; j++)
		for (int i = 0; i < 1000; i++)
		{
			nqfree(b[i * 10]);
			b[i * 10] = (char*)nqalloc(73);
		}
	printf("malloc elapsed time: %d\n", (int)(nq_time_now() - old_time));
	return 0;
}
