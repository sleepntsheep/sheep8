#include "audio.h"
#include <stdlib.h>

float frequency = 440.0f;
float audioFrequency = 440.0f;
float oldFrequency = 1.0f;
float sineIdx = 0.0f;

void AudioInputCallback(void *buffer, unsigned int frames)
{
    audioFrequency = frequency + (audioFrequency - frequency)*0.95f;
    audioFrequency += 1.0f;
    audioFrequency -= 1.0f;
    float incr = audioFrequency/44100.0f;
    short *d = (short *)buffer;

    for (int i = 0; i < frames; i++)
    {
        d[i] = (short)(32000.0f*sinf(2*PI*sineIdx));
        sineIdx += incr;
        if (sineIdx > 1.0f) sineIdx -= 1.0f;
    }
}

AudioDriver AudioDriver_Init() {
    AudioDriver driver;
    InitAudioDevice();

    SetAudioStreamBufferSizeDefault(MAX_SAMPLES_PER_UPDATE);
    driver.stream = LoadAudioStream(44100, 16, 1);

    frequency = 440.0f;
    audioFrequency = 440.0f;
    oldFrequency = 1.0f;
    sineIdx = 0.0f;

    SetAudioStreamCallback(driver.stream, AudioInputCallback);

    driver.data = malloc(sizeof(short)*MAX_SAMPLES);

    driver.buffer = malloc(sizeof(short)*MAX_SAMPLES_PER_UPDATE);

    driver.wavelength = (int)(22050/frequency);
    if (driver.wavelength > MAX_SAMPLES/2)
        driver.wavelength = MAX_SAMPLES/2;
    if (driver.wavelength < 1)
        driver.wavelength = 1;
    // Write sine wave
    for (int i = 0; i < MAX_SAMPLES; i++)
        driver.data[i] = sinf(2.0f*PI*i/driver.wavelength)*32000;
    return driver;
}

void AudioDriver_Play(AudioDriver *driver) {
    if (driver->playing)
        return;
    driver->playing = true;
    PlayAudioStream(driver->stream);
}

void AudioDriver_Pause(AudioDriver *driver) {
    if (!driver->playing)
        return;
    driver->playing = false;
    PauseAudioStream(driver->stream);
}

void AudioDriver_Drop(AudioDriver *driver) {
    free(driver->data);
    free(driver->buffer);
    UnloadAudioStream(driver->stream);
    CloseAudioDevice();
}


