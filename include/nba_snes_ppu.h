#ifndef NBA_SNES_PPU_H
#define NBA_SNES_PPU_H

#include "nba_types.h"
#include "nba_renderer.h"
#include <stdio.h>

typedef struct {
    int color_index;
    int palette;
    int priority;
} NbaSnesBgPixel;

typedef enum {
    NBA_SNES_LAYER_BACKDROP = 0,
    NBA_SNES_LAYER_BG1,
    NBA_SNES_LAYER_BG2,
    NBA_SNES_LAYER_BG3,
    NBA_SNES_LAYER_OBJ
} NbaSnesLayer;

typedef struct {
    NbaSnesLayer layer;
    uint8_t priority;
    uint8_t palette_index;
    uint8_t color_index;
    uint8_t oam_index;
    uint8_t rank;
    uint32_t argb;
} NbaSnesMode1Pixel;

typedef struct {
    uint32_t visible[5];
} NbaSnesMode1Stats;

typedef struct {
    bool active;
    bool inverted;
    uint8_t left;
    uint8_t right;
} NbaSnesWindow;

typedef enum {
    NBA_SNES_WINDOW_OR = 0,
    NBA_SNES_WINDOW_AND,
    NBA_SNES_WINDOW_XOR,
    NBA_SNES_WINDOW_XNOR
} NbaSnesWindowLogic;

uint32_t nba_snes_cgram_color(const uint8_t *cgram, int index, int brightness,
                              int subtract_r, int subtract_g, int subtract_b);

bool nba_snes_sample_bg(const uint8_t *vram, int map_base, int chr_base,
                        int bits_per_pixel, bool wide, bool tall,
                        int horizontal_scroll, int vertical_scroll,
                        int x, int y, NbaSnesBgPixel *pixel);

/* General SNES Mode-1 main-screen compositor. Priority follows BGMODE's
 * documented front-to-back ladder; lower OAM indexes win OBJ ties. */
bool nba_snes_mode1_begin(NbaRenderer *renderer, uint32_t backdrop,
                          bool bg3_priority_high);
void nba_snes_mode1_release(NbaRenderer *renderer);
bool nba_snes_mode1_submit_color(NbaRenderer *renderer, int x, int y,
                                 NbaSnesLayer layer, uint8_t priority,
                                 uint8_t oam_index, uint32_t argb);
bool nba_snes_mode1_submit_indexed(NbaRenderer *renderer, const uint8_t *cgram,
                                   int brightness, int x, int y,
                                   NbaSnesLayer layer, uint8_t priority,
                                   uint8_t palette_index, uint8_t color_index,
                                   uint8_t oam_index);
bool nba_snes_mode1_pixel(const NbaRenderer *renderer, int x, int y,
                          NbaSnesMode1Pixel *pixel);
void nba_snes_mode1_stats(const NbaRenderer *renderer,
                          NbaSnesMode1Stats *stats);
bool nba_snes_mode1_write_jsonl(FILE *file, const NbaRenderer *renderer,
                                uint32_t game_frame, uint32_t state_frame);
const char *nba_snes_layer_name(NbaSnesLayer layer);

/* Returns true when the layer is visible after a single enabled SNES window
 * is applied. TMW masks the region selected by the window result. */
bool nba_snes_window_visible(int x, uint8_t left, uint8_t right,
                             bool inverted);
bool nba_snes_window_masked(int x, const NbaSnesWindow *window1,
                            const NbaSnesWindow *window2,
                            NbaSnesWindowLogic logic);
bool nba_snes_mode1_self_test(void);

#endif /* NBA_SNES_PPU_H */
