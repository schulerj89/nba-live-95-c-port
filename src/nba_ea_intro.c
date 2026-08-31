#include "nba_ea_intro.h"
#include "nba_snes_ppu.h"
#include <string.h>

/* $82:F15C-$F4C3, F4C4-F67D; bounded recomp counterparts IntroBuilder,
 * EASequence, IntroFlash, Draw{E,A,Sports}Layer, Mode7Zoom, StepPalette and
 * StepPaletteGroup. $0836/$0838 -> matrix; $0828/$082E/$0830 -> palette.
 * Static indexed graphics are extracted separately. This code owns resource
 * publication, matrix updates and palette stepping, never captured RGB.
 * Evidence: intro-exact-20260830/capture-v4 and resource-schedule-comparison.
 * Cold-boot waits, skip/restart handling and audio remain separate work. */
enum { EA_INDEXED_SIZE = 71674,
       EA_VRAM = 32, EA_OBJ_CHR = 65568, EA_CGRAM = 69664,
       EA_PALETTES = 70176, EA_OAM_E = 70304, EA_OAM_EA = 70848,
       EA_GROUP_A = 71392, EA_GROUP_SPORTS = 71578 };

static uint16_t ea_word(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}
bool nba_ea_intro_payload_valid(const uint8_t *data, size_t size) {
    static const uint32_t header[6] = {1,71674,65536,4096,512,544};
    if (!data || size != EA_INDEXED_SIZE || memcmp(data,"NBEAIDX1",8)) return false;
    for (int i=0;i<6;i++) {
        const uint8_t *p=data+8+i*4;
        uint32_t word=(uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
        if(word!=header[i])return false;
    }
    return ea_word(data+EA_GROUP_A)==18 && ea_word(data+EA_GROUP_A+2)==10 &&
           ea_word(data+EA_GROUP_A+4)==16 && ea_word(data+EA_GROUP_SPORTS)==18 &&
           ea_word(data+EA_GROUP_SPORTS+2)==5 && ea_word(data+EA_GROUP_SPORTS+4)==16;
}
static void ea_put_word(uint8_t *p, uint16_t word) {
    p[0] = (uint8_t)word;
    p[1] = (uint8_t)(word >> 8);
}
static void ea_tilegroup(uint8_t *vram, const uint8_t *group, int x, int y) {
    /* $80:8FA3-$9012: row DMA writes low VRAM bytes, preserving characters. */
    int width = ea_word(group), height = ea_word(group + 2);
    for (int row = 0; row < height; row++)
        for (int col = 0; col < width; col++)
            vram[2 * ((y + row) * 128 + x + col)] = group[6 + row * width + col];
}
static bool ea_palette_step(uint16_t *colors) {
    /* $82:F5E7-$F649 includes color0 in the whole-group completion check. */
    bool complete = true;
    for (int i = 0; i < 16; i++) {
        int red = colors[i] & 31, green = (colors[i] >> 5) & 31;
        int blue = (colors[i] >> 10) & 31;
        if (red < 31) red++;
        if (green < 31) green++;
        if (blue < 31) blue++;
        colors[i] = (uint16_t)(red | green << 5 | blue << 10);
        if (colors[i] != 0x7FFF) complete = false;
    }
    return complete;
}
static void ea_flash(uint8_t *cgram, int index, const uint8_t *source, int waits) {
    uint16_t colors[16], base[16];
    for (int i = 0; i < 16; i++) colors[i] = base[i] = ea_word(source + i * 2);
    for (int frame = 0; frame < waits; frame++)
        /* F4C4 calls F64A twice before each of eight frame waits. F64A
         * abandons its remaining inner steps when it reloads the palette. */
        for (int call = 0; call < 2; call++)
            for (int step = 0; step < 3; step++)
                if (ea_palette_step(colors)) {
                    memcpy(colors, base, sizeof(colors));
                    break;
                }
    for (int i = 0; i < 16; i++) ea_put_word(cgram + (index + i) * 2, colors[i]);
}
static int ea_matrix(int local_frame) {
    if (local_frame <= 1) return 1;
    if (local_frame >= 23) return 256;
    return 1 + 12 * (local_frame - 1);
}

void nba_ea_intro_render(const NbaAssetPack *assets, NbaRenderer *ren, uint32_t motion_frame) {
    if (!assets || !ren) return;
    nba_renderer_clear(ren, 0xFF000000);
    const NbaAssetItem *item = nba_assets_get(assets, NBA_ASSET_EA_INDEXED);
    if (!item || !nba_ea_intro_payload_valid(item->data,item->size)) return;
    const uint8_t *data = item->data;
    /* Every animated branch is settled after frame130. Clamp before the
     * signed conversion; the caller supplies logical frames, never seconds. */
    int frame = motion_frame > 130u ? 130 : (int)motion_frame;
    uint8_t vram[65536], cgram[512];
    memcpy(vram, data + EA_VRAM, sizeof(vram));
    memcpy(cgram, data + EA_CGRAM, sizeof(cgram));
    /* F2FE/F37E build fixed E/EA objects; Mode7 continues to flash over them. */
    if (frame >= 23) memcpy(vram + 0xC000, data + EA_OBJ_CHR, 0x800);
    if (frame >= 33) ea_tilegroup(vram, data + EA_GROUP_A, 0x37, 0x38);
    if (frame >= 56) memcpy(vram + 0xC800, data + EA_OBJ_CHR + 0x800, 0x800);
    /* F52E has two tilegroup waits; F40C-F427 queues the palette afterward. */
    if (frame >= 66) ea_tilegroup(vram, data + EA_GROUP_SPORTS, 0x37, 0x38);
    if (frame >= 67) ea_tilegroup(vram, data + EA_GROUP_SPORTS, 0x37, 0x3D);
    if (frame >= 69) memcpy(cgram + 0x40, data + EA_PALETTES + 64, 32);
    if (frame >= 130) memcpy(cgram + 0x20, data + EA_PALETTES + 96, 32);
    if (frame >= 24 && frame <= 31) ea_flash(cgram, 0x30, data + EA_PALETTES, frame - 23);
    if (frame >= 57 && frame <= 64) ea_flash(cgram, 0x40, data + EA_PALETTES + 32, frame - 56);
    if (frame >= 91 && frame <= 98) ea_flash(cgram, 0x20, data + EA_PALETTES + 64, frame - 90);
    if (frame >= 23) {
        NbaSnesMode1Snapshot objects;
        memset(&objects, 0, sizeof(objects));
        objects.brightness = 15;
        objects.main_screen_layers = 0x10;
        objects.oam_base = 0xC000;
        objects.oam_name_offset = 0x2000;
        objects.enable_oam_priority = true;
        /* All intro OBJ use priority0. Shared OBJ decoding is mode-independent;
         * Mode7 BG1 is above these objects, not above every possible OBJ. */
        nba_snes_mode1_render_snapshot(ren, vram, cgram,
            data + (frame >= 56 ? EA_OAM_EA : EA_OAM_E), &objects);
    }
    int matrix = frame < 33 ? ea_matrix(frame) : frame < 67 ?
        ea_matrix(frame - 33) : ea_matrix(frame - 68);
    for (int y = 0; y < NBA_SNES_HEIGHT; y++) {
        /* Vertical matrix products truncate separately; visible scanlines
         * start at1. Folding scanline1 into vscroll caused moving-edge errors. */
        int sy = (((matrix * (400 - 512)) & ~63) + (512 << 8) +
                  ((matrix * (y + 1)) & ~63)) >> 8;
        for (int x = 0; x < NBA_SNES_WIDTH; x++) {
            int sx = (((matrix * (384 - 512)) & ~63) + (512 << 8) + matrix * x) >> 8;
            int tile = (sx < 0 || sx >= 1024 || sy < 0 || sy >= 1024) ? 0 :
                vram[2 * ((sy >> 3) * 128 + (sx >> 3))];
            int color = vram[2 * (tile * 64 + (sy & 7) * 8 + (sx & 7)) + 1];
            if (color) ren->pixels[y * NBA_SNES_WIDTH + x] =
                nba_snes_cgram_color(cgram, color, 15, 0, 0, 0);
        }
    }
}
