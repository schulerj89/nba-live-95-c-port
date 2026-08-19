#include "nba_renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BmpFileHeader;

typedef struct {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BmpInfoHeader;
#pragma pack(pop)

void nba_renderer_init(NbaRenderer *renderer) {
    if (!renderer) return;
    renderer->width = NBA_SNES_WIDTH;
    renderer->height = NBA_SNES_HEIGHT;
    renderer->bg_color = 0xFF000000;
    nba_renderer_clear(renderer, renderer->bg_color);
}

void nba_renderer_clear(NbaRenderer *renderer, uint32_t color) {
    if (!renderer) return;
    for (int i = 0; i < NBA_SNES_WIDTH * NBA_SNES_HEIGHT; i++) {
        renderer->pixels[i] = color;
    }
}

void nba_renderer_set_pixel(NbaRenderer *renderer, int x, int y, uint32_t color) {
    if (!renderer) return;
    if (x >= 0 && x < NBA_SNES_WIDTH && y >= 0 && y < NBA_SNES_HEIGHT) {
        renderer->pixels[y * NBA_SNES_WIDTH + x] = color;
    }
}

void nba_renderer_draw_rect(NbaRenderer *renderer, int x, int y, int w, int h, uint32_t color) {
    if (!renderer) return;
    for (int dy = 0; dy < h; dy++) {
        int py = y + dy;
        if (py < 0 || py >= NBA_SNES_HEIGHT) continue;
        for (int dx = 0; dx < w; dx++) {
            int px = x + dx;
            if (px < 0 || px >= NBA_SNES_WIDTH) continue;
            renderer->pixels[py * NBA_SNES_WIDTH + px] = color;
        }
    }
}

bool nba_renderer_save_bmp(const NbaRenderer *renderer, const char *filepath) {
    if (!renderer || !filepath) return false;

    FILE *f = fopen(filepath, "wb");
    if (!f) return false;

    int width = renderer->width;
    int height = renderer->height;
    int row_stride = ((width * 3 + 3) / 4) * 4;
    uint32_t image_size = (uint32_t)(row_stride * height);

    BmpFileHeader fh;
    fh.bfType = 0x4D42; /* "BM" */
    fh.bfSize = sizeof(BmpFileHeader) + sizeof(BmpInfoHeader) + image_size;
    fh.bfReserved1 = 0;
    fh.bfReserved2 = 0;
    fh.bfOffBits = sizeof(BmpFileHeader) + sizeof(BmpInfoHeader);

    BmpInfoHeader ih;
    memset(&ih, 0, sizeof(ih));
    ih.biSize = sizeof(BmpInfoHeader);
    ih.biWidth = width;
    ih.biHeight = height; /* Bottom-up DIB */
    ih.biPlanes = 1;
    ih.biBitCount = 24;
    ih.biCompression = 0;
    ih.biSizeImage = image_size;

    fwrite(&fh, sizeof(fh), 1, f);
    fwrite(&ih, sizeof(ih), 1, f);

    uint8_t *row_buffer = (uint8_t *)calloc(1, (size_t)row_stride);
    if (!row_buffer) {
        fclose(f);
        return false;
    }

    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            uint32_t argb = renderer->pixels[y * width + x];
            row_buffer[x * 3 + 0] = (uint8_t)(argb & 0xFF);         /* Blue */
            row_buffer[x * 3 + 1] = (uint8_t)((argb >> 8) & 0xFF);  /* Green */
            row_buffer[x * 3 + 2] = (uint8_t)((argb >> 16) & 0xFF); /* Red */
        }
        fwrite(row_buffer, 1, (size_t)row_stride, f);
    }

    free(row_buffer);
    fclose(f);
    return true;
}
