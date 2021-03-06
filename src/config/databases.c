#include "databases.h"
#include "../nqbwdb.h"
#include "../nqfdb.h"

#define NQ_DATABASE(a,b,c,d,e) {a,b,c,0,d,0,e},

ScanDatabase ScanDatabases[] = {
#include "dblist.i"
};

ScanDatabase* LastScanDatabase = ScanDatabases + sizeof(ScanDatabases) / sizeof(ScanDatabase);

const ScanDatabase* ScanDatabaseLookup(const char *text)
{
	int	len, i;
	char word[NAMEDATALEN];
	const ScanDatabase *low;
	const ScanDatabase *high;

	len = strlen(text);
	/* We assume all keywords are shorter than NAMEDATALEN. */
	if (len >= NAMEDATALEN)
		return NULL;

	/* Apply an ASCII-only downcasing. We must not use tolower() since it may
	 * produce the wrong translation in some locales (eg, Turkish). */
	for (i = 0; i < len; i++)
	{
		char ch = text[i];
		if (ch >= 'A' && ch <= 'Z')
			ch += 'a' - 'A';
		word[i] = ch;
	}
	word[len] = '\0';

	/* Now do a binary search using plain strcmp() comparison. */
	low = &ScanDatabases[0];
	high = LastScanDatabase - 1;
	while (low <= high)
	{
		const ScanDatabase *middle;
		int difference;

		middle = low + (high - low) / 2;
		difference = strcmp(middle->name, word);
		if (difference == 0)
			return middle;
		else if (difference < 0)
			low = middle + 1;
		else
			high = middle - 1;
	}

	return NULL;
}

