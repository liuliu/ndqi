#ifndef _GUARD_DATABASES_
#define _GUARD_DATABASES_

#define NAMEDATALEN (256)

#include <string.h>
#include "../nqqry.h"

typedef struct {
	const char* name;
	int type;
	int mode;
    void* ref;
	const char* refloc;
	void* hpr;
    const char* hprloc;
} ScanDatabase;

extern ScanDatabase ScanDatabases[];
extern ScanDatabase* LastScanDatabase;
extern const ScanDatabase* ScanDatabaseLookup(const char* text);

#endif

