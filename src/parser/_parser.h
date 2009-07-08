#ifndef _GUARD__PARSER_
#define _GUARD__PARSER_

#include "keywords.h"
#include "../config/databases.h"
#include "../nqpreqry.h"
#include "nqparser.h"

extern apr_pool_t* yymem(void);
extern NQPARSERESULT* yyresult(void);
extern int yyparse(void);
extern int yylex(void);
extern void yyerror(const char*);

#endif
