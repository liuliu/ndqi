
#include <tcutil.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "../nqrdb.h"
#include "../lib/frl_util_md5.h"

apr_pool_t* mempool;

void sumup(char* kstr, void* data, void* ud)
{
	uint32_t* t = (uint32_t*)ud;
	*t += *((int*)data);
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
	NQRDB* rdb = nqrdbnew();
	int a[] = {10, 11, 12, 13, 22};
	time = apr_time_now();
	for (int i = 0; i < 500000; i++)
		nqrdbput(rdb, (char*)key_cache[i].digest, a+i%5);
	time = apr_time_now()-time;
	printf("add 500,000 key-value pairs to rdb in %d microsecond(s).\n", time);

	TCMAP* tcmap = tcmapnew();
	time = apr_time_now();
	for (int i = 0; i < 500000; i++)
		tcmapput(tcmap, (char*)key_cache[i].digest, 16, a+i%5, 4);
	time = apr_time_now()-time;
	printf("add 500,000 key-value pairs to tcmap in %d microsecond(s).\n", time);

	time = apr_time_now();
	for (int i = 0; i < 500000; i++)
	{
		void* entry = nqrdbget(rdb, (char*)key_cache[i].digest);
		if (entry != a+i%5)
			printf("unexpected error in looking up %d %d.\n", entry, a+i%5);
	}
	time = apr_time_now()-time;
	printf("look up 500,000 key-value pairs in rdb in %d microsecond(s).\n", time);

	time = apr_time_now();
	for (int i = 0; i < 500000; i++)
	{
		int sp;
		const void* entry = tcmapget(tcmap, (char*)key_cache[i].digest, 16, &sp);
		if (*((int*)entry) != a[i%5])
			printf("unexpected error in looking up %d %d.\n", *((int*)entry), a[i%5]);
	}
	time = apr_time_now()-time;
	printf("look up 500,000 key-value pairs in tcmap in %d microsecond(s).\n", time);

	time = apr_time_now();
	for (int i = 0; i < 250000; i++)
		nqrdbout(rdb, (char*)key_cache[i].digest);
	time = apr_time_now()-time;
	printf("remove 250,000 key-value pairs from rdb in %d microsecond(s).\n", time);
	time = apr_time_now();
	for (int i = 0; i < 250000; i++)
	{
		void* entry = nqrdbget(rdb, (char*)key_cache[i].digest);
		if (entry != 0)
			printf("unexpected error in looking up %d %d.\n", entry, a+i%5);
	}
	for (int i = 250000; i < 500000; i++)
	{
		void* entry = nqrdbget(rdb, (char*)key_cache[i].digest);
		if (entry != a+i%5)
			printf("unexpected error in looking up %d %d.\n", entry, a+i%5);
	}
	time = apr_time_now()-time;
	printf("look up 250,000 null key-value pairs and 250,000 key-value pairs in rdb in %d microsecond(s).\n", time);
/*
	char** kstr = (char**)malloc(2000 * sizeof(char*));
	for (int i = 249000; i < 251000; i++)
		kstr[i - 249000] = (char*)key_cache[i].digest;
	time = apr_time_now();
	nqrdbfilter(rdb, kstr, 2000);
	time = apr_time_now()-time;
	free(kstr);
	printf("filter from 250,000 to only left 1,000 valid and 1,000 invalid key-value pairs in %d microsecond(s).\n", time);
*/
	uint32_t t = 0;
	time = apr_time_now();
	nqrdbforeach(rdb, sumup, &t);
	time = apr_time_now()-time;
	printf("foreach test: total sum is %d in %d microsecond(s).\n", t, time);
	nqrdbdel(rdb);
	apr_terminate();
	return 0;
}
