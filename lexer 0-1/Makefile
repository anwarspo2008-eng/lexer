CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude

SRC_DIR = src
BIN_DIR = bin
TARGET  = $(BIN_DIR)/lexer

SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/lexer.c
HEADERS = include/lexer.h

$(TARGET): $(SOURCES) $(HEADERS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

run: $(TARGET)
	./$(TARGET) sample.c

clean:
	rm -f $(TARGET)

.PHONY: run clean
