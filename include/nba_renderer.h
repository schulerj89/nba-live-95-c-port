#ifndef NBA_RENDERER_H
#define NBA_RENDERER_H

#include "nba_types.h"

typedef struct {
    uint32_t pixels[NBA_SNES_WIDTH * NBA_SNES_HEIGHT];
    int width;
    int height;
    uint32_t bg_color;
} NbaRenderer;

void nba_renderer_init(NbaRenderer *renderer);
void nba_renderer_clear(NbaRenderer *renderer, uint32_t color);
void nba_renderer_set_pixel(NbaRenderer *renderer, int x, int y, uint32_t color);
void nba_renderer_draw_rect(NbaRenderer *renderer, int x, int y, int w, int h, uint32_t color);
bool nba_renderer_save_bmp(const NbaRenderer *renderer, const char *filepath);

#endif /* NBA_RENDERER_H */
