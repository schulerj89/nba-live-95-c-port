#include "nba_snes_ppu.h"

static int snes_tile_pixel(const uint8_t *vram, int chr_base, int tile,
                           int bits_per_pixel, int x, int y) {
    int offset = (chr_base + tile * 8 * bits_per_pixel) & 0xFFFF;
    int bit = 7 - x;
    int value = 0;
    for (int plane = 0; plane < bits_per_pixel; plane += 2) {
        int low = vram[(offset + y * 2 + plane * 8) & 0xFFFF];
        int high = vram[(offset + y * 2 + 1 + plane * 8) & 0xFFFF];
        value |= ((low >> bit) & 1) << plane;
        value |= ((high >> bit) & 1) << (plane + 1);
    }
    return value;
}

uint32_t nba_snes_cgram_color(const uint8_t *cgram, int index, int brightness,
                              int subtract_r, int subtract_g, int subtract_b) {
    uint16_t word = (uint16_t)(cgram[(index * 2) & 0x1FF] |
                               ((uint16_t)cgram[(index * 2 + 1) & 0x1FF] << 8));
    int r = (word & 31) - subtract_r;
    int g = ((word >> 5) & 31) - subtract_g;
    int b = ((word >> 10) & 31) - subtract_b;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (brightness < 0) brightness = 0;
    if (brightness > 15) brightness = 15;

    uint32_t r8 = (uint32_t)((r << 3) | (r >> 2));
    uint32_t g8 = (uint32_t)((g << 3) | (g >> 2));
    uint32_t b8 = (uint32_t)((b << 3) | (b >> 2));
    if (brightness < 15) {
        r8 = r8 * (uint32_t)brightness / 15u;
        g8 = g8 * (uint32_t)brightness / 15u;
        b8 = b8 * (uint32_t)brightness / 15u;
    }
    return 0xFF000000u | (r8 << 16) | (g8 << 8) | b8;
}

bool nba_snes_sample_bg(const uint8_t *vram, int map_base, int chr_base,
                        int bits_per_pixel, bool wide, bool tall,
                        int horizontal_scroll, int vertical_scroll,
                        int x, int y, NbaSnesBgPixel *pixel) {
    if (!vram || !pixel) return false;
    int map_width = wide ? 512 : 256;
    int map_height = tall ? 512 : 256;
    int px = ((x + horizontal_scroll) % map_width + map_width) % map_width;
    int py = ((y + vertical_scroll + 1) % map_height + map_height) % map_height;
    int tile_x = px >> 3;
    int tile_y = py >> 3;
    int quadrant = 0;
    if (wide && tile_x >= 32) quadrant++;
    if (tall && tile_y >= 32) quadrant += wide ? 2 : 1;

    int entry_offset = map_base + quadrant * 0x800 +
                       ((tile_y & 31) * 32 + (tile_x & 31)) * 2;
    uint16_t entry = (uint16_t)(vram[entry_offset & 0xFFFF] |
                                ((uint16_t)vram[(entry_offset + 1) & 0xFFFF] << 8));
    int sample_x = (entry & 0x4000) ? 7 - (px & 7) : (px & 7);
    int sample_y = (entry & 0x8000) ? 7 - (py & 7) : (py & 7);
    int color_index = snes_tile_pixel(vram, chr_base, entry & 0x3FF,
                                      bits_per_pixel, sample_x, sample_y);
    if (color_index == 0) return false;

    pixel->color_index = color_index;
    pixel->palette = (entry >> 10) & 7;
    pixel->priority = (entry >> 13) & 1;
    return true;
}
