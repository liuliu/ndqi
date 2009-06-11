#include "parser.h"

int main()
{
	apr_initialize();
	NQPREQRY* result;
	char ql1[] = "SELECT # WHERE LFD LIKE #Dh2uakSd98fnwEi3ne-1i3 50\% AND EXIF.MAKE=\"SONY\";";
	result = nqparse(ql1, strlen(ql1));
	char ql2[] = "SELECT # WHERE EXIF.MODEL=\"A720\" OR EXIF.MAKE=\"CANON\" AND NOT LFD LIKE #Dh2uakSd98fnwEi3ne-1_3 AND LFD EXACT LIKE (SELECT LFD WHERE EXIF.MAKE=\"Panasonic\");";
	result = nqparse(ql2, strlen(ql2));
	char ql3[] = "select where lh exact like #Dh2uakSd98fnwEi3ne-1_3 and exif.gps.latitude<102.938 limit 30;";
	result = nqparse(ql3, strlen(ql3));
	char ql4[] = "select where lh like #Dh2uakSd98fnwEi3ne-1_3 or exif.gps.latitude between 10.3 and 20;";
	result = nqparse(ql4, strlen(ql4));
	nqparsedel();
}
