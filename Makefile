CFLAGS := -std=c99 -g -pedantic -Wall -Wextra
LIBS := -lSDL2 -lSDL2_ttf -lm
SRCS := main.c chip8.c

all: sheep8

sheep8: $(SRCS)
	cc $(SRCS) -o $@ $(CFLAGS) $(LIBS)

web: $(SRCS)
	emcc -o wasm/sheep8.html $(SRCS) -Os -Wall \
	-DPLATFORM_WEB \
	--shell-file wasm/shell.html \
	-sEXPORTED_RUNTIME_METHODS=[ccall] \
	-sUSE_SDL=2 -sUSE_SDL_TTF=2 \
	--preload-file roms/BRIX \
	--preload-file roms/TETRIS \
	--preload-file roms/octopaint.ch8

