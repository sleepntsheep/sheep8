#ifndef WIDGET_H
#define WIDGET_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>

typedef struct Button Button;

struct Button {
    int x, y, w, h;
    int r, g, b;
    char *text;
    void (*on_click)();
};

Button *btn_init(int x, int y, int w, int h, int r, int g, int b, char* text, void (*on_click)());

int btn_isover(Button *btn, int x, int y);

void btn_draw(SDL_Renderer *renderer, Button *btn, TTF_Font *font);

void draw_text(SDL_Renderer *renderer, char *text, int x, int y, int center, TTF_Font *font);

#endif
