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
#define MEMORY_SIZE 4096
#define NUM_REGISTERS 16
#define FPS 60
const int speed = 10;

uint16_t pop ();
void push (uint16_t num);

// chip8

bool screen[ROWS][COLS];
uint8_t memory[MEMORY_SIZE];
uint8_t v[NUM_REGISTERS];
uint8_t delayTimer, soundTimer;
bool paused = false;
int idx = 0;
int pc = 0x200;
uint16_t stack[256];
uint8_t sp;

//sdl stuff

Uint32 time_step_ms = 1000 / 60;
Uint32 next_game_step = SDL_GetTicks();

bool running = true;
SDL_Window* window = NULL;
SDL_Renderer* renderer;
SDL_Event event;
SDL_Surface* surface;
SDL_Rect rect;
Uint8* state;

int init();

void display();

bool flipPixel(int x, int y);

void clearScr();

void interpretOP(uint16_t op);

void loadSpriteToMem();

void loadProgramToMem(size_t size, uint8_t *arr);

void updateTimers();

bool loadRom(char* path);

int main(int argc, char** argv) {
	if (init() != 0) {
		exit(-1);
	}

	loadRom(argv[1]);

	while (running) {
		Uint32 now = SDL_GetTicks();
		// state = SDL_GetKeyboardState(NULL);
		while (SDL_PollEvent( &event )) {
			if ( event.type == SDL_QUIT ) {
				running = false;
			}
		}


		// sound();
		if (next_game_step <= now) {
			for (int i = 0; i < speed; i++) {
				if (!paused) {
					uint16_t opcode = (memory[pc] << 8 | memory[pc+1]);
					interpretOP(opcode);
				}
				if (!paused)
					updateTimers();
			}
			display();
		}
		else {
			SDL_Delay(next_game_step - now);
		}
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

uint16_t pop () {
	return stack[sp--];
}

void push (uint16_t num) {
	stack[++sp] = num;
}

bool flipPixel(int x, int y) {
	if (y < 0 || y >= ROWS) return 0;
	if (x < 0 || x >= COLS) return 0;
	screen[y][x] = screen[y][x] ^ 1;
	return !screen[y][x];
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

void interpretOP(uint16_t op) {
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
					pc = pop();
					break;
			}
			break;
		case 0x1000:
			// JUMP TO nnn
			pc = op & 0xFFF;
			break;
		case 0x2000:
			// CALL SUBROUTINE
			push(pc);
			pc = op & 0xFFF;
			break;
		case 0x3000:
			// SE Vx, byte
			if ((v[x] == op) & 0x00FF)
				pc += 2;
			break;
		case 0x4000:
			// SNE Vx, byte
			if ((v[x] != op) & 0x00FF)
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
			idx = op & 0x0FFF;
			break;
		case 0xB000:
			pc = v[0] + (op & 0x0FFF);
			break;
		case 0xC000:
			v[x] = (rand() % 256) & (op & 0x00FF);
			break;
		case 0xD000:
			{
				// DRW, Vx, Vy, nibble
				unsigned char width = 8;
				unsigned char height = op & 0xF; // nibble
				v[0xF] = 0;
				for (int row = 0; row < height; row++) {
					uint8_t sprite = memory[idx + row];

					for (int col = 0; col < width; col++) {
						if ((sprite & 0x80) > 0) {
							if (flipPixel(v[x] + col, v[y] + row))  {
								v[0xF] = 1; // collision
							}
						}
						sprite <<= 1;
					}
				}
			}
			break;
		case 0xE000:
			switch (op & 0x00FF) {
				case 0x9E:
					if (state[v[x]])
						pc += 2;
					break;
				case 0xA1:
					if (!state[v[x]])
						pc += 2;
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
					idx += v[x];
					break;
				case 0x29:
					idx = v[x] * 5;
					break;
				case 0x33:
					memory[(int)idx] = (v[x] / 100) % 10;
					memory[(int)idx+1] = (v[x] / 10) % 10;
					memory[(int)idx+2] = (v[x] % 10);
					break;
				case 0x55:
					for (int i = 0; i <= x; i++) {
						memory[idx+i] = v[i];
					}
					break;
				case 0x65:
					for (int i = 0; i <= x; i++) {
						v[i] = memory[idx+i];
					}
					break;
			}
	}
}

void loadSpriteToMem() {
	const uint8_t sprites[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};
	size_t n = sizeof(sprites)/sizeof(sprites[0]);
	for (int i = 0; i < n; i++) {
		memory[i] = sprites[i];
	}
}

void loadProgramToMem(size_t size, uint8_t *arr) {
	for (int i = 0; i < size; i++) {
		memory[0x200 + i] = arr[i];
	}
}

void updateTimers() {
	if (delayTimer > 0)
		delayTimer--;
	if (soundTimer > 0) 
		soundTimer--;
}

bool loadRom(char *path) {
	printf("Loading rom: %s\n", path);

	FILE *rom = fopen(path, "rb");
	if (rom == NULL) {
		printf("Failed to open ROM\n");
		exit(-1);
	}

	fseek(rom, 0, SEEK_END);
	size_t rom_size = ftell(rom);
	rewind(rom);

	uint8_t *rom_buffer = (uint8_t *) malloc(sizeof(uint8_t) * rom_size);
	if (rom_buffer == NULL) {
		printf("Failed to allocate memory for ROM\n");
		return false;
	}

	size_t result = fread(rom_buffer, sizeof(uint8_t), (size_t) rom_size, rom);
	if (result != rom_size) {
		printf("Failed to read ROM\n");
		return false;
	}

	if ((MEMORY_SIZE - 0x200) > rom_size) {
		for (int i = 0; i < rom_size; ++i) {
			memory[i+0x200] = rom_buffer[i];
		}
	}
	else  {
		printf("ROM is too large to fit in memory");
	}

	fclose(rom);
	free(rom_buffer);

	return true;
}
