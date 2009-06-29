%{
//#define YYDEBUG 1
#include "_parser.h"
static NQPREQRY* YY_RESULT = 0;
%}

%union
{
	int	ival;
	char chr;
	char *str;
	const char *keyword;
	NQPREQRY *qry;
}

%nonassoc ALL, ANY, ASC, BETWEEN, BY, DESC, DISTINCT, EXACT, EXISTS, FROM, IN, IS, LIKE, LIMIT, NULL_P, ORDER, SELECT, SOME, WHERE
%nonassoc UUID, UUIDENT, IDENT, FCONST, ICONST
%nonassoc STRTYPE, NUMTYPE, NOTYPE
%nonassoc NUMGT, NUMGE, NUMLT, NUMLE, COLNE, COLEQ
%left OR
%left AND
%left NOT

%%

SelectListStmt: SelectStmt |
				SelectStmt ';' |
				SelectStmt ';' SelectListStmt;

SelectStmt:	SELECT ColumnOptListStmt WHERE CondStmt
			{
				$$ = $4;
				YY_RESULT = $$.qry;
			};

ColumnOptListStmt:	/* empty value */ |
					ColumnListStmt;

CondStmt:	CondStmt OR CondStmt
			{
				$$.qry = nqpreqrynew(yymem());
				$$.qry->type = NQCTOR;
				$$.qry->cnum  = ($1.qry->type == NQCTOR) ? $1.qry->cnum : 1;
				$$.qry->cnum += ($3.qry->type == NQCTOR) ? $3.qry->cnum : 1;
				$$.qry->conds = (NQPREQRY**)apr_palloc(yymem(), $$.qry->cnum * sizeof(NQPREQRY*));
				NQPREQRY** condptr = $$.qry->conds;
				if ($1.qry->type == NQCTOR)
				{
					memcpy(condptr, $1.qry->conds, $1.qry->cnum * sizeof(NQPREQRY*));
					condptr += $1.qry->cnum;
				} else {
					*condptr = $1.qry;
					condptr++;
				}
				if ($3.qry->type == NQCTOR)
					memcpy(condptr, $3.qry->conds, $3.qry->cnum * sizeof(NQPREQRY*));
				else
					*condptr = $3.qry;
			} |
			CondStmt AND CondStmt
			{
				$$.qry = nqpreqrynew(yymem());
				$$.qry->type = NQCTAND;
				$$.qry->cnum  = ($1.qry->type == NQCTAND && !($1.qry->op & NQOPNOT)) ? $1.qry->cnum : 1;
				$$.qry->cnum += ($3.qry->type == NQCTAND && !($3.qry->op & NQOPNOT)) ? $3.qry->cnum : 1;
				$$.qry->conds = (NQPREQRY**)apr_palloc(yymem(), $$.qry->cnum * sizeof(NQPREQRY*));
				NQPREQRY** condptr = $$.qry->conds;
				if ($1.qry->type == NQCTAND && !($1.qry->op & NQOPNOT))
				{
					memcpy(condptr, $1.qry->conds, $1.qry->cnum * sizeof(NQPREQRY*));
					condptr += $1.qry->cnum;
				} else {
					*condptr = $1.qry;
					condptr++;
				}
				if ($3.qry->type == NQCTAND && !($3.qry->op & NQOPNOT))
					memcpy(condptr, $3.qry->conds, $3.qry->cnum * sizeof(NQPREQRY*));
				else
					*condptr = $3.qry;
			} |
			NOT CondStmt
			{
				$$.qry = $2.qry;
				$$.qry->op = ($$.qry->op & NQOPNOT) ? $$.qry->op & ~NQOPNOT : $$.qry->op | NQOPNOT;
			} |
			CondStmt LIMIT ICONST
			{
				$$.qry = $1.qry;
				$$.qry->lmt = strtol($3.str, NULL, 10);
			} |
			CondStmt ORDER BY ColumnStmt
			{
			} |
			'(' CondStmt ')' { $$ = $2; } |
			PredicateStmt { $$ = $1; };

