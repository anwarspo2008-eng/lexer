#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

/* List of words that are treated as keywords instead of identifiers.
 * Feel free to add more C keywords here. */
static const char *KEYWORDS[] = {
    "int", "float", "double", "char", "void", "long", "short",
    "unsigned", "signed", "struct", "union", "enum", "typedef",
    "if", "else", "while", "for", "do", "switch", "case", "default",
    "break", "continue", "return", "goto", "sizeof", "const",
    "static", "extern", "volatile", NULL
};

static int is_keyword(const char *word) {
    for (int i = 0; KEYWORDS[i] != NULL; i++) {
        if (strcmp(KEYWORDS[i], word) == 0) return 1;
    }
    return 0;
}

/* Look at the current character without consuming it. */
static char peek(Lexer *lex) {
    if (lex->pos >= lex->length) return '\0';
    return lex->src[lex->pos];
}

/* Look one character ahead without consuming anything. */
static char peek_next(Lexer *lex) {
    if (lex->pos + 1 >= lex->length) return '\0';
    return lex->src[lex->pos + 1];
}

/* Consume and return the current character, advancing the position
 * (and the line counter, if we crossed a newline). */
static char advance(Lexer *lex) {
    char c = lex->src[lex->pos++];
    if (c == '\n') lex->line++;
    return c;
}

/* Build a Token by copying `len` characters starting at `start`. */
static Token make_token(TokenType type, const char *start, size_t len, int line) {
    Token tok;
    tok.type = type;
    tok.line = line;
    tok.value = (char *)malloc(len + 1);
    memcpy(tok.value, start, len);
    tok.value[len] = '\0';
    return tok;
}

/* Skip spaces, tabs, newlines, and both kinds of comments. This is
 * called before reading every token so comments/whitespace never
 * turn into tokens themselves. */
static void skip_whitespace_and_comments(Lexer *lex) {
    for (;;) {
        char c = peek(lex);

        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance(lex);
        } else if (c == '/' && peek_next(lex) == '/') {
            /* line comment: skip to end of line */
            while (peek(lex) != '\n' && peek(lex) != '\0') advance(lex);
        } else if (c == '/' && peek_next(lex) == '*') {
            /* block comment: skip until closing */
            advance(lex); /* '/' */
            advance(lex); /* '*' */
            while (!(peek(lex) == '*' && peek_next(lex) == '/') && peek(lex) != '\0') {
                advance(lex);
            }
            if (peek(lex) != '\0') {
                advance(lex); /* '*' */
                advance(lex); /* '/' */
            }
        } else {
            break;
        }
    }
}

static Token read_identifier_or_keyword(Lexer *lex) {
    size_t start = lex->pos;
    int line = lex->line;
    while (isalnum((unsigned char)peek(lex)) || peek(lex) == '_') {
        advance(lex);
    }
    size_t len = lex->pos - start;
    Token tok = make_token(TOKEN_IDENTIFIER, lex->src + start, len, line);
    if (is_keyword(tok.value)) tok.type = TOKEN_KEYWORD;
    return tok;
}

static Token read_number(Lexer *lex) {
    size_t start = lex->pos;
    int line = lex->line;
    while (isdigit((unsigned char)peek(lex))) advance(lex);

    /* optional fractional part: 3.14 */
    if (peek(lex) == '.' && isdigit((unsigned char)peek_next(lex))) {
        advance(lex); /* consume '.' */
        while (isdigit((unsigned char)peek(lex))) advance(lex);
    }

    /* optional exponent part: 1e10, 2.5E-3 */
    if (peek(lex) == 'e' || peek(lex) == 'E') {
        size_t save = lex->pos;
        advance(lex);
        if (peek(lex) == '+' || peek(lex) == '-') advance(lex);
        if (isdigit((unsigned char)peek(lex))) {
            while (isdigit((unsigned char)peek(lex))) advance(lex);
        } else {
            lex->pos = save; /* not really an exponent, back off */
        }
    }

    size_t len = lex->pos - start;
    return make_token(TOKEN_NUMBER, lex->src + start, len, line);
}

