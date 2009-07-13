%{
#include "_parser.h"
#include "gram.h"
static char* nqstrdup(const char* str, unsigned int maxlen);
static char* downcase_truncate_identifier(const char* ident, int maxlen);
%}

%option 8bit
%option never-interactive
%option nodefault
%option noinput
%option nounput
%option noyywrap

space			[ \t\n\r\f]
horiz_space		[ \t\f]
newline			[\n\r]
non_newline		[^\n\r]

comment			("--"{non_newline}*)

whitespace		({space}+|{comment})

uuidstart		#
uuident			{uuidstart}[A-Za-z0-9\-_]{22}

/* Double quote */
dquote			\"

digit			[0-9]
ident_start		[A-Za-z\200-\377_]
ident_cont		[A-Za-z\200-\377_0-9\$]

identifier		{ident_start}{ident_cont}*

self			[,()\[\].;\:]
operator		[\^\&\+\-\*\/\%]{1}

integer			{digit}+
decimal			(({digit}*\.{digit}+)|({digit}+\.{digit}*))
real			({integer}|{decimal})[Ee][-+]?{digit}+

%%

{whitespace} |
{comment}
				{
					/* ignore */
				}

{dquote}   |
{self}     |
{operator}		{
					return yytext[0];
				}

">"				{ return NUMGT; }
">="	|
"=>"			{ return NUMGE; }
"<"				{ return NUMLT; }
"<="	|
"=<"			{ return NUMLE; }
"<>"	|
"!="			{ return COLNE; }
"="		|
"=="			{ return COLEQ; }

{integer}		{
					yylval.str = nqstrdup(yytext, yyleng);
					return ICONST;
				}

{decimal}	|
{real}			{
					yylval.str = nqstrdup(yytext, yyleng);
					return FCONST;
				}

{uuidstart}		{
					return UUID;
				}

{uuident}		{
					yylval.str = nqstrdup(yytext + 1, yyleng - 1);
					return UUIDENT;
				}

{identifier}	{
					const ScanKeyword *keyword;
					char *ident;
					keyword = ScanKeywordLookup(yytext);
					if (keyword != NULL)
					{
						yylval.keyword = keyword->name;
						//printf("keyword: %s\n", yylval.str);
						return keyword->value;
					}
					ident = downcase_truncate_identifier(yytext, yyleng);
					yylval.str = ident;
					//printf("other word: %s\n", yylval.str);
					return IDENT;
				}

%%

bool YY_ERROR;

void yyerror(const char* s)
{
	fprintf(stderr, "yyerror: %s at %s\n", s, yytext);
	YY_ERROR = true;
}

static apr_pool_t* YY_MEM = 0;

apr_pool_t* yymem()
{
	return YY_MEM;
}

static char* nqstrdup(const char* str, unsigned int maxlen)
{
	unsigned int tlen = strlen(str);
	int len = (tlen > maxlen) ? maxlen : tlen;
	char* result = (char*)apr_palloc(yymem(), len + 1);
	memcpy(result, str, len);
	result[len] = '\0';
	return result;
}

static char* nqstrdup_m(const char* str, unsigned int maxlen)
{
	unsigned int tlen = strlen(str);
	int len = (tlen > maxlen) ? maxlen : tlen;
	char* result = (char*)malloc(len + 1);
	memcpy(result, str, len);
	result[len] = '\0';
	return result;
}

#define HIGHBIT                 (0x80)
#define IS_HIGHBIT_SET(ch)      ((unsigned char)(ch) & HIGHBIT)

static char* downcase_truncate_identifier(const char* ident, int maxlen)
{
	unsigned int tlen = strlen(ident);
	int i, len = (tlen > maxlen) ? maxlen : tlen;

	char* result = (char*)apr_palloc(yymem(), len + 1);

	for (i = 0; i < len; i++)
	{
		unsigned char ch = (unsigned char)ident[i];

		if (ch >= 'A' && ch <= 'Z')
			ch += 'a' - 'A';
		else if (IS_HIGHBIT_SET(ch) && isupper(ch))
			ch = tolower(ch);
		result[i] = (char)ch;
	}
	result[i] = '\0';
	return result;
}

