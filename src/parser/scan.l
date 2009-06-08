%{
#include "parser.h"
#include "gram.h"

static char* nqstrdup(const char* str, int maxlen);
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
					ident = nqstrdup(yytext, NAMEDATALEN);
					//ident = downcase_truncate_identifier(yytext, yyleng, true);
					yylval.str = ident;
					return IDENT;
				}

%%

void yyerror(char* s)
{
	printf("yyerror: %s at %s\n", s, yytext);
}

static char* nqstrdup(const char* str, int maxlen)
{
	int tlen = strlen(str);
	int i, len = (tlen > maxlen) ? maxlen : tlen;
	char* ret = (char*)malloc(len + 1);
	for (i = 0; i < len; i++)
		ret[i] = str[i];
	ret[len] = '\0';
	return ret;
}

int main()
{
	yyparse();
}
