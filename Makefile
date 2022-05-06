CC	   := gcc
CFLAGS := -e main -O3 -Wall -Wextra -std=c89 -ansi -pedantic -lm `sdl2-config --cflags` -ggdb

BIN	 := bin
SRC	 := src
INCLUDE := include
LIB	 := lib
LIBRARIES   := `sdl2-config --libs`
EXECUTABLE  := sheep8

ifeq ($(OS), Windows_NT)
	LIBRARIES += -lSDL2main
endif

all: $(BIN)/$(EXECUTABLE)

run: clean all
	clear
	@echo "🚀 Executing..."
	./$(BIN)/$(EXECUTABLE)

$(BIN)/$(EXECUTABLE): $(SRC)/*.c
	@echo "🚧 Building..."
	$(CC) $(CFLAGS) -I$(INCLUDE) -L$(LIB) $^ -o $@ $(LIBRARIES)

clean:
	@echo "🧹 Clearing..."
	-rm $(BIN)/*
