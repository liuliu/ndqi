#include "nqrdb.h"
#include "assert.h"

inline bool kmatch(uint32_t* kstr1, uint32_t* kstr2, uint32_t k)
{
	for (; k < 4; k++)
		if (kstr1[k] != kstr2[k])
			return 0;
	return 1;
}

static apr_pool_t* mtx_pool = 0;
static frl_slab_pool_t* db_pool = 0;
static frl_slab_pool_t* rec_pool = 0;
static frl_slab_pool_t* b16_pool = 0;
static frl_slab_pool_t* b6_pool = 0;
static frl_slab_pool_t* b2_pool = 0;
static void nqrdbclear(NQRDB* rdb);

NQRDB* nqrdbnew(void)
{
	if (mtx_pool == 0)
		apr_pool_create(&mtx_pool, NULL);
	if (db_pool == 0)
		frl_slab_pool_create(&db_pool, mtx_pool, 64, sizeof(NQRDB), FRL_LOCK_WITH);
	if (rec_pool == 0)
		frl_slab_pool_create(&rec_pool, mtx_pool, 1024, sizeof(NQRDBDATUM), FRL_LOCK_WITH);
	if (b16_pool == 0)
		frl_slab_pool_create(&b16_pool, mtx_pool, 16, sizeof(NQRDBDATUM*) * 65536, FRL_LOCK_WITH);
	if (b6_pool == 0)
		frl_slab_pool_create(&b6_pool, mtx_pool, 1024, sizeof(NQRDBDATUM*) * 64, FRL_LOCK_WITH);
	if (b2_pool == 0)
		frl_slab_pool_create(&b2_pool, mtx_pool, 4096, sizeof(NQRDBDATUM*) * 4, FRL_LOCK_WITH);
	NQRDB* rdb = (NQRDB*)frl_slab_palloc(db_pool);
	nqrdbclear(rdb);
	rdb->head = (NQRDBDATUM*)frl_slab_palloc(rec_pool);
	rdb->head->chd = (NQRDBDATUM**)frl_slab_pcalloc(b16_pool);
	rdb->head->max = 65536;
	rdb->head->ht = 0;
	rdb->head->pr = 0;
	rdb->head->rnum = 0;
	rdb->head->vbuf = 0;
	rdb->head->prev = rdb->head->next = rdb->head;
#if APR_HAS_THREADS
	apr_thread_rwlock_create(&rdb->rwlock, mtx_pool);
#endif
	return rdb;
}

static void nqrdbclear(NQRDB* rdb)
{
	assert(rdb);
	rdb->rnum = 0;
	rdb->head = 0;
}

static NQRDBDATUM* nqrdbirt(NQRDB* rdb, NQRDBDATUM* rec, uint8_t ht, uint32_t i, uint32_t* kint, void* vbuf)
{
	NQRDBDATUM* nrec = (NQRDBDATUM*)frl_slab_palloc(rec_pool);
	nrec->rnum = 0;
	memcpy(nrec->kint, kint, 16);
	nrec->vbuf = vbuf;
	nrec->pr = rec;
	nrec->ht = ht;
	nrec->prev = rdb->head->prev;
	nrec->next = rdb->head;
	rdb->head->prev->next = nrec;
	rdb->head->prev = nrec;
	rec->chd[i] = nrec;
	rec->rnum++;
	rdb->rnum++;
	return nrec;
}

