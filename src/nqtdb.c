#include "nqtdb.h"
#include <string.h>

NQTDB* nqtdbnew(void)
{
	NQTDB* tdb = (NQTDB*)tcmalloc(sizeof(NQTDB));
	tdb->idx = tchdbnew();
	tdb->tokens = tcmapnew();
	tdb->dups = tcmapnew();
	return tdb;
}

bool nqtdbput(NQTDB* tdb, char* kstr, char* val)
{
	int vsiz = strlen(val);
	if (vsiz > NQTDBMAXSIZ)
		return false;
	char vbuf[NQTDBMAXSIZ + 20];
	memcpy(vbuf, kstr, 16);
	*(int32_t*)(vbuf + 16) = vsiz;
	memcpy(vbuf + 20, val, vsiz);
	int dsiz;
	if (tcmapget(tdb->dups, vbuf, vsiz + 20, &dsiz) != NULL)
		return false;
	int32_t flag = 0xdeadbeef;
	tcmapput(tdb->dups, vbuf, vsiz + 20, &flag, 4);
	tcmapputcat(tdb->tokens, val, vsiz, kstr, 16);
	return tchdbputcat(tdb->idx, kstr, 16, vbuf + 16, vsiz + 4);
}

TCLIST* nqtdbget(NQTDB* tdb, char* kstr)
{
	int siz;
	char* vals = (char*)tchdbget(tdb->idx, kstr, 16, &siz);
	if (vals == NULL || siz <= 0)
		return NULL;
	char* lvals = vals + siz;
	TCLIST* words = tclistnew();
	while (vals < lvals)
	{
		int vsiz = *(int32_t*)vals;
		vals += 4;
		tclistpush(words, vals, vsiz);
		vals += vsiz;
	}
	free(lvals - siz);
	return words;
}

TCLIST* nqtdbsearch(NQTDB* tdb, char* val)
{
	int vsiz = strlen(val);
	if (vsiz > NQTDBMAXSIZ)
		return NULL;
	int siz;
	char* kstrs = (char*)tcmapget(tdb->tokens, val, vsiz, &siz);
	if (kstrs == NULL || siz <= 0 || siz % 16 != 0)
		return NULL;
	char* lkstrs = kstrs + siz;
	TCLIST* list = tclistnew();
	while (kstrs < lkstrs)
	{
		tclistpush(list, kstrs, 16);
		kstrs += 16;
	}
	return list;
}

static void nqtdboutimpl(NQTDB* tdb, char* kstr, char* val, int vsiz)
{
	char vbuf[NQTDBMAXSIZ + 20];
	memcpy(vbuf, kstr, 16);
	*(int32_t*)(vbuf + 16) = vsiz;
	memcpy(vbuf + 20, val, vsiz);
	tcmapout(tdb->dups, vbuf, vsiz + 20);
	int siz, nsiz;
	char* kstrs = (char*)tcmapget(tdb->tokens, val, vsiz, &siz);
	if (kstrs == NULL || siz <= 0 || siz % 16 != 0)
		return;
	char* hkstrs = (char*)malloc(siz);
	memcpy(hkstrs, kstrs, siz);
	kstrs = hkstrs;
	nsiz = siz;
	while (kstrs < hkstrs + nsiz)
	{
		if (memcmp(kstrs, kstr, 16) == 0)
		{
			if (kstrs + 16 < hkstrs + nsiz)
				memcpy(kstrs, kstrs + 16, (hkstrs + nsiz) - (kstrs + 16));
			nsiz -= 16;
		} else
			kstrs += 16;
	}
	tcmapput(tdb->tokens, val, vsiz, hkstrs, nsiz);
	free(hkstrs);
}

bool nqtdbout(NQTDB* tdb, char* kstr)
{
	int siz;
	char* vals = (char*)tchdbget(tdb->idx, kstr, 16, &siz);
	if (vals == NULL || siz <= 0)
		return NULL;
	char* lvals = vals + siz;
	while (vals < lvals)
	{
		int vsiz = *(int32_t*)vals;
		vals += 4;
		nqtdboutimpl(tdb, kstr, vals, vsiz);
		vals += vsiz;
	}
	free(lvals - siz);
	return tchdbout(tdb->idx, kstr, 16);
}

bool nqtdbout(NQTDB* tdb, char* kstr, char* val)
{
	int vsiz = strlen(val);
	if (vsiz > NQTDBMAXSIZ)
		return false;
	nqtdboutimpl(tdb, kstr, val, vsiz);
	int siz;
	char* vals = (char*)tchdbget(tdb->idx, kstr, 16, &siz);
	if (vals == NULL || siz <= 0)
		return false;
	char* hvals = vals;
	int nsiz = siz;
	while (vals < hvals + nsiz)
	{
		int vsiz0 = *(int32_t*)vals;
		vals += 4;
		if (vsiz0 == vsiz && memcmp(vals, val, vsiz) == 0)
		{
			if (vals + vsiz < hvals + nsiz)
				memcpy(vals - 4, vals + vsiz, (hvals + nsiz) - (vals + vsiz));
			nsiz -= vsiz + 4;
		} else
			vals += vsiz0;
	}
	tchdbput(tdb->idx, kstr, 16, hvals, nsiz);
	free(hvals);
	return true;
}

bool nqtdbopen(NQTDB* tdb, const char* path, int omode)
{
	if (!tchdbopen(tdb->idx, path, omode))
		return false;
	if (!tchdbiterinit(tdb->idx))
		return false;
	for (;;)
	{
		int ksiz;
		char* kstr = (char*)tchdbiternext(tdb->idx, &ksiz);
		if (kstr == NULL)
			break;
		int siz;
		char* vals = (char*)tchdbget(tdb->idx, kstr, 16, &siz);
		if (vals == NULL || siz <= 0)
			break;
		char vbuf[NQTDBMAXSIZ + 20];
		memcpy(vbuf, kstr, 16);
		char* lvals = vals + siz;
		while (vals < lvals)
		{
			int vsiz = *(int32_t*)vals;
			vals += 4;
			*(int32_t*)(vbuf + 16) = vsiz;
			memcpy(vbuf + 20, vals, vsiz);
			int dsiz;
			if (tcmapget(tdb->dups, vbuf, vsiz + 20, &dsiz) == NULL)
			{
				int32_t flag = 0xdeadbeef;
				tcmapput(tdb->dups, vbuf, vsiz + 20, &flag, 4);
				tcmapputcat(tdb->tokens, vals, vsiz, kstr, 16);
			}
			vals += vsiz;
		}
		free(lvals - siz);
	}
	return true;
}

bool nqtdbsync(NQTDB* tdb)
{
	tchdbsync(tdb->idx);
}

bool nqtdbclose(NQTDB* tdb)
{
	tchdbclose(tdb->idx);
}

void nqtdbdel(NQTDB* tdb)
{
	tchdbdel(tdb->idx);
	tcmapdel(tdb->tokens);
	tcmapdel(tdb->dups);
	tcfree(tdb);
}
