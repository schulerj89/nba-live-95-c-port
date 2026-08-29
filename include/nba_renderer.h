#ifndef NBA_RENDERER_H
#define NBA_RENDERER_H

#include "nba_types.h"

/* SNES PPU (Picture Processing Unit) Memory & Register Mappings */
#define SNES_PPU_INIDISP   0x2100  /* Display Control & Master Brightness */
#define SNES_PPU_BGMODE    0x2105  /* BG Mode (Mode 1: 8x8 tiles) */
#define SNES_PPU_BG1SC     0x2107  /* BG1 Tilemap Base & Size */
#define SNES_PPU_BG1HOFS   0x210D  /* BG1 Horizontal Scroll */
#define SNES_PPU_BG1VOFS   0x210E  /* BG1 Vertical Scroll */
#define SNES_PPU_BG2HOFS   0x210F  /* BG2 Horizontal Scroll */
#define SNES_PPU_BG2VOFS   0x2110  /* BG2 Vertical Scroll */
#define SNES_PPU_M7A       0x211B  /* Mode 7 Matrix Parameter A (Horizontal Scale) */
#define SNES_PPU_M7B       0x211C  /* Mode 7 Matrix Parameter B (Horizontal Shear) */
#define SNES_PPU_M7C       0x211D  /* Mode 7 Matrix Parameter C (Vertical Shear) */
#define SNES_PPU_M7D       0x211E  /* Mode 7 Matrix Parameter D (Vertical Scale) */
#define SNES_PPU_M7X       0x211F  /* Mode 7 Center X Coordinate */
#define SNES_PPU_M7Y       0x2120  /* Mode 7 Center Y Coordinate */
#define SNES_PPU_CGADD     0x2121  /* CGRAM Palette Word Address */
#define SNES_PPU_CGDATA    0x2122  /* CGRAM Palette Data Write (BGR555) */
#define SNES_PPU_TM        0x212C  /* Main Screen Designation */

typedef struct {
    uint32_t pixels[NBA_SNES_WIDTH * NBA_SNES_HEIGHT];
    /* Opaque, lazily allocated Mode-1 candidate/provenance state. Ordinary
     * software-rendered screens do not pay its framebuffer-sized cost. */
    struct NbaSnesMode1State *ppu_state;
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
