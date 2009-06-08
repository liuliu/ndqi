%{
//#define YYDEBUG 1
#include "parser.h"
%}

%union
{
	int	ival;
	char chr;
	char *str;
	const char *keyword;
	NQPREQRY* qry;
}

%token ALL, ANY, ASC, BETWEEN, BY, DESC, DISTINCT, EXACT, EXISTS, FROM, IN, LIKE, LIMIT, ORDER, SELECT, SOME, WHERE
%token UUID, UUIDENT, IDENT, FCONST, ICONST
%token NUMGT, NUMGE, NUMLT, NUMLE, STRNE, STREQ
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
			};

ColumnOptListStmt:	/* empty value */ |
					ColumnListStmt;

CondStmt:	CondStmt OR CondStmt
			{
				$$.qry = nqpreqrynew();
				$$.qry->cnum = 2;
				$$.qry->op = NQCTOR;
				$$.qry->conds = (NQPREQRY**)malloc(2 * sizeof(NQPREQRY*));
				$$.qry->conds[0] = $1.qry;
				$$.qry->conds[1] = $3.qry;
			} |
			CondStmt AND CondStmt
			{
				$$.qry = nqpreqrynew();
				$$.qry->cnum = 2;
				$$.qry->op = NQCTAND;
				$$.qry->conds = (NQPREQRY**)malloc(2 * sizeof(NQPREQRY*));
				$$.qry->conds[0] = $1.qry;
				$$.qry->conds[1] = $3.qry;
			} |
			NOT CondStmt
			{
				$$.qry = $2.qry;
				$$.qry->op = ($$.qry->op & NQOPNOT) ? $$.qry->op & !NQOPNOT : $$.qry->op | NQOPNOT;
			} |
			'(' CondStmt ')' { $$ = $2; } |
			PredicateStmt { $$ = $1; };

PredicateStmt:	ComparisonStmt { $$ = $1; } |
				BetweenStmt { $$ = $1; } |
				InStmt { $$ = $1; } |
				LikeStmt { $$ = $1; };

ComparisonStmt:	ColumnStmt NUMGT ScalarExp
				{
					$$.qry = nqpreqrynew();
					if (nqqryident($$.qry, $1.str))
					{
						$$.qry->op = NQOPNUMGT;
						$$.qry->sbj.str = $3.str;
					} else {
						nqpreqrydel($$.qry);
						yyerror("column name doesn't exist.");
					}
				} |
				ColumnStmt NUMGE ScalarExp
				{
					$$.qry = nqpreqrynew();
					if (nqqryident($$.qry, $1.str))
					{
						$$.qry->op = NQOPNUMGE;
						$$.qry->sbj.str = $3.str;
					} else {
						nqpreqrydel($$.qry);
						yyerror("column name doesn't exist.");
					}
				} |
				ColumnStmt NUMLT ScalarExp
				{
					$$.qry = nqpreqrynew();
					if (nqqryident($$.qry, $1.str))
					{
						$$.qry->op = NQOPNUMLT;
						$$.qry->sbj.str = $3.str;
					} else {
						nqpreqrydel($$.qry);
						yyerror("column name doesn't exist.");
					}
				} |
				ColumnStmt NUMLE ScalarExp
				{
					$$.qry = nqpreqrynew();
					if (nqqryident($$.qry, $1.str))
					{
						$$.qry->op = NQOPNUMLE;
						$$.qry->sbj.str = $3.str;
					} else {
						nqpreqrydel($$.qry);
						yyerror("column name doesn't exist.");
					}
				} |
				ColumnStmt STRNE ScalarExp
				{
					$$.qry = nqpreqrynew();
					if (nqqryident($$.qry, $1.str))
					{
						$$.qry->op = NQOPSTREQ | NQOPNOT;
						$$.qry->sbj.str = $3.str;
					} else {
						nqpreqrydel($$.qry);
						yyerror("column name doesn't exist.");
					}
				} |
				ColumnStmt STREQ ScalarExp
				{
					$$.qry = nqpreqrynew();
					if (nqqryident($$.qry, $1.str))
					{
						$$.qry->op = NQOPSTREQ;
						$$.qry->sbj.str = $3.str;
					} else {
						nqpreqrydel($$.qry);
						yyerror("column name doesn't exist.");
					}
				};

