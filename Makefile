all:
	make build
	make run

build:
	gcc -std=c11 main.c -Wall -lSDL2 -lSDL2_ttf -lm -o out

run:
	./out