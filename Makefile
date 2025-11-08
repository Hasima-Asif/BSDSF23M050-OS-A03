CC = gcc
CFLAGS = -Wall -g -Iinclude
SRC = src/main.c src/shell.c src/execute.c
BIN = bin/myshell

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $(BIN) $(SRC)

clean:
	rm -f $(BIN)

