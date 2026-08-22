#include "nba_team_select.h"
#include "nba_snes_ppu.h"
#include <stdlib.h>
#include <string.h>

#define TEAM_BG1_MAP 0x1800
#define TEAM_BG1_CHR 0x6000
#define TEAM_BG2_MAP 0x1000
#define TEAM_BG2_CHR 0x2000
#define TEAM_BG3_MAP 0x0000
#define TEAM_BG3_CHR 0x8000

/* $80:D9AF: alphabetical team ID, then five displayed one-based ranks. */
const NbaTeamRecord nba_team_records[NBA_TEAM_COUNT] = {
    { "ATLANTA",       "HAWKS",         {11, 5,  9,  4,  6} },
    { "BOSTON",        "CELTICS",       {16, 21, 8,  21, 19} },
    { "CHARLOTTE",     "HORNETS",       {4,  15, 11, 24, 17} },
    { "CHICAGO",       "BULLS",         {23, 9,  13, 3,  7} },
    { "CLEVELAND",     "CAVALIERS",     {12, 19, 1,  7,  13} },
    { "DALLAS",        "MAVERICKS",     {27, 20, 21, 17, 27} },
    { "DENVER",        "NUGGETS",       {19, 6,  23, 10, 15} },
    { "DETROIT",       "PISTONS",       {24, 24, 7,  20, 25} },
    { "GOLDEN STATE",  "WARRIORS",      {2,  10, 24, 23, 10} },
    { "HOUSTON",       "ROCKETS",       {13, 11, 17, 5,  2} },
    { "INDIANA",       "PACERS",        {15, 12, 25, 8,  11} },
    { "LA CLIPPERS",   "CLIPPERS",      {9,  13, 26, 27, 20} },
    { "LA LAKERS",     "LAKERS",        {17, 17, 4,  19, 18} },
    { "MIAMI",         "HEAT",          {7,  8,  14, 11, 16} },
    { "MILWAUKEE",     "BUCKS",         {25, 27, 18, 15, 26} },
    { "MINNESOTA",     "TIMBERWOLVES",  {26, 25, 27, 16, 24} },
    { "NEW JERSEY",    "NETS",          {8,  7,  3,  12, 12} },
    { "NEW YORK",      "KNICKS",        {21, 3,  19, 1,  5} },
    { "ORLANDO",       "MAGIC",         {6,  14, 15, 13, 9} },
    { "PHILADELPHIA",  "76ERS",         {22, 22, 20, 22, 22} },
    { "PHOENIX",       "SUNS",          {1,  4,  12, 14, 3} },
    { "PORTLAND",      "TRAIL BLAZERS", {3,  2,  6,  18, 14} },
    { "SACRAMENTO",    "KINGS",         {14, 16, 16, 25, 21} },
    { "SAN ANTONIO",   "SPURS",         {20, 1,  5,  2,  4} },
    { "SEATTLE",       "SUPERSONICS",   {5,  23, 10, 6,  1} },
    { "UTAH",          "JAZZ",          {10, 18, 2,  9,  8} },
    { "WASHINGTON",    "BULLETS",       {18, 26, 22, 26, 23} },
    /* $80:D21D/$80:DA36 and $80:D222/$80:DA3B. Values beyond the
     * 27-team ordinal range render as the ROM's dash glyph. */
    { "EAST",          "ALL-STARS",      {28, 28, 28, 28, 28} },
    { "WEST",          "ALL-STARS",      {29, 29, 29, 29, 29} }
};

const NbaTeamRecord *nba_team_record(uint8_t team) {
    return team < NBA_TEAM_COUNT ? &nba_team_records[team] : NULL;
}

static const NbaAssetItem *team_asset(const NbaTeamSelect *screen,
                                      NbaAssetId base, uint8_t team) {
    return nba_assets_get(screen->assets, (NbaAssetId)((int)base + team));
}

