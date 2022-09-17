#pragma once
#ifndef CHIP8_H
#define CHIP8_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define WIDTH 64
#define HEIGHT 32
#define MEMORY_SIZE 30000
#define NUM_REGISTERS 16
#define DEFAULT_CLOCK 500

typedef struct {
    bool op_8xy6_8xye_do_vy;
    bool op_fx55_fx65_increment;
    bool op_8xy1_2_3_reset_vf;
} Chip8Settings;

typedef struct {
    bool key_waiting;
    bool register_waiting;
    uint32_t keys;
    uint32_t clockspeed;
    bool paused;
    uint16_t i;
    uint16_t pc; /* program counter */
    uint8_t memory[MEMORY_SIZE]; /* memory for loading the ROM - 2kb */
    uint8_t v[NUM_REGISTERS]; /* 16 8 bit register */
    uint16_t stack[256]; /* array for stack */
    bool screen[HEIGHT*2][WIDTH*2];
    uint8_t delaytimer, soundtimer; /* sound timer */
    uint8_t sp; /* stack pointer */
} Chip8;

static const uint8_t fonts[] = {
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

int Chip8_LoadRom(Chip8 *chip, uint8_t *buf, size_t size);

int Chip8_LoadRomFromFile(Chip8 *chip, char* path);

void Chip8_Init(Chip8 *chip);

void Chip8_UpdateTimer(Chip8 *chip);

void Chip8_Interpret(Chip8 *chip);

void Chip8_DoEvent(Chip8 *chip);

void Chip8_Input(Chip8 *chip);

void Chip8_WaitForKey(Chip8 *chip, int reg);

#endif
