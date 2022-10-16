#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#define SDL_DISABLE_IMMINTRIN_H
#define NK_BUTTON_TRIGGER_ON_RELEASE
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

#include "tinyfiledialogs.h"
#define SHEEP_LOG_IMPLEMENTATION
#include "log.h"
#include "chip8.h"
#include "beeper.h"
#include "input.h"

#ifdef SDL_GetTicks64
#define SDL_GetTicksCompat SDL_GetTicks64
#else
#define SDL_GetTicksCompat SDL_GetTicks
#endif

#ifdef PLATFORM_WEB
EM_JS(int, canvas_get_width, (), { return canvas.width; });
EM_JS(int, canvas_get_height, (), { return canvas.height; });
static const char *preset_roms[] = { "roms/BRIX", "roms/octopaint.ch8", "roms/TETRIS" };
#endif

static const int gui_top_px = 60;

struct app {
    bool quit;
    SDL_Window *win;
    SDL_Renderer *renderer;
    chip8 chip;
    int tab;
    bool touchscreen_keypad;
    bool debug_window;
    struct nk_colorf bg, fg;
    struct nk_context *nk;
    uint64_t tick_a, tick_b;
    int w, h;
    beeper_t beeper;
#ifdef PLATFORM_WEB
    int selected_preset_rom;
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
#ifdef PLATFORM_WEB
EMSCRIPTEN_KEEPALIVE int wasm_load_rom(uint8_t *buf, size_t size) {
    chip8_load_rom(&global_app.chip, buf, size);
    return 1;
}
#endif

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
    chip8_init(&app->chip);
    app->win = SDL_CreateWindow("chip8 Emulator :D", 0, 0, 800, 600, SDL_WINDOW_SHOWN
#ifndef PLATFORM_WEB
            | SDL_WINDOW_RESIZABLE
#endif
            );
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



void app_event(struct app *app)
{
    SDL_Event e;
    nk_input_begin(app->nk);
    while (SDL_PollEvent(&e)) {
        chip8_input_handle(&app->chip, e);
        switch (e.type) {
            case SDL_QUIT:
                app->quit = true;
#ifdef PLATFORM_WEB
                exit(EXIT_SUCCESS);
#endif
                break;
            default:
                break;
        }
        nk_sdl_handle_event(&e);
    }
    nk_input_end(app->nk);
}

void app_draw_touchscreen_keypad(struct app *app) {
    nk_button_set_behavior(app->nk, NK_BUTTON_REPEATER);
    
    if (nk_begin(app->nk, "Keypad", nk_rect(app->w / 2.0f, gui_top_px, app->w / 2.0f, app->h - gui_top_px), 0)) {
        for (int i = 0; i < sizeof keypad / sizeof *keypad; i+=4) {
            nk_layout_row_dynamic(app->nk, (app->h - gui_top_px) / 4.0f, 4);
            for (int j = 0; j < 4; j++) {
                int prev = app->chip.keys >> keypad[i+j].i & 1;
                int res = nk_button_label(app->nk, keypad[i+j].t);
                if (prev == res) continue;
                if (res) chip8_keydown(&app->chip, keypad[i+j].i);
                else chip8_keyup(&app->chip, keypad[i+j].i);
            }
        }
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
        if (nk_button_label(app->nk, "Load Rom")) {
            chip8_load_rom_from_file(&app->chip, 
                    tinyfd_openFileDialog("Select rom file", "", 1, (const char *[]){ "*" },
                        "binary file (chip8 rom)", false));
            app->tab = tab_chip8_screen;
        }

        nk_layout_row_dynamic(app->nk, 40, 2);
        if (nk_button_label(app->nk, "Save state")) {
            chip8_save_to_file(&app->chip, tinyfd_saveFileDialog(
                    "Select where to save", "state", 1, (const char *[]){ "*" }, "binary file"));
        }
        if (nk_button_label(app->nk, "Load state")) {
            chip8_restore_from_file(&app->chip, 
                    tinyfd_openFileDialog("Select saved file", "", 1, (const char *[]){ "*" },
                                          "binary file", false));
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
        nk_combobox(app->nk, preset_roms, sizeof preset_roms / sizeof *preset_roms,
                &app->selected_preset_rom, 20, (struct nk_vec2){150, 150});
        if (nk_button_label(app->nk, "Load selected preset")) {
            chip8_load_rom_from_file(&app->chip, preset_roms[app->selected_preset_rom]);
            app->tab = tab_chip8_screen;
        }
#endif
        nk_layout_row_dynamic(app->nk, 40, 2);
        nk_checkbox_label(app->nk, "Touchscreen Keypad", &app->touchscreen_keypad);
        nk_checkbox_label(app->nk, "Debug window", &app->debug_window);
    }
    nk_end(app->nk);
}

void app_draw_tab_chip8_screen(struct app *app) {
    int scalewidth = app->w / WIDTH;
    int scaleheight = (app->h - gui_top_px) / HEIGHT;
    if (scaleheight < 0) scaleheight = 0;
    if (app->touchscreen_keypad || app->debug_window) scalewidth /= 2;
    SDL_SetRenderDrawColor(app->renderer, app->fg.r * 255, app->fg.g * 255, app->fg.b * 255, 255);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (!app->chip.screen[y][x]) continue;
            SDL_RenderFillRect(app->renderer, &((SDL_Rect){
                .x = x * scalewidth,
                .y = y * scaleheight + gui_top_px,
                .w = scalewidth,
                .h = scaleheight
            }));
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
        nk_layout_row_dynamic(app->nk, 40, 1);
        nk_checkbox_label(app->nk, "Wrap screen", &app->chip.settings.screen_wrap_around);
    }
    nk_end(app->nk);
}

/* maybe this could be seperate SDL window?? 
 * or half-half layout like touchscreen keypad*/
void app_draw_debug_window(struct app *app) {
    char buf[2048] = { 0 };
    if (nk_begin(app->nk, "Debugger", nk_rect(app->w / 2.0f, gui_top_px,
                    app->w / 2.0f, app->h - gui_top_px), 0)) {
    /*if (nk_begin(app->nk, "Debugger", nk_rect(0, gui_top_px, 400, 300),
                NK_WINDOW_MOVABLE | NK_WINDOW_TITLE | NK_WINDOW_SCALABLE)) {
                */
        nk_layout_row_dynamic(app->nk, 200, 2);
        
        if (nk_group_begin(app->nk, "zefasofj", NK_WINDOW_BORDER)) {
            for (int i = 0; i < 4; i++) {
                nk_layout_row_dynamic(app->nk, 20, 1);
                snprintf(buf, sizeof buf, "%d %d %d %d",
                        chip8_keyisdown(&app->chip, keypad[i * 4].i),
                        chip8_keyisdown(&app->chip, keypad[i * 4+1].i),
                        chip8_keyisdown(&app->chip, keypad[i * 4+2].i),
                        chip8_keyisdown(&app->chip, keypad[i * 4+3].i));
                nk_label(app->nk, buf, NK_TEXT_LEFT);
            }
            nk_group_end(app->nk);
        }

        if (nk_group_begin(app->nk, "zefasofj2", NK_WINDOW_BORDER)) {
            nk_layout_row_dynamic(app->nk, 20, 2);
            snprintf(buf, sizeof buf, "ST: %d", app->chip.soundtimer);
            nk_label(app->nk, buf, NK_TEXT_LEFT);
            snprintf(buf, sizeof buf, "DT: %d", app->chip.delaytimer);
            nk_label(app->nk, buf, NK_TEXT_LEFT);
            nk_layout_row_dynamic(app->nk, 40, 2);
            snprintf(buf, sizeof buf, "PC: %d", app->chip.pc);
            nk_label(app->nk, buf, NK_TEXT_LEFT);
            snprintf(buf, sizeof buf, "I: %d", app->chip.i);
            nk_label(app->nk, buf, NK_TEXT_RIGHT);
            nk_group_end(app->nk);
        }
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
    if (app->debug_window)
        app_draw_debug_window(app);

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

#ifdef PLATFORM_WEB
void app_main_loop() {
    app_run(&global_app);
}
#endif

int main(void) {

    app_init(&global_app);
#ifdef PLATFORM_WEB
    emscripten_set_main_loop(app_main_loop, 0, 1);
#else
    while (!global_app.quit) app_run(&global_app);
    beeper_clean(&global_app.beeper);
#endif

    return EXIT_SUCCESS;
}

