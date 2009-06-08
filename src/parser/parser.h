#ifndef _GUARD_PARSER_
#define _GUARD_PARSER_

#include "keywords.h"
#include "../nqpreqry.h"

extern int yyparse(void);
extern int yylex(void);
extern void yyerror(char*);

#endif
