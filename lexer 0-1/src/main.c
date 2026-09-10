#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

/* Read an entire file into a single null-terminated heap buffer.
 * Returns NULL on failure. Caller must free() the result. */
static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror("fopen");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = (char *)malloc((size_t)size + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(buffer, 1, (size_t)size, f);
    buffer[read] = '\0';

    fclose(f);
    return buffer;
}

/* Read everything from stdin into a single null-terminated heap
 * buffer, growing the buffer as needed. */
static char *read_stdin(void) {
    size_t cap = 4096, len = 0;
    char *buffer = (char *)malloc(cap);
    if (!buffer) return NULL;

    int c;
    while ((c = fgetc(stdin)) != EOF) {
        if (len + 1 >= cap) {
            cap *= 2;
            buffer = (char *)realloc(buffer, cap);
            if (!buffer) return NULL;
        }
        buffer[len++] = (char)c;
    }
    buffer[len] = '\0';
    return buffer;
}

int main(int argc, char *argv[]) {
    char *source = NULL;

    if (argc >= 2) {
        source = read_file(argv[1]);
        if (!source) {
            fprintf(stderr, "Could not read file: %s\n", argv[1]);
            return 1;
        }
    } else {
        printf("No file given, reading source from stdin.\n");
        printf("(Type or paste code, then press Ctrl+D to end input.)\n\n");
        source = read_stdin();
        if (!source) {
            fprintf(stderr, "Could not read stdin.\n");
            return 1;
        }
    }

    Lexer lex;
    lexer_init(&lex, source);

    printf("%-6s %-12s %s\n", "LINE", "TYPE", "VALUE");
    printf("-----------------------------------\n");

    for (;;) {
        Token tok = lexer_next_token(&lex);

        if (tok.type == TOKEN_EOF) {
            token_free(&tok);
            break;
        }

        printf("%-6d %-12s %s\n", tok.line, token_type_name(tok.type), tok.value);
        token_free(&tok);
    }

    free(source);
    return 0;
}
