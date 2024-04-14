#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

    PTRACCESS,

    // literals
    NUMBER, STRING, CHARACTER, COMMENT, SYMBOL,

    UNKNOWN,
} TokenType;

typedef struct {
    TokenType type;
    char *str;
    size_t line;
    size_t column;
} Token;

static void parse_tokens(void);
static void parse_error(char *line_str, size_t line_num, size_t col_num, char *msg);

static void new_token(TokenType type, char *str, size_t len, size_t line, size_t column);
static void print_tokens(void);
static void destroy_tokens(void);

static TokenType lookup_keyword(char *s);

static inline bool
is_digit(char c)
{
    return BETWEEN(c, '0', '9');
}

static inline bool
is_alpha(char c)
{
    return BETWEEN(c, 'a', 'z') || BETWEEN(c, 'A', 'Z');
}

static inline bool
is_alphanum(char c)
{
    return is_digit(c) || is_alpha(c);
}

static inline bool
is_in(char c, char *s)
{
    while (*s != '\0')
        if (c == *s++)
            return true;
    return false;
}

const char *filename;

const char *tokentypenames[] = {
    "WHITESPACE",

    "~", "!", "#", "%", "^", "&", "*",
    "(", ")", "-", "+",
    "=", "[", "]", "{", "}",
    "<", ">", ".", ",", ":", ";",
    "'", "\"", "|", "/", "\\", "?", 

    "break", "case", "char", "const",
    "continue", "default", "do", "double",
    "else", "enum", "extern", "float",
    "for", "goto", "if", "inline",
    "int", "long", "register", "return",
    "short", "signed", "static", "struct",
    "switch", "typedef", "union", "unsigned",
    "void", "volatile", "while",

    "!=", "==", "<=", ">=", "||", "&&",

    "~=", "%=", "^=",
    "&=", "*=", "-=",
    "+=", "|=", "/=",
    "--", "++",

    "->",

    "NUMBER", "STRING", "CHARACTER", "COMMENT", "SYMBOL",

    "UNKNOWN",
};

static char *filebuffer;

static Token *tokens;

int
main(int argc, char *argv[])
{
    if (argc != 2)
        die("usage: %s <cfile>", argv[0]);

    filename = argv[1];
    if (access(filename, F_OK))
        die("Could not find %s", filename);

    FILE *f;
    f = fopen(filename, "r");
    if (!f)
        die("Could not open %s", filename);

    size_t fsize;
    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    filebuffer = emalloc(fsize + 1);

    fread(filebuffer, fsize, 1, f);
    fclose(f);

    filebuffer[fsize] = '\0';

    da_create(tokens, sizeof(Token), 256);
    if (!tokens)
        die("E: da_create failed");

    parse_tokens();
    print_tokens();

    destroy_tokens();
    return 0;
}

