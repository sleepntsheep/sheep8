CC = tcc
CFLAGS := -std=c99 -pedantic -Wall -Wextra -Ofast
LIBS := -lSDL2 -lm
SRCS := main.c chip8.c beeper.c tinyfiledialogs.c input.c

all: sheep8

sheep8: $(SRCS)
	$(CC) $(SRCS) -o $@ $(CFLAGS) $(LIBS)

run: sheep8
	./sheep8


web: $(SRCS)
	emcc -o wasm/sheep8.html $(SRCS) -Os -Wall \
	-DPLATFORM_WEB \
	--shell-file wasm/shell.html \
	-sEXPORTED_RUNTIME_METHODS=[ccall] \
	-sUSE_SDL=2 \
	--preload-file roms/BRIX \
	--preload-file roms/TETRIS \
	--preload-file roms/octopaint.ch8

