#include "parser.h"

int print_token(struct Token tok) {
    switch (tok.type) {
        case NAME: return printf("NAME\t(%d): %s\n", tok.len, tok.s);
        case INT:  return printf("INT\t(%d): %d\n", tok.len, tok.i);
        case STR:  return printf("STR\t(%d): \"%s\"\n", tok.len, tok.s);
        case REAL: return printf("REAL\t(%d): %f\n", tok.len, tok.f);
        case LARROW: return printf("LARROW\t(2)\n");
        case RARROW: return printf("RARROW\t(2)\n");
        case LPAREN: return printf("LPAREN\t(1)\n");
        case RPAREN: return printf("RPAREN\t(1)\n");
        case SEMICOLON: return printf("SEMICOLON\t(1)\n");
        case LBRACE: return printf("LBRACE\t(1)\n");
        case RBRACE: return printf("RBRACE\t(1)\n");
        case FN: return printf("FN\t(2)\n");
        case EQUALS: return printf("EQUALS\t(1)\n");
        case WHITESPACE: return printf("WHITESPACE\t(%d)\n", tok.len);
        case COMMENT: return printf("COMMENT\t(%d)\n", tok.len);
        case EOF_: return printf("EOF\n");
        case ERROR: return printf("ERROR\n");
    }
}

void print_ast(struct AST ast, int level) {
    for (int i = 0; i < level; i++) printf(" ");
    switch (ast.type) {
        case PROGRAM: printf("PROGRAM\n"); break;
        case STMT: printf("STMT\n"); break;
        case FN_DEF: printf("FN_DEF\n"); break;
        case APPLY: printf("APPLY\n"); break;
        case BLOCK: printf("BLOCK\n"); break;
        case LITERAL: printf("LITERAL: "); print_token(ast.literal); return;
        case BACKTRACK: printf("BACKTRACK\n"); break;
    }
    for (struct ASTList_* elem = ast.children.first; elem != NULL; elem = elem->next)
        print_ast(elem->here, level + 2);
}

// ===== LEXER ===============================

// Gets next token
struct Token lex(char* s) {
    // Eat whitespace
    int i = 0;
    while (isspace(s[i])) i++;
    if (i > 0) return (struct Token){ .type = WHITESPACE, .len = i };
    switch (*s) {
        case '(': return (struct Token){ .type = LPAREN, .len = 1 };
        case ')': return (struct Token){ .type = RPAREN, .len = 1 };
        case '{': return (struct Token){ .type = LBRACE, .len = 1 };
        case '}': return (struct Token){ .type = RBRACE, .len = 1 };
        case ';': return (struct Token){ .type = SEMICOLON, .len = 1 };
        case '#':
            i = 0;
            while (s[i] != '\0' || s[i] != '\n') i++;
            return (struct Token){ .type = COMMENT, .len = i };
        case '\0':
            return (struct Token){ .type = EOF_, .len = 0 };
    }
    if (*s == 'f' && *(s + 1) == 'n') return (struct Token){ .type = FN, .len = 2 };
    if (*s == '=' && *(s + 1) == '>') return (struct Token){ .type = RARROW, .len = 2 };
    if (*s == '<' && *(s + 1) == '-') return (struct Token){ .type = LARROW, .len = 2 };
    if (*s == '=') return (struct Token){ .type = EQUALS, .len = 1 }; // Must be after the rarrow test

    // Strings
    if (*s == '"') {
        char* start_str = ++s;    // TODO Add support for escapes
        i = 0;
        while (s[i] != '"') i++;  // Can run past end of file TODO add checking
        char* new_s = malloc(i * sizeof(char));
        strncpy(new_s, start_str, i);
        return (struct Token){ .type = STR, .s = new_s, .len = i + 2 };
    }
    if (isdigit(*s)) {
        char* start = s;
        i = 0;
        while (isdigit(s[i])) i++;
        if (s[i] != '.') return (struct Token){ .type = INT, .i = atoi(start), .len = i };
        i++;
        while (isdigit(s[i])) i++;
        return (struct Token){ .type = REAL, .f = atof(start), .len = i };
    }
    // Names
    if (35 <= *s && *s <= 136 && *s != ';') {
        char* start = s;
        i = 0;
        while (!isspace(s[i]) && s[i] != ';' && s[i] != '(' && s[i] != ')') i++;
        char* new_s = malloc(i * sizeof(char));
        strncpy(new_s, start, i);
        return (struct Token){ .type = NAME, .s = new_s, .len = i };
    }

    return (struct Token){ .type = ERROR };
}

// ===== PARSER===============================

const struct AST BACKTRACK_ = { .type = BACKTRACK };

struct Token ungotten_token[10];     // There's a limit here, but it should be fine...
int ungotten = 0;

char* program_source;
int i = 0;
struct Token get_token(void) {
    if (ungotten) return ungotten_token[--ungotten];
    struct Token tok = lex(program_source + i);
    i += tok.len;
    if (tok.type == WHITESPACE) return get_token();
    print_token(tok);
    return tok;
}

void unget_token(struct Token tok) {
    ungotten_token[ungotten++] = tok;
}

struct ASTList new_ASTList(void) {
    return (struct ASTList){ .first = NULL, .last = NULL };
}

bool debug = true;

