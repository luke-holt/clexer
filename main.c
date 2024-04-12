#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "dynamic_array.h"
#include "util.h"

#define MAX_TOKEN_LEN (32)

typedef enum {
    WHITESPACE,

    // single chars
    TILDE, BANG, HASH, MOD, XOR, AMP, STAR,
    LPAREN, RPAREN, MINUS, PLUS,
    EQ, LBRACK, RBRACK, LBRACE, RBRACE,
    LANGLE, RANGLE, DOT, COMMA, COLON, SEMICOLON,
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
    NUMBER, STRING, IDENTIFIER, SYMBOL,
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

const char *filename = "main.c";

const char *tokentypenames[] = {
    "WHITESPACE",

    "TILDE", "BANG", "HASH", "MOD", "XOR", "AMP", "STAR",
    "LPAREN", "RPAREN", "MINUS", "PLUS",
    "EQ", "LBRACK", "RBRACK", "LBRACE", "RBRACE",
    "LANGLE", "RANGLE", "DOT", "COMMA", "COLON", "SEMICOLON",
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

    "NUMBER", "STRING", "IDENTIFIER", "SYMBOL",
};

static char *filebuffer;

static Token *tokens;

int
main(int argc, char *argv[])
{
    FILE *f;
    f = fopen(filename, "r");
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
    tokens = da_create(sizeof(Token), 256);

    parse_tokens();

    print_tokens();

    destroy_tokens();

    return 0;
}

void
parse_tokens(void)
{
    size_t line, len;
    char *str, *cur;

    line = 1; // file line number count
    str = da_create(sizeof(*str), 64);
    cur = filebuffer;
    while (*cur != '\0') {
        TokenType type;
        char next;
        next = *(cur+1);
        switch (*cur) {
        case '~':
            if ('=' == next) { type = TILDEASSIGN; cur++; }
            else type = TILDE;
            break;
        case '!': 
            if ('=' == next) { type = NEQ; cur++; }
            else type = BANG;
            break;
        case '#': type = HASH; break;
        case '%':
            if ('=' == next) { type = MODASSIGN; cur++; }
            else type = MOD;
            break;
        case '^':
            if ('=' == next) { type = XORASSIGN; cur++; }
            else type = XOR;
            break;
        case '&':
            if ('&' == next) { type = AND; cur++; }
            else if ('=' == next) { type = AMPASSIGN; cur++; }
            else type = AMP;
            break;
        case '*':
            if ('=' == next) { type = STARASSIGN; cur++; }
            else type = STAR;
            break;
        case '(': type = LPAREN; break;
        case ')': type = RPAREN; break;
        case '-':
            if ('=' == next) { type = MINUSASSIGN; cur++; }
            else if ('-' == next) { type = MINUSMINUS; cur++; }
            else type = MINUS;
            break;
        case '+':
            if ('=' == next) { type = PLUSASSIGN; cur++; }
            else if ('+' == next) { type = PLUSPLUS; cur++; }
            else type = PLUS;
            break;
        case '=':
            if ('=' == next) { type = EQEQ; cur++; }
            else type = EQ;
            break;
        case '[': type = LBRACK; break;
        case ']': type = RBRACK; break;
        case '{': type = LBRACE; break;
        case '}': type = RBRACE; break;
        case '<':
            if ('=' == next) { type = LTEQ; cur++; }
            else type = LANGLE;
            break;
        case '>':
            if ('=' == next) { type = GTEQ; cur++; }
            else type = RANGLE;
            break;
        case '.': type = DOT; break;
        case ',': type = COMMA; break;
        case ':': type = COLON; break;
        case ';': type = SEMICOLON; break;
        case '\'': type = SQUOTE; break;
        case '"': 
            do {
                da_append(str, cur++);
                // TODO: handle escaped double quotes
                // if (*cur == '\n')
                    // die("E: String error at %s:%lu", filename, line);
            } while (*cur != '"');
            da_append(str, cur);
            type = STRING;
            break;
        case '|':
            if ('=' == next) { type = ORASSIGN; cur++; }
            else type = VBAR;
            break;
        case '/':
            if ('=' == next) { type = DIVASSIGN; cur++; }
            else if ('/' == next) {
                // comment detected. collect chars until newline
                while (*cur != '\n')
                    da_append(str, cur++);
                line++;
                type = WHITESPACE;
            }
            else if ('*' == next) {
                do {
                    da_append(str, cur++);
                    next = *(cur+1);
                } while (!(*cur == '*' && next == '/'));
                da_append(str, cur++);
                da_append(str, cur);
                type = /* test */WHITESPACE;
            }
            else type = FSLASH;
            break;
        case '\\': type = BSLASH; break;
        case '?': type = QMARK; break;
        // whitespace
        case '\n': line++;
        case '\r':
        case ' ':
        case '\t':
            type = WHITESPACE;
            break;
        case '_':
        default:
            type = SYMBOL;
            break;
        }

        if (STRING == type) {
            size_t len;
            len = da_len(str);
            if (len) {
                new_token(STRING, str, len, line);
                da_clear(str);
            }
        }
        else if (SYMBOL != type) {
            size_t len;
            len = da_len(str);
            if (len) {
                new_token(SYMBOL, str, len, line);
                da_clear(str);
            }
            if (WHITESPACE != type)
                new_token(type, NULL, 0, line);
        }
        else da_append(str, cur);
        cur++;
    }

    da_destroy(str);
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
        strncpy(p, str, len);
        p[len] = '\0';
        t.s = p;
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
    printf("1: ");
    while (i < da_len(tokens)) {
        Token *t = &tokens[i++];
        if (t->l > prev_line) {
            prev_line = t->l;
            printf("\n%lu: ", prev_line);
        }

        if ((t->t == SYMBOL) || (t->t == STRING))
            printf("%s ", t->s);
        else
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

