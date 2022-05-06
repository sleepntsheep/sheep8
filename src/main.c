#include "main.h"
#include "chip8.h"

#define green (nk_rgb(0,255,0))

#if defined _WIN32 && !defined __CYGWIN__
#define SDL_MAIN_HANDLED
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

SDL_AudioSpec audiowant, audiohave;
uint64_t audio_played = 0;
SDL_AudioDeviceID audio_id;

int
main(int argc, char *argv[])
{
    SDL_Window *win = NULL;
    SDL_Renderer *renderer = NULL;
    Chip8 *chip;
    uint64_t next_game_step = SDL_GetTicks64();
    char path[PATH_MAX] = { 0 };

    int running = 1;
    int paused = 0;

    struct nk_context *ctx;
    struct nk_colorf bg, fg;
    
    bg.r = 61 / 255.0f, bg.g = 38 / 255.0f, bg.b = 66 / 255.0f, bg.a = 1.0f;
    fg.r = 1.0f, fg.g = 1.0f, fg.b = 1.0f, fg.a = 1.0f;

    atexit(cleanup);

    if (argc < 2) {
	    init("Chip8 Emulator: Select Rom", &win, &renderer, &ctx, &chip);
    }
    else if (argc > 5) {
        strncpy(path, argv[1], PATH_MAX-1);
	    init(argv[1], &win, &renderer, &ctx, &chip);
	    loadrom(chip, argv[1]);
    }

    while (running)
    {
        uint64_t now = SDL_GetTicks64();
        /* Input */
        SDL_Event evt;
        nk_input_begin(ctx);
        while (SDL_PollEvent(&evt)) {
            handle_event(chip, evt);
        }
        nk_input_end(ctx);

        if (next_game_step <= now) {
            if (!paused) {
                int i;
                for (i = 0; i < chip->clockspeed / FPS; i++) {
                    if (!paused) {
                        interpretOP(chip, getop(chip));
                    }
                }
                updatetimer(chip);
            }

            gui_display(ctx, chip, &bg, &fg, &paused, path);

            SDL_SetRenderDrawColor(renderer, bg.r  *255, bg.g  *255, bg.b  *255, bg.a  *255);
            SDL_RenderClear(renderer);

            nk_sdl_render(NK_ANTI_ALIASING_ON);

            chip_display(chip, renderer, fg.r  *255, fg.g  *255, fg.b  *255, fg.a  *255);

            SDL_RenderPresent(renderer);

            next_game_step += 1000/FPS;
        }
        else {
            SDL_Delay(next_game_step - now);
        }
    }

    return 0;
}

void
init(char *title, SDL_Window **window, SDL_Renderer **renderer, struct nk_context **ctx, Chip8 **chip)
{
    *chip = chip_init();

    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);

    *window = SDL_CreateWindow(title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SWIDTH+GWIDTH, SHEIGHT+GHEIGHT, SDL_WINDOW_SHOWN|SDL_WINDOW_ALLOW_HIGHDPI);

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    *ctx = nk_sdl_init(*window, *renderer);

    if (!window)
        die(SDL_GetError());

    if (!renderer)
        die(SDL_GetError());

    /* nk font init */
    {
        /* for high-dpi */
        float font_scale = 1;
        {
            int render_w, render_h;
            int window_w, window_h;
            float scale_x, scale_y;
            SDL_GetRendererOutputSize(*renderer, &render_w, &render_h);
            SDL_GetWindowSize(*window, &window_w, &window_h);
            scale_x = (float)(render_w) / (float)(window_w);
            scale_y = (float)(render_h) / (float)(window_h);
            SDL_RenderSetScale(*renderer, scale_x, scale_y);
            font_scale = scale_y;
        }

        /* load font */
        {
            struct nk_font_atlas *atlas;
            struct nk_font_config config = nk_font_config(0);
            struct nk_font *font;

            nk_sdl_font_stash_begin(&atlas);
            font = nk_font_atlas_add_default(atlas, 16 *font_scale, &config);
            /*font = nk_font_atlas_add_from_file(atlas, "assets/terminus.ttf", 26 *font_scale, &config);*/
            nk_sdl_font_stash_end();

            font->handle.height /= font_scale;
            nk_style_load_all_cursors(*ctx, atlas->cursors);
            nk_style_set_font(*ctx, &font->handle);
        }
    }


    /* audio init */
    {
        audiowant.freq = 44100;
        audiowant.format = AUDIO_F32;
        audiowant.channels = 2;
        audiowant.samples = 512;
        audiowant.callback = audio_callback;
        audiowant.userdata = (void*)&audio_played;

        audio_id = SDL_OpenAudioDevice(
            NULL, 0, &audiowant, &audiohave, SDL_AUDIO_ALLOW_ANY_CHANGE);

        if (!audio_id)
            die(SDL_GetError());

        SDL_PauseAudioDevice(audio_id, 0);
    }
}