PredicateStmt:	ComparisonStmt { $$ = $1; } |
				BetweenStmt { $$ = $1; } |
				InStmt { $$ = $1; } |
				LikeCfdStmt { $$ = $1; };

ComparisonStmt:	ColumnStmt NUMGT ScalarExp
				{
					$$.qry = nqpreqrynew(yymem());
					if (nqqryident($$.qry, $1.str))
					{
						$$.qry->op = NQOPNUMGT;
						$$.qry->sbj.str = $3.str;
					} else {
						yyerror("column name doesn't exist.");
					}
				} |
				ColumnStmt NUMGE ScalarExp
				{
					$$.qry = nqpreqrynew(yymem());
					if (nqqryident($$.qry, $1.str))
					{
						$$.qry->op = NQOPNUMGE;
						$$.qry->sbj.str = $3.str;
					} else {
						yyerror("column name doesn't exist.");
					}
				} |
				ColumnStmt NUMLT ScalarExp
				{
					$$.qry = nqpreqrynew(yymem());
					if (nqqryident($$.qry, $1.str))
					{
						$$.qry->op = NQOPNUMLT;
						$$.qry->sbj.str = $3.str;
					} else {
						yyerror("column name doesn't exist.");
					}
				} |
				ColumnStmt NUMLE ScalarExp
				{
					$$.qry = nqpreqrynew(yymem());
					if (nqqryident($$.qry, $1.str))
					{
						$$.qry->op = NQOPNUMLE;
						$$.qry->sbj.str = $3.str;
					} else {
						yyerror("column name doesn't exist.");
					}
				} |
				ColumnStmt COLNE ScalarExp
				{
					$$.qry = nqpreqrynew(yymem());
					if (nqqryident($$.qry, $1.str))
					{
						$$.qry->op = (NUMTYPE == nqqrytype($1.str)) ? NQOPNUMEQ | NQOPNOT : NQOPSTREQ | NQOPNOT;
						$$.qry->sbj.str = $3.str;
					} else {
						yyerror("column name doesn't exist.");
					}
				} |
				ColumnStmt COLEQ ScalarExp
				{
					$$.qry = nqpreqrynew(yymem());
					if (nqqryident($$.qry, $1.str))
					{
						$$.qry->op = (NUMTYPE == nqqrytype($1.str)) ? NQOPNUMEQ : NQOPSTREQ;
						$$.qry->sbj.str = $3.str;
					} else {
						yyerror("column name doesn't exist.");
					}
				};

BetweenStmt:	ColumnStmt BETWEEN ScalarExp AND ScalarExp
				{
					$$.qry = nqpreqrynew(yymem());
					$$.qry->op = NQOPNUMBT;
					if (nqqryident($$.qry, $1.str))
					{
						int len = strlen($3.str) + 1 + strlen($5.str);
						$$.qry->sbj.str = (char*)apr_palloc(yymem(), len + 1);
						$$.qry->sbj.str[len] = '\0';
						memcpy($$.qry->sbj.str, $3.str, strlen($3.str));
						$$.qry->sbj.str[strlen($3.str)] = ' ';
						memcpy($$.qry->sbj.str + strlen($3.str) + 1, $5.str, strlen($5.str));
					} else {
						yyerror("column name doesn't exist.");
					}
				};

InStmt:	ScalarExp IN SubQueryStmt;

LikeCfdStmt:	LikeStmt |
				LikeStmt ScalarExp '%'
				{
					$$ = $1;
					$$.qry->cfd = strtod($2.str, NULL) * 0.01;
				};

