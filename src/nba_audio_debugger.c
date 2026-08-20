#include "nba_audio_debugger.h"
#include "nba_audio.h"
#include "nba_font.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void draw_rect_fill(NbaRenderer *ren, int x, int y, int w, int h, uint32_t color) {
    if (!ren || !ren->pixels) return;
    int x1 = (x < 0) ? 0 : x;
    int y1 = (y < 0) ? 0 : y;
    int x2 = (x + w > NBA_SNES_WIDTH) ? NBA_SNES_WIDTH : x + w;
    int y2 = (y + h > NBA_SNES_HEIGHT) ? NBA_SNES_HEIGHT : y + h;

    for (int py = y1; py < y2; py++) {
        for (int px = x1; px < x2; px++) {
            ren->pixels[py * NBA_SNES_WIDTH + px] = color;
        }
    }
}

static void draw_rect_outline(NbaRenderer *ren, int x, int y, int w, int h, uint32_t color) {
    if (!ren || !ren->pixels) return;
    for (int px = x; px < x + w && px < NBA_SNES_WIDTH; px++) {
        if (y >= 0 && y < NBA_SNES_HEIGHT) ren->pixels[y * NBA_SNES_WIDTH + px] = color;
        if (y + h - 1 >= 0 && y + h - 1 < NBA_SNES_HEIGHT) ren->pixels[(y + h - 1) * NBA_SNES_WIDTH + px] = color;
    }
    for (int py = y; py < y + h && py < NBA_SNES_HEIGHT; py++) {
        if (x >= 0 && x < NBA_SNES_WIDTH) ren->pixels[py * NBA_SNES_WIDTH + x] = color;
        if (x + w - 1 >= 0 && x + w - 1 < NBA_SNES_WIDTH) ren->pixels[py * NBA_SNES_WIDTH + (x + w - 1)] = color;
    }
}

static void get_sample_info(const NbaAssetItem *item, char *name_buf, size_t name_size, float *out_dur) {
    if (!item) {
        snprintf(name_buf, name_size, "UNKNOWN");
        if (out_dur) *out_dur = 0.0f;
        return;
    }

    float dur = 0.0f;
    if (item->data && item->size > 44) {
        const uint8_t *b = (const uint8_t *)item->data;
        if (memcmp(b, "RIFF", 4) == 0 && memcmp(b + 8, "WAVE", 4) == 0) {
            uint32_t rate = *(const uint32_t *)(b + 24);
            uint32_t data_len = *(const uint32_t *)(b + 40);
            if (rate > 0) {
                dur = (float)(data_len / 2) / (float)rate;
            }
        }
    }
    if (out_dur) *out_dur = dur;

    switch (item->id) {
        case NBA_ASSET_AUDIO_EA_INTRO:
            snprintf(name_buf, name_size, "EA INTRO COMP");
            break;
        case NBA_ASSET_AUDIO_EA_E:
            snprintf(name_buf, name_size, "VOICE: 'E'");
            break;
        case NBA_ASSET_AUDIO_EA_A:
            snprintf(name_buf, name_size, "VOICE: 'A'");
            break;
        case NBA_ASSET_AUDIO_EA_SPORTS:
            snprintf(name_buf, name_size, "VOICE: 'SPORTS'");
            break;
        case NBA_ASSET_AUDIO_EA_GAME:
            snprintf(name_buf, name_size, "VOICE: 'GAME'");
            break;
        default:
            if (item->flags != 0) {
                snprintf(name_buf, name_size, "0x%06X SMPL", item->flags);
            } else {
                snprintf(name_buf, name_size, "SAMPLE #%02u", item->id);
            }
            break;
    }
}

/**
 * Offset/Address/Size: N/A | Host UI Tool | size: N/A
 * Purpose: Initializes the in-game audio debugger state.
 */
void nba_audio_debugger_init(NbaAudioDebugger *dbg) {
    if (!dbg) return;
    memset(dbg, 0, sizeof(NbaAudioDebugger));
    dbg->is_active = false;
    dbg->selected_index = 0;
    dbg->scroll_offset = 0;
    dbg->total_audio_items = 0;
}

/**
 * Offset/Address/Size: N/A | Host Key Handler (VK_F11) | size: N/A
 * Purpose: Toggles the audio sample browser overlay and pauses/unpauses gameplay.
 */
void nba_audio_debugger_toggle(NbaAudioDebugger *dbg) {
    if (!dbg) return;
    dbg->is_active = !dbg->is_active;
    if (dbg->is_active) {
        printf("[DEBUGGER] Audio Sample Debugger activated (Press F11 to close).\n");
    } else {
        printf("[DEBUGGER] Audio Sample Debugger closed.\n");
    }
}

