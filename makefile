CC = gcc
CFLAGS = -Iinclude
SRC = src/main.c src/shell.c src/execute.c
OUT = bin/myshell

all: $(OUT)

$(OUT): $(SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

clean:
	rm -rf bin

