#ifndef MAIN_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_audio.h>

#include "chip8.h"

#define FONT_PATH "assets/terminus.ttf"
#define SCALE 15
#define FPS 60
#define AMPLITUDE 28000
#define FREQUENCY 44100
#define lastOPcount 10
#define SWIDTH WIDTH * SCALE
#define SHEIGHT HEIGHT * SCALE
#define SAVEFILE "save"

int
init(char* title);

void
die(const char *s);

void
display(Chip8 *chip);

void
updatetimer(Chip8 *chip);

void
quit(void);

void
audio_callback(void* userdata, uint8_t* stream, int len);

void
handle_event(Chip8 *chip, SDL_Event event);

void
save_state(Chip8 *chip);

void
load_save(Chip8 *chip);

#endif
