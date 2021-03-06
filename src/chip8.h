#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>

#if defined _WIN32 && !defined __MINGW32__
#include "SDL.h"
#include "SDL_events.h"
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#endif

#define WIDTH 64
#define HEIGHT 32
#define MEMORY_SIZE 2048
#define NUM_REGISTERS 16
#define DEFAULT_CLOCK 500

struct
Chip8 {
    int screen[HEIGHT][WIDTH];
    int idx;
    int pc; /* program counter */
    uint8_t memory[MEMORY_SIZE]; /* memory for loading the ROM - 2kb */
    uint8_t v[NUM_REGISTERS]; /* 16 8 bit register */
    uint8_t delayTimer, soundTimer; /* sound timer make sound if more than 0 */
    uint16_t stack[256]; /* array for stack */
    uint8_t sp; /* stack pointer */
    int state[16]; /* keypad state */
    int clockspeed;
};

static const uint8_t _fnts[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,
    0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0,
    0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10,
    0xF0, 0x80, 0xF0, 0x10, 0xF0,
    0xF0, 0x80, 0xF0, 0x90, 0xF0,
    0xF0, 0x10, 0x20, 0x40, 0x40,
    0xF0, 0x90, 0xF0, 0x90, 0xF0,
    0xF0, 0x90, 0xF0, 0x10, 0xF0,
    0xF0, 0x90, 0xF0, 0x90, 0x90,
    0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0,
    0xE0, 0x90, 0x90, 0x90, 0xE0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0,
    0xF0, 0x80, 0xF0, 0x80, 0x80
};

typedef struct Chip8 Chip8;

uint16_t
getop(Chip8 *chip); 

int
loadrom(Chip8 *chip, char* path);

Chip8
* chip_init(void);

void
loadfont(Chip8 *chip);

void
interpretOP(Chip8 *chip, uint16_t op);

void
clearscr(Chip8 *chip);

void
chip_handleevent(Chip8 *chip, SDL_Event evt);

int
flippixel(Chip8 *chip, int x, int y);

#endif