static apr_pool_t* root = 0;
static apr_thread_mutex_t* parse_mtx = 0;
static frl_slab_pool_t* prpool = 0;
static frl_slab_pool_t* mppool = 0;

void nqparseresultdel(NQPARSERESULT* result)
{
	switch (result->type)
	{
		case NQRTSELECT:
			nqpreqrydel((NQPREQRY*)result->result);
			break;
		case NQRTINSERT:
		case NQRTUPDATE:
		case NQRTDELETE:
			NQMANIPULATE* mp = (NQMANIPULATE*)result->result;
			switch (mp->type)
			{
				case NQMPSIMPLE:
					free(mp->sbj.str);
					break;
				case NQMPUUIDENT:
					free(mp->sbj.str);
					if (mp->col != 0)
						free(mp->col);
					free(mp->val);
					break;
				case NQMPWHERE:
					nqpreqrydel(mp->sbj.qry);
					if (mp->col != 0)
						free(mp->col);
					free(mp->val);
					break;
			}
			frl_slab_pfree(mp);
			break;
	}
	frl_slab_pfree(result);
}

NQPARSERESULT* nqparse(char* str, int siz)
{
	if (root == 0)
		apr_pool_create(&root, NULL);
	if (parse_mtx == 0)
		apr_thread_mutex_create(&parse_mtx, APR_THREAD_MUTEX_DEFAULT, root);
	apr_thread_mutex_lock(parse_mtx);
	if (yymem() == 0)
		apr_pool_create(&YY_MEM, root);
	if (YY_CURRENT_BUFFER)
		yy_delete_buffer(YY_CURRENT_BUFFER);
	char* scanbuf = (char*)apr_palloc(yymem(), siz + 2);
	memcpy(scanbuf, str, siz);
	scanbuf[siz] = scanbuf[siz + 1] = YY_END_OF_BUFFER_CHAR;
	YY_BUFFER_STATE scanbufhandle = yy_scan_buffer(scanbuf, siz + 2);
	YY_ERROR = false;
	yyparse();
	yy_delete_buffer(scanbufhandle);
	NQPARSERESULT* result = 0;
	if (!YY_ERROR)
	{
		if (prpool == 0)
			frl_slab_pool_create(&prpool, root, 256, sizeof(NQPARSERESULT), FRL_LOCK_WITH);
		result = (NQPARSERESULT*)frl_slab_palloc(prpool);
		result->type = yyresult()->type;
		switch (result->type)
		{
			case NQRTSELECT:
				result->result = nqpreqrydup((NQPREQRY*)yyresult()->result);
				break;
			case NQRTINSERT:
			case NQRTUPDATE:
			case NQRTDELETE:
				if (mppool == 0)
					frl_slab_pool_create(&mppool, root, 128, sizeof(NQMANIPULATE), FRL_LOCK_WITH);
				NQMANIPULATE* mp = (NQMANIPULATE*)frl_slab_palloc(mppool);
				memcpy(mp, yyresult()->result, sizeof(NQMANIPULATE));
				switch (mp->type)
				{
					case NQMPSIMPLE:
						mp->sbj.str = nqstrdup_m(mp->sbj.str, 256);
						break;
					case NQMPUUIDENT:
						mp->sbj.str = nqstrdup_m(mp->sbj.str, 256);
						mp->col = (mp->col == 0) ? 0 : nqstrdup_m(mp->col, 256);
						mp->val = nqstrdup_m(mp->val, 256);
						break;
					case NQMPWHERE:
						mp->sbj.qry = nqpreqrydup(mp->sbj.qry);
						mp->col = (mp->col == 0) ? 0 : nqstrdup_m(mp->col, 256);
						mp->val = nqstrdup_m(mp->val, 256);
						break;
				}
				result->result = mp;
				break;
		}
	}
	apr_pool_clear(yymem());
	apr_thread_mutex_unlock(parse_mtx);
	return result;
}

void nqparsedel()
{
	if (parse_mtx != 0)
		apr_thread_mutex_destroy(parse_mtx);
	if (mppool != 0)
		frl_slab_pool_destroy(mppool);
	if (prpool != 0)
		frl_slab_pool_destroy(prpool);
	if (YY_MEM != 0)
		apr_pool_destroy(YY_MEM);
	if (root != 0)
		apr_pool_destroy(root);
}
