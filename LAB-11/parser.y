%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void yyerror(const char* s);
int yylex(void);

// ---------- symbol table ----------
typedef struct {
    char* name;
    char* type;
} Symbol;

Symbol symtab[256];
int symcount = 0;

// ---------- quadruples ----------
typedef struct {
    char op[8];
    char arg1[32];
    char arg2[32];
    char res[32];
} Quad;

Quad quads[1024];
int quadcount = 0;

char* current_type = NULL;

char* newTemp() {
    static int tempNo = 0;
    char buf[32];
    sprintf(buf, "t%d", tempNo++);
    return strdup(buf);
}

void addSymbol(const char* name, const char* type) {
    for (int i = 0; i < symcount; ++i) {
        if (strcmp(symtab[i].name, name) == 0) return;
    }
    symtab[symcount].name = strdup(name);
    symtab[symcount].type = strdup(type);
    symcount++;
}

int findSymbol(const char* name) {
    for (int i = 0; i < symcount; ++i) {
        if (strcmp(symtab[i].name, name) == 0) return i;
    }
    return -1;
}
%}

%union {
    char* str;  // identifier / number / temp name
}

/* tokens */
%token INT FLOAT IF THEN ELSE WHILE DO
%token <str> ID NUM
%token <str> ROP          /* relational operator: <, >, ==, etc. */
%token LBRACE RBRACE
%token SEMI COMMA ASSIGN
%token LPAREN RPAREN
%token PLUS TIMES

/* precedence */
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
%left PLUS
%left TIMES

/* typed nonterminals */
%type <str> Type E IdList

%start Program

%%

Program
    : DeclList StmtList
    ;

DeclList
    : /* empty */
    | DeclList D
    ;

D
    : Type IdList SEMI
    ;

Type
    : INT   { current_type = "int"; }
    | FLOAT { current_type = "float"; }
    ;

IdList
    : ID              { addSymbol($1, current_type); }
    | IdList COMMA ID { addSymbol($3, current_type); }
    ;

StmtList
    : StmtList S
    | S
    ;

S
    : A
    | IF E THEN S %prec LOWER_THAN_ELSE
    | IF E THEN S ELSE S
    | WHILE E DO S
    | LBRACE StmtList RBRACE
    ;

A
    : ID ASSIGN E SEMI
      {
        if (findSymbol($1) < 0) {
            addSymbol($1, "int");
        }
        strcpy(quads[quadcount].op, "=");
        strcpy(quads[quadcount].arg1, $3);
        quads[quadcount].arg2[0] = '\0';
        strcpy(quads[quadcount].res, $1);
        quadcount++;
      }
    ;

/* Expressions: arithmetic + simple relational id ROP NUM */
E
    : E PLUS E
      {
        char* t = newTemp();
        strcpy(quads[quadcount].op, "+");
        strcpy(quads[quadcount].arg1, $1);
        strcpy(quads[quadcount].arg2, $3);
        strcpy(quads[quadcount].res, t);
        quadcount++;
        $$ = t;
      }
    | E TIMES E
      {
        char* t = newTemp();
        strcpy(quads[quadcount].op, "*");
        strcpy(quads[quadcount].arg1, $1);
        strcpy(quads[quadcount].arg2, $3);
        strcpy(quads[quadcount].res, t);
        quadcount++;
        $$ = t;
      }
    | LPAREN E RPAREN
      { $$ = $2; }
    | ID
      {
        if (findSymbol($1) < 0) {
            addSymbol($1, "int");
        }
        $$ = $1;
      }
    | NUM
      { $$ = $1; }
    | ID ROP NUM
      {
        char* t = newTemp();
        strcpy(quads[quadcount].op, $2);  /* "<", ">" etc */
        strcpy(quads[quadcount].arg1, $1);
        strcpy(quads[quadcount].arg2, $3);
        strcpy(quads[quadcount].res, t);
        quadcount++;
        $$ = t;
      }
    ;

%%

void yyerror(const char* s) {
    fprintf(stderr, "Parse error: %s\n", s);
}

int main(void) {
    if (yyparse() == 0) {
        printf("Parse success.\n");

        printf("\nSymbol table:\n");
        for (int i = 0; i < symcount; ++i) {
            printf("%d: %s\t%s\n", i, symtab[i].name, symtab[i].type);
        }

        printf("\nQuadruples:\n");
        for (int i = 0; i < quadcount; ++i) {
            printf("%d: (%s, %s, %s, %s)\n",
                   i,
                   quads[i].op,
                   quads[i].arg1,
                   quads[i].arg2,
                   quads[i].res);
        }
    }
    return 0;
}
