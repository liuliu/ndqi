#ifndef _GUARD_DATABASES_
#define _GUARD_DATABASES_

#define NAMEDATALEN (256)

#include <string.h>
#include "../nqqry.h"

typedef struct {
	const char* name;
	int type;
	const char* location;
    void* ref;
} ScanDatabase;

extern const ScanDatabase ScanDatabases[];
extern const ScanDatabase* ScanDatabaseLookup(const char* text);

#endif

