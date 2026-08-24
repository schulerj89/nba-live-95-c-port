#include "nba_player_setup.h"
#include "nba_snes_ppu.h"
#include <stdlib.h>
#include <string.h>

#define PLAYER_SETUP_BG1_MAP 0x1800
#define PLAYER_SETUP_BG1_CHR 0x6000
#define PLAYER_SETUP_BG2_MAP 0x1000
#define PLAYER_SETUP_BG2_CHR 0x2000
#define PLAYER_SETUP_BG3_MAP 0x0000
#define PLAYER_SETUP_BG3_CHR 0x8000
#define PLAYER_SETUP_HOME_WALLPAPER_OFFSET 0x20a0u
#define PLAYER_SETUP_HOME_WALLPAPER_SIZE   0x0320u

static const NbaAssetItem *player_setup_asset(const NbaPlayerSetup *screen,
                                               NbaAssetId id, uint32_t size) {
    const NbaAssetItem *item = nba_assets_get(screen->assets, id);
    return item && item->data && item->size == size ? item : NULL;
}

bool nba_player_setup_init(NbaPlayerSetup *screen, const NbaAssetPack *assets,
                           NbaSession *session, const uint32_t *outgoing_pixels) {
    if (!screen || !assets || !session) return false;
    memset(screen, 0, sizeof(*screen));
    screen->assets = assets;
    screen->session = session;
    /* The live Exhibition path enters with Player 1 assigned to home/right. */
    screen->player_one_side = (NbaTeamSide)session->player_one_side;
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

    const NbaAssetItem *base_vram = player_setup_asset(
        screen, NBA_ASSET_PLAYER_SETUP_VRAM, 0x10000u);
    const NbaAssetItem *home_vram = player_setup_asset(screen,
        (NbaAssetId)(NBA_ASSET_TEAM_VRAM_BASE + session->right_team), 0x10000u);
    if (!base_vram || !home_vram ||
        !player_setup_asset(screen, NBA_ASSET_PLAYER_SETUP_CGRAM, 0x200u) ||
        !player_setup_asset(screen, NBA_ASSET_PLAYER_SETUP_OAM, 0x220u) ||
        !player_setup_asset(screen, NBA_ASSET_TEAM_SELECTED_PALETTE_CYCLE, 26u)) {
        nba_player_setup_shutdown(screen);
        return false;
    }
    screen->scene_vram = (uint8_t *)malloc(0x10000u);
    if (!screen->scene_vram) {
        nba_player_setup_shutdown(screen);
        return false;
    }
    memcpy(screen->scene_vram, base_vram->data, 0x10000u);
    /* $82:863C uses the home-team pointer from $80:D0E2 to rebuild these
     * twenty-five BG2 tiles. Team Select's raw per-team VRAM assets preserve
     * the exact result, including each logo's grayscale/dither treatment. */
    memcpy(screen->scene_vram + PLAYER_SETUP_HOME_WALLPAPER_OFFSET,
           (const uint8_t *)home_vram->data + PLAYER_SETUP_HOME_WALLPAPER_OFFSET,
           PLAYER_SETUP_HOME_WALLPAPER_SIZE);
    for (uint8_t team = 0; team < NBA_TEAM_COUNT; ++team) {
        if (!player_setup_asset(screen,
                (NbaAssetId)(NBA_ASSET_TEAM_LOGO_BASE + team), 48u * 56u * 4u)) {
            nba_player_setup_shutdown(screen);
            return false;
        }
    }
    screen->is_initialized = true;
    return true;
}

void nba_player_setup_shutdown(NbaPlayerSetup *screen) {
    if (!screen) return;
    free(screen->outgoing_pixels);
    screen->outgoing_pixels = NULL;
    free(screen->scene_vram);
    screen->scene_vram = NULL;
    screen->is_initialized = false;
}

NbaPlayerSetupSound nba_player_setup_update(NbaPlayerSetup *screen,
                                             const NbaInput *input) {
    if (!screen || !screen->is_initialized) return NBA_PLAYER_SETUP_SOUND_NONE;
    if (screen->transition_frame < NBA_PLAYER_SETUP_TRANSITION_FRAMES) {
        screen->transition_frame++;
        return NBA_PLAYER_SETUP_SOUND_NONE;
    }
    screen->steady_frame++;
    if (!input) return NBA_PLAYER_SETUP_SOUND_NONE;

    /* $81:B62C-$B719 rebuilds the controller ownership row after a change. */
    if (input->pressed & (NBA_BTN_LEFT | NBA_BTN_RIGHT)) {
        NbaTeamSide requested = (input->pressed & NBA_BTN_LEFT) ?
                                NBA_TEAM_SIDE_LEFT : NBA_TEAM_SIDE_RIGHT;
        if (requested != screen->player_one_side) {
            screen->player_one_side = requested;
            screen->session->player_one_side = (uint8_t)requested;
            return NBA_PLAYER_SETUP_SOUND_MOVE;
        }
    }
    if (input->pressed & NBA_BTN_START) {
        screen->confirm_requested = true;
        return NBA_PLAYER_SETUP_SOUND_CONFIRM;
    }
    return NBA_PLAYER_SETUP_SOUND_NONE;
}

