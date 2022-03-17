all:
	make build
	make run

build:
	gcc -std=c11 main.c -Wall -lSDL2 -o out

run:
	./out