LikeStmt:	ColumnStmt EXACT LIKE UUIDENT
			{
				$$.qry = nqpreqrynew(yymem());
				if (nqqryident($$.qry, $1.str))
				{
					$$.qry->op = NQOPELIKE;
					$$.qry->sbj.str = $4.str;
				} else {
					yyerror("column name doesn't exist.");
				}
			} |
			ColumnStmt LIKE UUIDENT
			{
				$$.qry = nqpreqrynew(yymem());
				if (nqqryident($$.qry, $1.str))
				{
					$$.qry->op = NQOPLIKE;
					$$.qry->sbj.str = $3.str;
				} else {
					yyerror("column name doesn't exist.");
				}
			} |
			ColumnStmt EXACT LIKE SubQueryStmt
			{
				$$.qry = nqpreqrynew(yymem());
				if (nqqryident($$.qry, $1.str))
				{
					$$.qry->op = NQOPELIKE;
					$$.qry->sbj.subqry = $4.qry;
				} else {
					yyerror("column name doesn't exist.");
				}
			} |
			ColumnStmt LIKE SubQueryStmt
			{
				$$.qry = nqpreqrynew(yymem());
				if (nqqryident($$.qry, $1.str))
				{
					$$.qry->op = NQOPLIKE;
					$$.qry->sbj.subqry = $3.qry;
				} else {
					yyerror("column name doesn't exist.");
				}
			};

ColumnListStmt:	ColumnStmt |
				ColumnListStmt ',' ColumnStmt;

ColumnStmt:	UUID { $$ = $1; } |
			IDENT { $$ = $1; } |
			IDENT '.' IDENT
			{
				int len = strlen($1.str) + 1 + strlen($3.str);
				$$.str = (char*)apr_palloc(yymem(), len + 1);
				$$.str[len] = '\0';
				memcpy($$.str, $1.str, strlen($1.str));
				$$.str[strlen($1.str)] = '.';
				memcpy($$.str + strlen($1.str) + 1, $3.str, strlen($3.str));
			} |
			IDENT '.' IDENT '.' IDENT
			{
				int len = strlen($1.str) + 1 + strlen($3.str) + 1 + strlen($5.str);
				$$.str = (char*)apr_palloc(yymem(), len + 1);
				$$.str[len] = '\0';
				memcpy($$.str, $1.str, strlen($1.str));
				$$.str[strlen($1.str)] = '.';
				memcpy($$.str + strlen($1.str) + 1, $3.str, strlen($3.str));
				$$.str[strlen($1.str) + 1 + strlen($3.str)] = '.';
				memcpy($$.str + strlen($1.str) + 1 + strlen($3.str) + 1, $5.str, strlen($5.str));
			};

SubQueryStmt: '(' SelectStmt ')' { $$ = $2; };

ScalarExp:	ICONST { $$ = $1; } |
			FCONST { $$ = $1; } |
			'"' IDENT '"' { $$ = $2; };

%%

NQPREQRY* yyresult()
{
	return YY_RESULT;
}

static int nqqrytype(char* str)
{
	switch (str[0])
	{
		case 'e': /* "exif" */
			switch (str[5])
			{
				case 'm':
					switch (str[7])
					{
						case 'k': /* "make" */
						case 'd': /* "model" */
							return STRTYPE;
						default:
							return NUMTYPE;
					}
				default:
					return NUMTYPE;
			}
		default:
			return NOTYPE;
	}
	return NOTYPE;
}

static bool nqqryident(NQPREQRY* qry, char* str)
{
	char* spliter = strchr(str, '.');
	if (spliter != NULL)
		spliter[0] = '\0';
	const ScanDatabase* db = ScanDatabaseLookup(str);
	if (db != NULL)
	{
		qry->type = db->type;
		qry->db = db->name;
		if (qry->type == NQTTCTDB && spliter != NULL)
		{
			qry->col = (char*)apr_palloc(yymem(), strlen(spliter + 1) + 1);
			qry->col[strlen(spliter + 1)] = '\0';
		}
		return true;
	}
	return false;
}