/**
 * Offset/Address/Size: N/A | Host Input / Sample Navigation | size: N/A
 * Purpose: Scans loaded audio assets, processes Up/Down selection, and triggers instant sample playback.
 */
void nba_audio_debugger_update(NbaAudioDebugger *dbg, const NbaAssetPack *assets, const NbaInput *input) {
    if (!dbg || !assets || !input) return;

    /* Build table of audio asset indices from pack */
    dbg->total_audio_items = 0;
    if (assets->is_loaded) {
        for (uint32_t i = 0; i < assets->item_count && i < NBA_ASSET_MAX; i++) {
            const NbaAssetItem *item = &assets->items[i];
            if (item->data && item->size > 44) {
                const uint8_t *b = (const uint8_t *)item->data;
                if (memcmp(b, "RIFF", 4) == 0 && memcmp(b + 8, "WAVE", 4) == 0) {
                    dbg->audio_item_indices[dbg->total_audio_items++] = i;
                }
            }
        }
    }

    if (!dbg->is_active) return;

    if (dbg->total_audio_items == 0) return;

    /* Handle Up / Down navigation */
    if (input->pressed & NBA_BTN_UP) {
        if (dbg->selected_index > 0) {
            dbg->selected_index--;
        } else {
            dbg->selected_index = dbg->total_audio_items - 1;
        }
    }
    if (input->pressed & NBA_BTN_DOWN) {
        if (dbg->selected_index + 1 < dbg->total_audio_items) {
            dbg->selected_index++;
        } else {
            dbg->selected_index = 0;
        }
    }

    /* Auto adjust scroll window (show 7 items at a time) */
    const int visible_count = 7;
    if (dbg->selected_index < dbg->scroll_offset) {
        dbg->scroll_offset = dbg->selected_index;
    }
    if (dbg->selected_index >= dbg->scroll_offset + visible_count) {
        dbg->scroll_offset = dbg->selected_index - visible_count + 1;
    }

    /* Handle Play trigger on Start / A / B */
    if (input->pressed & (NBA_BTN_START | NBA_BTN_A | NBA_BTN_B)) {
        if (dbg->selected_index >= 0 && dbg->selected_index < dbg->total_audio_items) {
            uint32_t pack_idx = dbg->audio_item_indices[dbg->selected_index];
            const NbaAssetItem *item = &assets->items[pack_idx];
            if (item && item->data && item->size > 0) {
                char name[64];
                float dur = 0.0f;
                get_sample_info(item, name, sizeof(name), &dur);
                printf("[DEBUGGER] Playing: [%02d/%02d] %s (%.2fs, %u bytes)\n",
                       dbg->selected_index + 1, dbg->total_audio_items, name, dur, item->size);
                nba_audio_play_wav(item->data, (size_t)item->size);
            }
        }
    }
}

/**
 * Offset/Address/Size: N/A | Host UI / Waveform Oscilloscope Render | size: N/A
 * Purpose: Renders the interactive audio debugger HUD, sample directory, and 16-bit PCM waveform oscilloscope graph.
 */
