#include "nba_asset_debugger.h"
#include "nba_font.h"
#include "nba_snes_ppu.h"
#include <stdio.h>
#include <string.h>

static void asset_fill(NbaRenderer *ren, int x, int y, int w, int h,
                       uint32_t color) {
    for (int py = y; py < y + h && py < NBA_SNES_HEIGHT; ++py)
        for (int px = x; px < x + w && px < NBA_SNES_WIDTH; ++px)
            if (px >= 0 && py >= 0)
                ren->pixels[py * NBA_SNES_WIDTH + px] = color;
}

static const char *asset_kind(const NbaAssetItem *item) {
    if (!item) return "NONE";
    if (item->size == 0x10000u) return "SNES VRAM";
    if (item->size == 0x200u) return "SNES CGRAM";
    if (item->size == 0x220u) return "SNES OAM";
    if (item->size > 12u && memcmp(item->data, "RIFF", 4) == 0) return "PCM/BRR";
    return "BINARY";
}

static const uint8_t *asset_palette_for(const NbaAssetPack *assets,
                                        uint32_t id) {
    NbaAssetId palette = NBA_ASSET_NONE;
    if (id == NBA_ASSET_SETUP_VRAM) palette = NBA_ASSET_SETUP_CGRAM;
    if (id == NBA_ASSET_SET_RULES_VRAM) palette = NBA_ASSET_SET_RULES_CGRAM;
    if (id == NBA_ASSET_SET_OPTIONS_VRAM) palette = NBA_ASSET_SET_OPTIONS_CGRAM;
    if (id == NBA_ASSET_RULES_OPEN_VRAM) palette = NBA_ASSET_RULES_OPEN_CGRAM;
    if (id == NBA_ASSET_OPTIONS_OPEN_VRAM) palette = NBA_ASSET_OPTIONS_OPEN_CGRAM;
    if (id == NBA_ASSET_SETUP_RETURN_VRAM) palette = NBA_ASSET_SETUP_RETURN_CGRAM;
    if (id == NBA_ASSET_RULES_RETURN_VRAM) palette = NBA_ASSET_RULES_RETURN_CGRAM;
    if (id == NBA_ASSET_OPTIONS_OFF_VRAM || id == NBA_ASSET_OPTIONS_MONO_VRAM ||
        id == NBA_ASSET_OPTIONS_CPU_VRAM ||
        id == NBA_ASSET_OPTIONS_CROWD_OFF_VRAM ||
        id == NBA_ASSET_OPTIONS_SLOW_ON_VRAM ||
        id == NBA_ASSET_OPTIONS_ASSISTANCE_ON_VRAM)
        palette = NBA_ASSET_SET_OPTIONS_CGRAM;
    if (id >= NBA_ASSET_SETUP_MODE_SEASON_VRAM &&
        id <= NBA_ASSET_SETUP_QUARTER_12_VRAM) palette = NBA_ASSET_SETUP_CGRAM;
    const NbaAssetItem *item = nba_assets_get(assets, palette);
    return item && item->size == 0x200u ? (const uint8_t *)item->data : NULL;
}

static int asset_tile_pixel(const uint8_t *tile, int bpp, int x, int y) {
    int value = ((tile[y * 2] >> (7 - x)) & 1) |
                (((tile[y * 2 + 1] >> (7 - x)) & 1) << 1);
    if (bpp == 4) {
        value |= ((tile[16 + y * 2] >> (7 - x)) & 1) << 2;
        value |= ((tile[17 + y * 2] >> (7 - x)) & 1) << 3;
    }
    return value;
}

static void asset_render_oam(const NbaAssetPack *assets,
                             const NbaAssetItem *item, NbaRenderer *ren) {
    NbaAssetId vram_id = item->id == NBA_ASSET_SET_RULES_OAM ?
                         NBA_ASSET_SET_RULES_VRAM : NBA_ASSET_SET_OPTIONS_VRAM;
    NbaAssetId cgram_id = item->id == NBA_ASSET_SET_RULES_OAM ?
                          NBA_ASSET_SET_RULES_CGRAM : NBA_ASSET_SET_OPTIONS_CGRAM;
    const NbaAssetItem *vram_item = nba_assets_get(assets, vram_id);
    const NbaAssetItem *cgram_item = nba_assets_get(assets, cgram_id);
    if (!vram_item || vram_item->size != 0x10000u ||
        !cgram_item || cgram_item->size != 0x200u) return;
    const uint8_t *oam = (const uint8_t *)item->data;
    const uint8_t *vram = (const uint8_t *)vram_item->data;
    const uint8_t *cgram = (const uint8_t *)cgram_item->data;
    for (int index = 127; index >= 0; --index) {
        int high = (oam[512 + index / 4] >> ((index & 3) * 2)) & 3;
        int x = oam[index * 4] | ((high & 1) << 8);
        if (x >= 256) x -= 512;
        int y = oam[index * 4 + 1];
        int tile = oam[index * 4 + 2];
        int attr = oam[index * 4 + 3];
        int size = (high & 2) ? 16 : 8;
        int palette = (attr >> 1) & 7;
        tile += (attr & 1) ? 256 : 0;
        for (int py = 0; py < size; ++py) {
            int sy = (attr & 0x80) ? size - 1 - py : py;
            for (int px = 0; px < size; ++px) {
                int sx = (attr & 0x40) ? size - 1 - px : px;
                int subtile = tile + (sx >> 3) + (sy >> 3) * 16;
                size_t offset = 0xC000u + (size_t)subtile * 32u;
                if (offset + 32u > 0x10000u) continue;
                int color = asset_tile_pixel(vram + offset, 4, sx & 7, sy & 7);
                int ox = x + px, oy = y + py;
                if (color && ox >= 0 && ox < NBA_SNES_WIDTH &&
                    oy >= 55 && oy < NBA_SNES_HEIGHT)
                    ren->pixels[oy * NBA_SNES_WIDTH + ox] =
                        nba_snes_cgram_color(cgram, 128 + palette * 16 + color,
                                             15, 0, 0, 0);
            }
        }
    }
}

