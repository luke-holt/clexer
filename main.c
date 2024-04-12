#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "dynamic_array.h"
#include "util.h"

#define MAX_TOKEN_LEN (32)

typedef enum {
    // single chars
    TILDE, BANG, HASH, MOD, XOR, AMP, STAR,
    LPAREN, RPAREN, MINUS, PLUS, UNDERSCORE,
    EQ, LBRACK, RBRACK, LBRACE, RBRACE,
    LARROW, RARROW, DOT, COMMA, COLON, SEMICOLON,
    SQUOTE, DQUOTE, VBAR, FSLASH, BSLASH, QMARK,

    // keywords
    BREAK,    CASE,     CHAR,     CONST,
    CONTINUE, DEFAULT,  DO,       DOUBLE,
    ELSE,     ENUM,     EXTERN,   FLOAT,
    FOR,      GOTO,     IF,       INLINE,
    INT,      LONG,     REGISTER, RETURN,
    SHORT,    SIGNED,   STATIC,   STRUCT,
    SWITCH,   TYPEDEF,  UNION,    UNSIGNED,
    VOID,     VOLATILE, WHILE,

    // logical operators
    NEQ, EQEQ, LTEQ, GTEQ, OR, AND,

    // special assignment
    TILDEASSIGN, MODASSIGN, XORASSIGN,
    AMPASSIGN, STARASSIGN, MINUSASSIGN,
    PLUSASSIGN, ORASSIGN, DIVASSIGN,
    MINUSMINUS, PLUSPLUS,

    // symbols
    NUMBER, STRING, IDENTIFIER,
} TokenType;

typedef struct {
    TokenType t;
    char *s;
    size_t l;
} Token;

static void parse_tokens(void);
static void new_token(TokenType type, char *str, size_t len, size_t line);
static void print_tokens(void);
static void destroy_tokens(void);

const char *tokentypenames[] = {
    "TILDE", "BANG", "HASH", "MOD", "XOR", "AMP", "STAR",
    "LPAREN", "RPAREN", "MINUS", "PLUS", "UNDERSCORE",
    "EQ", "LBRACK", "RBRACK", "LBRACE", "RBRACE",
    "LARROW", "RARROW", "DOT", "COMMA", "COLON", "SEMICOLON",
    "SQUOTE", "DQUOTE", "VBAR", "FSLASH", "BSLASH", "QMARK",

    "BREAK", "CASE", "CHAR", "CONST",
    "CONTINUE", "DEFAULT", "DO", "DOUBLE",
    "ELSE", "ENUM", "EXTERN", "FLOAT",
    "FOR", "GOTO", "IF", "INLINE",
    "INT", "LONG", "REGISTER", "RETURN",
    "SHORT", "SIGNED", "STATIC", "STRUCT",
    "SWITCH", "TYPEDEF", "UNION", "UNSIGNED",
    "VOID", "VOLATILE", "WHILE",

    "NEQ", "EQEQ", "LTEQ", "GTEQ", "OR", "AND",

    "TILDEASSIGN", "MODASSIGN", "XORASSIGN",
    "AMPASSIGN", "STARASSIGN", "MINUSASSIGN",
    "PLUSASSIGN", "ORASSIGN", "DIVASSIGN",
    "MINUSMINUS", "PLUSPLUS",

    "NUMBER", "STRING", "IDENTIFIER",
};

static char *filebuffer;

static Token *tokens;

int
main(int argc, char *argv[])
{
    FILE *f;
    f = fopen("main.c", "r");
    if (!f)
        die("Could not open main.c");

    size_t fsize;
    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    filebuffer = malloc(fsize + 1);
    if (!filebuffer)
        die("malloc failed");

    fread(filebuffer, fsize, 1, f);
    fclose(f);

    filebuffer[fsize] = '\0';

    // create dynamic array
    tokens = da_create(sizeof(*tokens), 256);

    parse_tokens();

    print_tokens();

    destroy_tokens();

    return 0;
}