bool nba_team_select_init(NbaTeamSelect *screen, const NbaAssetPack *assets,
                          NbaSession *session, const uint32_t *outgoing_pixels) {
    if (!screen || !assets || !session) return false;
    memset(screen, 0, sizeof(*screen));
    screen->assets = assets;
    screen->session = session;
    screen->active_side = NBA_TEAM_SIDE_RIGHT;
    /* $82:8360-$8370 initializes $16B5=2 and $1693=1. */
    screen->selector = NBA_TEAM_SELECT_RIGHT_NAME;
    screen->outgoing_pixels = (uint32_t *)malloc(
        (size_t)NBA_SNES_WIDTH * NBA_SNES_HEIGHT * sizeof(uint32_t));
    if (!screen->outgoing_pixels) return false;
    if (outgoing_pixels) {
        memcpy(screen->outgoing_pixels, outgoing_pixels,
               (size_t)NBA_SNES_WIDTH * NBA_SNES_HEIGHT * sizeof(uint32_t));
    } else {
        memset(screen->outgoing_pixels, 0,
               (size_t)NBA_SNES_WIDTH * NBA_SNES_HEIGHT * sizeof(uint32_t));
    }
    for (uint8_t team = 0; team < NBA_TEAM_COUNT; ++team) {
        const NbaAssetItem *logo = team_asset(screen, NBA_ASSET_TEAM_LOGO_BASE, team);
        const NbaAssetItem *vram = team_asset(screen, NBA_ASSET_TEAM_VRAM_BASE, team);
        const NbaAssetItem *cgram = team_asset(screen, NBA_ASSET_TEAM_CGRAM_BASE, team);
        if (!logo || logo->size != 48u * 56u * 4u ||
            !vram || vram->size != 0x10000u || !cgram || cgram->size != 0x200u) {
            nba_team_select_shutdown(screen);
            return false;
        }
    }
    const NbaAssetItem *plate_cycle = nba_assets_get(
        assets, NBA_ASSET_TEAM_SELECTED_PALETTE_CYCLE);
    if (!plate_cycle || plate_cycle->size != 26u) {
        nba_team_select_shutdown(screen);
        return false;
    }
    screen->is_initialized = true;
    return true;
}

void nba_team_select_shutdown(NbaTeamSelect *screen) {
    if (!screen) return;
    free(screen->outgoing_pixels);
    screen->outgoing_pixels = NULL;
    screen->is_initialized = false;
}

static uint8_t team_for_rank(NbaTeamRankCategory category, uint8_t rank) {
    for (uint8_t team = 0; team < NBA_REGULAR_TEAM_COUNT; ++team)
        if (nba_team_records[team].rank[category] == rank) return team;
    return 0;
}

static NbaTeamSelectPosition active_name_position(const NbaTeamSelect *screen) {
    return screen->active_side == NBA_TEAM_SIDE_LEFT ?
           NBA_TEAM_SELECT_LEFT_NAME : NBA_TEAM_SELECT_RIGHT_NAME;
}

static bool selected_rank_category(const NbaTeamSelect *screen,
                                   NbaTeamRankCategory *category) {
    if (screen->selector < NBA_TEAM_SELECT_SCORING ||
        screen->selector > NBA_TEAM_SELECT_OVERALL)
        return false;
    if (category)
        *category = (NbaTeamRankCategory)(screen->selector - NBA_TEAM_SELECT_SCORING);
    return true;
}

