CC = gcc
WARNINGS = -Wall -Wextra
BIN = ./bin/
SRC = ./src/
FLAGS = -static -static-libgcc
DEBUG = -g
default: bot

$(BIN)bot.o: $(SRC)bot.c
	$(CC) $(WARNINGS) $(FLAGS) $(DEBUG) -c $(SRC)bot.c -o $(BIN)bot.o

bot: $(BIN)bot.o $(BIN)irc.o $(BIN)scanner.o $(BIN)attack.o
	$(CC) $(WARNINGS) $(FLAGS) $(DEBUG) $(BIN)bot.o $(BIN)irc.o $(BIN)scanner.o $(BIN)attack.o -o $(BIN)bot

$(BIN)scanner.o: $(SRC)scanner.c
	$(CC) $(WARNINGS) $(FLAGS) $(DEBUG) -c $(SRC)scanner.c -o $(BIN)scanner.o

scanner: $(BIN)scanner.o
	$(CC) $(WARNINGS) $(FLAGS) $(DEBUG) $(BIN)scanner.o -o $(BIN)scanner


$(BIN)irc.o: $(SRC)irc.c
	$(CC) $(WARNINGS) $(FLAGS) $(DEBUG) -c $(SRC)irc.c -o $(BIN)irc.o

$(BIN)attack.o: $(SRC)attack.c
	$(CC) $(WARNINGS) $(FLAGS) $(DEBUG) -c $(SRC)attack.c -o $(BIN)attack.o

clean:
	-rm -f $(BIN)bot.o
	-rm -f $(BIN)bot
	-rm -f $(BIN)scanner.o
	-rm -f $(BIN)scanner
	-rm -f $(BIN)irc.o
	-rm -f $(BIN)attack.o
