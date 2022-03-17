all:
	make build
	make run

build:
	gcc -std=c11 main.c -e main -Wall -lSDL2 -lSDL2_ttf -lm -o out

mingw:
	gcc -std=c11 main.c -e main -Wall -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lm -o out

run:
	./out
