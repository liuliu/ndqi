#include "../nqrdb.h"
#include "../lib/frl_util_md5.h"

apr_pool_t* mempool;

int main()
{
	apr_initialize();
	apr_pool_create(&mempool, NULL);
	apr_atomic_init(mempool);
	frl_md5 key_cache[500000];
	for (int i = 0; i < 500000; i++)
		key_cache[i] = frl_md5((char*)&i, 4);
	NQRDB* rdb = nqrdbnew();
	int a[] = {10, 11, 12, 13, 22};
	printf("data generated\n");
	apr_time_t time = apr_time_now();
	for (int i = 0; i < 500000; i++)
		nqrdbput(rdb, (char*)key_cache[i].digest, a+i%5);
	time = apr_time_now()-time;
	printf("add 500,000 key-value pairs to rdb in %d microsecond(s).\n", time);
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
		nqrdbout(rdb, (char*)key_cache[i].digest);
	time = apr_time_now()-time;
	printf("remove 500,000 key-value pairs from rdb in %d microsecond(s).\n", time);
	time = apr_time_now();
	for (int i = 0; i < 500000; i++)
	{
		void* entry = nqrdbget(rdb, (char*)key_cache[i].digest);
		if (entry != 0)
			printf("unexpected error in looking up %d %d.\n", entry, a+i%5);
	}
	time = apr_time_now()-time;
	printf("look up 500,000 null key-value pairs in rdb in %d microsecond(s).\n", time);
	apr_terminate();
	return 0;
}