void
gui_display(struct nk_context *ctx, Chip8 *chip, struct nk_colorf *bg, struct nk_colorf *fg, int *paused, char *rompath)
{
    int nk_flags = 0;

    nk_flags |= NK_WINDOW_BORDER;
    nk_flags |= NK_WINDOW_TITLE;
#if 1
    nk_flags |= NK_WINDOW_MOVABLE;
    nk_flags |= NK_WINDOW_SCALABLE;
#endif

    if (nk_begin(ctx, "Settings", nk_rect(0, 0, 560, GHEIGHT), nk_flags))
    {
        nk_layout_row_dynamic(ctx, 25, 2);
        nk_label(ctx, "background:", NK_TEXT_LEFT);
        if (nk_combo_begin_color(ctx, nk_rgb_cf(*bg), nk_vec2(nk_widget_width(ctx), 200))) {
            nk_layout_row_dynamic(ctx, 120, 1);
            *bg = nk_color_picker(ctx, *bg, NK_RGBA);
            nk_layout_row_dynamic(ctx, 25, 1);
            bg->r = nk_propertyf(ctx, "#R:", 0, bg->r, 1.0f, 0.01f,0.005f);
            bg->g = nk_propertyf(ctx, "#G:", 0, bg->g, 1.0f, 0.01f,0.005f);
            bg->b = nk_propertyf(ctx, "#B:", 0, bg->b, 1.0f, 0.01f,0.005f);
            bg->a = nk_propertyf(ctx, "#A:", 0, bg->a, 1.0f, 0.01f,0.005f);
            nk_combo_end(ctx);
        }

        nk_layout_row_dynamic(ctx, 25, 2);
        nk_label(ctx, "foreground:", NK_TEXT_LEFT);
        if (nk_combo_begin_color(ctx, nk_rgb_cf(*fg), nk_vec2(nk_widget_width(ctx), 200))) {
            nk_layout_row_dynamic(ctx, 120, 1);
            *fg = nk_color_picker(ctx, *fg, NK_RGBA);
            nk_layout_row_dynamic(ctx, 25, 1);
            fg->r = nk_propertyf(ctx, "#R:", 0, fg->r, 1.0f, 0.01f,0.005f);
            fg->g = nk_propertyf(ctx, "#G:", 0, fg->g, 1.0f, 0.01f,0.005f);
            fg->b = nk_propertyf(ctx, "#B:", 0, fg->b, 1.0f, 0.01f,0.005f);
            fg->a = nk_propertyf(ctx, "#A:", 0, fg->a, 1.0f, 0.01f,0.005f);
            nk_combo_end(ctx);
        }

        nk_layout_row_dynamic(ctx, 25, 1);
        nk_property_int(ctx, "Clock (Hz):", 0, &chip->clockspeed, 2000, 50, 1);

        nk_layout_row_dynamic(ctx, 30, 2);
        nk_label(ctx, "Rom path", NK_TEXT_LEFT);
        nk_edit_string_zero_terminated(ctx,
            NK_EDIT_BOX | NK_EDIT_AUTO_SELECT,
            rompath, PATH_MAX, nk_filter_ascii);

        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_button_label(ctx, "Load rom")) {
            memset(chip, 0, sizeof(Chip8));
            chip->pc = 0x200;
            chip->clockspeed = DEFAULT_CLOCK;
            loadfont(chip);
            loadrom(chip, rompath);
        }

        nk_end(ctx);
    }

    if (nk_begin(ctx, "Control", nk_rect(560, 0, 200, GHEIGHT), nk_flags)) {

        /* toggle paused */
        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_button_label(ctx, *paused ? "Resume" : "Pause"))
            *paused ^= 1;

        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_button_label(ctx, "Save"))
            save_state(chip);

        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_button_label(ctx, "Load"))
            load_save(chip);

        nk_layout_row_dynamic(ctx, 25, 1);
        /* interpret the next opcode */
        if (nk_button_label(ctx, "Next"))
            interpretOP(chip, getop(chip));

        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_button_label(ctx, "Quit"))
            exit(0);

        nk_end(ctx);
    }

    if (nk_begin(ctx, "Keypad", nk_rect(SWIDTH - 100, 0, 100, GHEIGHT), nk_flags)) {
        char buf[100];

        sprintf(buf, "%d %d %d %d", chip->state[0x1], chip->state[0x2], chip->state[0x3], chip->state[0xC]);
        nk_layout_row_dynamic(ctx, 15, 1);
        nk_label(ctx, buf, NK_TEXT_LEFT);
        sprintf(buf, "%d %d %d %d", chip->state[0x4], chip->state[0x5], chip->state[0x6], chip->state[0xD]);
        nk_layout_row_dynamic(ctx, 15, 1);
        nk_label(ctx, buf, NK_TEXT_LEFT);
        sprintf(buf, "%d %d %d %d", chip->state[0x7], chip->state[0x8], chip->state[0x9], chip->state[0xE]);
        nk_layout_row_dynamic(ctx, 15, 1);
        nk_label(ctx, buf, NK_TEXT_LEFT);
        sprintf(buf, "%d %d %d %d", chip->state[0xA], chip->state[0x0], chip->state[0xB], chip->state[0xF]);
        nk_layout_row_dynamic(ctx, 15, 1);
        nk_label(ctx, buf, NK_TEXT_LEFT);
        nk_end(ctx);
    }

    if (nk_begin(ctx, "CPU", nk_rect(SWIDTH - 200, 0, 100, GHEIGHT), nk_flags)) {
        char buf[100];

        /* program counter */
        sprintf(buf, "PC %X", chip->pc);
        nk_layout_row_dynamic(ctx, 15, 1);
        nk_label(ctx, buf, NK_TEXT_LEFT);

        /* index */
        sprintf(buf, "ID %X", chip->idx);
        nk_layout_row_dynamic(ctx, 15, 1);
        nk_label(ctx, buf, NK_TEXT_LEFT);

        /* stack pointer */
        sprintf(buf, "SP %X", chip->sp);
        nk_layout_row_dynamic(ctx, 15, 1);
        nk_label(ctx, buf, NK_TEXT_LEFT);

        /* op at pc */
        sprintf(buf, "OP %4x", getop(chip));
        nk_layout_row_dynamic(ctx, 15, 1);
        nk_label(ctx, buf, NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 15, 1);
        sprintf(buf, "%2d", chip->delayTimer);
        nk_label(ctx, "Delay", NK_TEXT_LEFT);
        nk_label(ctx, buf, NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 15, 1);
        sprintf(buf, "%2d", chip->soundTimer);
        nk_label(ctx, "Sound", NK_TEXT_LEFT);
        nk_label(ctx, buf, NK_TEXT_LEFT);

        nk_end(ctx);
    }

    if (nk_begin(ctx, "CPU - Register", nk_rect(SWIDTH + 150, 0, 150, SHEIGHT + GHEIGHT), nk_flags)) {
        char buf[100];

        int i;
        for (i = 0; i < 16; i++) {
            /* nuklear nk_property_int accept int *val as parameter
            out chip->v is uint8_t array
            so we can't pass that directly
            */
            int temp = chip->v[i];
            nk_layout_row_dynamic(ctx, 15, 1);
            sprintf(buf, "%2d", i);
            nk_property_int(ctx, buf, 0, &temp, 255, 1, 1);
            chip->v[i] = (uint8_t) temp;
        }

        nk_end(ctx);
    }

    if (nk_begin(ctx, "Stack", nk_rect(SWIDTH, 0, 150, SHEIGHT + GHEIGHT), nk_flags)) {
        char buf[100];

        int i;
        for (i = 0; i < (int)(sizeof chip->stack / sizeof *chip->stack); i++) {
            sprintf(buf, "%2X %4X", i, chip->stack[i]);
            nk_layout_row_dynamic(ctx, 15, 1);
            if (chip->stack[i])
                nk_label_colored(ctx, buf, NK_TEXT_LEFT, nk_rgb(0, 255, 0));
            else
                nk_label(ctx, buf, NK_TEXT_LEFT);
        }
        nk_end(ctx);
    }

}

