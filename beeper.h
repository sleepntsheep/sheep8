#pragma once
#ifndef CHIP8_AUDIO_H
#define CHIP8_AUDIO_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <SDL2/SDL.h>

#define MAX_SAMPLES               512
#define MAX_SAMPLES_PER_UPDATE   4096

#define PI2 6.28318530718

typedef struct beeper beeper_t;

struct beeper {
    SDL_AudioDeviceID device;
    SDL_AudioSpec spec;
    bool playing;
    float time;
    float freq;
};

void beeper_init(beeper_t *beeper);
void beeper_play(beeper_t *beeper);
void beeper_pause(beeper_t *beeper);
void beeper_callback(void *userdata, uint8_t *stream, int len);
void beeper_clean(beeper_t *beeper);

#endif /* CHIP8_AUDIO_H */