NbaTeamSelectSound nba_team_select_update(NbaTeamSelect *screen,
                                           const NbaInput *input) {
    if (!screen || !screen->is_initialized) return NBA_TEAM_SOUND_NONE;
    if (screen->transition_frame < NBA_TEAM_TRANSITION_FRAMES) {
        screen->transition_frame++;
        return NBA_TEAM_SOUND_NONE;
    }
    screen->steady_frame++;
    if (!input) return NBA_TEAM_SOUND_NONE;

    /* $82:83B7 BIT #$C0F0: A/B/X/Y/L/R all use the same side toggle. */
    if (input->pressed & (NBA_BTN_A | NBA_BTN_B | NBA_BTN_X | NBA_BTN_Y |
                          NBA_BTN_L | NBA_BTN_R)) {
        /* $82:83C3-$83D6 swaps 0/1 only when a name row is selected.
         * Ranking selectors 2..6 remain on the same ranking. */
        if (screen->selector < NBA_TEAM_SELECT_SCORING)
            screen->selector = screen->selector == NBA_TEAM_SELECT_LEFT_NAME ?
                               NBA_TEAM_SELECT_RIGHT_NAME : NBA_TEAM_SELECT_LEFT_NAME;
        screen->active_side = screen->active_side == NBA_TEAM_SIDE_LEFT ?
                              NBA_TEAM_SIDE_RIGHT : NBA_TEAM_SIDE_LEFT;
        return NBA_TEAM_SOUND_SIDE;
    }
    if (input->pressed & NBA_BTN_UP) {
        /* $82:8406-$844B: name -> Overall; Scoring -> active name. */
        if (screen->selector == active_name_position(screen))
            screen->selector = NBA_TEAM_SELECT_OVERALL;
        else if (screen->selector == NBA_TEAM_SELECT_SCORING)
            screen->selector = active_name_position(screen);
        else
            screen->selector = (NbaTeamSelectPosition)(screen->selector - 1);
        return NBA_TEAM_SOUND_CATEGORY;
    }
    if (input->pressed & NBA_BTN_DOWN) {
        /* $82:844C-$846D: Overall -> active name; name -> Scoring. */
        if (screen->selector == active_name_position(screen))
            screen->selector = NBA_TEAM_SELECT_SCORING;
        else if (screen->selector == NBA_TEAM_SELECT_OVERALL)
            screen->selector = active_name_position(screen);
        else
            screen->selector = (NbaTeamSelectPosition)(screen->selector + 1);
        return NBA_TEAM_SOUND_CATEGORY;
    }
    if (input->pressed & (NBA_BTN_LEFT | NBA_BTN_RIGHT)) {
        uint8_t *selected = screen->active_side == NBA_TEAM_SIDE_LEFT ?
                            &screen->session->left_team : &screen->session->right_team;
        NbaTeamRankCategory category;
        if (!selected_rank_category(screen, &category)) {
            /* $82:8477-$84A0/$82:84F7-$8513: name rows use team-ID order. */
            if (input->pressed & NBA_BTN_LEFT)
                *selected = *selected == 0 ? NBA_TEAM_COUNT - 1 : (uint8_t)(*selected - 1);
            else
                *selected = *selected + 1 >= NBA_TEAM_COUNT ? 0 : (uint8_t)(*selected + 1);
        } else {
            uint8_t rank = nba_team_records[*selected].rank[category];
            if (input->pressed & NBA_BTN_LEFT)
                rank = rank <= 1 ? NBA_REGULAR_TEAM_COUNT : (uint8_t)(rank - 1);
            else
                rank = rank >= NBA_REGULAR_TEAM_COUNT ? 1 : (uint8_t)(rank + 1);
            *selected = team_for_rank(category, rank);
        }
        return NBA_TEAM_SOUND_CHANGE;
    }
    return NBA_TEAM_SOUND_NONE;
}

static bool team_dynamic_bg3_pixel(int x, int y) {
    /* The ROM's left/right text writers can occupy the complete name field.
     * Short-name bounds hid this when the raw capture contained ORLANDO. */
    if (y >= 60 && y < 77 && x >= 76 && x < 182) return true;
    if (y >= 94 && y < 112 && x >= 76 && x < 182) return true;
    if (y >= 116 && y < 206 && ((x >= 34 && x < 75) || (x >= 188 && x < 232)))
        return true;
    return false;
}

static void team_render_layers(NbaRenderer *renderer, const uint8_t *vram,
                               const uint8_t *cgram, int bg1_hscroll,
                               int bg2_hscroll, int bg2_vscroll,
                               int bg3_vscroll, int brightness, bool show_bg3,
                               NbaTeamSelectPosition selector) {
    uint32_t backdrop = nba_snes_cgram_color(cgram, 0, brightness, 0, 0, 0);
    int category_top = selector >= NBA_TEAM_SELECT_SCORING ?
                       116 + ((int)selector - NBA_TEAM_SELECT_SCORING) * 18 : -100;
    bool entering = bg1_hscroll != 512 || bg2_hscroll != 0;
    for (int y = 0; y < NBA_SNES_HEIGHT; ++y) {
        for (int x = 0; x < NBA_SNES_WIDTH; ++x) {
            NbaSnesBgPixel pixel;
            uint32_t out = backdrop;
            if (nba_snes_sample_bg(vram, TEAM_BG2_MAP, TEAM_BG2_CHR, 4,
                                   true, false, bg2_hscroll, bg2_vscroll,
                                   x, y, &pixel) && !(entering && pixel.palette == 5))
                out = nba_snes_cgram_color(cgram, pixel.palette * 16 + pixel.color_index,
                                           brightness, 0, 0, 0);
            if (nba_snes_sample_bg(vram, TEAM_BG1_MAP, TEAM_BG1_CHR, 4,
                                   true, false, bg1_hscroll, 1023,
                                   x, y, &pixel) && (!entering ||
                                   (pixel.palette == 5 && y >= 16 && y < 56)))
                out = nba_snes_cgram_color(cgram, pixel.palette * 16 + pixel.color_index,
                                           brightness, 0, 0, 0);
            int source_bg3_y = (y + bg3_vscroll) & 511;
            if (show_bg3 && !team_dynamic_bg3_pixel(x, source_bg3_y) &&
                nba_snes_sample_bg(vram, TEAM_BG3_MAP, TEAM_BG3_CHR, 2,
                                   false, true, 0, bg3_vscroll, x, y, &pixel)) {
                bool highlighted = y >= category_top && y < category_top + 16 &&
                                   x >= 78 && x < 188;
                out = nba_snes_cgram_color(cgram, pixel.palette * 4 + pixel.color_index,
                                           brightness, 0, highlighted ? 11 : 0,
                                           highlighted ? 25 : 0);
            }
            renderer->pixels[y * NBA_SNES_WIDTH + x] = out;
        }
    }
}