void add_child(struct ASTList* l, struct AST ast) {
    struct ASTList_* elem = malloc(sizeof(struct ASTList_));
    elem->here = ast;
    elem->next = NULL;
    if (l->first == NULL) {
        l->first = elem;
        l->last = elem;
    } else {
        l->last->next = elem;
        l->last = elem;
    }
}

struct AST parse_literal(enum TokenType type) {
    struct Token tok = get_token();
    if (tok.type == type)
        return (struct AST){ .type = LITERAL, .literal = tok };
    unget_token(tok);
    return BACKTRACK_;
}

struct AST parse_any_literal(void) {
    struct Token tok = get_token();
    if (tok.type == NAME || tok.type == INT || tok.type == STR || tok.type == REAL)
        return (struct AST){ .type = LITERAL, .literal = tok };
    unget_token(tok);
    return BACKTRACK_;
}

struct AST parse_block(void) {
    if (debug) puts("trying parse_block");
    if (parse_literal(LBRACE).type == BACKTRACK) return BACKTRACK_;
    struct ASTList l = new_ASTList();
    struct AST subtree;
    while ((subtree = parse_stmt()).type != BACKTRACK) add_child(&l, subtree);
    subtree = parse_expr();
    if (subtree.type == BACKTRACK) {    // TODO make better error messages
        puts("Invalid syntax");
        exit(1);
    }
    add_child(&l, subtree);
    if (parse_literal(RBRACE).type == BACKTRACK) {
        puts("Unclosed brace");
        exit(1);
    }
    return (struct AST){ .type = BLOCK, .children = l };
}

struct AST parse_expr_no_apply(void) {
    if (debug) puts("trying parse_expr_no_apply");
    if (parse_literal(LPAREN).type != BACKTRACK) {
        if (debug) puts("trying parens");
        struct AST subtree = parse_expr();
        if (parse_literal(RPAREN).type == BACKTRACK) {
            puts("Unclosed paren");
            exit(1);
        }
        return subtree;
    }
    if (parse_literal(FN).type != BACKTRACK) {
        if (debug) puts("trying function definition");
        struct ASTList l = new_ASTList();
        struct AST varname;
        while ((varname = parse_literal(NAME)).type != BACKTRACK) add_child(&l, varname);
        if (parse_literal(RARROW).type == BACKTRACK) {
            puts("Invalid function definition");
            exit(1);
        }
        if ((varname = parse_block()).type == BACKTRACK) {
            puts("Expected block for function definition body");
            exit(1);
        }
        add_child(&l, varname); // maybe change variable name from varname?
        return (struct AST){ .type = FN_DEF, .children = l };
    }
    struct AST subtree;
    if ((subtree = parse_block()).type != BACKTRACK) return subtree;
    return BACKTRACK_;
}

struct AST parse_expr(void) {
    if (debug) puts("trying parse_expr");
    struct AST subtree;
    // TODO rewrite with short circuiting
    if ((subtree = parse_expr_no_apply()).type != BACKTRACK) return subtree;
    if ((subtree = parse_apply()).type != BACKTRACK) return subtree;
    if ((subtree = parse_any_literal()).type != BACKTRACK) return subtree;
    return BACKTRACK_;
}

struct AST parse_apply(void) {
    if (debug) puts("trying parse_apply");
    struct ASTList l = new_ASTList();
    struct AST subtree;
    while (true) {
        // TODO use short circuiting
        if ((subtree = parse_any_literal()).type != BACKTRACK) add_child(&l, subtree);
        else if ((subtree = parse_expr_no_apply()).type != BACKTRACK) add_child(&l, subtree);
        else break;
    }
    return (struct AST){ .type = APPLY, .children = l };
}

struct AST parse_stmt(void) {
    if (debug) puts("trying parse_stmt");
    struct AST subtree;
    struct ASTList l = new_ASTList();
    if ((subtree = parse_literal(NAME)).type == BACKTRACK) return BACKTRACK_;
    add_child(&l, subtree);
    if ((subtree = parse_literal(EQUALS)).type == BACKTRACK) {
        printf("Ungetting: ");
        print_token(l.first->here.literal);
        unget_token(l.first->here.literal);   // FIXME memory leak: free the pointers
        return BACKTRACK_;
    }
    if ((subtree = parse_expr()).type == BACKTRACK) {
        puts("Expected expression");
        exit(1);
    }
    add_child(&l, subtree);
    if ((subtree = parse_literal(SEMICOLON)).type == BACKTRACK) {
        puts("Expected semicolon");
        exit(1);
    }
    return (struct AST){ .type = STMT, .children = l };
}

struct AST parse_program(void) {
    if (debug) puts("trying parse_program");
    struct ASTList program = new_ASTList();
    struct AST subtree;
    while ((subtree = parse_stmt()).type != BACKTRACK) add_child(&program, subtree);
    if (parse_literal(EOF_).type == BACKTRACK) {
        puts("Expected EOF.");
        exit(1);
    }
    return (struct AST){ .type = PROGRAM, .children = program };
}

int main() {
    FILE* fp = fopen("program.sfl", "r");
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    program_source = malloc(size);
    fread(program_source, 1, size, fp);
    struct AST ast = parse_program();

    puts("Parsed successfully!");
    print_ast(ast, 0);
    return 0;
}
