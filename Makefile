CC	   := cc
CFLAGS := -O0 -Wall -Wextra -std=c89 -ansi -pedantic `sdl2-config --cflags` -ggdb

BIN	 := bin
SRC	 := src
INCLUDE := include
LIB	 := lib
LIBRARIES   := `sdl2-config --libs` -ldl -lm
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
