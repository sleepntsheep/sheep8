#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_BOOL
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_renderer.h"
#include <SDL2/SDL.h>
#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

#define SHEEP_LOG_IMPLEMENTATION
#include "log.h"
#include "chip8.h"
#include "beeper.h"

#ifdef SDL_GetTicks64
#define SDL_GetTicksCompat SDL_GetTicks64
#else
#define SDL_GetTicksCompat SDL_GetTicks
#endif

#define LENGTH(a) (sizeof(a) / sizeof(*(a)))

#ifdef PLATFORM_WEB
EMSCRIPTEN_KEEPALIVE int wasm_load_rom(uint8_t *buf, size_t size) {
    chip8_load_rom(&global_app.chip, buf, size);
    return 1;
}
EM_JS(int, canvas_get_width, (), { return canvas.width; });
EM_JS(int, canvas_get_height, (), { return canvas.height; });
static const char *preset_roms[] = { "roms/BRIX", "roms/octopaint.ch8", "roms/TETRIS" };
#endif

static const int kb2pad[] = {
    [SDLK_1] = 1+1, [SDLK_2] = 2+1, [SDLK_3] = 3+1, [SDLK_4] = 0xC+1,
    [SDLK_q] = 4+1, [SDLK_w] = 5+1, [SDLK_e] = 6+1, [SDLK_r] = 0xD+1,
    [SDLK_a] = 7+1, [SDLK_s] = 8+1, [SDLK_d] = 9+1, [SDLK_f] = 0xE +1,
    [SDLK_z] = 0xA+1, [SDLK_x] = 0x0+1, [SDLK_c] = 0xB+1, [SDLK_v] = 0xF+1,
    /**** TO AVOID 0x0 COLLISION, EVERY VALUE HERE HAS BEEN OFFSET BY +1 */
};

static const int gui_top_px = 60;

struct app {
    bool quit;
    SDL_Window *win;
    SDL_Renderer *renderer;
    chip8 chip;
    int tab;
    bool touchscreen_keypad;
    struct nk_colorf bg, fg;
    struct nk_context *nk;
    uint64_t tick_a, tick_b;
    int w, h;
    struct beeper beeper;
#ifdef PLATFORM_WEB
    int selected_preset_rom;
#else
    char rom_path[4096];
#endif
};

enum app_tabs {
    tab_chip8_screen,
    tab_app_settings,
    tab_chip8_settings,
    tab_chip8_debug,
    tab_counts,
};

static const char *tab_names[] = {
    "Chip8 Screen",
    "Setting",
    "Chip8 Setting", 
    "Debug",
    "About",
    NULL,
};

static struct app global_app = { 0 };

void app_init(struct app *app);
void app_event(struct app *app);
void app_draw(struct app *app);
void app_draw_touchscreen_keypad(struct app *app);
void app_draw_tab_chip8_screen(struct app *app);
void app_draw_tab_settings(struct app *app);
void app_draw_tab_chip8_settings(struct app *app);

void app_init(struct app *app) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
        panic("Failed to init SDL");

    memset(app, 0, sizeof *app);
    chip8_init(&global_app.chip);
    app->win = SDL_CreateWindow("chip8 Emulator :D", 0, 0,
            800, 600, 
#ifndef PLATFORM_WEB
            SDL_WINDOW_RESIZABLE |
#endif
            SDL_WINDOW_SHOWN);
    app->renderer = SDL_CreateRenderer(app->win, -1, SDL_RENDERER_ACCELERATED);
    app->touchscreen_keypad = false;
    app->tab = tab_chip8_screen;
    app->tick_a = app->tick_b = SDL_GetTicksCompat();
    app->nk = nk_sdl_init(app->win, app->renderer);
    app->fg = (struct nk_colorf) {1.0f, 1.0f, 1.0f, 1.0f};
    app->bg = (struct nk_colorf) {0.0f, 0.0f, 0.0f, 1.0f};
    {
        struct nk_font_atlas *atlas;
        struct nk_font_config config = nk_font_config(0);
        nk_sdl_font_stash_begin(&atlas);
        struct nk_font *font = nk_font_atlas_add_default(atlas, 24, &config);
        nk_style_set_font(app->nk, &font->handle);
        nk_sdl_font_stash_end();
    }
    beeper_init(&app->beeper);
}

void chip8_keydown(chip8 *chip, int key) {
    chip->keys |= 1 << key;
}