void
parse_tokens(void)
{
    size_t line;
    line = 1; // start count at 1
    for (size_t i = 0; i < strlen(filebuffer); i++) {
        char c = filebuffer[i];
        char next = filebuffer[i+1];
        TokenType type;
        switch (c) {
        case '~':
            if ('=' == next) { type = TILDEASSIGN; i++; }
            else type = TILDE;
            new_token(type, NULL, 0, line);
            break;
        case '!': 
            if ('=' == next) { type = NEQ; i++; }
            else type = BANG;
            new_token(type, NULL, 0, line);
            break;
        case '#': new_token(HASH, NULL, 0, line); break;
        case '%':
            if ('=' == next) { type = MODASSIGN; i++; }
            else type = MOD;
            new_token(type, NULL, 0, line);
            break;
        case '^':
            if ('=' == next) { type = XORASSIGN; i++; }
            else type = XOR;
            new_token(type, NULL, 0, line);
            break;
        case '&':
            if ('&' == next) { type = AND; i++; }
            else if ('=' == next) { type = AMPASSIGN; i++; }
            else type = AMP;
            new_token(type, NULL, 0, line);
            break;
        case '*':
            if ('=' == next) { type = STARASSIGN; i++; }
            else type = STAR;
            new_token(type, NULL, 0, line);
            break;
        case '(': new_token(LPAREN, NULL, 0, line); break;
        case ')': new_token(RPAREN, NULL, 0, line); break;
        case '-':
            if ('=' == next) { type = MINUSASSIGN; i++; }
            else if ('-' == next) { type = MINUSMINUS; i++; }
            else type = MINUS;
            new_token(type, NULL, 0, line);
            break;
        case '+':
            if ('=' == next) { type = PLUSASSIGN; i++; }
            else if ('+' == next) { type = PLUSPLUS; i++; }
            else type = PLUS;
            new_token(type, NULL, 0, line);
            break;
        case '_': new_token(UNDERSCORE, NULL, 0, line); break;
        case '=':
            if ('=' == next) { type = EQEQ; i++; }
            else type = EQ;
            new_token(type, NULL, 0, line);
            break;
        case '[': new_token(LBRACK, NULL, 0, line); break;
        case ']': new_token(RBRACK, NULL, 0, line); break;
        case '{': new_token(LBRACE, NULL, 0, line); break;
        case '}': new_token(RBRACE, NULL, 0, line); break;
        case '<':
            if ('=' == next) { type = LTEQ; i++; }
            else type = LARROW;
            new_token(type, NULL, 0, line);
            break;
        case '>':
            if ('=' == next) { type = GTEQ; i++; }
            else type = RARROW;
            new_token(type, NULL, 0, line);
            break;
        case '.': new_token(DOT, NULL, 0, line); break;
        case ',': new_token(COMMA, NULL, 0, line); break;
        case ':': new_token(COLON, NULL, 0, line); break;
        case ';': new_token(SEMICOLON, NULL, 0, line); break;
        case '\'': new_token(SQUOTE, NULL, 0, line); break;
        case '"': new_token(DQUOTE, NULL, 0, line); break;
        case '|':
            if ('=' == next) { type = ORASSIGN; i++; }
            else type = VBAR;
            new_token(type, NULL, 0, line);
            break;
        case '/':
            if ('=' == next) { type = DIVASSIGN; i++; }
            else type = FSLASH;
            new_token(type, NULL, 0, line);
            break;
        case '\\': new_token(BSLASH, NULL, 0, line); break;
        case '?': new_token(QMARK, NULL, 0, line); break;
        case '\n': line++; break;
        case ' ':
        case '\t': break; // whitespace
        default:
            // fputc(*c, stdout);
            break;
        }
    }
}

void
new_token(TokenType type, char *str, size_t len, size_t line)
{
    Token t;
    t.t = type;
    t.l = line;
    if (str && len) {
        char *p = malloc(len + 1);
        if (!p)
            die("malloc failed");
        strncpy(t.s, str, len);
        t.s[len] = '\0';
    }
    else t.s = NULL;
    da_append(tokens, &t);
}

void
print_tokens(void)
{
    size_t i, prev_line;
    i = 0;
    prev_line = 1;
    while (i < da_len(tokens)) {
        Token *t = &tokens[i++];
        if (t->l > prev_line) {
            prev_line = t->l;
            printf("\n%lu: ", prev_line);
        }
        printf("%s ", tokentypenames[t->t]);
    }
    fputc('\n', stdout);
}

void
destroy_tokens(void)
{
    for (size_t i = 0; i < da_len(tokens); i++)
        if (tokens[i].s)
            free(tokens[i].s);
    da_destroy(tokens);
}

