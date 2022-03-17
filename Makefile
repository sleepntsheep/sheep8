all:
	make build
	make run

build:
	g++ main.c -Wall -lSDL2 -o out

run:
	./out