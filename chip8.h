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
#define DEFAULT_CLOCK 700

typedef struct {
    bool op_8xy6_8xye_do_vy;
    bool op_fx55_fx65_increment;
    bool op_8xy1_2_3_reset_vf;
    bool screen_wrap_around;
} chip8_settings;

typedef struct {
    bool key_waiting;
    bool register_waiting;
    uint32_t keys;
    int clockspeed;
    uint16_t i;
    uint16_t pc; /* program counter */
    uint8_t memory[MEMORY_SIZE]; /* memory for loading the ROM - 2kb */
    uint8_t v[NUM_REGISTERS]; /* 16 8 bit register */
    uint16_t stack[256]; /* array for stack */
    bool screen[HEIGHT*2][WIDTH*2];
    uint8_t delaytimer, soundtimer; /* sound timer */
    uint8_t sp; /* stack pointer */
    chip8_settings settings;
} chip8;

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

void chip8_init(chip8 *chip);
void chip8_load_rom(chip8 *chip, uint8_t *buf, size_t size);
int chip8_load_rom_from_file(chip8 *chip, const char* path);
void chip8_update_timer(chip8 *chip);
void chip8_interpret(chip8 *chip);
void chip8_wait_for_key(chip8 *chip, int reg);
void chip8_save_to_file(chip8 *chip, const char *path);
void chip8_restore_from_file(chip8 *chip, const char *path);
void chip8_keydown(chip8 *chip, int key);
void chip8_keyup(chip8 *chip, int key);
bool chip8_keyisdown(chip8 *chip, int key);

static const chip8_settings chip8_default_settings = {
    .op_8xy6_8xye_do_vy = true,
    .op_fx55_fx65_increment = false,
    .op_8xy1_2_3_reset_vf = true,
};

#endif /* CHIP8_H */

