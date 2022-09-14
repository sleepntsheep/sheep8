#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdbool.h>

#define WIDTH 64
#define HEIGHT 32
#define MEMORY_SIZE 2048
#define NUM_REGISTERS 16
#define DEFAULT_CLOCK 500

struct
Chip8 {
    bool screen[HEIGHT][WIDTH];
    int idx;
    int pc; /* program counter */
    uint8_t memory[MEMORY_SIZE]; /* memory for loading the ROM - 2kb */
    uint8_t v[NUM_REGISTERS]; /* 16 8 bit register */
    uint8_t delaytimer, soundtimer; /* sound timer make sound if more than 0 */
    uint16_t stack[256]; /* array for stack */
    uint8_t sp; /* stack pointer */
    int state[16]; /* keypad state */
    int clockspeed;
    bool paused;
    bool waiting_for_key;
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

uint16_t chip8_getop(Chip8 chip); 

int chip8_loadrom(Chip8 *chip, char* path);

void chip8_init(Chip8 *chip);

void chip8_loadfont(Chip8 *chip);

void chip8_updatetimer(Chip8 *chip);

void chip8_interpretop(Chip8 *chip, uint16_t op);

void chip8_clearscr(Chip8 *chip);

int chip8_flippixel(Chip8 *chip, int x, int y);

void chip8_doevent(Chip8 *chip);

#endif
