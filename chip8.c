#include "chip8.h"
#include <SDL2/SDL_events.h>
#include "log.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void chip8_init(chip8 *chip)
{
    srand(time(NULL));
    memset(chip, 0, sizeof *chip);
    memcpy(chip->memory, fonts, sizeof fonts);
    chip->pc = 0x200;
    chip->i = 0;
    chip->clockspeed = DEFAULT_CLOCK;
    chip->settings = chip8_default_settings;
}

void chip8_interpret(chip8 *chip)
{
    for (int i = 0; i < chip->clockspeed / 60; ++i) {
        uint16_t op = chip->memory[chip->pc] << 8 | chip->memory[chip->pc+1];
        if (chip->key_waiting) return;
        if (op == 0x0) return;

        /* OP -> AxyB */
        int x = (op & 0x0F00) >> 8;
        int y = (op & 0x00F0) >> 4;
        int n = op & 0x000F;
        int nn = op & 0x00FF;
        int nnn = op & 0x0FFF;

        chip->pc += 2;
        switch (op & 0xF000) {
            case 0x0000:
                switch (nn) {
                    case 0xE0:
                        /* CLR */
                        memset(chip->screen, 0, sizeof chip->screen);
                        break;
                    case 0xEE:
                        /* RET */
                        chip->pc = chip->stack[--chip->sp];
                        break;
                    case 0xFA:
                        /* COMPAT */
                        //chip->FX55_FX65_change_I ^= 1;
                        break;
                    /* 0NNN - sys is not implemented as is not needed anymore */
                }
                break;
            case 0x1000:
                /* JUMP TO nnn */
                chip->pc = nnn;
                break;
            case 0x2000:
                /* call subroutine at nnn */
                chip->stack[chip->sp++] = chip->pc;
                chip->pc = nnn;
                break;
            case 0x3000:
                /* SE Vx, byte */
                if (chip->v[x] == nn)
                    chip->pc += 2;
                break;
            case 0x4000:
                /* SNE Vx, byte */
                if (chip->v[x] != nn)
                    chip->pc += 2;
                break;
            case 0x5000:
                /* SE Vx, Vy */
                if (chip->v[x] == chip->v[y])
                    chip->pc += 2;
                break;
            case 0x6000:
                chip->v[x] = nn;
                break;
            case 0x7000:
                chip->v[x] += nn;
                break;
            case 0x8000:
                switch (n) {
                    case 0x0:
                        chip->v[x] = chip->v[y];
                        break;
                    case 0x1:
                        chip->v[x] |= chip->v[y];
                        if (chip->settings.op_8xy1_2_3_reset_vf)
                            chip->v[0xF] = 0;
                        break;
                    case 0x2:
                        chip->v[x] &= chip->v[y];
                        if (chip->settings.op_8xy1_2_3_reset_vf)
                            chip->v[0xF] = 0;
                        break;
                    case 0x3:
                        chip->v[x] ^= chip->v[y];
                        if (chip->settings.op_8xy1_2_3_reset_vf)
                            chip->v[0xF] = 0;
                        break;
                    case 0x4: {
                        /* Vx = Vx + Vy, VF = carry */
                        int flag = ((int)chip->v[x] + (int)chip->v[y]) > 0xFF;
                        chip->v[x] += chip->v[y];
                        chip->v[0xF] = flag;
                        break;
                              }
                    case 0x5: {
                        /* Vx = Vx - Vy, VF = NOT borrow */
                        int flag = chip->v[x] > chip->v[y];
                        chip->v[x] = chip->v[x] - chip->v[y];
                        chip->v[0xF] = flag;
                        break;
                              }
                    case 0x6: {
                        /* Vx = Vx SHR 1 */
                        if (chip->settings.op_8xy6_8xye_do_vy)
                            chip->v[x] = chip->v[y];
                        int flag =  chip->v[x] & 1;
                        chip->v[x] >>= 1;
                        chip->v[0xF] = flag;
                        break;
                              }
                    case 0x7: {
                        /* Vx = Vy - Vx, VF = NOT borrow */
                        int flag = chip->v[y] > chip->v[x];
                        chip->v[x] = chip->v[y] - chip->v[x];
                        chip->v[0xF] = flag;
                        break;
                              }
                    case 0xE: {
                        /* Vx = Vx SHL 1 */
                        if (chip->settings.op_8xy6_8xye_do_vy)
                            chip->v[x] = chip->v[y];
                        int flag =  chip->v[x] >> 7;
                        chip->v[x] <<= 1;
                        chip->v[0xF] = flag;
                        break;
                              }
                }
                break;
            case 0x9000:
                if (chip->v[x] != chip->v[y])
                    chip->pc += 2;
                break;
            case 0xA000:
                chip->i = nnn;
                break;
            case 0xB000:
                chip->pc = chip->v[0] + nnn;
                break;
            case 0xC000:
                chip->v[x] = (rand() % 256) & nn;
                break;
            case 0xD000:
                chip->v[0xF] = 0;
                for (int row = 0; row < n; row++) {
                    uint8_t sprite = chip->memory[chip->i + row];
                    for (int col = 0; col < 8; col++) {
                        int bit = sprite >> (7 - col) & 1;
                        uint8_t dx = chip->v[x] + col;
                        uint8_t dy = chip->v[y] + row;
                        if (chip->settings.screen_wrap_around) {
                            dx %= WIDTH;
                            dy %= HEIGHT;
                        } else if (dx >= WIDTH || dy >= HEIGHT) {
                            continue;
                        }
                        if (bit && chip->screen[dy][dx])
                            chip->v[0xF] = 1;
                        chip->screen[dy][dx] ^= bit;
                    }
                }
                break;
            case 0xE000:
                switch (nn) {
                    case 0x9E:
                        /* SKP Vx */
                        if (chip->keys & (1 << chip->v[x]))
                            chip->pc += 2;
                        break;
                    case 0xA1:
                        /* SKNP Vx */
                        if (!(chip->keys & (1 << chip->v[x])))
                            chip->pc += 2;
                        break;
                }
                break;
            case 0xF000:
                switch (nn) {
                    case 0x07:
                        chip->v[x] = chip->delaytimer;
                        break;
                    case 0x0A:
                        chip8_wait_for_key(chip, x);
                        break;
                    case 0x15:
                        chip->delaytimer = chip->v[x];
                        break;
                    case 0x18:
                        chip->soundtimer = chip->v[x];
                        break;
                    case 0x1E:
                        chip->i += chip->v[x];
                        chip->v[0xF] = chip->i > 0x0FFF;
                        break;
                    case 0x29:
                        chip->i = chip->v[x] * 5;
                        break;
                    case 0x33:
                        chip->memory[chip->i] = chip->v[x] / 100;
                        chip->memory[chip->i+1] = (chip->v[x] % 100) / 10;
                        chip->memory[chip->i+2] = chip->v[x] % 10;
                        break;
                    case 0x55:
                        for (int i = 0; i <= x; i++)
                            chip->memory[chip->i+i] = chip->v[i];
                        if (chip->settings.op_fx55_fx65_increment)
                            chip->i += x + 1;
                        break;
                    case 0x65:
                        for (int i = 0; i <= x; i++)
                            chip->v[i] = chip->memory[chip->i+i];
                        if (chip->settings.op_fx55_fx65_increment)
                            chip->i += x + 1;
                        break;
                }
                break;
            default:
                printf("Invalid OPCODE %4x\n", op);
                break;
        }
    }
}