void chip8_keyup(chip8 *chip, int key) {
    if (chip->key_waiting) {
        chip->key_waiting = false;
        chip->v[chip->register_waiting] = key;
    }
    chip->keys &= ~(1 << key);
}

void app_event(struct app *app)
{
    SDL_Event e;
    nk_input_begin(app->nk);
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                if (app->touchscreen_keypad)
                    break;
                unsigned int sym = e.key.keysym.sym;
                if (sym >= LENGTH(kb2pad))
                    break;
                int pad = kb2pad[e.key.keysym.sym] - 1;
                if (pad == -1)
                    break;
                if (e.type == SDL_KEYDOWN)
                    chip8_keydown(&app->chip, pad);
                else
                    chip8_keyup(&app->chip, pad);
                break;
                            }
            case SDL_QUIT:
                app->quit = true;
                break;
            default:
                break;
        }
        nk_sdl_handle_event(&e);
    }
    nk_input_end(app->nk);
}

#define KEYBTN(s, p) \
        do { \
            int prev = app->chip.keys >> p & 1; \
            int res = nk_button_label(app->nk, s); \
            if (prev == res) break; \
            if (res) chip8_keydown(&app->chip, p); \
            else chip8_keyup(&app->chip, p); \
        } while (0)
void app_draw_touchscreen_keypad(struct app *app) {
    nk_button_set_behavior(app->nk, NK_BUTTON_REPEATER);
    if (nk_begin(app->nk, "Keypad", nk_rect(app->w / 2.0f, gui_top_px, app->w / 2.0f, app->h - gui_top_px), 0)) {
        nk_layout_row_dynamic(app->nk, app->h / 4.0f, 4);
        KEYBTN("1", 1);
        KEYBTN("2", 2);
        KEYBTN("3", 3);
        KEYBTN("4", 0xC);
        nk_layout_row_dynamic(app->nk, app->h / 4.0f, 4);
        KEYBTN("Q", 4);
        KEYBTN("W", 5);
        KEYBTN("E", 6);
        KEYBTN("R", 0xD);
        nk_layout_row_dynamic(app->nk, app->h / 4.0f, 4);
        KEYBTN("A", 7);
        KEYBTN("S", 8);
        KEYBTN("D", 9);
        KEYBTN("F", 0xE);
        nk_layout_row_dynamic(app->nk, app->h / 4.0f, 4);
        KEYBTN("Z", 0xA);
        KEYBTN("X", 0);
        KEYBTN("C", 0xB);
        KEYBTN("V", 0xF);
        nk_end(app->nk);
    }
    nk_button_set_behavior(app->nk, NK_BUTTON_DEFAULT);
}

void app_draw_tab_settings(struct app *app) {
    if (nk_begin(app->nk, "Settings", nk_rect(0, gui_top_px, app->w, 300), 0)) {
        nk_layout_row_dynamic(app->nk, 150, 2);
        app->fg = nk_color_picker(app->nk, app->fg, NK_RGBA);
        app->bg = nk_color_picker(app->nk, app->bg, NK_RGBA);
#ifndef PLATFORM_WEB
        nk_layout_row_dynamic(app->nk, 40, 2);
        nk_edit_string_zero_terminated(
            app->nk, 
            NK_EDIT_BOX | NK_EDIT_AUTO_SELECT,
            app->rom_path, sizeof app->rom_path, NULL
        );
        if (nk_button_label(app->nk, "Load Rom")) {
            chip8_load_rom_from_file(&app->chip, app->rom_path);
            app->tab = tab_chip8_screen;
        }
#else
        nk_layout_row_dynamic(app->nk, 40, 1);
        if (nk_button_label(app->nk, "Upload Rom")) {
            EM_ASM(
                var file_selector = document.createElement('input');
                file_selector.setAttribute('type', 'file');
                file_selector.setAttribute('onchange','open_file(event)');
                file_selector.click();
            );
            app->tab = tab_chip8_screen;
        }
        nk_layout_row_dynamic(app->nk, 40, 2);
        nk_combobox(app->nk, preset_roms, LENGTH(preset_roms),
                &app->selected_preset_rom, 20, (struct nk_vec2){150, 150});
        if (nk_button_label(app->nk, "Load selected preset")) {
            chip8_load_rom_from_file(&app->chip, preset_roms[app->selected_preset_rom]);
            app->tab = tab_chip8_screen;
        }
#endif
        nk_layout_row_dynamic(app->nk, 40, 1);
        nk_checkbox_label(app->nk, "Touchscreen Keypad", &app->touchscreen_keypad);
    }
    nk_end(app->nk);
}

