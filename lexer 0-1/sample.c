// sample source to test the lexer
#include <stdio.h>

int main() {
    int count = 10;      /* block comment */
    float pi = 3.14159;
    char letter = 'A';
    char *msg = "Hello, world!\n";

    if (count >= 5 && count != 0) {
        count += 1;
    } else {
        count--;
    }

    return 0;
}