void chip8_load_rom(chip8 *chip, uint8_t *buf, size_t size)
{
    chip->pc = 0x200;
    chip->i = 0;
    memset(chip->screen, 0, sizeof chip->screen);
    memcpy(chip->memory + 0x200, buf, size);
}

int chip8_load_rom_from_file(chip8 *chip, const char *path)
{
    chip->pc = 0x200;
    memset(chip->screen, 0, sizeof chip->screen);
    chip->i = 0;
	FILE *rom = fopen(path, "rb");
	if (rom == NULL) {
        warn("Failed opening rom file");
        return -1;
    }
	fseek(rom, 0, SEEK_END);
	size_t rom_size = ftell(rom);
	rewind(rom);
	if ((MEMORY_SIZE - 0x200) < rom_size) {
        warn("Rom too big!");
        return -1;
    }
	size_t result = fread(chip->memory+0x200, 1, rom_size, rom);
	if (result != rom_size) {
        warn("Error reading rom to memory");
        return -1;
    }
	fclose(rom);
	return 0;
}

void chip8_update_timer(chip8 *chip)
{
    if (chip->delaytimer > 0)
        chip->delaytimer--;
    if (chip->soundtimer > 0)
        chip->soundtimer--;
}

void chip8_wait_for_key(chip8 *chip, int reg) {
    if (chip->key_waiting) return;
    chip->key_waiting = true;
    chip->register_waiting = reg;
}

void chip8_save_to_file(chip8 *chip, const char *path) {
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        warn("Failed to open file for saving");
        return;
    }
    fwrite(chip, sizeof *chip, 1, fp);
    fclose(fp);
}

void chip8_restore_from_file(chip8 *chip, const char *path) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        warn("Failed to open file for restoring");
        return;
    }
    fread(chip, sizeof *chip, 1, fp);
    fclose(fp);
}

bool chip8_keyisdown(chip8 *chip, int key) {
    return chip->keys & (1 << key);
}

void chip8_keydown(chip8 *chip, int key) {
    chip->keys |= 1 << key;
}

void chip8_keyup(chip8 *chip, int key) {
    if (chip->key_waiting) {
        chip->key_waiting = false;
        chip->v[chip->register_waiting] = key;
    }
    chip->keys &= ~(1 << key);
}