void
parse_tokens(void)
{
    size_t line;
    char *str, *cur, *line_start;

    da_create(str, sizeof(*str), 64);
    line = 1;
    cur = filebuffer;

    while (*cur != '\0') {
        TokenType type;
        bool newline;

        newline = false;

        switch (*cur) {
        case '~':
            if ('=' == *(cur+1)) { type = TILDEASSIGN; cur++; }
            else type = TILDE;
            break;
        case '!': 
            if ('=' == *(cur+1)) { type = NEQ; cur++; }
            else type = BANG;
            break;
        case '#': type = HASH; break;
        case '%':
            if ('=' == *(cur+1)) { type = MODASSIGN; cur++; }
            else type = MOD;
            break;
        case '^':
            if ('=' == *(cur+1)) { type = XORASSIGN; cur++; }
            else type = XOR;
            break;
        case '&':
            if ('&' == *(cur+1)) { type = AND; cur++; }
            else if ('=' == *(cur+1)) { type = AMPASSIGN; cur++; }
            else type = AMP;
            break;
        case '*':
            if ('=' == *(cur+1)) { type = STARASSIGN; cur++; }
            else type = STAR;
            break;
        case '(': type = LPAREN; break;
        case ')': type = RPAREN; break;
        case '-':
            if ('=' == *(cur+1)) { type = MINUSASSIGN; cur++; }
            else if ('-' == *(cur+1)) { type = MINUSMINUS; cur++; }
            else if ('>' == *(cur+1)) { type = PTRACCESS; cur++; }
            else type = MINUS;
            break;
        case '+':
            if ('=' == *(cur+1)) { type = PLUSASSIGN; cur++; }
            else if ('+' == *(cur+1)) { type = PLUSPLUS; cur++; }
            else type = PLUS;
            break;
        case '=':
            if ('=' == *(cur+1)) { type = EQEQ; cur++; }
            else type = EQ;
            break;
        case '[': type = LBRACK; break;
        case ']': type = RBRACK; break;
        case '{': type = LBRACE; break;
        case '}': type = RBRACE; break;
        case '<':
            if ('=' == *(cur+1)) { type = LTEQ; cur++; }
            else type = LANGLE;
            break;
        case '>':
            if ('=' == *(cur+1)) { type = GTEQ; cur++; }
            else type = RANGLE;
            break;
        case '.': type = DOT; break;
        case ',': type = COMMA; break;
        case ':': type = COLON; break;
        case ';': type = SEMICOLON; break;
        case '\'': 
            da_append(str, cur++); // opening '
            if ('\'' == *cur)
                parse_error(line_start, line, (size_t)(cur - line_start), "empty char literal");
            if ('\\' == *cur)
                da_append(str, cur++); // escape backslash
            da_append(str, cur++); // TODO: validate escaped characters
            if ('\'' != *cur)
                parse_error(line_start, line, (size_t)(cur - line_start), "invalid char literal");
            da_append(str, cur); // closing '
            type = CHARACTER;
            break;
        case '"':
            do {
                if (*cur == '\\')
                    da_append(str, cur++); // TODO: validate escaped characters
                da_append(str, cur++);
                if (*cur == '\n')
                    parse_error(line_start, line, (size_t)(cur - line_start), "string literal error");
            } while (*cur != '"');
            da_append(str, cur);
            type = STRING;
            break;
        case '|':
            if ('=' == *(cur+1)) { type = ORASSIGN; cur++; }
            else if ('|' == *(cur+1)) { type = OR; cur++; }
            else type = VBAR;
            break;
        case '/':
            if ('=' == *(cur+1)) { type = DIVASSIGN; cur++; }
            else if ('/' == *(cur+1)) {
                do da_append(str, cur++);
                while (*cur != '\n');
                newline = true;
                type = COMMENT;
            }
            else if ('*' == *(cur+1)) {
                do da_append(str, cur++);
                while (!(*cur == '*' && *(cur+1) == '/'));
                da_append(str, cur++);
                da_append(str, cur);
                type = COMMENT;
            }
            else type = FSLASH;
            break;
        case '\\': type = BSLASH; break;
        case '?': type = QMARK; break;
        // whitespace
        case '\n': newline = true;
        case '\r':
        case ' ':
        case '\t':
            type = WHITESPACE;
            break;

        default:
            if (!da_len(str) && is_digit(*cur)) {
                bool fp;
                fp = false;
                if (*cur == '0') {
                    da_append(str, cur++);
                    // hex
                    if (*(cur+1) == 'x' || *(cur+1) == 'X')
                        do da_append(str, cur++);
                        while (is_digit(*cur) || BETWEEN(*cur, 'a', 'f') || BETWEEN(*cur, 'A', 'F'));
                    // bin
                    else if (*(cur+1) == 'b' || *(cur+1) == 'B')
                        do da_append(str, cur++);
                        while (*cur == '0' || *cur == '1');
                    // octal
                    else
                        do da_append(str, cur++);
                        while (is_digit(*cur));
                }
                // decimal and floating point
                else {
                    do da_append(str, cur++);
                    while (is_digit(*cur));
                    if (*cur == '.') {
                        da_append(str, cur++);
                        fp = true;
                        while (is_digit(*cur))
                            da_append(str, cur++);
                    }
                }
                // suffix
                if (fp && ((*(cur+1) == 'f') || (*(cur+1) == 'F')))
                    da_append(str, cur++);
                else if (*(cur+1) == 'l' || *(cur+1) == 'l' || *(cur+1) == 'l' || 0)
                    ; // TODO: int literal suffix
                cur--;
                type = NUMBER;
            }
            else if (!da_len(str) && (is_alpha(*cur) || '_' == *cur)) {
                do da_append(str, cur++);
                while (is_alphanum(*cur) || '_' == *cur);
                cur--;
                type = SYMBOL;
            }
            else {
            da_append(str, cur++);
                type = UNKNOWN;
            }
            break;
        }

        if (SYMBOL == type) {
            char c;
            c = '\0';
            da_append(str, &c);
            type = lookup_keyword(str);
            if (SYMBOL != type)
                da_clear(str);
        }

        if (WHITESPACE != type) {
            new_token(type, str, da_len(str), line, (size_t)(cur - line_start));
        }

        da_clear(str);
        
        cur++;
        if (newline)
            line_start = cur;
        line += newline;
    }

    da_destroy(str);
}

