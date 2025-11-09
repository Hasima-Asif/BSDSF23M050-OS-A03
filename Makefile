CC = gcc
CFLAGS = -Wall -g -Iinclude
LDFLAGS = -lreadline
SRC = src/main.c src/shell.c src/execute.c
BIN = bin/myshell

all: $(BIN)

# Create bin directory if it doesn't exist
$(BIN): $(SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) -o $(BIN) $(SRC) $(LDFLAGS)

clean:
	rm -f $(BIN)