bool nqrdbput(NQRDB* rdb, char* kstr, void* vbuf)
{
	uint32_t* kint = (uint32_t*)kstr;
	uint32_t* kp = kint;
#if APR_HAS_THREADS
	apr_thread_rwlock_wrlock(rdb->rwlock);
#endif
	NQRDBDATUM* rec = rdb->head;
	uint32_t b16 = (*kp)>>16;
	if (rec->chd[b16] == 0)
	{
		nqrdbirt(rdb, rec, 1, b16, kint, vbuf);
#if APR_HAS_THREADS
		apr_thread_rwlock_unlock(rdb->rwlock);
#endif
		return 1;
	} else {
		rec = rec->chd[b16];
		if (kmatch(rec->kint, kint, 0))
		{
#if APR_HAS_THREADS
			apr_thread_rwlock_unlock(rdb->rwlock);
#endif
			return 0;
		}
	}
	uint32_t b6 = ((*kp)>>10)&0x3F;
	if (rec->chd == 0)
	{
		rec->chd = (NQRDBDATUM**)frl_slab_pcalloc(b6_pool);
		rec->max = 64;
	}
	if (rec->chd[b6] == 0)
	{
		nqrdbirt(rdb, rec, 2, b6, kint, vbuf);
#if APR_HAS_THREADS
		apr_thread_rwlock_unlock(rdb->rwlock);
#endif
		return 1;
	} else {
		rec = rec->chd[b6];
		if (kmatch(rec->kint, kint, 0))
		{
#if APR_HAS_THREADS
			apr_thread_rwlock_unlock(rdb->rwlock);
#endif
			return 0;
		}
	}
	uint32_t i = 8;
	uint32_t ht = 3;
	do {
		uint32_t b2 = ((*kp)>>i)&0x3;
		if (rec->chd == 0)
		{
			rec->chd = (NQRDBDATUM**)frl_slab_pcalloc(b2_pool);
			rec->max = 4;
		}
		if (rec->chd[b2] == 0)
		{
			nqrdbirt(rdb, rec, ht, b2, kint, vbuf);
#if APR_HAS_THREADS
			apr_thread_rwlock_unlock(rdb->rwlock);
#endif
			return 1;
		} else {
			rec = rec->chd[b2];
			if (kmatch(rec->kint, kint, 0))
			{
#if APR_HAS_THREADS
				apr_thread_rwlock_unlock(rdb->rwlock);
#endif
				return 0;
			}
		}
		ht++;
		kp += (i == 0);
		i = (i+30)&0x1F;
	} while (kp < kint+4);
#if APR_HAS_THREADS
	apr_thread_rwlock_unlock(rdb->rwlock);
#endif
	return 0;
}

void* nqrdbget(NQRDB* rdb, char* kstr)
{
	uint32_t* kint = (uint32_t*)kstr;
	uint32_t* kp = kint;
#if APR_HAS_THREADS
	apr_thread_rwlock_rdlock(rdb->rwlock);
#endif
	NQRDBDATUM* rec = rdb->head;
	uint32_t b16 = (*kp)>>16;
	if (rec->chd[b16] == 0)
	{
#if APR_HAS_THREADS
		apr_thread_rwlock_unlock(rdb->rwlock);
#endif
		return 0;
	} else {
		rec = rec->chd[b16];
		if (kmatch(rec->kint, kint, 0))
		{
			void* vbuf = rec->vbuf;
#if APR_HAS_THREADS
			apr_thread_rwlock_unlock(rdb->rwlock);
#endif
			return vbuf;
		}
	}
	uint32_t b6 = ((*kp)>>10)&0x3F;
	if (rec->chd == 0 || rec->chd[b6] == 0)
	{
#if APR_HAS_THREADS
		apr_thread_rwlock_unlock(rdb->rwlock);
#endif
		return 0;
	} else {
		rec = rec->chd[b6];
		if (kmatch(rec->kint, kint, 0))
		{
			void* vbuf = rec->vbuf;
#if APR_HAS_THREADS
			apr_thread_rwlock_unlock(rdb->rwlock);
#endif
			return vbuf;
		}
	}
	uint32_t i = 8;
	do {
		uint32_t b2 = ((*kp)>>i)&0x3;
		if (rec->chd == 0 || rec->chd[b2] == 0)
		{
#if APR_HAS_THREADS
			apr_thread_rwlock_unlock(rdb->rwlock);
#endif
			return 0;
		} else {
			rec = rec->chd[b2];
			if (kmatch(rec->kint, kint, 0))
			{
				void* vbuf = rec->vbuf;
#if APR_HAS_THREADS
				apr_thread_rwlock_unlock(rdb->rwlock);
#endif
				return vbuf;
			}
		}
		kp += (i == 0);
		i = (i+30)&0x1F;
	} while (kp < kint+4);
#if APR_HAS_THREADS
		apr_thread_rwlock_unlock(rdb->rwlock);
#endif
	return 0;
}

