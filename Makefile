build:
	gcc *.c -o emu -std=c89 -Wall -lSDL2 -lSDL2_ttf -lm

mingw:
	gcc *.c -std=c89 -e main -Wall -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lm -o emu

run:
	./out