static void team_draw_bg3_crop(NbaRenderer *renderer, const uint8_t *vram,
                               const uint8_t *cgram, int sx, int sy, int w, int h,
                               int dx, int dy, int brightness, bool highlighted) {
    for (int py = 0; py < h; ++py) for (int px = 0; px < w; ++px) {
        NbaSnesBgPixel pixel;
        if (!nba_snes_sample_bg(vram, TEAM_BG3_MAP, TEAM_BG3_CHR, 2, false, true,
                                0, 0, sx + px, sy + py, &pixel)) continue;
        int x = dx + px, y = dy + py;
        if (x >= 0 && x < NBA_SNES_WIDTH && y >= 0 && y < NBA_SNES_HEIGHT)
            renderer->pixels[y * NBA_SNES_WIDTH + x] = nba_snes_cgram_color(
                cgram, pixel.palette * 4 + pixel.color_index, brightness, 0,
                highlighted ? 11 : 0, highlighted ? 25 : 0);
    }
}

static void team_draw_rank_column(NbaRenderer *renderer, const uint8_t *vram,
                                  const uint8_t *cgram, int side, int y,
                                  int brightness, int selected_rank) {
    const int source_x = 188;
    const int destination_x = side == NBA_TEAM_SIDE_LEFT ? 36 : 188;
    for (int py = 0; py < 90; ++py) for (int px = 0; px < 44; ++px) {
        NbaSnesBgPixel pixel;
        if (!nba_snes_sample_bg(vram, TEAM_BG3_MAP, TEAM_BG3_CHR, 2,
                                false, true, 0, 0,
                                source_x + px, 116 + py, &pixel))
            continue;
        int x = destination_x + px, destination_y = y + py;
        int rank_top = selected_rank >= 0 ? selected_rank * 18 : -100;
        bool highlighted = py >= rank_top && py < rank_top + 16;
        if (x >= 0 && x < NBA_SNES_WIDTH && destination_y >= 0 &&
            destination_y < NBA_SNES_HEIGHT)
            renderer->pixels[destination_y * NBA_SNES_WIDTH + x] =
                nba_snes_cgram_color(cgram,
                    pixel.palette * 4 + pixel.color_index, brightness, 0,
                    highlighted ? 11 : 0, highlighted ? 25 : 0);
    }
}

static bool team_name_source_bounds(const uint8_t *vram, int *left, int *right) {
    int first = 182, last = 75;
    for (int y = 95; y < 111; ++y) {
        for (int x = 76; x < 182; ++x) {
            NbaSnesBgPixel pixel;
            if (nba_snes_sample_bg(vram, TEAM_BG3_MAP, TEAM_BG3_CHR, 2,
                                   false, true, 0, 0, x, y, &pixel)) {
                if (x < first) first = x;
                if (x > last) last = x;
            }
        }
    }
    if (last < first) return false;
    *left = first;
    *right = last;
    return true;
}

/* $82:863C-$8792 uses the right-aligned $81:A01F writer at X=$00B4;
 * $82:8793-$88D8 uses the left-aligned $81:9FD4 writer at X=$0050. The
 * packed per-team VRAM was captured from the right writer, so recover its
 * complete nontransparent span and re-anchor it for the requested side. */
static void team_draw_name(NbaRenderer *renderer, const uint8_t *vram,
                           const uint8_t *cgram, int side, int y,
                           int brightness, bool highlighted) {
    int source_left, source_right;
    if (!team_name_source_bounds(vram, &source_left, &source_right)) return;
    int width = source_right - source_left + 1;
    /* The text pixels land at X=80 on the left and finish at X=177 on the
     * right in the live captures (the writer origins include their margins). */
    int x = side == NBA_TEAM_SIDE_LEFT ? 80 : 180 - width;
    team_draw_bg3_crop(renderer, vram, cgram, source_left, 95, width, 16,
                       x, y, brightness, highlighted);
}

