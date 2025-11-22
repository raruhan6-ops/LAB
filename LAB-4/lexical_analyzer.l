%{
#include <stdio.h>
#include <string.h>

int line = 1;

void print_token(const char *type, const char *text);
%}

%option noyywrap

%%
main|if|then|else|while|do|repeat|until|\
for|from|to|step|switch|of|case|default|\
return|integer|real|char|bool|\
and|or|not|mod|read|write          { print_token("KEYWORD", yytext); }

"=="|"="|"<="|">="|"!="|"<"|">"|"+"|"-"|"*"|"/"   { print_token("OPERATOR", yytext); }

","|";"|":"|"{"|"}"|"["|"]"|"("|")"               { print_token("DELIMITER", yytext); }

[a-zA-Z][a-zA-Z0-9]*                              { print_token("IDENTIFIER", yytext); }

[0-9]+                                             { print_token("NUMBER", yytext); }

[ \t]+                                             { /* ignore whitespace */ }

\n                                                 { line++; }

.                                                  { printf("Unknown symbol: %s\n", yytext); }
%%

void print_token(const char *type, const char *text) {
    FILE *fp = fopen("tokens.txt", "a");
    fprintf(fp, "<%s, %s>\n", type, text);
    fclose(fp);
    printf("<%s, %s>\n", type, text);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <inputfile>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) {
        printf("Cannot open file %s\n", argv[1]);
        return 1;
    }

    yyin = f;
    remove("tokens.txt");   // clear old file
    yylex();
    fclose(f);
    printf("\nTokens saved in tokens.txt\n");
    return 0;
}