bool nqrdbout(NQRDB* rdb, char* kstr)
{
	uint32_t* kint = (uint32_t*)kstr;
	uint32_t* kp = kint;
#if APR_HAS_THREADS
	apr_thread_rwlock_wrlock(rdb->rwlock);
#endif
	NQRDBDATUM* rec = rdb->head;
	uint32_t b16, b6, b2, i;
	b16 = (*kp)>>16;
	if (rec->chd[b16] == 0)
	{
#if APR_HAS_THREADS
		apr_thread_rwlock_unlock(rdb->rwlock);
#endif
		return 0;
	} else {
		rec = rec->chd[b16];
		if (kmatch(rec->kint, kint, 0))
			goto RMPS;
	}
	b6 = ((*kp)>>10)&0x3F;
	if (rec->chd == 0 || rec->chd[b6] == 0)
	{
#if APR_HAS_THREADS
		apr_thread_rwlock_unlock(rdb->rwlock);
#endif
		return 0;
	} else {
		rec = rec->chd[b6];
		if (kmatch(rec->kint, kint, 0))
			goto RMPS;
	}
	i = 8;
	do {
		b2 = ((*kp)>>i)&0x3;
		if (rec->chd == 0 || rec->chd[b2] == 0)
		{
#if APR_HAS_THREADS
			apr_thread_rwlock_unlock(rdb->rwlock);
#endif
			return 0;
		} else {
			rec = rec->chd[b2];
			if (kmatch(rec->kint, kint, 0))
				goto RMPS;
		}
		kp += (i == 0);
		i = (i+30)&0x1F;
	} while (kp < kint+4);
RMPS:
	NQRDBDATUM* lrec = rec;
	NQRDBDATUM* nrec;
	while (lrec->rnum > 0)
	{
		nrec = 0;
		for (i = 0; i < lrec->max; i++)
		{
			if (lrec->chd[i] != 0)
			{
				if (lrec->chd[i]->rnum == 0)
				{
					nrec = lrec->chd[i];
					break;
				}
				if (nrec == 0)
					nrec = lrec->chd[i];
			}
		}
		lrec = nrec;
	}
	uint32_t k;
	if (lrec->ht == 1)
		k = lrec->kint[0]>>16;
	else if (lrec->ht == 2)
		k = (lrec->kint[0]>>10)&0x3F;
	else {
		k = (lrec->kint[(lrec->ht+8)>>4]>>(32-(((lrec->ht+9)<<1)&0x1F)))&0x3;
	}
	lrec->pr->chd[k] = 0;
	lrec->pr->rnum--;
	if (lrec != rec)
	{
		memcpy(rec->kint, lrec->kint, 16);
		rec->vbuf = lrec->vbuf;
	}
	lrec->prev->next = lrec->next;
	lrec->next->prev = lrec->prev;
	if (lrec->chd != 0)
		frl_slab_pfree(lrec->chd);
	frl_slab_pfree(lrec);
	rdb->rnum--;
#if APR_HAS_THREADS
	apr_thread_rwlock_unlock(rdb->rwlock);
#endif
	return 1;
}

bool nqrdbforeach(NQRDB* rdb, NQFOREACH op, void* ud)
{
#if APR_HAS_THREADS
	apr_thread_rwlock_rdlock(rdb->rwlock);
#endif
	NQRDBDATUM* cur = rdb->head;
	for (cur = cur->next; cur != rdb->head; cur = cur->next)
		op((char*)cur->kint, cur->vbuf, ud);
#if APR_HAS_THREADS
	apr_thread_rwlock_unlock(rdb->rwlock);
#endif
}

void nqrdbdel(NQRDB* rdb)
{
#if APR_HAS_THREADS
	apr_thread_rwlock_destroy(rdb->rwlock);
#endif
	NQRDBDATUM* cur = rdb->head;
	frl_slab_pfree(cur);
	for (cur = cur->next; cur != rdb->head; cur = cur->next)
	{
		if (cur->chd != 0)
			frl_slab_pfree(cur->chd);
		frl_slab_pfree(cur);
	}
	frl_slab_pfree(rdb);
}
