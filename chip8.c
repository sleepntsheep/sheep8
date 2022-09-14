#include "chip8.h"
#include <raylib.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void chip8_init(Chip8 *chip)
{
    memset(chip, 0, sizeof *chip);
    chip->pc = 0x200;
    chip->paused = false;
    chip->clockspeed = DEFAULT_CLOCK;
    chip8_loadfont(chip);
}

uint16_t chip8_getop(Chip8 chip)
{
    return chip.memory[chip.pc] << 8 | chip.memory[chip.pc+1];
}

void chip8_clearscr(Chip8 *chip)
{
	memset(chip->screen, 0, sizeof(int) * WIDTH * HEIGHT);
}

int chip8_flippixel(Chip8 *chip, int x, int y)
{
    /* return pixel before flipping */
	if (y < 0 || y >= HEIGHT) return 0;
	if (x < 0 || x >= WIDTH) return 0;
	chip->screen[y][x] ^= 1;
	return !chip->screen[y][x];
}

void chip8_interpretop(Chip8 *chip, uint16_t op)
{
    if (chip->paused || chip->waiting_for_key)
        return;
	/* OP -> AxyB */
	uint8_t x = (op & 0x0F00) >> 8;
	uint8_t y = (op & 0x00F0) >> 4;

	chip->pc += 2;

	switch (op & 0xF000) {
		case 0x0000:
			switch (op) {
				case 0x00E0:
					/* CLR */
                    chip8_clearscr(chip);
					break;
				case 0x00EE:
					/* RET */
					chip->pc = chip->stack[chip->sp--];
					break;
			}
			break;
		case 0x1000:
		{
			/* JUMP TO nnn */
			chip->pc = op & 0xFFF;
			break;
		}
		case 0x2000:
		{
			/* call subroutine at nnn */
			chip->stack[++chip->sp] = chip->pc;
			chip->pc = op & 0xFFF;
			break;
		}
		case 0x3000:
			/* SE Vx, byte */
			if (chip->v[x] == (op & 0x00FF))
				chip->pc += 2;
			break;
		case 0x4000:
			/* SNE Vx, byte */
			if (chip->v[x] != (op & 0x00FF))
				chip->pc += 2;
			break;
		case 0x5000:
			/* SE Vx, Vy */
			if (chip->v[x] == chip->v[y])
				chip->pc += 2;
			break;
		case 0x6000:
			chip->v[x] = op & 0x00FF;
			break;
		case 0x7000:
			chip->v[x] += op & 0x00FF;
			break;
		case 0x8000:
			switch (op & 0xF) {
				case 0x0:
					chip->v[x] = chip->v[y];
					break;
				case 0x1:
					chip->v[x] = chip->v[x] | chip->v[y];
					break;
				case 0x2:
					chip->v[x] = chip->v[x] & chip->v[y];
					break;
				case 0x3:
					chip->v[x] = chip->v[x] ^ chip->v[y];
					break;
				case 0x4:
					if (chip->v[x] + chip->v[y] > 0xFF)
						chip->v[0xF] = 1;
					else
						chip->v[0xF] = 0;
					chip->v[x] += chip->v[y];
					break;
				case 0x5:
					if (chip->v[x] > chip->v[y])
						chip->v[0xF] = 1;
					else
						chip->v[0xF] = 0;
					chip->v[x] -= chip->v[y];
					break;
				case 0x6:
					chip->v[0xF] = chip->v[x] & 0x1;
					chip->v[x] >>= 1;
					break;
				case 0x7:
					if (chip->v[y] > chip->v[x])
						chip->v[0xF] = 1;
					else
						chip->v[0xF] = 0;
					chip->v[x] = chip->v[y] - chip->v[x];
					break;
				case 0xE:
					chip->v[0xF] = chip->v[x] & 0x80;
					chip->v[x] <<= 2;
					break;
			}
			break;
		case 0x9000:
			if (chip->v[x] != chip->v[y])
				chip->pc += 2;
			break;
		case 0xA000:
			chip->idx = op & 0x0FFF;
			break;
		case 0xB000:
			chip->pc = chip->v[0] + (op & 0x0FFF);
			break;
		case 0xC000:
			chip->v[x] = (rand() % 256) & (op & 0x00FF);
			break;
		case 0xD000:
			{
				/* DRW, Vx, Vy, nibble */
				int width = 8;
				int height = op & 0xF;
				int row, col;
				chip->v[0xF] = 0;
				for (row = 0; row < height; row++) {
					uint8_t sprite = chip->memory[chip->idx + row];

					for (col = 0; col < width; col++) {
						if ((sprite & 0x80) > 0) {
							if (chip8_flippixel(chip, chip->v[x] + col, chip->v[y] + row))  {
								chip->v[0xF] = 1;
								/* if register 0xF is set to 1, collision happened */
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
					if (chip->state[chip->v[x]])
						chip->pc += 2;
					break;
				case 0xA1:
					if (!chip->state[chip->v[x]])
						chip->pc += 2;
					break;
			}
			break;
		case 0xF000:
			switch (op & 0x00FF) {
				case 0x07:
					chip->v[x] = chip->delaytimer;
					break;
				case 0x0A:
                    chip->waiting_for_key = true;
					break;
				case 0x15:
					chip->delaytimer = chip->v[x];
					break;
				case 0x18:
					chip->soundtimer = chip->v[x];
					break;
				case 0x1E:
					chip->idx += chip->v[x];
					break;
				case 0x29:
					chip->idx = chip->v[x] * 5;
					break;
				case 0x33:
					chip->memory[chip->idx] = (chip->v[x] / 100) % 10;
					chip->memory[chip->idx+1] = (chip->v[x] / 10) % 10;
					chip->memory[chip->idx+2] = (chip->v[x] % 10);
					break;
				case 0x55: {
                    int i;
                    for (i = 0; i <= x; i++)
						chip->memory[chip->idx+i] = chip->v[i];
					break;
				}
				case 0x65: {
                    int i;
                    for (i = 0; i <= x; i++)
						chip->v[i] = chip->memory[chip->idx+i];
					break;
				}
			}
	}
}

void chip8_loadfont(Chip8 *chip)
{
	/* font for 0-F */
	size_t n = sizeof(_fnts)/sizeof(*_fnts);
    size_t i;
    for (i = 0; i < n; i++)
		chip->memory[i] = _fnts[i];
}

int chip8_loadrom(Chip8 *chip, char *path)
{
	FILE *rom = NULL;
    size_t rom_size;
    uint8_t *rom_buffer;
    size_t result;

    rom = fopen(path, "rb");
	if (rom == NULL)
        return -1;

	fseek(rom, 0, SEEK_END);
	rom_size = ftell(rom);
	rewind(rom);

	rom_buffer = (uint8_t *) malloc(sizeof(uint8_t) * rom_size);
	if (rom_buffer == NULL)
        return -1;

	result = fread(rom_buffer, sizeof(uint8_t), (size_t) rom_size, rom);
	if (result != rom_size)
        return -1;

	if ((MEMORY_SIZE - 0x200) > rom_size)
        memcpy(chip->memory+0x200, rom_buffer, rom_size);
	else 
        return -1;

	fclose(rom);
	free(rom_buffer);

	return 0;
}

void chip8_doevent(Chip8 *chip)
{
    chip->state[1] = IsKeyDown(KEY_ONE);
    chip->state[2] = IsKeyDown(KEY_TWO);
    chip->state[3] = IsKeyDown(KEY_THREE);
    chip->state[0xC] = IsKeyDown(KEY_FOUR);
    chip->state[4] = IsKeyDown(KEY_Q);
    chip->state[5] = IsKeyDown(KEY_W);
    chip->state[6] = IsKeyDown(KEY_E);
    chip->state[0xD] = IsKeyDown(KEY_R);
    chip->state[7] = IsKeyDown(KEY_A);
    chip->state[8] = IsKeyDown(KEY_S);
    chip->state[9] = IsKeyDown(KEY_D);
    chip->state[0xE] = IsKeyDown(KEY_F);
    chip->state[0xA] = IsKeyDown(KEY_Z);
    chip->state[0] = IsKeyDown(KEY_X);
    chip->state[0xB] = IsKeyDown(KEY_C);
    chip->state[0xF] = IsKeyDown(KEY_V);
}

void chip8_updatetimer(Chip8 *chip)
{
    if (chip->delaytimer > 0)
        chip->delaytimer--;
    if (chip->soundtimer > 0)
        chip->soundtimer--;
}
