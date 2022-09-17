#include "chip8.h"
#include <raylib.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


static const int kbs[] = { KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_A, KEY_S, KEY_D, KEY_F, KEY_Z, KEY_X, KEY_C, KEY_V };
static const int kb2pad[] = {
    [KEY_ONE] = 1+1, [KEY_TWO] = 2+1, [KEY_THREE] = 3+1, [KEY_FOUR] = 0xC+1,
    [KEY_Q] = 4+1, [KEY_W] = 5+1, [KEY_E] = 6+1, [KEY_R] = 0xD+1,
    [KEY_A] = 7+1, [KEY_S] = 8+1, [KEY_D] = 9+1, [KEY_F] = 0xE +1,
    [KEY_Z] = 0xA+1, [KEY_X] = 0x0+1, [KEY_C] = 0xB+1, [KEY_V] = 0xF+1,
    /**** TO AVOID 0x0 COLLISION, EVERY VALUE HERE HAS BEEN OFFSET BY +1 */
};


void Chip8_Init(Chip8 *chip)
{
    srand(time(NULL));
    memset(chip, 0, sizeof *chip);
    memcpy(chip->memory, fonts, sizeof fonts);
    chip->pc = 0x200;
    chip->clockspeed = DEFAULT_CLOCK;
}

void Chip8_Interpret(Chip8 *chip)
{
    for (int i = 0; i < chip->clockspeed / 60; ++i) {
        uint16_t op = chip->memory[chip->pc] << 8
            | chip->memory[chip->pc+1];
        if (chip->paused)
            return;
        if (chip->key_waiting)
            return;

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
                        chip->v[0xF] = 0;
                        break;
                    case 0x2:
                        chip->v[x] &= chip->v[y];
                        chip->v[0xF] = 0;
                        break;
                    case 0x3:
                        chip->v[x] ^= chip->v[y];
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
                        uint8_t dx = (chip->v[x] + col) % WIDTH;
                        uint8_t dy = (chip->v[y] + row) % HEIGHT;
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
                        Chip8_WaitForKey(chip, x);
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
                        break;
                    case 0x65:
                        for (int i = 0; i <= x; i++)
                            chip->v[i] = chip->memory[chip->i+i];
                        break;
                }
                break;
            default:
                printf("Invalid OPCODE %4x\n", op);
                break;
        }
    }
}

int Chip8_LoadRom(Chip8 *chip, uint8_t *buf, size_t size)
{
    memcpy(chip->memory+0x200, buf, size);
}

int Chip8_LoadRomFromFile(Chip8 *chip, char *path)
{
	FILE *rom = fopen(path, "rb");
	if (rom == NULL) {
        fprintf(stderr, "Failed opening rom\n");
        return -1;
    }
	fseek(rom, 0, SEEK_END);
	size_t rom_size = ftell(rom);
	rewind(rom);
	if ((MEMORY_SIZE - 0x200) < rom_size) {
        fprintf(stderr, "Rom too big\n");
        return -1;
    }
	size_t result = fread(chip->memory+0x200, 1, rom_size, rom);
	if (result != rom_size) {
        fprintf(stderr, "Error reading rom\n");
        return -1;
    }
	fclose(rom);
	return 0;
}

void Chip8_UpdateTimer(Chip8 *chip)
{
    if (chip->delaytimer > 0)
        chip->delaytimer--;
    if (chip->soundtimer > 0)
        chip->soundtimer--;
}

void Chip8_Input(Chip8 *chip) {
    for (int i = 0; i < sizeof kbs / sizeof *kbs; i++) {
        int pad = kb2pad[kbs[i]] - 1;
        if (IsKeyDown(kbs[i])) {
            chip->keys |= (1 << pad);
        } else {
            if (chip->keys & (1 << pad)
                    && chip->key_waiting) {
                chip->key_waiting = false;
                chip->v[chip->register_waiting] = pad;
            }
            chip->keys &= ~(1 << pad);
        }
    }
}

void Chip8_WaitForKey(Chip8 *chip, int reg) {
    if (chip->key_waiting)
        return;
    chip->key_waiting = true;
    chip->register_waiting = reg;
}

