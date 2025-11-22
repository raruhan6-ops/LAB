%{
#include "parser.tab.h"
#include <string.h>
#include <stdlib.h>
%}

%%

"int"      { return INT; }
"float"    { return FLOAT; }
"if"       { return IF; }
"then"     { return THEN; }
"else"     { return ELSE; }
"while"    { return WHILE; }
"do"       { return DO; }

"=="       { yylval.str = strdup(yytext); return ROP; }
"<="       { yylval.str = strdup(yytext); return ROP; }
">="       { yylval.str = strdup(yytext); return ROP; }
"!="       { yylval.str = strdup(yytext); return ROP; }
"<"        { yylval.str = strdup(yytext); return ROP; }
">"        { yylval.str = strdup(yytext); return ROP; }

"{"        { return LBRACE; }
"}"        { return RBRACE; }
";"        { return SEMI; }
","        { return COMMA; }
"="        { return ASSIGN; }
"("        { return LPAREN; }
")"        { return RPAREN; }
"+"        { return PLUS; }
"*"        { return TIMES; }

[0-9]+     { yylval.str = strdup(yytext); return NUM; }

[a-zA-Z_][a-zA-Z0-9_]* {
               yylval.str = strdup(yytext);
               return ID;
           }

[ \t\r\n]+  { /* skip whitespace */ }

.           { /* ignore unknown char */ }

%%

int yywrap(void) { return 1; }
