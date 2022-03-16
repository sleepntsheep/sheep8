all:
	make build
	make run

build:
	g++ main.c -lSDL2 -o out

run:
	./out