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

struct beeper {
    SDL_AudioDeviceID device;
    SDL_AudioSpec spec;
    bool playing;
    float time;
    float freq;
};

void beeper_init(struct beeper *beeper);
void beeper_play(struct beeper *beeper);
void beeper_pause(struct beeper *beeper);
void beeper_callback(void *userdata, uint8_t *stream, int len);
void beeper_clean(struct beeper *beeper);

#endif /* CHIP8_AUDIO_H */
