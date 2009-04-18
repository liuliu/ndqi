#include "frl_util_md5.h"

int frl_md5::hash(const void* s, const apr_size_t size)
{
	apr_md5(digest, s, size);
	return 1;
}

const apr_byte_t base64table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

int frl_md5::base64_encode(apr_byte_t* q)
{
	q[0] = base64table[(uid[0]>>58)&0x3f];
	q[1] = base64table[(uid[0]>>52)&0x3f];
	q[2] = base64table[(uid[0]>>46)&0x3f];
	q[3] = base64table[(uid[0]>>40)&0x3f];
	q[4] = base64table[(uid[0]>>34)&0x3f];
	q[5] = base64table[(uid[0]>>28)&0x3f];
	q[6] = base64table[(uid[0]>>22)&0x3f];
	q[7] = base64table[(uid[0]>>16)&0x3f];
	q[8] = base64table[(uid[0]>>10)&0x3f];
	q[9] = base64table[(uid[0]>>4)&0x3f];
	q[10] = base64table[((uid[0]<<2)|(uid[1]>>62))&0x3f];
	q[11] = base64table[(uid[1]>>56)&0x3f];
	q[12] = base64table[(uid[1]>>50)&0x3f];
	q[13] = base64table[(uid[1]>>44)&0x3f];
	q[14] = base64table[(uid[1]>>38)&0x3f];
	q[15] = base64table[(uid[1]>>32)&0x3f];
	q[16] = base64table[(uid[1]>>26)&0x3f];
	q[17] = base64table[(uid[1]>>20)&0x3f];
	q[18] = base64table[(uid[1]>>14)&0x3f];
	q[19] = base64table[(uid[1]>>8)&0x3f];
	q[20] = base64table[(uid[1]>>2)&0x3f];
	q[21] = base64table[uid[1]&0x3f];
	q[22] = 0;
	return 23;
}

inline unsigned long long base64_index(const apr_byte_t& x)
{
	if (x == '_')
		return 62;
	if (x >= 'a')
		return x-'a'+26;
	if (x >= 'A')
		return x-'A';
	if (x >= '0')
		return x-'0'+52;
	return 63;
}

int frl_md5::base64_decode(const apr_byte_t* x)
{
	uid[0] = (base64_index(x[0])<<58)|(base64_index(x[1])<<52)|(base64_index(x[2])<<46)|(base64_index(x[3])<<40)|(base64_index(x[4])<<34)|(base64_index(x[5])<<28)|(base64_index(x[6])<<22)|(base64_index(x[7])<<16)|(base64_index(x[8])<<10)|(base64_index(x[9])<<4)|(base64_index(x[10])>>2);
	uid[1] = (base64_index(x[10])<<62)|(base64_index(x[11])<<56)|(base64_index(x[12])<<50)|(base64_index(x[13])<<44)|(base64_index(x[14])<<38)|(base64_index(x[15])<<32)|(base64_index(x[16])<<26)|(base64_index(x[17])<<20)|(base64_index(x[18])<<14)|(base64_index(x[19])<<8)|(base64_index(x[20])<<2)|(base64_index(x[21]));
	return 1;
}