static int team_obj_pixel(const uint8_t *tile, int x, int y) {
    return ((tile[y * 2] >> (7 - x)) & 1) |
           (((tile[y * 2 + 1] >> (7 - x)) & 1) << 1) |
           (((tile[16 + y * 2] >> (7 - x)) & 1) << 2) |
           (((tile[17 + y * 2] >> (7 - x)) & 1) << 3);
}

static void team_draw_panel_sprite(NbaRenderer *renderer, const uint8_t *vram,
                                   const uint8_t *cgram, const uint8_t *oam,
                                   int index, int forced_palette) {
    int high = (oam[512 + index / 4] >> ((index & 3) * 2)) & 3;
    int x = oam[index * 4] | ((high & 1) << 8);
    if (x >= 256) x -= 512;
    int y = oam[index * 4 + 1], tile = oam[index * 4 + 2];
    int attr = oam[index * 4 + 3], size = (high & 2) ? 16 : 8;
    tile += (attr & 1) ? 256 : 0;
    for (int py = 0; py < size; ++py) for (int px = 0; px < size; ++px) {
        int sx = (attr & 0x40) ? size - 1 - px : px;
        int sy = (attr & 0x80) ? size - 1 - py : py;
        int subtile = tile + (sx >> 3) + (sy >> 3) * 16;
        size_t offset = 0xC000u + (size_t)subtile * 32u;
        if (offset + 32u > 0x10000u) continue;
        int color = team_obj_pixel(vram + offset, sx & 7, sy & 7);
        if (color && x + px >= 0 && x + px < NBA_SNES_WIDTH && y + py < NBA_SNES_HEIGHT) {
            renderer->pixels[(y + py) * NBA_SNES_WIDTH + x + px] =
                nba_snes_cgram_color(cgram,
                                     128 + forced_palette * 16 + color,
                                     15, 0, 0, 0);
        }
    }
}

static void team_draw_logo(NbaRenderer *renderer, const NbaAssetItem *asset,
                           int x, int y) {
    const uint32_t *pixels = (const uint32_t *)asset->data;
    for (int py = 0; py < 56; ++py) for (int px = 0; px < 48; ++px) {
        uint32_t color = pixels[py * 48 + px];
        if ((color >> 24) != 0 && x + px >= 0 && x + px < NBA_SNES_WIDTH &&
            y + py >= 0 && y + py < NBA_SNES_HEIGHT)
            renderer->pixels[(y + py) * NBA_SNES_WIDTH + x + px] = color;
    }
}

