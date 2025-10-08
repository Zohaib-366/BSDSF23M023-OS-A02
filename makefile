CC = gcc
CFLAGS = -Wall -Wextra -std=c18
SRC = src/lsv1.5.0.c
OBJ = obj/lsv1.5.0.o
BIN = bin/lsv1.5.0

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJ)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

clean:
	rm -f $(OBJ) $(BIN)
