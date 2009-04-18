#ifndef GUARD_frl_util_md5_h
#define GUARD_frl_util_md5_h

#include "apr_md5.h"
#include <memory>

class frl_md5
{
	public:
		frl_md5()
		{
			memset(digest, 0, 16);
		}
		frl_md5(const apr_byte_t* x)
		{
			base64_decode(x);
		}
		frl_md5(const void* s, const apr_size_t size)
		{
			hash(s, size);
		}
		int hash(const void* s, const apr_size_t size);
		int base64_encode(apr_byte_t* q);
		int base64_decode(const apr_byte_t* x);
		union {
			apr_uint64_t uid[2];
			apr_byte_t digest[16];
		};
};

inline bool operator==(const frl_md5& x, const frl_md5& y)
{
	return (x.uid[0] == y.uid[0])&&(x.uid[1] == y.uid[1]);
}
inline bool operator!=(const frl_md5& x, const frl_md5& y)
{
	return (x.uid[0] != y.uid[0])||(x.uid[1] != y.uid[1]);
}
inline bool operator>(const frl_md5& x, const frl_md5& y)
{
	return (x.uid[0] > y.uid[0])||((x.uid[0] == y.uid[0])&&(x.uid[1] > y.uid[1]));
}
inline bool operator<=(const frl_md5& x, const frl_md5& y)
{
	return (x.uid[0] <= y.uid[0])||((x.uid[0] == y.uid[0])&&(x.uid[1] <= y.uid[1]));
}
inline bool operator<(const frl_md5& x, const frl_md5& y)
{
	return (x.uid[0] < y.uid[0])||((x.uid[0] == y.uid[0])&&(x.uid[1] < y.uid[1]));
}
inline bool operator>=(const frl_md5& x, const frl_md5& y)
{
	return (x.uid[0] >= y.uid[0])||((x.uid[0] == y.uid[0])&&(x.uid[1] >= y.uid[1]));
}

const unsigned int SIZEOF_FRL_MD5 = sizeof(frl_md5);

#endif

