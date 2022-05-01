#include "main.h"
#include "chip8.h"
#include "widget/button.h"
#include <SDL2/SDL_video.h>

int SPEED = 5;
int paused = 0;

const int time_step_ms = 1000 / 60; /* fps */

int running = 1;
SDL_Window *window;
SDL_Renderer *renderer;
TTF_Font *font;

SDL_AudioSpec want, have;
uint64_t samples_played = 0;
SDL_AudioDeviceID audio_device_id;

/* array of last op osd */
uint16_t lastop[lastOPcount];

Button *pausebtn;
Button *savebtn;
Button *loadbtn;
Button *quitbtn;
Button *spdupbtn;
Button *spddownbtn;

const SDL_Color gray = { 100, 100, 100, 255 };
const SDL_Color white = { 255, 255, 255, 255 };
const SDL_Color tsp = { 0, 0, 0, 0 };

int
main(int argc, char** argv) {
    Chip8 *chip = chip_init();
    uint64_t next_game_step = SDL_GetTicks();
    SDL_Event event;

    if (argc == 1)
        die("Supply rom path to open in arg");
    else if (argc > 2)
        load_save(chip);
    else
        loadrom(chip, argv[1]);

    init(argv[1]);

    while (running) {
        uint64_t now = SDL_GetTicks();
        while (SDL_PollEvent( &event ))
            handle_event(chip, event);

        if (next_game_step <= now) {
            int i;
            for (i = 0; i < SPEED; i++) {
                if (!paused) {
                    uint16_t opcode = getop(chip);
                    interpretOP(chip, opcode);

                    {int i; 
                        for (i = lastOPcount - 2; i >= 0; i--)
                            lastop[i] = lastop[i+1];
                    }
                    lastop[lastOPcount - 1] = opcode;
                }
            }
            display(chip);
            next_game_step += time_step_ms;
            if (!paused)
                updatetimer(chip);
        }
        else {
            SDL_Delay(next_game_step - now);
        }
    }

    free(chip);
    quit();

    return 0;
}

void
quit(void) {
    TTF_Quit();
    SDL_CloseAudio();
    SDL_Quit();
    exit(0);
}

void
die(const char *s) {
    perror(s);
    exit(1);
}

int
init(char* title) {
    char buffer[300];
    int window_flags = 0, renderer_flags = 0;

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        die(SDL_GetError());

    if (!TTF_WasInit() && TTF_Init() == -1)
        die(TTF_GetError());

    sprintf(buffer, "Chip8 emulator: %s", title);

    window_flags = SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_BORDERLESS;
    window = SDL_CreateWindow(buffer, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SWIDTH + 200, SHEIGHT + 100, window_flags);

    if (!window)
        die(SDL_GetError());

    renderer = SDL_CreateRenderer(window, -1, renderer_flags);

    if (!renderer)
        die(SDL_GetError());

    font = TTF_OpenFont(FONT_PATH, 20);
    if (!font)
        die(TTF_GetError());

    SDL_memset(&want, 0, sizeof want);

    want.freq = 44100;
    want.format = AUDIO_F32;
    want.channels = 2;
    want.samples = 512;
    want.callback = audio_callback;
    want.userdata = (void*)&samples_played;

    audio_device_id = SDL_OpenAudioDevice(
        NULL, 0, &want, &have, SDL_AUDIO_ALLOW_ANY_CHANGE);

    if (!audio_device_id)
        die(SDL_GetError());

    SDL_PauseAudioDevice(audio_device_id, 0);

    pausebtn = btn_init(10, SHEIGHT+10, 100, 40, gray, white, "Pause", NULL);
    savebtn = btn_init(120, SHEIGHT+10, 100, 40, gray, white, "Save", save_state);
    loadbtn = btn_init(230, SHEIGHT+10, 100, 40, gray, white, "Load", load_save);
    quitbtn = btn_init(340, SHEIGHT+10, 100, 40, gray, white, "Exit", quit);
    spddownbtn = btn_init(450, SHEIGHT+10, 100, 40, gray, white, "Speed--", NULL);
    spdupbtn = btn_init(560, SHEIGHT+10, 100, 40, gray, white, "Speed++", NULL);

    return 0;
}

