#include "_parser.h"
#include "gram.h"

#define NQ_KEYWORD(a,b,c) {a,b,c},

const ScanKeyword ScanKeywords[] = {
#include "kwlist.h"
};

const ScanKeyword* LastScanKeyword = ScanKeywords + sizeof(ScanKeywords) / sizeof(ScanKeyword);

const ScanKeyword* ScanKeywordLookup(const char *text)
{
	int	len, i;
	char word[NAMEDATALEN];
	const ScanKeyword *low;
	const ScanKeyword *high;

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
	low = &ScanKeywords[0];
	high = LastScanKeyword - 1;
	while (low <= high)
	{
		const ScanKeyword *middle;
		int			difference;

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
