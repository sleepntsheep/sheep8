#define SDL_DISABLE_IMMINTRIN_H
#include "input.h"

static const int kb2pad[] = {
    [SDLK_1] = 1+1, [SDLK_2] = 2+1, [SDLK_3] = 3+1, [SDLK_4] = 0xC+1,
    [SDLK_q] = 4+1, [SDLK_w] = 5+1, [SDLK_e] = 6+1, [SDLK_r] = 0xD+1,
    [SDLK_a] = 7+1, [SDLK_s] = 8+1, [SDLK_d] = 9+1, [SDLK_f] = 0xE +1,
    [SDLK_z] = 0xA+1, [SDLK_x] = 0x0+1, [SDLK_c] = 0xB+1, [SDLK_v] = 0xF+1,
    /**** TO AVOID 0x0 COLLISION, EVERY VALUE HERE HAS BEEN OFFSET BY +1 */
};

void chip8_input_handle(chip8 *chip, SDL_Event e) {
    switch (e.type) {
        case SDL_KEYDOWN: /* FALLTHROUGH */
        case SDL_KEYUP:
            if (e.key.keysym.sym >= sizeof kb2pad / sizeof *kb2pad)
                return;
            int pad = kb2pad[e.key.keysym.sym] - 1;
            if (pad == -1)
                return;
            if (e.type == SDL_KEYDOWN)
                chip8_keydown(chip, pad);
            else
                chip8_keyup(chip, pad);
    }
}

