#ifndef _GUARD_KEYWORDS_
#define _GUARD_KEYWORDS_

#define UNRESERVED_KEYWORD		0
#define COL_NAME_KEYWORD		1
#define TYPE_FUNC_NAME_KEYWORD	2
#define RESERVED_KEYWORD		3

#define NAMEDATALEN (256)

#include "string.h"

typedef struct {
	const char* name;
	short int value;
	short int category;
} ScanKeyword;

extern const ScanKeyword ScanKeywords[];
extern const ScanKeyword* ScanKeywordLookup(const char* text);

#endif
