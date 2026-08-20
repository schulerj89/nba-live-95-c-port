#ifndef NBA_SNES_PPU_H
#define NBA_SNES_PPU_H

#include "nba_types.h"

typedef struct {
    int color_index;
    int palette;
    int priority;
} NbaSnesBgPixel;

uint32_t nba_snes_cgram_color(const uint8_t *cgram, int index, int brightness,
                              int subtract_r, int subtract_g, int subtract_b);

bool nba_snes_sample_bg(const uint8_t *vram, int map_base, int chr_base,
                        int bits_per_pixel, bool wide, bool tall,
                        int horizontal_scroll, int vertical_scroll,
                        int x, int y, NbaSnesBgPixel *pixel);

#endif /* NBA_SNES_PPU_H */