static Token read_string(Lexer *lex) {
    int line = lex->line;
    size_t start = lex->pos;
    advance(lex); /* consume opening quote */
    while (peek(lex) != '"' && peek(lex) != '\0') {
        if (peek(lex) == '\\' && peek_next(lex) != '\0') {
            advance(lex); /* skip escape backslash */
        }
        advance(lex);
    }
    if (peek(lex) == '"') advance(lex); /* consume closing quote */
    size_t len = lex->pos - start;
    return make_token(TOKEN_STRING, lex->src + start, len, line);
}

static Token read_char(Lexer *lex) {
    int line = lex->line;
    size_t start = lex->pos;
    advance(lex); /* consume opening quote */
    while (peek(lex) != '\'' && peek(lex) != '\0') {
        if (peek(lex) == '\\' && peek_next(lex) != '\0') {
            advance(lex);
        }
        advance(lex);
    }
    if (peek(lex) == '\'') advance(lex); /* consume closing quote */
    size_t len = lex->pos - start;
    return make_token(TOKEN_CHAR, lex->src + start, len, line);
}

/* Try to match a multi-character operator starting at the current
 * position. Returns the matched operator length (0 if none). Longest
 * operators are checked first so e.g. "==" isn't split into two "=". */
static size_t match_operator(Lexer *lex, const char **out) {
    static const char *ops[] = {
        "<<=", ">>=",
        "==", "!=", "<=", ">=", "&&", "||", "++", "--",
        "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
        "->", "<<", ">>",
        "+", "-", "*", "/", "%", "=", "<", ">", "!", "&", "|", "^", "~",
        NULL
    };
    for (int i = 0; ops[i] != NULL; i++) {
        size_t len = strlen(ops[i]);
        if (lex->pos + len <= lex->length &&
            strncmp(lex->src + lex->pos, ops[i], len) == 0) {
            *out = ops[i];
            return len;
        }
    }
    return 0;
}

void lexer_init(Lexer *lex, const char *source) {
    lex->src = source;
    lex->pos = 0;
    lex->length = strlen(source);
    lex->line = 1;
}

Token lexer_next_token(Lexer *lex) {
    skip_whitespace_and_comments(lex);

    int line = lex->line;
    char c = peek(lex);

    if (c == '\0') {
        return make_token(TOKEN_EOF, "", 0, line);
    }

    if (isalpha((unsigned char)c) || c == '_') {
        return read_identifier_or_keyword(lex);
    }

    if (isdigit((unsigned char)c)) {
        return read_number(lex);
    }

    if (c == '"') {
        return read_string(lex);
    }

    if (c == '\'') {
        return read_char(lex);
    }

    const char *op = NULL;
    size_t op_len = match_operator(lex, &op);
    if (op_len > 0) {
        size_t start = lex->pos;
        for (size_t i = 0; i < op_len; i++) advance(lex);
        return make_token(TOKEN_OPERATOR, lex->src + start, op_len, line);
    }

    if (strchr("(){}[];,.", c) != NULL) {
        size_t start = lex->pos;
        advance(lex);
        return make_token(TOKEN_PUNCTUATION, lex->src + start, 1, line);
    }

    /* Unrecognised single character; consume it so we don't loop forever. */
    size_t start = lex->pos;
    advance(lex);
    return make_token(TOKEN_UNKNOWN, lex->src + start, 1, line);
}

void token_free(Token *tok) {
    free(tok->value);
    tok->value = NULL;
}

const char *token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_EOF:         return "EOF";
        case TOKEN_IDENTIFIER:  return "IDENTIFIER";
        case TOKEN_KEYWORD:     return "KEYWORD";
        case TOKEN_NUMBER:      return "NUMBER";
        case TOKEN_STRING:      return "STRING";
        case TOKEN_CHAR:        return "CHAR";
        case TOKEN_OPERATOR:    return "OPERATOR";
        case TOKEN_PUNCTUATION: return "PUNCTUATION";
        case TOKEN_UNKNOWN:     return "UNKNOWN";
    }
    return "UNKNOWN";
}
