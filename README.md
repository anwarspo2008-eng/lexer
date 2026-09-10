# lexer 0-1

A small, self-contained **lexer** (also called a *tokenizer* or *scanner*)
written in plain C. You give it a piece of source code — either from a
file or typed into the terminal — and it breaks that code up into a
stream of **tokens**: identifiers, keywords, numbers, strings, operators,
and punctuation.

This is the first stage of how any compiler or interpreter works:

```
source code  --[ LEXER ]-->  tokens  --[ parser ]--> ... --> program
```

This project only implements the lexer stage — it does not parse or
run the code, it just tells you what "words" the code is made of.

---

## 1. What's in this folder

```
lexer 0-1/
├── include/
│   └── lexer.h      # Token/Lexer type declarations + public functions
├── src/
│   ├── lexer.c      # The actual lexer logic — all the scanning happens here
│   └── main.c       # Command-line program that uses the lexer
├── bin/             # Build output goes here (created by `make`)
├── sample.c         # A tiny example C file you can run the lexer on
├── Makefile         # Convenience commands to build/run/clean the project
└── README.md        # This file
```

| Path              | Purpose                                                              |
|-------------------|-----------------------------------------------------------------------|
| `include/lexer.h` | Declares the `Token`/`Lexer` types and the public lexer functions.    |
| `src/lexer.c`     | The actual lexer logic — this is where all the scanning happens.      |
| `src/main.c`      | A small command-line program that uses the lexer and prints tokens.   |
| `sample.c`        | A tiny example C file you can run the lexer on.                       |
| `bin/lexer`       | The compiled program (appears after you build; not committed).        |
| `Makefile`        | Convenience commands to build/run/clean the project.                  |
| `README.md`       | This file.                                                             |

---

## 2. How to use — commands

You need `gcc` (or any C compiler) installed. All commands below are
run from inside the `lexer 0-1/` folder.

### Build it

```bash
make
```

Under the hood this runs:

```bash
gcc -Wall -Wextra -std=c99 -Iinclude -o bin/lexer src/main.c src/lexer.c
```