void
chip_display(Chip8 *chip, SDL_Renderer *renderer, int r, int g, int b, int a)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    {int i, j;
    for (i = 0; i < HEIGHT; i++) {
        for (j = 0; j < WIDTH; j++) {
            if (chip->screen[i][j]) {
                SDL_Rect rect;
                rect.x = j * SCALE;
                rect.y = i * SCALE + GHEIGHT;
                rect.w = SCALE;
                rect.h = SCALE;
                SDL_SetRenderDrawColor(renderer, r, g, b, a);
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }}
}

void
save_state(Chip8 *chip) {
    FILE *fp = fopen(SAVEFILE, "wb");
    if (!fp) die("Can't open savefile");

    fwrite(chip, sizeof(Chip8), 1, fp);
    fclose(fp);
}

void
load_save(Chip8 *chip) {
    FILE *fp = fopen(SAVEFILE, "rb");
    if (!fp) die("Can't open savefile");

    fread(chip, sizeof(Chip8), 1, fp);
    fclose(fp);
}

void
updatetimer(Chip8 *chip) {
    if (chip->delayTimer > 0)
        chip->delayTimer--;
    if (chip->soundTimer > 0)
        chip->soundTimer--;
    SDL_PauseAudioDevice(audio_id, !chip->soundTimer);
}

void
die(const char *s)
{
    perror(s);
    exit(-1);
}

void
cleanup(void)
{
    nk_sdl_shutdown();
    SDL_Quit();
    exit(0);
}

void
handle_event(Chip8 *chip, SDL_Event event) {
    if (event.type == SDL_QUIT) exit(0);
    chip_handleevent(chip, event);
    nk_sdl_handle_event(&event);
}

void
audio_callback(void *userdata, uint8_t *stream, int len) {
    uint64_t *samples_played = (uint64_t*)userdata;
    float *fstream = (float*)(stream);

    static const float volume = 0.2;
    static const float frequency = 441.0;

    int sid;
    for (sid = 0; sid < (len / 8); ++sid) {
        double time = (*samples_played + sid) / 44100.0;
        double x = 2.0 * 3.14159 * time * frequency;
        fstream[2 * sid + 0] = volume * sin(x);
        fstream[2 * sid + 1] = volume * sin(x);
    }

    *samples_played += len / 8;
}
