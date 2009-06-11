#include "parser.h"

int main()
{
	apr_initialize();
	NQPREQRY* result;
	char ql1[] = "SELECT # WHERE LFD LIKE #Dh2uakSd98fnwEi3ne-1i3 AND EXIF.MAKE=\"SONY\";";
	result = nqparse(ql1, strlen(ql1));
	char ql3[] = "select where lh exact like #Dh2uakSd98fnwEi3ne-1_3 and exif.gps.latitude<102.938;";
	result = nqparse(ql3, strlen(ql3));
	char ql2[] = "SELECT # WHERE EXIF.MODEL=\"A720\" AND LFD EXACT LIKE (SELECT LFD WHERE EXIF.MAKE=\"CANON\");";
	result = nqparse(ql2, strlen(ql2));
	nqparsedel();
}
