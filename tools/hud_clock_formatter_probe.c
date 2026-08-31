/* Controlled native-entry replay only. Each line supplies the exact eight
 * captured WRAM words and eight clock-text bytes before BAF5/BB59. This is
 * deliberately separate from normal publisher sequencing and scheduling. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_gameplay_hud.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    NbaAssetPack assets = {0};
    if (!nba_assets_load(&assets, argv[1])) return 3;
    unsigned count = 0;
    for (;;) {
        unsigned routine, values[16];
        int parsed = scanf("%u", &routine);
        if (parsed == EOF) break;
        if (parsed != 1 || (routine != 0x87BAF5u && routine != 0x87BB59u) || count >= 128u)
            return 4;
        for (unsigned i = 0; i < 16u; ++i)
            if (scanf("%u", values + i) != 1 || values[i] > (i < 8u ? 65535u : 255u))
                return 5;
        NbaGameplayHud hud;
        if (!nba_gameplay_hud_init(&hud, &assets)) return 6;
        NbaGameplayHudInput input = {0};
        input.clock_gate_raw_492b = (uint16_t)values[0];
        input.presentation_timer_raw_08de = (uint16_t)values[1];
        input.presentation_kind_raw_08e8 = (uint16_t)values[2];
        hud.clock_mirror_raw_08f6 = (uint16_t)values[3];
        input.clock_raw_0928 = (uint16_t)values[4];
        input.clock_snapshot_raw_092a = (uint16_t)values[5];
        input.dead_ball_busy_raw_09b4 = (uint16_t)values[6];
        input.event_bits_raw_13e7 = (uint16_t)values[7];
        for (unsigned i = 0; i < 8u; ++i)
            hud.clock_text_raw_4a60[i] = (uint8_t)values[8u + i];
        if (!nba_gameplay_hud_publish(&hud, &assets, routine, &input)) return 7;
        printf("HUD_CLOCK %u %u %u %u %u %u %u %u %u %u", ++count, routine,
            input.clock_gate_raw_492b, input.presentation_timer_raw_08de,
            input.presentation_kind_raw_08e8, hud.clock_mirror_raw_08f6,
            input.clock_raw_0928, input.clock_snapshot_raw_092a,
            input.dead_ball_busy_raw_09b4, input.event_bits_raw_13e7);
        for (unsigned i = 0; i < 8u; ++i) printf(" %u", hud.clock_text_raw_4a60[i]);
        putchar('\n');
    }
    nba_assets_free(&assets);
    return count ? 0 : 8;
}
