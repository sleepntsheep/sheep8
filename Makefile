all:
	make build
	make run

build:
	gcc main.c -std=c89 -Wall -lSDL2 -lSDL2_ttf -lm -o out

mingw:
	gcc main.c -std=c89 -e main -Wall -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lm -o out

run:
	./out
