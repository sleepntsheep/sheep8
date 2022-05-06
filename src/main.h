#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>

#if defined _WIN32 && !defined __MINGW32__
#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_audio.h"
#include "SDL_video.h"
#include "SDL_events.h"
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_events.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif


#include "chip8.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "nuklear/nuklear.h"
#include "nuklear/nuklear_sdl_renderer.h"

#define FONT_PATH "assets/terminus.ttf"
#define SCALE 15
#define FPS 60
#define AMPLITUDE 28000
#define FREQUENCY 44100
#define SWIDTH WIDTH * SCALE
#define SHEIGHT HEIGHT * SCALE
#define GWIDTH 300
#define GHEIGHT 200
#define SAVEFILE "save"

void
init(char* title, SDL_Window **window, SDL_Renderer **renderer, struct nk_context **ctx, Chip8 **chip);

void
die(const char *s);

void
chip_display(Chip8 *chip, SDL_Renderer *renderer, int r, int g, int b, int a);

void
gui_display(struct nk_context *ctx, Chip8 *chip, struct nk_colorf *bg, struct nk_colorf *fg, int *paused, char *rompath);

void
updatetimer(Chip8 *chip);

void
cleanup(void);

void
audio_callback(void* userdata, uint8_t* stream, int len);

void
handle_event(Chip8 *chip, SDL_Event event);

void
save_state(Chip8 *chip);

void
load_save(Chip8 *chip);

#endif
