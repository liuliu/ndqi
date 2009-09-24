#include "nqidb.h"
#include "assert.h"

static apr_pool_t* mtx_pool = 0;
static frl_slab_pool_t* db_pool = 0;
static frl_slab_pool_t* dt_pool[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int bits_in_16bits[0x1u << 16];
static bool bits_in_16bits_init = false;

static int sparse_bitcount(unsigned int n)
{
	int count = 0;
	while (n)
	{
		++count;
		n &= (n - 1);
	}
	return count;
}

static void precomputed_16bits()
{
	int i;
	for (i = 0; i < (0x1u << 16); ++i)
		bits_in_16bits[i] = sparse_bitcount(i);
	bits_in_16bits_init = true;
}

static inline bool kmatch(uint32_t* kstr1, uint32_t* kstr2, uint32_t k)
{
	for (; k < 4; k++)
		if (kstr1[k] != kstr2[k])
			return 0;
	return 1;
}

NQIDB* nqidbnew(void)
{
	if (mtx_pool == 0)
		apr_pool_create(&mtx_pool, NULL);
	if (db_pool == 0)
		frl_slab_pool_create(&db_pool, mtx_pool, 64, sizeof(NQIDB), FRL_LOCK_WITH);
	if (!bits_in_16bits_init)
		precomputed_16bits();
	NQIDB* idb = (NQIDB*)frl_slab_palloc(db_pool);
	idb->rnum = 0;
	idb->con = NULL;
	return idb;
}

bool nqidbput(NQIDB* idb, char* kstr)
{
	uint32_t* kint = (uint32_t*)kstr;
	unsigned char* kustr = (unsigned char*)kstr;
	if (idb->rnum == 0)
	{
		if (idb->con == NULL)
		{
			if (dt_pool[0] == 0)
				frl_slab_pool_create(&dt_pool[0], mtx_pool, 1024, NQIDBDATUMSIZ, FRL_LOCK_WITH);
			idb->con = (NQIDBDATUM*)frl_slab_palloc(dt_pool[0]);
		}
		idb->con->i[0] = 0;
		memcpy(&idb->con->i[1], kint, 16);
		idb->con->con[0] = NULL;
	} else {
		NQIDBDATUM* con = idb->con;
		int k = 0;
		int spot = kustr[0] & 0xf;
		while ((con->i[0] & (1 << spot)) && (k < 32))
		{
			if (kmatch(kint, con->i + 1, k >> 2))
				return false;
			int p = bits_in_16bits[con->i[0] & ((1 << spot) - 1)];
			con = (NQIDBDATUM*)((char*)con->con[0] + p * NQIDBDATUMSIZ);
			++k;
			spot = (kustr[k >> 1] >> ((k & 0x1) << 2)) & 0xf;
		}
		if (k >= 32)
			return false;
		if (con->con[0] == NULL)
		{
			con->i[0] = (1 << spot) | 0x10000;
			if (dt_pool[0] == 0)
				frl_slab_pool_create(&dt_pool[0], mtx_pool, 1024, NQIDBDATUMSIZ, FRL_LOCK_WITH);
			con->con[0] = (NQIDBDATUM*)frl_slab_palloc(dt_pool[0]);
			con->con[0]->i[0] = 0;
			memcpy(&con->con[0]->i[1], kint, 16);
			con->con[0]->con[0] = NULL;
		} else {
			int p = bits_in_16bits[con->i[0] & ((1 << spot) - 1)];
			int c = con->i[0] >> 16;
			con->i[0] |= (1 << spot);
			con->i[0] += 0x10000;
			NQIDBDATUM* precon = con->con[0];
			if (dt_pool[c] == 0)
				frl_slab_pool_create(&dt_pool[c], mtx_pool, 1024, NQIDBDATUMSIZ * (c + 1), FRL_LOCK_WITH);
			con->con[0] = (NQIDBDATUM*)frl_slab_palloc(dt_pool[c]);
			memcpy(con->con[0], precon, NQIDBDATUMSIZ * p);
			NQIDBDATUM* conp = (NQIDBDATUM*)((char*)con->con[0] + NQIDBDATUMSIZ * p);
			conp->i[0] = 0;
			memcpy(&conp->i[1], kint, 16);
			conp->con[0] = NULL;
			memcpy((char*)con->con[0] + (p + 1) * NQIDBDATUMSIZ, (char*)precon + p * NQIDBDATUMSIZ, NQIDBDATUMSIZ * (c - p));
			frl_slab_pfree(precon);
		}
	}
	++idb->rnum;
	return true;
}

bool nqidbget(NQIDB* idb, char* kstr)
{
	if (idb->rnum == 0)
		return false;
	uint32_t* kint = (uint32_t*)kstr;
	unsigned char* kustr = (unsigned char*)kstr;
	NQIDBDATUM* con = idb->con;
	if (kmatch(kint, con->i + 1, 0))
		return true;
	int k = 0;
	int spot = kustr[0] & 0xf;
	while ((con->i[0] & (1 << spot)) && (k < 32))
	{
		int p = bits_in_16bits[con->i[0] & ((1 << spot) - 1)];
		con = (NQIDBDATUM*)((char*)con->con[0] + p * NQIDBDATUMSIZ);
		if (kmatch(kint, con->i + 1, k >> 2))
			return true;
		++k;
		spot = (kustr[k >> 1] >> ((k & 0x1) << 2)) & 0xf;
	}
	return false;
}

bool nqidbout(NQIDB* idb, char* kstr)
{
	if (idb->rnum == 0)
		return false;
	if (idb->rnum == 1)
	{
		idb->rnum = 0;
		return true;
	}
	uint32_t* kint = (uint32_t*)kstr;
	unsigned char* kustr = (unsigned char*)kstr;
	NQIDBDATUM* con = idb->con;
	NQIDBDATUM* pcon = NULL;
	NQIDBDATUM* found = NULL;
	int k = 0;
	int spot = kustr[0] & 0xf;
	if (!kmatch(kint, con->i + 1, 0))
	{
		while ((con->i[0] & (1 << spot)) && (k < 32))
		{
			int p = bits_in_16bits[con->i[0] & ((1 << spot) - 1)];
			pcon = con;
			con = (NQIDBDATUM*)((char*)con->con[0] + p * NQIDBDATUMSIZ);
			++k;
			if (kmatch(kint, con->i + 1, k >> 2))
			{
				found = con;
				break;
			}
			spot = (kustr[k >> 1] >> ((k & 0x1) << 2)) & 0xf;
		}
	} else
		found = con;
	if (!found)
		return false;
	while (((con->i[0] >> 16) != 0) && (k < 32))
	{
		NQIDBDATUM* tcon = NULL;
		int i, max = con->i[0] >> 16;
		NQIDBDATUM* coni = con->con[0];
		for (i = 0; i < max; ++i)
		{
			if ((coni->i[0] >> 16) == 0)
			{
				tcon = coni;
				break;
			}
			if (tcon == NULL)
				tcon = coni;
			coni = (NQIDBDATUM*)((char*)coni + NQIDBDATUMSIZ);
		}
		pcon = con;
		con = tcon;
		++k;
	}
	memcpy(&found->i[1], &con->i[1], 16);
	unsigned char* kmstr = (unsigned char*)(con->i + 1);
	--k;
	spot = (kmstr[k >> 1] >> ((k & 0x1) << 2)) & 0xf;
	pcon->i[0] -= 0x10000;
	pcon->i[0] &= ~(1 << spot);
	int p = bits_in_16bits[pcon->i[0] & ((1 << spot) - 1)];
	int c = pcon->i[0] >> 16;
	if (c > 0)
	{
		NQIDBDATUM* precon = pcon->con[0];
		if (dt_pool[c - 1] == 0)
			frl_slab_pool_create(&dt_pool[c - 1], mtx_pool, 1024, NQIDBDATUMSIZ * c, FRL_LOCK_WITH);
		pcon->con[0] = (NQIDBDATUM*)frl_slab_palloc(dt_pool[c - 1]);
		memcpy(pcon->con[0], precon, NQIDBDATUMSIZ * p);
		memcpy((char*)pcon->con[0] + p * NQIDBDATUMSIZ, (char*)precon + (p + 1) * NQIDBDATUMSIZ, NQIDBDATUMSIZ * (c - p));
		frl_slab_pfree(precon);
	} else {
		frl_slab_pfree(pcon->con[0]);
		pcon->con[0] = NULL;
	}
	--idb->rnum;
	return true;
}

bool nqidbforeach(NQIDB* idb, NQFOREACH op, void* ud)
{
}

void nqidbdel(NQIDB* idb)
{
}
