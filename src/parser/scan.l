%{
#include "parser.h"
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
"!="			{ return STRNE; }
"="		|
"=="			{ return STREQ; }

{integer}		{
					yylval.str = nqstrdup(yytext, NAMEDATALEN);
					return ICONST;
				}

{decimal}		{
					yylval.str = nqstrdup(yytext, NAMEDATALEN);
					return FCONST;
				}

{real}			{
					yylval.str = nqstrdup(yytext, NAMEDATALEN);
					return FCONST;
				}

{uuidstart}		{
					return UUID;
				}

{uuident}		{
					yylval.str = nqstrdup(yytext + 1, NAMEDATALEN);
					return UUIDENT;
				}

{identifier}	{
					const ScanKeyword *keyword;
					char *ident;
					keyword = ScanKeywordLookup(yytext);
					if (keyword != NULL)
					{
						yylval.keyword = keyword->name;
						return keyword->value;
					}
					ident = downcase_truncate_identifier(yytext, NAMEDATALEN);
					yylval.str = ident;
					return IDENT;
				}

%%

bool YY_ERROR;

void yyerror(char* s)
{
	printf("yyerror: %s at %s\n", s, yytext);
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
	int i, len = (tlen > maxlen) ? maxlen : tlen;
	char* result = (char*)apr_palloc(yymem(), len + 1);
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

NQPREQRY* nqparse(char* str, int siz)
{
	if (yymem() == 0)
		apr_pool_create(&YY_MEM, NULL);
	if (YY_CURRENT_BUFFER)
		yy_delete_buffer(YY_CURRENT_BUFFER);
	char* scanbuf = (char*)apr_palloc(yymem(), siz + 2);
	memcpy(scanbuf, str, siz);
	scanbuf[siz] = scanbuf[siz + 1] = YY_END_OF_BUFFER_CHAR;
	YY_BUFFER_STATE scanbufhandle = yy_scan_buffer(scanbuf, siz + 2);
	YY_ERROR = false;
	yyparse();
	yy_delete_buffer(scanbufhandle);
	NQPREQRY* result = (YY_ERROR) ? 0 : nqpreqrydup(yyresult());
	apr_pool_clear(yymem());
	return result;
}

int main()
{
	apr_initialize();
	NQPREQRY* result;
	char ql1[] = "SELECT # WHERE LFD LIKE #Dh2uakSd98fnwEi3ne-1i3 AND EXIF.MAKE=\"SONY\";";
	result = nqparse(ql1, strlen(ql1));
	char ql3[] = "select where lh exact like #Dh2uakSd98fnwEi3ne-1_3 and exif.gps.latitude<102.938;";
	result = nqparse(ql3, strlen(ql3));
	char ql2[] = "SELECT # WHERE EXIF.MODEL=\"A720\" AND LFD EXACT LIKE (SELECT LFD WHERE EXIF.MAKE=\"CANON\");";
	result = nqparse(ql2, strlen(ql2));
}