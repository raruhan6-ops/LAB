%{
#include "parser.tab.h"
#include <string.h>
%}

%%

int         { return INT; }
float       { return FLOAT; }
if          { return IF; }
then        { return THEN; }
else        { return ELSE; }
while       { return WHILE; }
do          { return DO; }

/* relational operators */
"=="        { yylval.str = _strdup(yytext); return ROP; }
"<="        { yylval.str = _strdup(yytext); return ROP; }
">="        { yylval.str = _strdup(yytext); return ROP; }
"!="        { yylval.str = _strdup(yytext); return ROP; }
"<"         { yylval.str = _strdup(yytext); return ROP; }
">"         { yylval.str = _strdup(yytext); return ROP; }

/* braces and operators */
"{"         { return LBRACE; }
"}"         { return RBRACE; }
";"         { return SEMI; }
","         { return COMMA; }
"="         { return ASSIGN; }
"("         { return LPAREN; }
")"         { return RPAREN; }
"+"         { return PLUS; }
"*"         { return TIMES; }

/* numbers */
[0-9]+      { yylval.str = _strdup(yytext); return NUM; }

/* identifiers */
[a-zA-Z_][a-zA-Z0-9_]* {
    yylval.str = _strdup(yytext);
    return ID;
}

/* whitespace */
[ \t\r\n]+  ;

/* ignore everything else */
.           ;

%%

int yywrap(void) { return 1; }