void
audio_callback(void* userdata, uint8_t* stream, int len) {
    uint64_t* samples_played = (uint64_t*) userdata;
    float* fstream = (float*)(stream);

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

void
display(Chip8 *chip) {
    char buffer[16]; /* for osd */

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    {int i, j;
    for (i = 0; i < HEIGHT; i++) {
        for (j = 0; j < WIDTH; j++) {
            if (chip->screen[i][j]) {
                SDL_Rect rect;
                rect.x = j * SCALE;
                rect.y = i * SCALE;
                rect.w = SCALE;
                rect.h = SCALE;
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }}
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderDrawLine(renderer, 0, SHEIGHT, SWIDTH, SHEIGHT);
    SDL_RenderDrawLine(renderer, SWIDTH, 0, SWIDTH, SHEIGHT);

    /* display n last op code */
    draw_text(renderer, "Operations", SWIDTH + 10, 10, 0, font, white);
    {int i;
    for (i = 0; i < lastOPcount; i++) {
        sprintf(buffer, "%x", lastop[i]);
        draw_text(renderer, buffer, SWIDTH + 10, i * 20 + 40 , 0, font, white);
    }}

    /* osd keystate */
    draw_text(renderer, "KeyStates", SWIDTH + 10, 250, 0, font, white);
    sprintf(buffer, "%d %d %d %d", chip->state[1], chip->state[2], chip->state[3], chip->state[0xC]);
    draw_text(renderer, buffer, SWIDTH + 10, 280, 0, font, white);
    sprintf(buffer, "%d %d %d %d", chip->state[4], chip->state[5], chip->state[6], chip->state[0xD]);
    draw_text(renderer, buffer, SWIDTH + 10, 300, 0, font, white);
    sprintf(buffer, "%d %d %d %d", chip->state[7], chip->state[8], chip->state[9], chip->state[0xE]);
    draw_text(renderer, buffer, SWIDTH + 10, 320, 0, font, white);
    sprintf(buffer, "%d %d %d %d", chip->state[0xA], chip->state[0], chip->state[0xB], chip->state[0xF]);
    draw_text(renderer, buffer, SWIDTH + 10, 340, 0, font, white);

    /* osd info */
    sprintf(buffer, "Index: %x", chip->idx);
    draw_text(renderer, buffer, SWIDTH + 10, 370, 0, font, white);
    sprintf(buffer, "PC: %x", chip->pc);
    draw_text(renderer, buffer, SWIDTH + 10, 390, 0, font, white);
    sprintf(buffer, "Delay timer: %x", chip->delayTimer);
    draw_text(renderer, buffer, SWIDTH + 10, 410, 0, font, white);
    sprintf(buffer, "Sound timer: %x", chip->soundTimer);
    draw_text(renderer, buffer, SWIDTH + 10, 430, 0, font, white);
    sprintf(buffer, "Speed: %d", SPEED);
    draw_text(renderer, buffer, SWIDTH + 10, 450, 0, font, white);

    /* btns */
    btn_draw(renderer, pausebtn, font);
    btn_draw(renderer, savebtn, font);
    btn_draw(renderer, loadbtn, font);
    btn_draw(renderer, quitbtn, font);
    btn_draw(renderer, spddownbtn, font);
    btn_draw(renderer, spdupbtn, font);

    SDL_RenderPresent(renderer);
}

void
handle_event(Chip8 *chip, SDL_Event event) {
    switch ( event.type ) {
        case SDL_QUIT:
            running = 0;
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            uint8_t sym = event.key.keysym.sym;
            int keydown = (event.type == SDL_KEYDOWN);
            switch (sym) {
                case SDLK_1:
                    chip->state[0x1] = keydown;
                    break;
                case SDLK_2:
                    chip->state[0x2] = keydown;
                    break;
                case SDLK_3:
                    chip->state[0x3] = keydown;
                    break;
                case SDLK_4:
                    chip->state[0xC] = keydown;
                    break;
                case SDLK_q:
                    chip->state[0x4] = keydown;
                    break;
                case SDLK_w:
                    chip->state[0x5] = keydown;
                    break;
                case SDLK_e:
                    chip->state[0x6] = keydown;
                    break;
                case SDLK_r:
                    chip->state[0xD] = keydown;
                    break;
                case SDLK_a:
                    chip->state[0x7] = keydown;
                    break;
                case SDLK_s:
                    chip->state[0x8] = keydown;
                    break;
                case SDLK_d:
                    chip->state[0x9] = keydown;
                    break;
                case SDLK_f:
                    chip->state[0xE] = keydown;
                    break;
                case SDLK_z:
                    chip->state[0xA] = keydown;
                    break;
                case SDLK_x:
                    chip->state[0x0] = keydown;
                    break;
                case SDLK_c:
                    chip->state[0xB] = keydown;
                    break;
                case SDLK_v:
                    chip->state[0xF] = keydown;
                    break;
            }
            if (event.type == SDL_KEYDOWN) {
                if ( sym == SDLK_SPACE )
                    paused ^= 1;
            }
            break;
        }
        case SDL_MOUSEBUTTONDOWN:
            if (btn_isover(pausebtn, event.motion.x, event.motion.y))
                paused ^= 1;
            else if (btn_isover(savebtn, event.motion.x, event.motion.y))
                savebtn->on_click(chip);
            else if (btn_isover(loadbtn, event.motion.x, event.motion.y))
                loadbtn->on_click(chip);
            else if (btn_isover(quitbtn, event.motion.x, event.motion.y))
                quitbtn->on_click(chip);
            else if (btn_isover(spdupbtn, event.motion.x, event.motion.y))
                SPEED++;
            else if (btn_isover(spddownbtn, event.motion.x, event.motion.y))
                SPEED--;
            break;
    }
}

void
updatetimer(Chip8 *chip) {
    if (chip->delayTimer > 0)
        chip->delayTimer--;
    if (chip->soundTimer > 0)
        chip->soundTimer--;
    SDL_PauseAudioDevice(audio_device_id, !chip->soundTimer);
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
