build:
	gcc *.c widget/*.c -o emu -std=c89 -ansi -pedantic -Wall -lSDL2 -lSDL2_ttf -lm -Wall -Werror -g

mingw:
	gcc *.c -std=c89 -e main -Wall -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lm -o emu

run:
	./out
