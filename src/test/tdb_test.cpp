#include "../nqtdb.h"

int main(int argc, char** argv)
{
	NQTDB* tdb = nqtdbnew();
	nqtdbopen(tdb, "tdb.tch", HDBOWRITER | HDBOREADER | HDBOCREAT);
	nqtdbput(tdb, "1234567890abcdef", "john");
	nqtdbput(tdb, "1234567890abcdef", "carmark");
	nqtdbput(tdb, "1234567890abcdef", "jerry");
	nqtdbput(tdb, "1234567890abcdef", "tommy");
	nqtdbput(tdb, "1234567890qwerty", "marry");
	nqtdbput(tdb, "1234567890qwerty", "carmark");
	nqtdbput(tdb, "1234567890qwerty", "miya");
	nqtdbput(tdb, "1234567890tomeir", "john");
	nqtdbout(tdb, "1234567890abcdef", "carmark");
	TCLIST* list = nqtdbsearch(tdb, "john");
	int i;
	int rsiz;
	char* rbuf;
	for(i = 0; i < tclistnum(list); i++)
	{
		rbuf = (char*)tclistval(list, i, &rsiz);
		printf("%s\n", rbuf);
	}
	tclistdel(list);
	TCLIST* words = nqtdbget(tdb, "1234567890qwerty");
	for(i = 0; i < tclistnum(words); i++)
	{
		rbuf = (char*)tclistval(words, i, &rsiz);
		printf("%s\n", rbuf);
	}
	tclistdel(words);
	nqtdbclose(tdb);
	nqtdbdel(tdb);
	return 0;
}