void nba_asset_debugger_init(NbaAssetDebugger *dbg) {
    if (dbg) memset(dbg, 0, sizeof(*dbg));
}

void nba_asset_debugger_toggle(NbaAssetDebugger *dbg) {
    if (!dbg) return;
    dbg->is_active = !dbg->is_active;
    printf("[DEBUGGER] Asset browser %s (F12).\n",
           dbg->is_active ? "opened" : "closed");
}

void nba_asset_debugger_update(NbaAssetDebugger *dbg,
                               const NbaAssetPack *assets,
                               const NbaInput *input) {
    if (!dbg || !dbg->is_active || !assets || !input || assets->item_count == 0)
        return;
    if (input->pressed & NBA_BTN_UP)
        dbg->selected_index = (dbg->selected_index + (int)assets->item_count - 1) %
                              (int)assets->item_count;
    if (input->pressed & NBA_BTN_DOWN)
        dbg->selected_index = (dbg->selected_index + 1) % (int)assets->item_count;
    if (input->pressed & NBA_BTN_LEFT) dbg->tile_page--;
    if (input->pressed & NBA_BTN_RIGHT) dbg->tile_page++;
    if (dbg->tile_page < 0) dbg->tile_page = 23;
    if (dbg->tile_page > 23) dbg->tile_page = 0;
}

void nba_asset_debugger_render(const NbaAssetDebugger *dbg,
                               const NbaAssetPack *assets,
                               NbaRenderer *ren) {
    if (!dbg || !dbg->is_active || !assets || !ren || assets->item_count == 0)
        return;
    int index = dbg->selected_index;
    if (index < 0 || index >= (int)assets->item_count) index = 0;
    const NbaAssetItem *item = &assets->items[index];
    asset_fill(ren, 0, 0, NBA_SNES_WIDTH, NBA_SNES_HEIGHT, 0xFF081018u);
    char line[80];
    nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, 8, 7,
                         "ROM ASSET DEBUGGER [F12]", 0xFF6FDBFFu, 0, 1);
    snprintf(line, sizeof(line), "%02d/%02u ID:%03u  %s", index + 1,
             assets->item_count, item->id, asset_kind(item));
    nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, 8, 19, line,
                         0xFFFFFFFFu, 0, 1);
    snprintf(line, sizeof(line), "SIZE:%u W:%u H:%u F:%08X", item->size,
             item->width, item->height, item->flags);
    nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, 8, 30, line,
                         0xFF9FB2C8u, 0, 1);
    nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, 8, 41,
                         "UP/DN ASSET  LEFT/RIGHT TILE PAGE", 0xFF8CFF9Du, 0, 1);

    if (item->size == 0x220u &&
        (item->id == NBA_ASSET_SET_RULES_OAM ||
         item->id == NBA_ASSET_SET_OPTIONS_OAM)) {
        nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, 8, 55,
                             "OAM AT CAPTURED SCREEN COORDINATES", 0xFFFFC66Du, 0, 1);
        asset_render_oam(assets, item, ren);
    } else if (item->size == 0x200u) {
        const uint8_t *cgram = (const uint8_t *)item->data;
        for (int color = 0; color < 256; ++color) {
            int x = 8 + (color & 15) * 15;
            int y = 58 + (color >> 4) * 9;
            asset_fill(ren, x, y, 13, 7,
                       nba_snes_cgram_color(cgram, color, 15, 0, 0, 0));
        }
    } else if (item->size == 0x10000u) {
        const uint8_t *vram = (const uint8_t *)item->data;
        const uint8_t *cgram = asset_palette_for(assets, item->id);
        int bpp = dbg->tile_page < 8 ? 4 : 2;
        int tile_size = bpp == 4 ? 32 : 16;
        int first_tile = (dbg->tile_page < 8 ? dbg->tile_page :
                          dbg->tile_page - 8) * 256;
        snprintf(line, sizeof(line), "PAGE:%d  %dBPP  TILE:%04X", dbg->tile_page,
                 bpp, first_tile);
        nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, 8, 53, line,
                             0xFFFFC66Du, 0, 1);
        for (int tile = 0; tile < 256; ++tile) {
            size_t offset = (size_t)(first_tile + tile) * (size_t)tile_size;
            if (offset + (size_t)tile_size > item->size) break;
            int ox = 8 + (tile & 15) * 8;
            int oy = 66 + (tile >> 4) * 8;
            for (int py = 0; py < 8; ++py)
                for (int px = 0; px < 8; ++px) {
                    int color = asset_tile_pixel(vram + offset, bpp, px, py);
                    ren->pixels[(oy + py) * NBA_SNES_WIDTH + ox + px] = cgram ?
                        nba_snes_cgram_color(cgram, color, 15, 0, 0, 0) :
                        (0xFF000000u | (uint32_t)(color * (255 / ((1 << bpp) - 1))) * 0x010101u);
                }
        }
    } else {
        nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, 8, 70,
                             "METADATA VIEW - NO TILE DECODER", 0xFFFF9A7Au, 0, 1);
    }
}