void nba_team_select_render(const NbaTeamSelect *screen, NbaRenderer *renderer) {
    if (!screen || !screen->is_initialized || !renderer) return;
    int frame = screen->transition_frame;
    if (frame < 52) {
        int brightness = frame <= 20 ? 15 : 15 - (frame - 20) * 14 / 31;
        for (int i = 0; i < NBA_SNES_WIDTH * NBA_SNES_HEIGHT; ++i) {
            uint32_t color = screen->outgoing_pixels[i];
            uint32_t r = ((color >> 16) & 255u) * (uint32_t)brightness / 15u;
            uint32_t g = ((color >> 8) & 255u) * (uint32_t)brightness / 15u;
            uint32_t b = (color & 255u) * (uint32_t)brightness / 15u;
            renderer->pixels[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
        }
        return;
    }
    if (frame < 113) { nba_renderer_clear(renderer, 0xFF000000u); return; }

    /* The right side is the home team. Its tiles/palette own the wallpaper;
     * changing the visitor or merely moving the active cursor does not. */
    uint8_t home_team = screen->session->right_team;
    const NbaAssetItem *home_vram = team_asset(screen, NBA_ASSET_TEAM_VRAM_BASE, home_team);
    const NbaAssetItem *home_cgram = team_asset(screen, NBA_ASSET_TEAM_CGRAM_BASE, home_team);
    int k = frame - 113, bg1_h = 512, bg2_h = 0, bg3_v = 0, brightness = 15;
    bool show_bg3 = true, show_objects = true;
    if (k < 38) {
        bg1_h = 768 - k * 8; if (bg1_h < 512) bg1_h = 512;
        bg2_h = (768 + k * 8) & 1023;
        brightness = k + 2; if (brightness > 15) brightness = 15;
        show_bg3 = false; show_objects = false;
    } else if (k < 62) {
        bg3_v = 280 - (k - 38) * 14; if (bg3_v < 0) bg3_v = 0;
        show_objects = k >= 58;
    }
    int bg2_v = k < 63 ? k / 3 : 21 + screen->steady_frame / 3;
    team_render_layers(renderer, home_vram->data, home_cgram->data,
                       bg1_h, bg2_h, bg2_v,
                       bg3_v, brightness, show_bg3, screen->selector);
    if (!show_bg3) return;

    uint8_t teams[2] = { screen->session->left_team, screen->session->right_team };
    for (int side = 0; side < 2; ++side) {
        const NbaAssetItem *vram = team_asset(screen, NBA_ASSET_TEAM_VRAM_BASE, teams[side]);
        const NbaAssetItem *cgram = team_asset(screen, NBA_ASSET_TEAM_CGRAM_BASE, teams[side]);
        int name_y = (side == 0 ? 63 : 95) - bg3_v;
        team_draw_name(renderer, vram->data, cgram->data, side, name_y,
                       brightness,
                       screen->selector == (NbaTeamSelectPosition)side);
        int selected_rank = side == (int)screen->active_side &&
                            screen->selector >= NBA_TEAM_SELECT_SCORING ?
                            (int)screen->selector - NBA_TEAM_SELECT_SCORING : -1;
        /* $82:85D1-$88D8 rebuilds all five ordinal values in the shared BG3
         * field. Copying its complete 44x90 ROM field preserves irregular
         * row baselines (notably Ball Control and Overall) and avoids clipped
         * scanlines from independent fixed-height row slices. */
        team_draw_rank_column(renderer, vram->data, cgram->data, side,
                              116 - bg3_v, brightness, selected_rank);
    }
    if (!show_objects) return;
    const NbaAssetItem *oam = nba_assets_get(screen->assets, NBA_ASSET_TEAM_SELECT_OAM);
    const NbaAssetItem *plate_cycle = nba_assets_get(
        screen->assets, NBA_ASSET_TEAM_SELECTED_PALETTE_CYCLE);
    if (oam && oam->size == 0x220u && plate_cycle && plate_cycle->size == 26u) {
        int left_palette = screen->active_side == NBA_TEAM_SIDE_LEFT ? 2 : 1;
        int right_palette = screen->active_side == NBA_TEAM_SIDE_RIGHT ? 2 : 1;
        int animation_frame = screen->transition_frame + screen->steady_frame;
        int plate_elapsed = animation_frame - NBA_TEAM_PLATE_ANIM_START_FRAME;
        int plate_counter = plate_elapsed >= 0 ?
            plate_elapsed % NBA_TEAM_PLATE_ANIM_PERIOD + 1 : 1;
        int source_offset = (plate_counter & 0x38) >> 2;
        /* $82:8942-$894A maps the $0E source-offset sentinel back to zero. */
        if (source_offset == 14) source_offset = 0;
        uint8_t selected_cgram[0x200];
        memcpy(selected_cgram, home_cgram->data, sizeof(selected_cgram));
        /* $82:8933-$8967 queues 14 bytes from $82:8968+phase*2 to
         * CGRAM $A1. The overlapping source windows form a seven-color
         * rotation with an eight-frame cadence; OAM itself never moves. */
        memcpy(selected_cgram + 0x142,
               (const uint8_t *)plate_cycle->data + source_offset, 14);
        /* In the team-18 geometry asset, the variable right logo occupies
         * 0..5, its fixed 15-piece plate is 6..20, the left logo is 21..31,
         * and its fixed plate is 32..46. Logo assets are decoded separately
         * by palette so none of these ranges may be approximated. */
        for (int i = 20; i >= 6; --i)
            team_draw_panel_sprite(renderer, home_vram->data,
                                   right_palette == 2 ? selected_cgram : home_cgram->data,
                                   oam->data, i, right_palette);
        for (int i = 46; i >= 32; --i)
            team_draw_panel_sprite(renderer, home_vram->data,
                                   left_palette == 2 ? selected_cgram : home_cgram->data,
                                   oam->data, i, left_palette);
    }
    team_draw_logo(renderer, team_asset(screen, NBA_ASSET_TEAM_LOGO_BASE, teams[0]), 30, 62);
    team_draw_logo(renderer, team_asset(screen, NBA_ASSET_TEAM_LOGO_BASE, teams[1]), 182, 62);
}