void nba_audio_debugger_render(const NbaAudioDebugger *dbg, const NbaAssetPack *assets, NbaRenderer *ren) {
    if (!dbg || !dbg->is_active || !assets || !ren) return;

    /* 1. Backdrop Panel */
    draw_rect_fill(ren, 4, 4, 248, 216, 0xFF0D1117); /* Dark Slate Background */
    draw_rect_outline(ren, 4, 4, 248, 216, 0xFF58A6FF); /* Blue Border */
    draw_rect_outline(ren, 5, 5, 246, 214, 0xFF21262D);

    /* 2. Title & Instructions */
    nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, 10, 10, "AUDIO DEBUGGER [F11]", 0xFF58A6FF, 0xFF000000, 1);
    nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, 10, 21, "UP/DN:CHOOSE  A/ENT:PLAY", 0xFF8B949E, 0xFF000000, 1);
    draw_rect_fill(ren, 8, 31, 240, 1, 0xFF30363D);

    if (dbg->total_audio_items == 0) {
        nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, 16, 60, "NO AUDIO LOADED IN PACK", 0xFFF85149, 0xFF000000, 1);
        return;
    }

    /* 3. Render Sample List */
    const int visible_count = 7;
    int list_y = 36;
    for (int i = 0; i < visible_count; i++) {
        int item_idx = dbg->scroll_offset + i;
        if (item_idx >= dbg->total_audio_items) break;

        uint32_t pack_idx = dbg->audio_item_indices[item_idx];
        const NbaAssetItem *item = &assets->items[pack_idx];

        char name[48];
        float dur = 0.0f;
        get_sample_info(item, name, sizeof(name), &dur);

        char line[64];
        bool is_selected = (item_idx == dbg->selected_index);

        if (is_selected) {
            draw_rect_fill(ren, 8, list_y - 2, 240, 11, 0xFF1F6FEB);
            snprintf(line, sizeof(line), "> %02d %-16.16s %4.2fs", item_idx + 1, name, dur);
            nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, 10, list_y, line, 0xFFFFFFFF, 0xFF000000, 1);
        } else {
            snprintf(line, sizeof(line), "  %02d %-16.16s %4.2fs", item_idx + 1, name, dur);
            nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, 10, list_y, line, 0xFFC9D1D9, 0xFF000000, 1);
        }
        list_y += 12;
    }

    /* 4. Selected Sample Details Box */
    draw_rect_fill(ren, 8, 122, 240, 1, 0xFF30363D);

    uint32_t sel_pack_idx = dbg->audio_item_indices[dbg->selected_index];
    const NbaAssetItem *sel_item = &assets->items[sel_pack_idx];
    char sel_name[64];
    float sel_dur = 0.0f;
    get_sample_info(sel_item, sel_name, sizeof(sel_name), &sel_dur);

    char meta_line1[64];
    snprintf(meta_line1, sizeof(meta_line1), "[%02d/%02d] %-20.20s",
             dbg->selected_index + 1, dbg->total_audio_items, sel_name);
    nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, 10, 126, meta_line1, 0xFFF0883E, 0xFF000000, 1);

    char meta_line2[64];
    snprintf(meta_line2, sizeof(meta_line2), "SIZE:%-7u B  TIME:%.2fS", sel_item->size, sel_dur);
    nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, 10, 137, meta_line2, 0xFF7EE787, 0xFF000000, 1);

    /* 5. Audio Waveform Oscilloscope */
    int wave_x = 10;
    int wave_y = 150;
    int wave_w = 236;
    int wave_h = 62;

    draw_rect_fill(ren, wave_x, wave_y, wave_w, wave_h, 0xFF040D1A);
    draw_rect_outline(ren, wave_x, wave_y, wave_w, wave_h, 0xFF388BFD);

    /* Center zero line */
    int mid_y = wave_y + (wave_h / 2);
    for (int px = wave_x + 1; px < wave_x + wave_w - 1; px++) {
        ren->pixels[mid_y * NBA_SNES_WIDTH + px] = 0xFF1B385C;
    }

    /* Render actual 16-bit PCM waveform bars if valid WAV */
    if (sel_item && sel_item->data && sel_item->size > 44) {
        const uint8_t *wav_bytes = (const uint8_t *)sel_item->data;
        const int16_t *pcm_data = (const int16_t *)(wav_bytes + 44);
        int total_pcm_samples = (int)(sel_item->size - 44) / 2;

        if (total_pcm_samples > 0) {
            float step = (float)total_pcm_samples / (float)(wave_w - 2);
            int max_amp = (wave_h / 2) - 3;

            for (int col = 0; col < wave_w - 2; col++) {
                int sample_start = (int)(col * step);
                int sample_end = (int)((col + 1) * step);
                if (sample_end <= sample_start) sample_end = sample_start + 1;
                if (sample_end > total_pcm_samples) sample_end = total_pcm_samples;

                int16_t min_v = 0, max_v = 0;
                for (int s = sample_start; s < sample_end; s++) {
                    int16_t v = pcm_data[s];
                    if (v < min_v) min_v = v;
                    if (v > max_v) max_v = v;
                }

                int top_offset = (int)((float)(-max_v) / 32768.0f * (float)max_amp);
                int bot_offset = (int)((float)(-min_v) / 32768.0f * (float)max_amp);

                int y_top = mid_y + top_offset;
                int y_bot = mid_y + bot_offset;

                if (y_top > y_bot) {
                    int tmp = y_top; y_top = y_bot; y_bot = tmp;
                }
                if (y_top < wave_y + 1) y_top = wave_y + 1;
                if (y_bot >= wave_y + wave_h - 1) y_bot = wave_y + wave_h - 2;

                int cur_x = wave_x + 1 + col;
                for (int py = y_top; py <= y_bot; py++) {
                    ren->pixels[py * NBA_SNES_WIDTH + cur_x] = 0xFF58A6FF;
                }
            }
        }
    }
}