static int player_setup_obj_pixel(const uint8_t *tile, int x, int y) {
    return ((tile[y * 2] >> (7 - x)) & 1) |
           (((tile[y * 2 + 1] >> (7 - x)) & 1) << 1) |
           (((tile[16 + y * 2] >> (7 - x)) & 1) << 2) |
           (((tile[17 + y * 2] >> (7 - x)) & 1) << 3);
}

static void player_setup_draw_object(NbaRenderer *renderer, const uint8_t *vram,
                                     const uint8_t *cgram, const uint8_t *oam,
                                     int index, int forced_palette, int offset_x) {
    int high = (oam[512 + index / 4] >> ((index & 3) * 2)) & 3;
    int x = oam[index * 4] | ((high & 1) << 8);
    if (x >= 256) x -= 512;
    x += offset_x;
    int y = oam[index * 4 + 1], tile = oam[index * 4 + 2];
    int attr = oam[index * 4 + 3], size = (high & 2) ? 16 : 8;
    int palette = forced_palette >= 0 ? forced_palette : (attr >> 1) & 7;
    tile += (attr & 1) ? 256 : 0;
    for (int py = 0; py < size; ++py) for (int px = 0; px < size; ++px) {
        int sx = (attr & 0x40) ? size - 1 - px : px;
        int sy = (attr & 0x80) ? size - 1 - py : py;
        int subtile = tile + (sx >> 3) + (sy >> 3) * 16;
        size_t offset = 0xC000u + (size_t)subtile * 32u;
        if (offset + 32u > 0x10000u) continue;
        int color = player_setup_obj_pixel(vram + offset, sx & 7, sy & 7);
        int dx = x + px, dy = y + py;
        if (color && dx >= 0 && dx < NBA_SNES_WIDTH &&
            dy >= 0 && dy < NBA_SNES_HEIGHT) {
            renderer->pixels[dy * NBA_SNES_WIDTH + dx] = nba_snes_cgram_color(
                cgram, 128 + palette * 16 + color, 15, 0, 0, 0);
        }
    }
}

static void player_setup_draw_logo(NbaRenderer *renderer,
                                   const NbaAssetItem *logo, int x, int y) {
    const uint32_t *pixels = (const uint32_t *)logo->data;
    for (int py = 0; py < 56; ++py) for (int px = 0; px < 48; ++px) {
        uint32_t color = pixels[py * 48 + px];
        int dx = x + px, dy = y + py;
        if ((color >> 24) && dx >= 0 && dx < NBA_SNES_WIDTH &&
            dy >= 0 && dy < NBA_SNES_HEIGHT)
            renderer->pixels[dy * NBA_SNES_WIDTH + dx] = color;
    }
}

static void player_setup_render_layers(NbaRenderer *renderer,
                                       const uint8_t *vram,
                                       const uint8_t *cgram,
                                       int bg1_hscroll, int bg2_hscroll,
                                       int bg2_vscroll, int bg3_vscroll,
                                       int brightness, bool show_bg3) {
    uint32_t backdrop = nba_snes_cgram_color(cgram, 0, brightness, 0, 0, 0);
    bool entering = bg1_hscroll != 512 || bg2_hscroll != 0;
    for (int y = 0; y < NBA_SNES_HEIGHT; ++y) for (int x = 0; x < NBA_SNES_WIDTH; ++x) {
        NbaSnesBgPixel pixel;
        uint32_t out = backdrop;
        if (nba_snes_sample_bg(vram, PLAYER_SETUP_BG2_MAP, PLAYER_SETUP_BG2_CHR,
                               4, true, false, bg2_hscroll, bg2_vscroll,
                               x, y, &pixel) && !(entering && pixel.palette == 5))
            out = nba_snes_cgram_color(cgram,
                pixel.palette * 16 + pixel.color_index, brightness, 0, 0, 0);
        if (nba_snes_sample_bg(vram, PLAYER_SETUP_BG1_MAP, PLAYER_SETUP_BG1_CHR,
                               4, true, false, bg1_hscroll, 1023,
                               x, y, &pixel) && (!entering ||
                               (pixel.palette == 5 && y >= 16 && y < 56)))
            out = nba_snes_cgram_color(cgram,
                pixel.palette * 16 + pixel.color_index, brightness, 0, 0, 0);
        if (show_bg3 && nba_snes_sample_bg(vram, PLAYER_SETUP_BG3_MAP,
                                           PLAYER_SETUP_BG3_CHR, 2, false, true,
                                           0, bg3_vscroll, x, y, &pixel))
            out = nba_snes_cgram_color(cgram,
                pixel.palette * 4 + pixel.color_index, brightness, 0, 0, 0);
        renderer->pixels[y * NBA_SNES_WIDTH + x] = out;
    }
}

