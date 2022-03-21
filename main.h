#ifndef MAIN_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32a
#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_audio.h"
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_audio.h>
#endif

#define FONT_PATH "terminus.ttf"
#define WIDTH 64
#define HEIGHT 32
#define SCALE 15
#define MEMORY_SIZE 4096
#define NUM_REGISTERS 16
#define FPS 60
#define AMPLITUDE 28000
#define FREQUENCY 44100
#define lastOPcount 10

int init(char* title);

void display();

int flipPixel(int x, int y);

void clearScr();

void interpretOP(uint16_t op);

void loadSpriteToMem();

void loadProgramToMem(size_t size, uint8_t *arr);

void updateTimers();

void drawText(char * text, int x, int y);

int loadRom(char* path);

void audio_callback(void* userdata, uint8_t* stream, int len);

void handleEvent();


#endif