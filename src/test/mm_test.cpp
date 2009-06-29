#include "../lib/frl_managed_mem.h"

apr_pool_t* mempool;

int main()
{
	apr_initialize();
	apr_pool_create(&mempool, NULL);
	apr_atomic_init(mempool);
	char* str = (char*)frl_managed_malloc(10);
	str[0] = 'u';
	str[1] = '\0';
	frl_managed_unref(str);
	printf("%s\n", str);
	return 0;
}
