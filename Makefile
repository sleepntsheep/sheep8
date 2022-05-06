CC	   := gcc
CFLAGS := -Wall -Wextra -std=c89 -ansi -pedantic -lm -ldl `sdl2-config --cflags`

BIN	 := bin
SRC	 := src
INCLUDE := include
LIB	 := lib
LIBRARIES   := `sdl2-config --libs`
EXECUTABLE  := sheep8

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

build:
	gcc main.c chip8.c -o emu -std=c89 -ansi -pedantic -Wall `sdl2-config --cflags` -lm -ldl -Wall -g -lSDL2 -lSDL2_ttf

mingw:
	gcc main.c chip8.c -std=c89 -e main -Wall -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lm -o emu

clean:
	rm emu
	rm emu.exe
