/* Replays one raw emulator Mode-1 snapshot through the production C PPU. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_snes_ppu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char *read_file(const char *path, size_t expected, size_t *size) {
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) || (*size = (size_t)ftell(file)) < expected ||
        fseek(file, 0, SEEK_SET)) { fclose(file); return NULL; }
    unsigned char *data = (unsigned char *)malloc(*size + 1u);
    if (!data || fread(data, 1, *size, file) != *size) {
        free(data); fclose(file); return NULL;
    }
    fclose(file); data[*size] = 0; return data;
}

static int state_int(const char *state, const char *key, int *value) {
    size_t length = strlen(key);
    const char *found = strstr(state, key);
    if (!found || found[length] != '=') return 0;
    *value = atoi(found + length + 1); return 1;
}

static int state_bool(const char *state, const char *key, bool *value) {
    size_t length = strlen(key);
    const char *found = strstr(state, key);
    if (!found || found[length] != '=') return 0;
    *value = strncmp(found + length + 1, "true", 4) == 0; return 1;
}

int main(int argc, char **argv) {
    size_t vs, cs, os, ss;
    if (argc != 6) return 2;
    unsigned char *vram = read_file(argv[1], 0x10000, &vs);
    unsigned char *cgram = read_file(argv[2], 0x200, &cs);
    unsigned char *oam = read_file(argv[3], 0x220, &os);
    unsigned char *state_data = read_file(argv[4], 1, &ss);
    if (!vram || !cgram || !oam || !state_data) return 3;
    const char *state = (const char *)state_data;
    NbaSnesMode1Snapshot snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    int value, mode;
    if (!state_int(state, "ppu.bgMode", &mode) || mode != 1 ||
        !state_int(state, "ppu.screenBrightness", &value)) return 4;
    snapshot.brightness = (uint8_t)value;
    if (!state_int(state, "ppu.mainScreenLayers", &value)) return 4;
    snapshot.main_screen_layers = (uint8_t)value;
    for (int scanline = 0; scanline < NBA_SNES_HEIGHT; ++scanline) {
        char raster_key[64];
        sprintf(raster_key, "audit.mainScreenLayers[%d]", scanline);
        if (state_int(state, raster_key, &value)) {
            snapshot.raster_main_screen_layers = true;
            snapshot.main_screen_layers_by_scanline[scanline] = (uint8_t)value;
        } else {
            snapshot.main_screen_layers_by_scanline[scanline] =
                snapshot.main_screen_layers;
        }
    }
    if (!state_bool(state, "ppu.mode1Bg3Priority", &snapshot.bg3_priority_high)) return 4;
    for (int layer = 0; layer < 3; ++layer) {
        NbaSnesMode1BgConfig *bg = &snapshot.backgrounds[layer];
        char key[96];
        bg->enabled = true;
        bg->bits_per_pixel = (uint8_t)(layer < 2 ? 4 : 2);
#define READ_LAYER_INT(suffix, field, scale) do { \
    sprintf(key, "ppu.layers[%d]." suffix, layer); \
    if (!state_int(state, key, &value)) return 4; bg->field = (value) * (scale); \
} while (0)
#define READ_LAYER_BOOL(suffix, field) do { \
    sprintf(key, "ppu.layers[%d]." suffix, layer); \
    if (!state_bool(state, key, &bg->field)) return 4; \
} while (0)
        sprintf(key, "ppu.layers[%d].tilemapAddress", layer);
        if (!state_int(state, key, &value)) return 4;
        bg->map_base = (uint16_t)(value * 2);
        sprintf(key, "ppu.layers[%d].chrAddress", layer);
        if (!state_int(state, key, &value)) return 4;
        bg->chr_base = (uint16_t)(value * 2);
        READ_LAYER_INT("hscroll", horizontal_scroll, 1);
        READ_LAYER_INT("vscroll", vertical_scroll, 1);
        READ_LAYER_BOOL("doubleWidth", wide);
        READ_LAYER_BOOL("doubleHeight", tall);
        sprintf(key, "ppu.windowMaskMain[%d]", layer);
        if (!state_bool(state, key, &bg->window_mask_main)) return 4;
        bg->window_logic = NBA_SNES_WINDOW_OR;
        for (int window = 0; window < 2; ++window) {
            sprintf(key, "ppu.window[%d].activeLayers[%d]", window, layer);
            if (!state_bool(state, key, &bg->windows[window].active)) return 4;
            sprintf(key, "ppu.window[%d].invertedLayers[%d]", window, layer);
            if (!state_bool(state, key, &bg->windows[window].inverted)) return 4;
            sprintf(key, "ppu.window[%d].left", window);
            if (!state_int(state, key, &value)) return 4;
            bg->windows[window].left = (uint8_t)value;
            sprintf(key, "ppu.window[%d].right", window);
            if (!state_int(state, key, &value)) return 4;
            bg->windows[window].right = (uint8_t)value;
        }
#undef READ_LAYER_INT
#undef READ_LAYER_BOOL
    }
    if (!state_int(state, "ppu.oamBaseAddress", &value)) return 4;
    snapshot.oam_base = (uint16_t)(value * 2);
    if (!state_int(state, "ppu.oamAddressOffset", &value)) return 4;
    snapshot.oam_name_offset = (uint16_t)(value * 2);
    if (!state_int(state, "ppu.oamRamAddress", &value)) return 4;
    snapshot.oam_ram_address = (uint16_t)value;
    if (!state_int(state, "ppu.oamMode", &value)) return 4;
    snapshot.oam_mode = (uint8_t)value;
    if (!state_bool(state, "ppu.enableOamPriority", &snapshot.enable_oam_priority)) return 4;

    NbaRenderer renderer;
    nba_renderer_init(&renderer);
    bool ok = nba_snes_mode1_render_snapshot(&renderer, vram, cgram, oam,
                                              &snapshot) &&
              nba_renderer_save_bmp(&renderer, argv[5]);
    nba_snes_mode1_release(&renderer);
    free(vram); free(cgram); free(oam); free(state_data);
    return ok ? 0 : 5;
}
