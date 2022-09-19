#pragma once
#ifndef CHIP8_AUDIO_H
#define CHIP8_AUDIO_H

#include <stdbool.h>
#include <math.h>

#define MAX_SAMPLES               512
#define MAX_SAMPLES_PER_UPDATE   4096

typedef struct {
    short *data;
    short *buffer;
    int wavelength;
    bool playing;
} audio_driver;

audio_driver audio_driver_Init();
void audio_driver_Play(audio_driver *driver);
void audio_driver_Pause(audio_driver *driver);
void audio_driver_Drop(audio_driver *driver);

#endif /* CHIP8_AUDIO_H */
