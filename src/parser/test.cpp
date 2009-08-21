#include "nqparser.h"

int main()
{
	apr_initialize();
	NQPARSERESULT* result;
	char ql1[] = "SELECT # WHERE LFD LIKE #Dh2uakSd98fnwEi3ne-1i3 50\% AND EXIF.MAKE=\"SONY\";";
	result = nqparse(ql1, strlen(ql1));
	printf("cnum: %d\n", ((NQPREQRY*)(result->result))->cnum);
	char ql2[] = "SELECT # WHERE (EXIF.MODEL=\"A720\" OR EXIF.MAKE=\"CANON\") AND NOT LFD LIKE #Dh2uakSd98fnwEi3ne-1_3 AND LFD EXACT LIKE (SELECT LFD WHERE EXIF.MAKE=\"Panasonic\");";
	result = nqparse(ql2, strlen(ql2));
	printf("cnum: %d\n", ((NQPREQRY*)(result->result))->cnum);
	char ql3[] = "select where lh exact like #Dh2uakSd98fnwEi3ne-1_3 and exif.gps.latitude<102.938 limit 30;";
	result = nqparse(ql3, strlen(ql3));
	printf("cnum: %d\n", ((NQPREQRY*)(result->result))->cnum);
	char ql4[] = "select where lh like #Dh2uakSd98fnwEi3ne-1_3 or exif.gps.latitude between 10.3 and 20;";
	result = nqparse(ql4, strlen(ql4));
	printf("cnum: %d\n", ((NQPREQRY*)(result->result))->cnum);
	char ql5[] = "insert #Dh2uakSd98fnwEi3ne-1_3";
	result = nqparse(ql5, strlen(ql5));
	printf("uuid: %s\n", ((NQMANIPULATE*)(result->result))->sbj.str);
	char ql6[] = "insert tag=\"beach\" into #Dh2uakSd98fnwEi3ne-1_3";
	result = nqparse(ql6, strlen(ql6));
	printf("db: %s, val: %s, uuid: %s\n", ((NQMANIPULATE*)(result->result))->db, ((NQMANIPULATE*)(result->result))->val, ((NQMANIPULATE*)(result->result))->sbj.str);
	char ql7[] = "delete tag=\"beach\" from #Dh2uakSd98fnwEi3ne-1_3";
	result = nqparse(ql7, strlen(ql7));
	printf("db: %s, val: %s, uuid: %s\n", ((NQMANIPULATE*)(result->result))->db, ((NQMANIPULATE*)(result->result))->val, ((NQMANIPULATE*)(result->result))->sbj.str);
    char ql8[] = "select # where lfd like #Dh2uakSd98fnwEi3ne-1_3 above 0.5";
	result = nqparse(ql8, strlen(ql8));
	printf("db: %s, uuid: %s, thr: %f\n", ((NQPREQRY*)(result->result))->db, ((NQPREQRY*)(result->result))->sbj.str, ((NQPREQRY*)(result->result))->thr);
	nqparsedel();
}
