#pragma once
#ifndef CHIP8_AUDIO_H
#define CHIP8_AUDIO_H

#include <raylib.h>
#include <stdbool.h>
#include <math.h>

#define MAX_SAMPLES               512
#define MAX_SAMPLES_PER_UPDATE   4096

typedef struct {
    AudioStream stream;
    short *data;
    short *buffer;
    int wavelength;
    bool playing;
} AudioDriver;

AudioDriver AudioDriver_Init();
void AudioDriver_Play(AudioDriver *driver);
void AudioDriver_Pause(AudioDriver *driver);
void AudioDriver_Drop(AudioDriver *driver);

#endif /* CHIP8_AUDIO_H */