void nba_player_setup_render(const NbaPlayerSetup *screen, NbaRenderer *renderer) {
    if (!screen || !screen->is_initialized || !renderer) return;
    const NbaAssetItem *vram_asset = player_setup_asset(
        screen, NBA_ASSET_PLAYER_SETUP_VRAM, 0x10000u);
    const NbaAssetItem *cgram_asset = player_setup_asset(
        screen, NBA_ASSET_PLAYER_SETUP_CGRAM, 0x200u);
    const NbaAssetItem *oam_asset = player_setup_asset(
        screen, NBA_ASSET_PLAYER_SETUP_OAM, 0x220u);
    const uint8_t *vram = screen->scene_vram ? screen->scene_vram :
                          (const uint8_t *)vram_asset->data;
    const uint8_t *base_cgram = (const uint8_t *)cgram_asset->data;
    const uint8_t *oam = (const uint8_t *)oam_asset->data;
    int frame = screen->transition_frame;

    if (frame < 32) {
        int brightness = 15 - frame * 15 / 31;
        for (int index = 0; index < NBA_SNES_WIDTH * NBA_SNES_HEIGHT; ++index) {
            uint32_t color = screen->outgoing_pixels[index];
            uint32_t r = ((color >> 16) & 255u) * (uint32_t)brightness / 15u;
            uint32_t g = ((color >> 8) & 255u) * (uint32_t)brightness / 15u;
            uint32_t b = (color & 255u) * (uint32_t)brightness / 15u;
            renderer->pixels[index] = 0xFF000000u | (r << 16) | (g << 8) | b;
        }
        return;
    }
    if (frame < 117) {
        nba_renderer_clear(renderer, 0xFF000000u);
        return;
    }

    int k = frame - 117;
    int bg1_h = 512, bg2_h = 0, bg3_v = 0, brightness = 15;
    bool show_bg3 = true, show_objects = frame >= NBA_PLAYER_SETUP_TRANSITION_FRAMES;
    if (k < 38) {
        bg1_h = 768 - k * 8; if (bg1_h < 512) bg1_h = 512;
        bg2_h = (768 + k * 8) & 1023;
        brightness = k + 2; if (brightness > 15) brightness = 15;
        show_bg3 = false;
    } else if (k < 62) {
        bg3_v = 280 - (k - 38) * 14; if (bg3_v < 0) bg3_v = 0;
    }
    int bg2_v = 85 + screen->steady_frame / 3;
    player_setup_render_layers(renderer, vram, base_cgram, bg1_h, bg2_h,
                               bg2_v, bg3_v, brightness, show_bg3);
    if (!show_objects) return;

    uint8_t selected_cgram[0x200];
    memcpy(selected_cgram, base_cgram, sizeof(selected_cgram));
    const NbaAssetItem *cycle = player_setup_asset(
        screen, NBA_ASSET_TEAM_SELECTED_PALETTE_CYCLE, 26u);
    int source_offset = ((screen->steady_frame + 1) & 0x38) >> 2;
    if (source_offset == 14) source_offset = 0;
    memcpy(selected_cgram + 0x142, (const uint8_t *)cycle->data + source_offset, 14);

    int left_palette = screen->player_one_side == NBA_TEAM_SIDE_LEFT ? 2 : 1;
    int right_palette = screen->player_one_side == NBA_TEAM_SIDE_RIGHT ? 2 : 1;
    for (int index = 20; index >= 6; --index)
        player_setup_draw_object(renderer, vram,
            right_palette == 2 ? selected_cgram : base_cgram,
            oam, index, right_palette, 0);
    for (int index = 46; index >= 32; --index)
        player_setup_draw_object(renderer, vram,
            left_palette == 2 ? selected_cgram : base_cgram,
            oam, index, left_palette, 0);

    player_setup_draw_logo(renderer, nba_assets_get(screen->assets,
        (NbaAssetId)(NBA_ASSET_TEAM_LOGO_BASE + screen->session->left_team)), 36, 56);
    player_setup_draw_logo(renderer, nba_assets_get(screen->assets,
        (NbaAssetId)(NBA_ASSET_TEAM_LOGO_BASE + screen->session->right_team)), 168, 56);

    /* Capture group 47..51 is the arrow/controller. Move the complete group
     * together, matching the redraw path instead of leaving stale pieces. */
    int controller_offset = screen->player_one_side == NBA_TEAM_SIDE_LEFT ? -132 : 0;
    for (int index = 51; index >= 47; --index)
        player_setup_draw_object(renderer, vram, base_cgram, oam,
                                 index, -1, controller_offset);
}
