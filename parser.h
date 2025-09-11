#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>

/* ==== BNF GRAMMAR ==========================
 * program = stmt*
 *    stmt = NAME "=" expr ";"
 *    expr = "(" expr ")"
 *         | "fn" NAME* "=>" block     -> fn_def
 *         | block
 *         | expr expr*                -> apply
 *         | INT | STR | REAL | NAME
 *   block = "{" stmt* expr "}"
 *
 *   STR, INT, REAL are all normal
 *   NAME can be pretty much anything
 *   COMMENT: "#(.*)$"
 * ===========================================
 */

// ===== LEXER ===============================

struct Token {
    enum TokenType { NAME, INT, STR, REAL,
        LARROW, RARROW, LPAREN, RPAREN, SEMICOLON, LBRACE, RBRACE, FN, EQUALS,
        WHITESPACE, COMMENT, EOF_, ERROR } type;
    union {
        char* s;
        int i;
        double f;
    };
    int len;
};

int print_token(struct Token tok);

struct Token lex(char* s);

// ===== PARSER===============================

struct ASTList;
struct ASTList_;

struct ASTList {
    struct ASTList_* first;
    struct ASTList_* last;
};

struct AST {
    enum TreeType { PROGRAM, STMT, FN_DEF, APPLY, BLOCK, LITERAL, BACKTRACK } type;
    union {
        struct ASTList children;
        struct Token literal;
    };
};

struct ASTList_ {
    struct AST here;
    struct ASTList_* next;
};

void print_ast(struct AST ast, int level);

struct Token get_token(void);
void unget_token(struct Token tok);

struct ASTList new_ASTList(void);
void add_child(struct ASTList* l, struct AST ast);

struct AST parse_literal(enum TokenType type);
struct AST parse_any_literal(void);
struct AST parse_block(void);
struct AST parse_apply(void);
struct AST parse_expr_no_apply(void);
struct AST parse_expr(void);
struct AST parse_stmt(void);
struct AST parse_program(void);

