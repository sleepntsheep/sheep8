#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>

#include <raylib.h>
#include "chip8.h"
#include "config.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

char path[PATH_MAX] = { 0 };

int main(int argc, char *argv[])
{
    Chip8 chip;
    chip8_init(&chip);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WIDTH * scale, HEIGHT * scale, "Chip8 Emulator :D");
    SetTargetFPS(fps);

    if (argc >= 2) {
        chip8_loadrom(&chip, argv[1]);
    }

    while (!WindowShouldClose()) {
        chip8_doevent(&chip);
        chip8_updatetimer(&chip);
        int screenwidth = GetScreenWidth();
        int screenheight = GetScreenHeight();
        int scalewidth = screenwidth / WIDTH;
        int scaleheight = screenheight / HEIGHT;

        for (int i = 0; i < chip.clockspeed / fps; ++i)
            chip8_interpretop(&chip, chip8_getop(chip));

        BeginDrawing();

        ClearBackground((Color){0,0,0,255});

        for (int y = 0; y < HEIGHT; y++)
            for (int x = 0; x < WIDTH; x++)
                if (chip.screen[y][x])
                    DrawRectangle(x*scalewidth, y*scaleheight,
                            scalewidth, scaleheight, (Color){255,255,255,255});

        EndDrawing();
    }

    return EXIT_SUCCESS;
}

