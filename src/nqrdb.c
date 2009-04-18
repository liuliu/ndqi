#include "nqrdb.h"
#include "assert.h"

inline bool kmatch(unsigned char* kstr1, unsigned char* kstr2, uint32_t k)
{
	for (; k < 16; k++)
		if (kstr1[k] != kstr2[k])
			return 0;
	return 1;
}

static apr_pool_t* mtx_pool = 0;
static frl_slab_pool_t* db_pool = 0;
static frl_slab_pool_t* rec_pool = 0;
static void nqrdbclear(NQRDB* rdb);

NQRDB* nqrdbnew(void)
{
	if (mtx_pool == 0)
		apr_pool_create(&mtx_pool, NULL);
	if (rec_pool == 0)
		frl_slab_pool_create(&rec_pool, mtx_pool, 1024, sizeof(NQRDBREC), FRL_LOCK_WITH);
	if (db_pool == 0)
		frl_slab_pool_create(&db_pool, mtx_pool, 64, sizeof(NQRDB), FRL_LOCK_WITH);
	NQRDB* rdb = (NQRDB*)frl_slab_palloc(db_pool);
	nqrdbclear(rdb);
	rdb->head = (NQRDBREC*)frl_slab_palloc(rec_pool);
	memset(rdb->head->chd, 0, sizeof(rdb->head->chd[0]) * 256);
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

bool nqrdbput(NQRDB* rdb, char* kstr, void* vbuf)
{
	int i;
	unsigned char* kp = (unsigned char*)kstr;
#if APR_HAS_THREADS
	apr_thread_rwlock_wrlock(rdb->rwlock);
#endif
	NQRDBREC* rec = rdb->head;
	for (i = 0; i < 16; i++)
	{
		if (rec->chd[*kp] == 0)
		{
			NQRDBREC* nrec = (NQRDBREC*)frl_slab_palloc(rec_pool);
			memset(nrec->chd, 0, sizeof(nrec->chd[0]) * 256);
			nrec->rnum = 0;
			memcpy(nrec->kstr, kstr, 16);
			nrec->vbuf = vbuf;
			nrec->pr = rec;
			nrec->ht = i;
			nrec->prev = rdb->head->prev;
			nrec->next = rdb->head;
			rdb->head->prev->next = nrec;
			rdb->head->prev = nrec;
			rec->chd[*kp] = nrec;
			rec->rnum++;
			rdb->rnum++;
#if APR_HAS_THREADS
			apr_thread_rwlock_unlock(rdb->rwlock);
#endif
			return 1;
		} else {
			if (kmatch(rec->kstr, (unsigned char*)kstr, i+1))
				break;
			rec = rec->chd[*kp];
		}
		kp++;
	}
#if APR_HAS_THREADS
	apr_thread_rwlock_unlock(rdb->rwlock);
#endif
	return 0;
}

void* nqrdbget(NQRDB* rdb, char* kstr)
{
	int i;
	unsigned char* kp = (unsigned char*)kstr;
#if APR_HAS_THREADS
	apr_thread_rwlock_rdlock(rdb->rwlock);
#endif
	NQRDBREC* rec = rdb->head;
	for (i = 0; i < 16; i++)
	{
		rec = rec->chd[*kp];
		if (rec == 0)
		{
#if APR_HAS_THREADS
			apr_thread_rwlock_unlock(rdb->rwlock);
#endif
			return 0;
		}
		if (kmatch(rec->kstr, (unsigned char*)kstr, i+1))
		{
			void* vbuf = rec->vbuf;
#if APR_HAS_THREADS
			apr_thread_rwlock_unlock(rdb->rwlock);
#endif
			return vbuf;
		}
		kp++;
	}
#if APR_HAS_THREADS
		apr_thread_rwlock_unlock(rdb->rwlock);
#endif
	return 0;
}

bool nqrdbout(NQRDB* rdb, char* kstr)
{
	int i;
	unsigned char* kp = (unsigned char*)kstr;
#if APR_HAS_THREADS
	apr_thread_rwlock_wrlock(rdb->rwlock);
#endif
	NQRDBREC* rec = rdb->head;
	for (i = 0; i < 16; i++)
	{
		rec = rec->chd[*kp];
		if (rec == 0)
		{
#if APR_HAS_THREADS
			apr_thread_rwlock_unlock(rdb->rwlock);
#endif
			return 0;
		}
		if (kmatch(rec->kstr, (unsigned char*)kstr, i+1))
			break;
		kp++;
	}
	NQRDBREC* lrec = rec;
	NQRDBREC* nrec;
	while (lrec->rnum > 0)
	{
		nrec = 0;
		int i;
		for (i = 0; i < 256; i++)
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
	lrec->pr->chd[lrec->kstr[lrec->ht]] = 0;
	lrec->pr->rnum--;
	if (lrec != rec)
	{
		memcpy(rec->kstr, lrec->kstr, 16);
		rec->vbuf = lrec->vbuf;
	}
	lrec->prev->next = lrec->next;
	lrec->next->prev = lrec->prev;
	frl_slab_pfree(lrec);
	rdb->rnum--;
#if APR_HAS_THREADS
	apr_thread_rwlock_unlock(rdb->rwlock);
#endif
	return 1;
}

void nqrdbdel(NQRDB* rdb)
{
#if APR_HAS_THREADS
	apr_thread_rwlock_destroy(rdb->rwlock);
#endif
	NQRDBREC* cur = rdb->head;
	frl_slab_pfree(cur);
	for (cur = cur->next; cur != rdb->head; cur = cur->next)
		frl_slab_pfree(cur);
	frl_slab_pfree(rdb);
}
