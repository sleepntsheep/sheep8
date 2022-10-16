#define SDL_DISABLE_IMMINTRIN_H
#include "beeper.h"
#include <stdlib.h>

void beeper_callback(void *userdata, uint8_t *stream, int len)
{
    beeper_t *beeper = (beeper_t*)userdata;
    short *snd = (short*)stream;
    len /= sizeof *snd;
    for(int i = 0; i < len; i++) {
        snd[i] = 32000 * sin(beeper->time);
        beeper->time += beeper->freq * PI2 / 48000.0;
        if (beeper->time >= PI2)
            beeper->time -= PI2;
    }
}

/* note - user must call SDL_Init(SDL_INIT_AUDIO) before calling this */
void beeper_init(beeper_t *beeper) {
    memset(beeper, 0, sizeof *beeper);
    beeper->time = 0;
    beeper->freq = 441;
    beeper->spec.freq = 44100;
    beeper->spec.format = AUDIO_S16SYS;
    beeper->spec.channels = 1;
    beeper->spec.samples = 1024;
    beeper->spec.callback = beeper_callback;
    beeper->spec.userdata = beeper;
    beeper->device = SDL_OpenAudioDevice(NULL, 0, &beeper->spec, NULL, 0);
    SDL_PauseAudioDevice(beeper->device, true);
    beeper->playing = false;
}

void beeper_play(beeper_t *beeper) {
    if (beeper->playing) return;
    beeper->playing = true;
    SDL_PauseAudioDevice(beeper->device, false);
}

void beeper_pause(beeper_t *beeper) {
    if (!beeper->playing) return;
    beeper->playing = false;
    SDL_PauseAudioDevice(beeper->device, true);
}

void beeper_clean(beeper_t *beeper) {
    SDL_CloseAudioDevice(beeper->device);
}


