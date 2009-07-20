#include "nqplan.h"
#include "nqclient.h"
#include "../lib/frl_managed_mem.h"
#include "../lib/frl_util_md5.h"
#include "../config/databases.h"

static apr_pool_t* mtx_pool = 0;
static frl_slab_pool_t* plan_pool = 0;
static frl_slab_pool_t* plan_iter_pool = 0;

static NQQRY* nqqrytrans(NQPLAN* plan, NQPREQRY* preqry)
{
	NQQRY* qry = nqqrynew();
	const ScanDatabase* db = ScanDatabaseLookup(preqry->db);
	qry->db = db->ref;
	qry->cfd = preqry->cfd;
	/* for tcwdb, no need for col, but helper to keep the map of uuid to uid64 */
	qry->col = (preqry->type & NQTTCWDB) ? db->hpr : preqry->col;
	qry->op = preqry->op;
	qry->type = preqry->type & ~(NQSUBQRY | NQSQRYANY | NQSQRYALL);
	switch (qry->type)
	{
		case NQTBWDB:
			qry->sbj.desc = ncbwdbget(preqry->db, (char*)frl_md5((apr_byte_t*)preqry->sbj.str).digest);
			break;
		case NQTFDB:
			qry->sbj.desc = ncfdbget(preqry->db, (char*)frl_md5((apr_byte_t*)preqry->sbj.str).digest);
			break;
		default:
			qry->sbj.str = preqry->sbj.str;
			frl_managed_ref(preqry->sbj.str);
	}
	qry->lmt = preqry->lmt;
	qry->mode = preqry->mode;
	qry->cnum = preqry->cnum;
	if (preqry->cnum > 0)
	{
		qry->conds = (NQQRY**)malloc(qry->cnum * sizeof(NQQRY*));
		int i;
		for (i = 0; i < qry->cnum; i++)
			qry->conds[i] = nqqrytrans(plan, preqry->conds[i]);
	}
	if (preqry->type & NQSUBQRY)
	{
		NQPLANITER* prev = (NQPLANITER*)frl_slab_palloc(plan_iter_pool);
		prev->prev = 0;
		prev->dbname = preqry->db;
		prev->type = preqry->type;
		prev->postqry = qry;
		plan->head->prev = prev;
		plan->head = prev;
		plan->cnum++;
		prev->qry = nqqrytrans(plan, preqry);
	}
	return qry;
}

NQPLAN* nqplannew(NQPREQRY* preqry)
{
	if (mtx_pool == 0)
		apr_pool_create(&mtx_pool, NULL);
	if (plan_pool == 0)
		frl_slab_pool_create(&plan_pool, mtx_pool, 32, sizeof(NQPLAN), FRL_LOCK_WITH);
	if (plan_iter_pool == 0)
		frl_slab_pool_create(&plan_iter_pool, mtx_pool, 1024, sizeof(NQPLANITER), FRL_LOCK_WITH);
	NQPLAN* plan = (NQPLAN*)frl_slab_palloc(plan_pool);
	memset(plan, 0, sizeof(NQPLAN));
	plan->tail = (NQPLANITER*)frl_slab_palloc(plan_iter_pool);
	plan->tail->prev = 0;
	plan->tail->dbname = 0;
	plan->tail->type = 0;
	plan->tail->postqry = 0;
	plan->head = plan->tail;
	plan->cnum++;
	plan->tail->qry = nqqrytrans(plan, preqry);
	return plan;
}

int nqplanrun(NQPLAN* plan, char** kstr, float* likeness)
{
	NQPLANITER* cur = plan->tail;
	int i, j, t;
	for (i == 0; i < plan->cnum; i++)
	{
		t = ncqrysearch(cur->qry, kstr, likeness);
		if (cur->postqry != 0)
		{
			char** ksptr = kstr;
			NQQRY **condptr, *qry = cur->postqry;
			if (cur->type == NQSUBQRY)
			{
				switch (qry->type)
				{
					case NQTBWDB:
						qry->sbj.desc = ncbwdbget(cur->dbname, *ksptr);
						break;
					case NQTFDB:
						qry->sbj.desc = ncfdbget(cur->dbname, *ksptr);
						break;
				}
			} else {
				qry->cnum = t;
				condptr = qry->conds = (NQQRY**)malloc(qry->cnum * sizeof(NQQRY*));
				for (j = 0; j < qry->cnum; j++, condptr++, ksptr++)
				{
					(*condptr)->db = qry->db;
					(*condptr)->cfd = qry->cfd;
					(*condptr)->col = qry->col;
					(*condptr)->op = qry->op;
					(*condptr)->type = qry->type;
					switch (qry->type)
					{
						case NQTBWDB:
							(*condptr)->sbj.desc = ncbwdbget(cur->dbname, *ksptr);
							break;
						case NQTFDB:
							(*condptr)->sbj.desc = ncfdbget(cur->dbname, *ksptr);
							break;
					}
				}
				qry->type = (cur->type == NQSQRYALL) ? NQCTAND : NQCTOR;
			}
		}
		cur = cur->prev;
	}
	return t;
}

void nqplandel(NQPLAN* plan)
{
	NQPLANITER *last, *cur = plan->tail;
	int i;
	for (i == 0; i < plan->cnum; i++)
	{
		last = cur;
		cur = cur->prev;
		nqqrydel(last->qry);
		frl_slab_pfree(last);
	}
	frl_slab_pfree(plan);
}