void app_draw_tab_chip8_screen(struct app *app) {
    int scalewidth = app->w / WIDTH;
    int scaleheight = (app->h - gui_top_px) / HEIGHT;
    if (scaleheight < 0) scaleheight = 0;
    if (app->touchscreen_keypad) scalewidth /= 2;
    SDL_SetRenderDrawColor(app->renderer, app->fg.r * 255, app->fg.g * 255, app->fg.b * 255, 255);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (!app->chip.screen[y][x]) continue;
            const SDL_Rect bound = {
                .x = x * scalewidth,
                .y = y * scaleheight + gui_top_px,
                .w = scalewidth,
                .h = scaleheight
            };
            SDL_RenderFillRect(app->renderer, &bound);
        }
    }
}

void app_draw_tab_chip8_settings(struct app *app) {
    if (nk_begin(app->nk, "Chip8 Settings", nk_rect(0, gui_top_px, app->w, 300), 0)) {
        nk_layout_row_dynamic(app->nk, 40, 2);
        nk_label(app->nk, "Clock Speed(Hz)", NK_TEXT_LEFT);
        nk_slider_int(app->nk, 0, &app->chip.clockspeed, 2000, 20);
        nk_label(app->nk, "Advanced Settings", NK_TEXT_LEFT);
        nk_layout_row_dynamic(app->nk, 40, 1);
        nk_checkbox_label(app->nk, "8xy1 8xy2 8xy3 Reset VF", &app->chip.settings.op_8xy1_2_3_reset_vf);
        nk_layout_row_dynamic(app->nk, 40, 1);
        nk_checkbox_label(app->nk, "fx55 fx65 increment I", &app->chip.settings.op_fx55_fx65_increment);
        nk_layout_row_dynamic(app->nk, 40, 1);
        nk_checkbox_label(app->nk, "8xy6 8xye to Vx = Vy", &app->chip.settings.op_8xy6_8xye_do_vy);
    }
    nk_end(app->nk);
}

void app_draw(struct app *app) {
    SDL_SetRenderDrawColor(app->renderer, app->bg.r * 255, app->bg.g * 255, app->bg.b * 255, 255);
    SDL_RenderClear(app->renderer);

#ifndef PLATFORM_WEB
    SDL_GetWindowSize(app->win, &app->w, &app->h);
#else
    app->w = canvas_get_width();
    app->h = canvas_get_height();
    SDL_SetWindowSize(app->win, app->w, app->h);
#endif

    if (nk_begin(app->nk, "Tabs", nk_rect(0, 0, app->w, gui_top_px), NK_WINDOW_NO_SCROLLBAR)) {
        nk_layout_row_dynamic(app->nk, gui_top_px, tab_counts);
        for (int i = 0; i < tab_counts; i++)
            if (app->tab == i)
                nk_button_label(app->nk, "");
            else if (nk_button_label(app->nk, tab_names[i]))
                app->tab = i;
    }
    nk_end(app->nk);

    switch (app->tab) {
        case tab_chip8_screen:
            app_draw_tab_chip8_screen(app);
            break;
        case tab_app_settings:
            app_draw_tab_settings(app);
            break;
        case tab_chip8_settings:
            app_draw_tab_chip8_settings(app);
            break;
    }

    if (app->touchscreen_keypad)
        app_draw_touchscreen_keypad(app);

    nk_sdl_render(NK_ANTI_ALIASING_ON);
    SDL_RenderPresent(app->renderer);
}

void app_run(struct app *app) {
    app->tick_b = SDL_GetTicksCompat();
    if ((double)app->tick_b - app->tick_a < 1000.0f / 60)
        return;
    app->tick_a = SDL_GetTicksCompat();

    app_event(app);
    if (app->tab == tab_chip8_screen) {
        chip8_interpret(&app->chip);
        chip8_update_timer(&app->chip);
    }

    if (app->chip.soundtimer > 0)
        beeper_play(&app->beeper);
    else
        beeper_pause(&app->beeper);

    app_draw(app);
}

void app_cleanup(struct app *app) {
    beeper_clean(&app->beeper);
}

void app_main_loop() {
    while (!global_app.quit)
        app_run(&global_app);
}


int main(void) {
    app_init(&global_app);

#ifdef PLATFORM_WEB
    emscripten_set_main_loop(app_main_loop, 0, 1);
#else
    while (!global_app.quit) app_main_loop();
#endif

    app_cleanup(&global_app);

    /* unreachable */
    return EXIT_SUCCESS;
}

