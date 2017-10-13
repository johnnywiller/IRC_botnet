CC = gcc
WARNINGS = -Wall -Wextra
BIN = ./bin/
SRC = ./src/
FLAGS = -pthread

default: scanner

$(BIN)bot.o: $(SRC)bot.c
	$(CC) $(WARNINGS) $(FLAGS) -c $(SRC)bot.c -o $(BIN)bot.o

bot: $(BIN)bot.o
	$(CC) $(WARNINGS) $(FLAGS) $(BIN)bot.o -o $(BIN)bot

$(BIN)scanner.o: $(SRC)scanner.c
	$(CC) $(WARNINGS) $(FLAGS) -c $(SRC)scanner.c -o $(BIN)scanner.o

scanner: $(BIN)scanner.o
	$(CC) $(WARNINGS) $(FLAGS) $(BIN)scanner.o -o $(BIN)scanner

clean:
	-rm -f $(BIN)bot.o
	-rm -f $(BIN)bot
	-rm -f $(BIN)scanner.o
	-rm -f $(BIN)scanner
