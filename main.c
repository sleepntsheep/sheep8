#ifdef _WIN32a
#include "SDL.h"
#else
#include <SDL2/SDL.h>
#endif
// #include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#define COLS 64
#define ROWS 32
#define SCALE 15
#define MEMORY_SIZE 4096;
#define NUM_REGISTERS 16;

struct stack {
	int pt;
	uint16_t ar[2000];
}

uint16_t pop (struct stack st);

void push (struct stack *st, uint16_t num);

// chip8

bool screen[ROWS][COLS];
uint8_t memory[MEMORY_SIZE];
uint8_t v[NUM_REGISTERS]
int index = 0;
int pc = 0x200;
uint8_t delayTimer, soundTimer;


//sdl stuff

bool running = true;
SDL_Window* window = NULL;
SDL_Renderer* renderer;
SDL_Event event;
SDL_Surface* surface;
SDL_Rect rect;
Uint8* state;

int init();

void display();

void flipPixel(int x, int y);

void clearScr();

void interpret(uint16_t op);

int main(int argc, char* argv[]) {
	if (init() != 0) {
		exit(-1);
	}

	while (running) {
		state = SDL_GetKeyboardState(nullptr);
		while (SDL_PollEvent( &event )) {
			if ( event.type == SDL_QUIT ) {
				running = false;
			}
		}

		display();
	}
}

int init() {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("Error initing SDL: %s\n", SDL_GetError());
		return -1;
	}

	window = SDL_CreateWindow("CHIP8 Emu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, COLS * SCALE, ROWS * SCALE, 0);

	if (!window) {
		printf("failed creating window: %s", SDL_GetError());
		return -1;
	}

	surface = SDL_GetWindowSurface(window);

	if (!surface) {
		printf("failed creating surface: %s", SDL_GetError());
		return -1;
	}

	renderer = SDL_CreateRenderer(window, -1, 0);

	if (!renderer) {
		printf("failed creating renderer: %s", SDL_GetError());
		return -1;
	}

	return 0;
}

uint16_t pop (struct stack *st) {
	st->pt--;
	return st[pt+1];
}

void push (struct stack *st, uint16_t num) {
	st->pt++;
	st[pt] = num;
}

void flipPixel(int x, int y) {
	screen[y][x] = screen[y][x] ^ 1;
	// return !screen[y][x];
}

void clearScr() {
	memset(screen, false, sizeof(bool) * COLS * ROWS);
}

void display() {
	for (int i = 0; i < ROWS; i++) {
		for (int j = 0; j < COLS; j++) {
			if (!screen[i][j])
				continue;
			rect = { j * SCALE, i * SCALE, SCALE, SCALE };
			SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
			SDL_RenderFillRect(renderer, &rect);
		}
	}
	SDL_RenderPresent(renderer);
}

void interpret(uint16_t op) {
	pc += 2;

	//AxyB
	unsigned char x = (op & 0x0F00) >> 8;
	unsigned char y = (op & 0x00F0) >> 4;

	switch (op & 0xF000) {
		case 0x0000:
			switch (op) {
				case 0x00E0:
					//CLR
					clearScr();
					break;
				case 0x00EE:
					//RET
					pc = pop(&stack);
					break;
			}
			break;
		case 0x1000:
			// JUMP TO nnn
			pc = op & 0xFFF;
			break;
		case 0x2000:
			// CALL SUBROUTINE
			push(&stack, pc);
			pc = op & 0xFFF;
			break;
		case 0x3000:
			// SE Vx, byte
			if (v[x] == op & 0x00FF)
				pc += 2;
			break;
		case 0x4000:
			// SNE Vx, byte
			if (v[x] != op & 0x00FF)
				pc += 2;
			break;
		case 0x5000:
			// SE Vx, Vy
			if (v[x] == v[y])
				pc += 2;
			break;
		case 0x6000:
			v[x] = op & 0x00FF;
			break;
		case 0x7000:
			v[x] += op & 0x00FF;
			break;
		case 0x8000:
			switch (op & 0xF) {
				case 0x1:
					v[x] = v[x] | v[y];
					break;
				case 0x2:
					v[x] = v[x] & v[y];
					break;
				case 0x3:
					v[x] = v[x] ^ v[y];
					break;
				case 0x4:
					if ((int)v[x] + (int)v[y] > 0xFF)
						v[0xF] = 1;
					v[x] += v[y];
					break;
				case 0x5:
					if (v[x] > v[y])
						v[0xF] = 1;
					else
						v[0xF] = 0;
					v[x] -= v[y];
					break;
				case 0x6:
					v[0xF] = v[x] & 0x1;
					v[x] >>= 1;
					break;
				case 0x7:
					if (v[y] > v[x])
						v[0xF] = 1;
					else
						v[0xF] = 0;
					v[x] = v[y] - v[x];
					break;
				case 0xE:
					v[0xF] = v[x] & 0x10000000;
					v[x] <<= 2;
					break;
			}
			break;
		case 0x9000:
			if (v[x] != v[y])
				pc += 2;
			break;
		case 0xA000:
			index = op & 0x0FFF;
			break;
		case 0xB000:
			pc = v[0] + (op & 0x0FFF);
			break;
		case 0xC000:
			v[x] = (rand() % 256) & kk;
			break;
		case 0xD000:
		// display sprite
			break;
		case 0xE000:
			switch (op & 0x00FF) {
				case 0x9E:
					if (state[v[x]])
						pc += 2;
					break;
				case 0xA1:
					if (!state[v[x]])
						pc ++ 2;
					break;
			}
			break;
		case 0xF000:
			switch (op & 0x00FF) {
				case 0x07:
					v[x] = delayTimer;
					break;
				case 0x0A:
					v[x] = SDL_WaitEvent(&event);
					break;
				case 0x15:
					delayTimer = v[x];
					break;
				case 0x1E:
					index += v[x];
					break;
				case 0x29:
				//sprite thing
					break;
				case 0x33:
					memory[index] = (v[x] / 100) % 10;
					memory[index+1] = (v[x] / 10) % 10;
					memory[index+2] = (v[x] % 10);
					break;
				case 0x55:
					for (int i = 0; i <= x; i++) {
						memory[index+i] = v[i];
					}
					break;
				case 0x65:
					for (int i = 0; i <= x; i++) {
						v[i] = memory[index+i];
					}
					break;
			}
	}
}
