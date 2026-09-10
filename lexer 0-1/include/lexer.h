#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

/* Every kind of token our lexer is able to recognise. */
typedef enum {
    TOKEN_EOF,          /* end of input                              */
    TOKEN_IDENTIFIER,   /* names: foo, my_var, counter2               */
    TOKEN_KEYWORD,      /* reserved words: int, if, while, return ... */
    TOKEN_NUMBER,       /* integer or float literals: 42, 3.14        */
    TOKEN_STRING,       /* "double quoted string"                     */
    TOKEN_CHAR,         /* 'c' single character literal               */
    TOKEN_OPERATOR,     /* + - * / = == != <= >= && || etc.           */
    TOKEN_PUNCTUATION,  /* ( ) { } [ ] ; , .                          */
    TOKEN_UNKNOWN       /* anything the lexer does not recognise      */
} TokenType;

/* A single token produced by the lexer. */
typedef struct {
    TokenType type;   /* what kind of token this is                */
    char *value;      /* the raw text of the token (heap-allocated) */
    int line;         /* line number in the source where it starts  */
} Token;

/* The lexer keeps track of where it currently is in the source. */
typedef struct {
    const char *src;  /* pointer to the full source text (not owned) */
    size_t pos;        /* current index into src                     */
    size_t length;      /* total length of src                        */
    int line;          /* current line number (starts at 1)          */
} Lexer;

/* Prepare a lexer to scan the given null-terminated source string. */
void lexer_init(Lexer *lex, const char *source);

/* Pull the next token out of the source. Call repeatedly until the
 * returned token has type TOKEN_EOF. The caller owns the returned
 * token's `value` string and must free it with token_free(). */
Token lexer_next_token(Lexer *lex);

/* Release the memory owned by a token. */
void token_free(Token *tok);

/* Human readable name for a token type, used when printing tokens. */
const char *token_type_name(TokenType type);

#endif /* LEXER_H */
