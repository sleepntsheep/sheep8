#pragma once
#ifndef INPUT_H_
#define INPUT_H_

#include <SDL2/SDL.h>
#include "chip8.h"

static const struct {
    const char *t;
    int i;
} keypad[] = {
    "1", 1, "2", 2,
    "3", 3, "4", 0xC,
    "Q", 4, "W", 5,
    "E", 6, "R", 0xD,
    "A", 7, "S", 8,
    "D", 9, "F", 0xE,
    "Z", 0xA, "X", 0,
    "C", 0xB, "V", 0xF,
};

void chip8_input_handle(chip8 *chip, SDL_Event e);

#endif /* INPUT_H_ */