void
new_token(TokenType type, char *str, size_t len, size_t line, size_t column)
{
    Token t;
    t.type = type;
    t.line = line;
    t.column = column;
    if (str && len) {
        char *p = emalloc(len + 1);
        strncpy(p, str, len);
        p[len] = '\0';
        t.str = p;
    }
    else t.str = NULL;
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
        if (t->line > prev_line) {
            prev_line = t->line;
            printf("\n%lu: ", prev_line);
        }
        if (t->type == SYMBOL || t->type == STRING || t->type == CHARACTER || t->type == NUMBER || t->type == COMMENT)
            printf("%s ", t->str);
        else if (t->type == UNKNOWN)
            printf("UNKNOWN(%s) ", t->str);
        else
            printf("%s ", tokentypenames[t->type]);
    }
    fputc('\n', stdout);
}

void
destroy_tokens(void)
{
    for (size_t i = 0; i < da_len(tokens); i++)
        if (tokens[i].str)
            free(tokens[i].str);
    da_destroy(tokens);
}

void
parse_error(char *line_str, size_t line_num, size_t col_num, char *msg)
{
    fprintf(stderr, "E: %s at %s:%lu:%lu\n", msg, filename, line_num, col_num);
    while (*line_str != '\n')
        fputc(*line_str++, stderr);
    fputc('\n', stderr);
    while (--col_num)
        fputc(' ', stderr);
    fputc('^', stderr);
    die("");
}

TokenType
lookup_keyword(char *s)
{
    TokenType type;
    if (!strcmp(s, tokentypenames[BREAK])) type = BREAK;
    else if (!strcmp(s, tokentypenames[CASE])) type = CASE;
    else if (!strcmp(s, tokentypenames[CHAR])) type = CHAR;
    else if (!strcmp(s, tokentypenames[CONST])) type = CONST;
    else if (!strcmp(s, tokentypenames[CONTINUE])) type = CONTINUE;
    else if (!strcmp(s, tokentypenames[DEFAULT])) type = DEFAULT;
    else if (!strcmp(s, tokentypenames[DO])) type = DO;
    else if (!strcmp(s, tokentypenames[DOUBLE])) type = DOUBLE;
    else if (!strcmp(s, tokentypenames[ELSE])) type = ELSE;
    else if (!strcmp(s, tokentypenames[ENUM])) type = ENUM;
    else if (!strcmp(s, tokentypenames[EXTERN])) type = EXTERN;
    else if (!strcmp(s, tokentypenames[FLOAT])) type = FLOAT;
    else if (!strcmp(s, tokentypenames[FOR])) type = FOR;
    else if (!strcmp(s, tokentypenames[GOTO])) type = GOTO;
    else if (!strcmp(s, tokentypenames[IF])) type = IF;
    else if (!strcmp(s, tokentypenames[INLINE])) type = INLINE;
    else if (!strcmp(s, tokentypenames[INT])) type = INT;
    else if (!strcmp(s, tokentypenames[LONG])) type = LONG;
    else if (!strcmp(s, tokentypenames[REGISTER])) type = REGISTER;
    else if (!strcmp(s, tokentypenames[RETURN])) type = RETURN;
    else if (!strcmp(s, tokentypenames[SHORT])) type = SHORT;
    else if (!strcmp(s, tokentypenames[SIGNED])) type = SIGNED;
    else if (!strcmp(s, tokentypenames[STATIC])) type = STATIC;
    else if (!strcmp(s, tokentypenames[STRUCT])) type = STRUCT;
    else if (!strcmp(s, tokentypenames[SWITCH])) type = SWITCH;
    else if (!strcmp(s, tokentypenames[TYPEDEF])) type = TYPEDEF;
    else if (!strcmp(s, tokentypenames[UNION])) type = UNION;
    else if (!strcmp(s, tokentypenames[UNSIGNED])) type = UNSIGNED;
    else if (!strcmp(s, tokentypenames[VOID])) type = VOID;
    else if (!strcmp(s, tokentypenames[VOLATILE])) type = VOLATILE;
    else if (!strcmp(s, tokentypenames[WHILE])) type = WHILE;
    else type = SYMBOL;
    return type;
}

