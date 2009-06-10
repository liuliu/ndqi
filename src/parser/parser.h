#ifndef _GUARD_PARSER_
#define _GUARD_PARSER_

#include "keywords.h"
#include "../nqpreqry.h"

extern apr_pool_t* yymem(void);
extern NQPREQRY* yyresult(void);
extern int yyparse(void);
extern int yylex(void);
extern void yyerror(char*);

#endif
