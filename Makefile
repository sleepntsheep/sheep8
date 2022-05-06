build:
	gcc main.c chip8.c -o emu -std=c89 -ansi -pedantic -Wall `sdl2-config --cflags` -lm -ldl -Wall -g -lSDL2 -lSDL2_ttf

mingw:
	gcc main.c chip8.c -std=c89 -e main -Wall -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lm -o emu

clean:
	rm emu
	rm emu.exe