BetweenStmt:	ColumnStmt BETWEEN ScalarExp AND ScalarExp;

InStmt:	ScalarExp IN SubQueryStmt;

LikeStmt:	ColumnStmt EXACT LIKE UUIDENT
			{
				$$.qry = nqpreqrynew();
				if (nqqryident($$.qry, $1.str))
				{
					$$.qry->op = NQOPELIKE;
					$$.qry->sbj.str = $4.str;
				} else {
					nqpreqrydel($$.qry);
					yyerror("column name doesn't exist.");
				}
			} |
			ColumnStmt LIKE UUIDENT
			{
				$$.qry = nqpreqrynew();
				if (nqqryident($$.qry, $1.str))
				{
					$$.qry->op = NQOPLIKE;
					$$.qry->sbj.str = $3.str;
				} else {
					nqpreqrydel($$.qry);
					yyerror("column name doesn't exist.");
				}
			} |
			ColumnStmt EXACT LIKE SubQueryStmt
			{
				$$.qry = nqpreqrynew();
				if (nqqryident($$.qry, $1.str))
				{
					$$.qry->op = NQOPELIKE;
					$$.qry->sbj.subqry = $4.qry;
				} else {
					nqpreqrydel($$.qry);
					yyerror("column name doesn't exist.");
				}
			} |
			ColumnStmt LIKE SubQueryStmt
			{
				$$.qry = nqpreqrynew();
				if (nqqryident($$.qry, $1.str))
				{
					$$.qry->op = NQOPLIKE;
					$$.qry->sbj.subqry = $3.qry;
				} else {
					nqpreqrydel($$.qry);
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
				$$.str = (char*)malloc(len + 1);
				$$.str[len] = '\0';
				memcpy($$.str, $1.str, strlen($1.str));
				$$.str[strlen($1.str)] = '.';
				memcpy($$.str + strlen($1.str) + 1, $3.str, strlen($3.str));
				free($1.str);
				free($3.str);
			} |
			IDENT '.' IDENT '.' IDENT
			{
				int len = strlen($1.str) + 1 + strlen($3.str) + 1 + strlen($5.str);
				$$.str = (char*)malloc(len + 1);
				$$.str[len] = '\0';
				memcpy($$.str, $1.str, strlen($1.str));
				$$.str[strlen($1.str)] = '.';
				memcpy($$.str + strlen($1.str) + 1, $3.str, strlen($3.str));
				$$.str[strlen($1.str) + 1 + strlen($3.str)] = '.';
				memcpy($$.str + strlen($1.str) + 1 + strlen($3.str) + 1, $5.str, strlen($5.str));
				free($1.str);
				free($3.str);
				free($5.str);
			};

SubQueryStmt: '(' SelectStmt ')' { $$ = $2; };

ScalarExp:	ICONST { $$ = $1; } |
			FCONST { $$ = $1; } |
			'"' IDENT '"' { $$ = $2; };

%%

static bool nqqryident(NQPREQRY* qry, char* str)
{
	switch (str[0])
	{
		case 'e': /* "exif" */
			qry->type = NQTTCTDB;
			qry->db = NQDBEXIF;
			qry->col = (char*)malloc(strlen(str) - 4);
			memcpy(qry->col, str + 5, strlen(str) - 5);
			qry->col[strlen(str) - 5] = '\0';
			break;
		case 'g': /* "gist" */
			qry->type = NQTFDB;
			qry->db = NQDBGIST;
			break;
		case 'l':
			switch (str[1])
			{
				case 'f': /* "lfd" */
					qry->type = NQTBWDB;
					qry->db = NQDBLFD;
					break;
				case 'h': /* "lh" */
					qry->type = NQTFDB;
					qry->db = NQDBLH;
					break;
				default:
					return false;
			}
			break;
		case 't':
			qry->type = NQTTCWDB;
			qry->db = NQDBTAG;
			break;
		default:
			return false;
	}
	return true;
}
