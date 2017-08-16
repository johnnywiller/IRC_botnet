CC = gcc
WARNINGS = -Wall -Wextra
BIN = ./bin/
SRC = ./src/

default: bot

$(BIN)bot.o: $(SRC)bot.c
	$(CC) $(WARNINGS) -c $(SRC)bot.c -o $(BIN)bot.o

bot: $(BIN)bot.o
	$(CC) $(WARNINGS) $(BIN)bot.o -o $(BIN)bot

clean:
	-rm -f $(BIN)bot.o
	-rm -f $(BIN)bot