- `-Iinclude` tells the compiler where to find `lexer.h`.
- The compiled program is written to `bin/lexer` (the `bin/` folder is
  created automatically if it doesn't exist yet).

If you'd rather skip `make` entirely, you can run that exact `gcc`
command yourself.

### Run it

**Tokenize a file:**

```bash
./bin/lexer sample.c
```

**Tokenize your own file** (any path):

```bash
./bin/lexer path/to/your_file.c
```

**Tokenize whatever you type/paste in the terminal** (no filename given):

```bash
./bin/lexer
```

Type or paste your code, then press `Ctrl+D` (Linux/macOS) or `Ctrl+Z`
then `Enter` (Windows) to signal end-of-input.

**Build and run in one step**, using `sample.c` as input:

```bash
make run
```

### Clean up

Remove the compiled binary (`bin/lexer`):

```bash
make clean
```

### Command cheat sheet

| What you want to do                        | Command                          |
|---------------------------------------------|-----------------------------------|
| Build the project                            | `make`                            |
| Build + run against `sample.c`               | `make run`                        |
| Run against a specific file                  | `./bin/lexer <file>`              |
| Run interactively (type code, `Ctrl+D` to end) | `./bin/lexer`                   |
| Remove the compiled binary                   | `make clean`                      |
| Rebuild from scratch                         | `make clean && make`              |

### Example

Given this input (`sample.c`):

```c
int main() {
    int count = 10;
    return 0;
}
```

Running `./bin/lexer sample.c` prints a table like this:

```
LINE   TYPE         VALUE
-----------------------------------
1      KEYWORD      int
1      IDENTIFIER   main
1      PUNCTUATION  (
1      PUNCTUATION  )
1      PUNCTUATION  {
2      KEYWORD      int
2      IDENTIFIER   count
2      OPERATOR     =
2      NUMBER       10
2      PUNCTUATION  ;
3      KEYWORD      return
3      NUMBER       0
3      PUNCTUATION  ;
4      PUNCTUATION  }
```

Each row is one token: which **line** it started on, what **type** of
token it is, and the exact **text** the lexer read for it.

---

## 3. How it works internally

### 4.1 The `Lexer` struct

```c
typedef struct {
    const char *src;   // pointer to the whole source text
    size_t pos;         // index of the next character to read
    size_t length;       // total length of the source
    int line;           // current line number (starts at 1)
} Lexer;
```

The lexer doesn't copy the source or split it up in advance — it just
keeps a pointer into the original text (`src`) and a cursor (`pos`)
that walks forward through it one character at a time. `line` is
updated every time a `\n` is consumed, so every token knows where it
came from.

### 4.2 The `Token` struct

```c
typedef struct {
    TokenType type;   // what kind of token this is
    char *value;      // the actual text of the token
    int line;         // line number where it starts
} Token;
```

Every call to `lexer_next_token()` returns one of these. `value` is
heap-allocated (with `malloc`), so the caller is responsible for
freeing it — that's what `token_free()` is for.

### 4.3 The main loop (`main.c`)

`main.c` does three things:

1. Reads the whole input (file or stdin) into one big string.
2. Calls `lexer_init()` once to set up the lexer over that string.
3. Repeatedly calls `lexer_next_token()` and prints each token until
   it gets a token of type `TOKEN_EOF`, then stops.

### 4.4 Token types (`TokenType` enum)

| Type                | Meaning                                   | Examples                    |
|---------------------|--------------------------------------------|------------------------------|
| `TOKEN_IDENTIFIER`  | Names the programmer made up               | `count`, `my_var`, `main`    |
| `TOKEN_KEYWORD`     | Reserved words built into the language     | `int`, `if`, `while`, `return` |
| `TOKEN_NUMBER`      | Integer or floating point literals         | `42`, `3.14`, `1e10`         |
| `TOKEN_STRING`      | Double-quoted text                         | `"hello"`                    |
| `TOKEN_CHAR`        | Single-quoted character                    | `'A'`                        |
| `TOKEN_OPERATOR`    | Symbols that act on values                 | `+`, `==`, `&&`, `+=`, `->`  |
| `TOKEN_PUNCTUATION` | Structural symbols                         | `( ) { } [ ] ; , .`          |
| `TOKEN_UNKNOWN`     | A character the lexer doesn't understand   | `#`, `@`, `$`                |
| `TOKEN_EOF`         | Signals there is nothing left to read      | (returned once, at the end)  |

### 4.5 How each kind of token is recognised

The core function is `lexer_next_token()`. Every time it's called, it:

1. **Skips whitespace and comments** (`skip_whitespace_and_comments`)
   — spaces, tabs, newlines, `// line comments`, and
   `/* block comments */` are all silently consumed and never turned
   into tokens.
2. **Looks at the next character** and decides what kind of token to
   read based on it:
   - Starts with a letter or `_` &rarr; read a whole
     **identifier/keyword** (`read_identifier_or_keyword`). After
     reading it, the word is checked against a fixed list of C
     keywords (`int`, `if`, `while`, `return`, ...); if it matches,
     the token becomes `TOKEN_KEYWORD`, otherwise it stays
     `TOKEN_IDENTIFIER`.
   - Starts with a digit &rarr; read a whole **number**
     (`read_number`), including an optional decimal point (`3.14`)
     and an optional exponent (`1e10`, `2.5E-3`).
   - Starts with `"` &rarr; read a **string** (`read_string`) up to
     the closing `"`, correctly skipping over escaped characters like
     `\"` so the string doesn't end early.
   - Starts with `'` &rarr; read a **character literal**
     (`read_char`) the same way.
   - Otherwise, try to match an **operator** (`match_operator`). This
     checks a table of operators from longest to shortest (`<<=`
     before `<<` before `<`) so that, for example, `==` is read as
     one token and not as two `=` tokens.
   - If it's one of `(){}[];,.` &rarr; a single-character
     **punctuation** token.
   - Anything else &rarr; a single-character `TOKEN_UNKNOWN` token
     (the character is still consumed, so the lexer never gets stuck).
3. **Returns the token** it built, made with `make_token()`, which
   copies the matched text out of the source into a fresh,
   null-terminated string.

The caller (`main.c`) just keeps calling `lexer_next_token()` in a
loop until it sees `TOKEN_EOF`.

---

PARSER WILL BE ADDED SOON 
