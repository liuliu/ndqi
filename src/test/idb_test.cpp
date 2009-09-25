/*
#include <tcutil.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
*/
#include "../nqidb.h"
#include "../lib/frl_util_md5.h"

apr_pool_t* mempool;

void sumup(char* kstr, void*, void* ud)
{
	uint32_t* t = (uint32_t*)ud;
	*t += *((int*)kstr);
}

int main()
{
	apr_initialize();
	apr_pool_create(&mempool, NULL);
	apr_atomic_init(mempool);
	frl_md5 key_cache[500000];
	apr_time_t time = apr_time_now();
	for (int i = 0; i < 500000; i++)
		key_cache[i] = frl_md5((char*)&i, 4);
	time = apr_time_now()-time;
	printf("data generated in %d microsecond(s).\n", time);
	NQIDB* idb = nqidbnew();
	time = apr_time_now();
	for (int i = 0; i < 500000; i++)
		nqidbput(idb, (char*)key_cache[i].digest);
	time = apr_time_now()-time;
	printf("add 500,000 key to idb in %d microsecond(s).\n", time);
	int miss = 0;
	time = apr_time_now();
	for (int i = 0; i < 500000; i++)
	{
		bool result = nqidbget(idb, (char*)key_cache[i].digest);
		if (!result)
			printf("unexpected error in looking up %d.\n", i);
	}
	time = apr_time_now()-time;
	printf("look up 500,000 key in idb in %d microsecond(s).\n", time);
//	printf("cannot find %d entry.\n", miss);

	time = apr_time_now();
	for (int i = 0; i < 250000; i++)
	{
		bool result = nqidbout(idb, (char*)key_cache[i].digest);
		if (!result)
			printf("unexpected error in removing %d.\n", i);
	}
	time = apr_time_now()-time;
	printf("remove 250,000 key from idb in %d microsecond(s).\n", time);
	time = apr_time_now();
	for (int i = 0; i < 250000; i++)
	{
		bool result = nqidbget(idb, (char*)key_cache[i].digest);
		if (result)
			printf("unexpected error in looking up %d.\n", i);
	}
	for (int i = 250000; i < 500000; i++)
	{
		bool result = nqidbget(idb, (char*)key_cache[i].digest);
		if (!result)
			printf("unexpected error in looking up %d.\n", i);
	}
	time = apr_time_now()-time;
	printf("look up 250,000 null key and 250,000 key in idb in %d microsecond(s).\n", time);
	uint32_t t = 0;
	time = apr_time_now();
	nqidbforeach(idb, sumup, &t);
	time = apr_time_now()-time;
	printf("foreach test: total sum is %d in %d microsecond(s).\n", t, time);
	nqidbdel(idb);
	apr_terminate();
	return 0;
}
