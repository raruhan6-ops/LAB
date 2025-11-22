%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- Symbol table ---------- */

typedef struct {
    char name[32];
    char type[16];
} Symbol;

Symbol symtab[100];
int symcount = 0;

char curType[16];        /* current declaration type: "int" or "float" */

int lookup(char *name) {
    for (int i = 0; i < symcount; i++) {
        if (strcmp(symtab[i].name, name) == 0)
            return i;
    }
    return -1;
}

void addSymbol(char *name, char *type) {
    if (lookup(name) != -1) return;  /* already exists */
    strcpy(symtab[symcount].name, name);
    strcpy(symtab[symcount].type, type);
    symcount++;
}

void printSymtab(void) {
    printf("Index\tName\tType\n");
    for (int i = 0; i < symcount; i++) {
        printf("%d\t%s\t%s\n", i, symtab[i].name, symtab[i].type);
    }
}

/* ---------- Quadruples ---------- */

typedef struct {
    char op[10];
    char arg1[32];
    char arg2[32];
    char result[32];
} Quad;

Quad quads[200];
int qc = 0;   /* count of quads */

int nextInstr(void) { return qc; }

void emit(const char *op, const char *a1, const char *a2, const char *res) {
    strcpy(quads[qc].op,    op  ? op  : "");
    strcpy(quads[qc].arg1,  a1  ? a1  : "");
    strcpy(quads[qc].arg2,  a2  ? a2  : "");
    strcpy(quads[qc].result,res ? res : "");
    qc++;
}

int tempCount = 0;
char *newTemp(void) {
    char buf[32];
    sprintf(buf, "t%d", tempCount++);
    char *p = (char*)malloc(strlen(buf) + 1);
    strcpy(p, buf);
    return p;
}

/* we’ll keep the last expression’s result here, for while conditions */
char *lastExprTemp = NULL;

/* ---------- declarations for bison ---------- */

int yylex(void);
void yyerror(const char *s);
%}

/* ---------- Bison definitions ---------- */

%union {
    char *str;   /* for IDs, NUMs, temporaries */
    int   num;   /* for instruction indices (M, N) */
}

%token INT FLOAT IF THEN ELSE WHILE DO
%token <str> ID NUM
%token <str> ROP
%token LBRACE RBRACE SEMI COMMA ASSIGN LPAREN RPAREN PLUS TIMES

%type <str> E
%type <num> M N

%%

Program
    : Decls StmtList
      {
          printf("\n=== Symbol Table ===\n");
          printSymtab();

          printf("\n=== Quadruples ===\n");
          for (int i = 0; i < qc; i++) {
              printf("%2d: (%s, %s, %s, %s)\n",
                     i,
                     quads[i].op,
                     quads[i].arg1,
                     quads[i].arg2,
                     quads[i].result);
          }
      }
    ;

/* Declarations: int s,i;  float x,y,z; */

Decls
    : /* empty */
    | Decls Decl
    ;

Decl
    : Type IdList SEMI
    ;

Type
    : INT   { strcpy(curType, "int"); }
    | FLOAT { strcpy(curType, "float"); }
    ;

IdList
    : ID                { addSymbol($1, curType); }
    | IdList COMMA ID   { addSymbol($3, curType); }
    ;

/* Statement list */

StmtList
    : Stmt
    | StmtList Stmt
    ;

Stmt
    : ID ASSIGN E SEMI               /* assignment */
      {
          emit("=", $3, "-", $1);
      }
    | WHILE M LPAREN E RPAREN N Stmt /* while(E) S */
      {
          /* $2 = start of condition (from M) */
          /* $6 = index of IF_FALSE quad (from N) */

          char buf[32];

          /* add GOTO back to beginning of condition */
          sprintf(buf, "%d", $2);
          emit("GOTO", "", "", buf);

          /* patch IF_FALSE target to instruction after the loop */
          sprintf(buf, "%d", nextInstr());
          strcpy(quads[$6].result, buf);
      }
    | LBRACE StmtList RBRACE         /* block { ... } */
      {
          /* nothing extra needed */
      }
    ;

/* Marker before condition: remember the index of the next instruction */
M
    : /* empty */
      {
          $$ = nextInstr();
      }
    ;

/* After condition: emit IF_FALSE temp, -, ? and remember its position */
N
    : /* empty */
      {
          emit("IF_FALSE", lastExprTemp, "", "");
          $$ = nextInstr() - 1;   /* index of that IF_FALSE */
      }
    ;

/* Expressions: arithmetic + relational id rop num */

E
    : E PLUS E
      {
          char *t = newTemp();
          emit("+", $1, $3, t);
          $$ = t;
          lastExprTemp = t;
      }
    | E TIMES E
      {
          char *t = newTemp();
          emit("*", $1, $3, t);
          $$ = t;
          lastExprTemp = t;
      }
    | LPAREN E RPAREN
      {
          $$ = $2;
          lastExprTemp = $2;
      }
    | ID
      {
          $$ = $1;
          lastExprTemp = $1;
      }
    | NUM
      {
          $$ = $1;
          lastExprTemp = $1;
      }
    | ID ROP NUM       /* relational: id < num, id == num, etc. */
      {
          char *t = newTemp();
          emit($2, $1, $3, t);
          $$ = t;
          lastExprTemp = t;
      }
    ;

%%

int main(void) {
    printf("Parsing...\n");
    if (yyparse() == 0) {
        printf("\nParse success.\n");
    } else {
        printf("\nParse failed.\n");
    }
    return 0;
}

void yyerror(const char *s) {
    fprintf(stderr, "Error: %s\n", s);
}
