#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include <raylib.h>
#include "chip8.h"
#include "config.h"
#include "audio.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

typedef struct {
    AudioDriver audio;
    Color fg, bg;
    Chip8 chip;
    bool show_gui;
    char rom[4096];
} App;

App app = {
    .fg = (Color){255, 255, 255, 255},
    .bg = (Color){0, 0, 0, 255},
    .show_gui = true,
    .rom = { 0 },
};

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>

EMSCRIPTEN_KEEPALIVE int wasm_load_rom(uint8_t *buf, size_t size) {
    Chip8_Init(&app.chip);
    Chip8_LoadRom(&app.chip, buf, size);
    return 1;
}

int dropdownactive = 0;
bool dropdowneditmode = false;

static const char *preset_roms[] = {
    "roms/BRIX",
    "roms/octopaint.ch8",
    "roms/TETRIS",
};

#endif


void AppMainLoop()
{
    Chip8_Input(&app.chip);
    Chip8_Interpret(&app.chip);
    Chip8_UpdateTimer(&app.chip);

    if (app.chip.soundtimer > 0)
        AudioDriver_Play(&app.audio);
    else
        AudioDriver_Pause(&app.audio);

    BeginDrawing();
    ClearBackground(app.bg);

    int scalewidth = GetScreenWidth() / WIDTH;
    int scaleheight = GetScreenHeight() / HEIGHT;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (app.chip.screen[y][x]) {
                DrawRectangle(x*scalewidth, y*scaleheight,
                        scalewidth, scaleheight,
                        app.fg);
            }
        }
    }

    if (!app.show_gui) {
        if (GuiButton((Rectangle){0, 0, 10, 10}, "O"))
            app.show_gui = 1;
    } else {
        app.fg = GuiColorPicker((Rectangle){150,0,100,100}, "FG Color", app.fg);
        app.bg = GuiColorPicker((Rectangle){0,0,100,100}, "BG Color", app.bg);
        if (GuiButton((Rectangle){0, 270, 150, 50}, "Close GUI"))
            app.show_gui = 0;

        app.chip.clockspeed = GuiSliderPro((Rectangle){30, 100, 200, 30}, "0 Hz", "2000 Hz", app.chip.clockspeed, 0, 2000, 20);

#ifndef PLATFORM_WEB
        if (GuiTextInputBox((Rectangle){0, 130, 200, 130}, "Load Rom From File", "Path", "Load", app.rom, sizeof app.rom - 1, NULL) == 1) {
            Chip8_Init(&app.chip);
            Chip8_LoadRomFromFile(&app.chip, app.rom);
        }
#else
        if (GuiButton((Rectangle){0, 130, 100, 70}, "UPLOAD ROM"))
        EM_ASM(
            var file_selector = document.createElement('input');
            file_selector.setAttribute('type', 'file');
            file_selector.setAttribute('onchange','open_file(event)');
            file_selector.click();
        );

        if (GuiDropdownBox((Rectangle){100, 130, 100, 70}, "#01#BRIX;#02#Octopaint;#03#Tetris", &dropdownactive, dropdowneditmode)) {
            dropdowneditmode ^= 1;
            Chip8_Init(&app.chip);
            Chip8_LoadRomFromFile(&app.chip, preset_roms[dropdownactive]);
        }
#endif

        app.chip.paused = GuiCheckBox((Rectangle){200, 130, 50, 50}, "Pause", app.chip.paused);
    }

    EndDrawing();
}



int main(int argc, char *argv[])
{
    app.audio = AudioDriver_Init();
    Chip8_Init(&app.chip);

    if (argc >= 2)
        Chip8_LoadRomFromFile(&app.chip, argv[1]);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WIDTH * 15, HEIGHT * 15, "Chip8 Emulator :D");
    GuiLoadStyleDefault();

#ifdef PLATFORM_WEB
    emscripten_set_main_loop(AppMainLoop, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose())
        AppMainLoop();
#endif

    CloseWindow();

    return EXIT_SUCCESS;
}

