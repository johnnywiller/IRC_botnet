CC = gcc
WARNINGS = -Wall -Wextra
BIN = ./bin/
SRC = ./src/
FLAGS = -pthread

default: bot

$(BIN)bot.o: $(SRC)bot.c
	$(CC) $(WARNINGS) $(FLAGS) -c $(SRC)bot.c -o $(BIN)bot.o

bot: $(BIN)bot.o
	$(CC) $(WARNINGS) $(FLAGS) $(BIN)bot.o -o $(BIN)bot

clean:
	-rm -f $(BIN)bot.o
	-rm -f $(BIN)bot

