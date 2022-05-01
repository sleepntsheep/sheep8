#include "widget.h" 

Button
*btn_init(int x, int y, int w, int h, int r, int g, int b, char *text, void (*on_click)()) {
    Button *btn = malloc(sizeof(Button));
    btn->x = x; btn->w = w;
    btn->y = y; btn->h = h;
    btn->r = r;
    btn->g = g;
    btn->b = b;
    btn->text = text;
    btn->on_click = on_click;
    return btn;
}

int
btn_isover(Button *btn, int x, int y) {
    return (btn->x <= x && btn->y <= y && btn->x + btn->w >= x && btn->y + btn->h >= y);
}

void
btn_draw(SDL_Renderer *renderer, Button *btn, TTF_Font *font) {
    SDL_Rect rect;
    rect.x = btn->x; rect.y = btn->y; rect.w = btn->w; rect.h = btn->h;
    
	SDL_SetRenderDrawColor(renderer, btn->r, btn->g, btn->b, 0);
    SDL_RenderFillRect(renderer, &rect);
    draw_text(renderer, btn->text, btn->x+btn->w/2, btn->y+btn->h/2, 1, font);
}

void
draw_text(SDL_Renderer *renderer, char *text, int x, int y, int center, TTF_Font *font) {
	SDL_Rect dstrect;
	int textW, textH;

    SDL_Color color = {255,255,255};
	SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_QueryTexture(texture, NULL,  NULL, &textW, &textH);

    dstrect.x = x; dstrect.y = y; dstrect.w = textW; dstrect.h = textH;

    if (center) {
        dstrect.x -= textW / 2;
        dstrect.y -= textH / 2;
    }
	SDL_FreeSurface(surface);
	SDL_RenderCopy(renderer, texture, NULL, &dstrect);
}

