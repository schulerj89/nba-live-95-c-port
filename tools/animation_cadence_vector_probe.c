/* Replays the common `$87:AAB2-$AD5A` descriptor cadence through production
 * asset-pack code. The separate upper mode-2 `$87:AD5B` helper is reported
 * as unsupported rather than silently approximated. */
#include <stdint.h>
#include <stdio.h>

#include "nba_assets.h"
#include "nba_player_lab.h"

int main(int argc, char **argv) {
    NbaAssetPack assets;
    if (argc != 2 || !nba_assets_load(&assets, argv[1])) return 2;
    unsigned upper, lower, direction, speed, alternate, variant;
    unsigned upper_acc, lower_acc, upper_phase, lower_phase;
    while (scanf_s("%x %x %x %x %x %x %x %x %x %x", &upper, &lower,
                 &direction, &speed, &alternate, &variant, &upper_acc, &lower_acc,
                   &upper_phase, &lower_phase) == 10) {
        uint16_t ua = (uint16_t)upper_acc, la = (uint16_t)lower_acc;
        uint16_t up = (uint16_t)upper_phase, lp = (uint16_t)lower_phase;
        uint16_t ur = 0u, lr = 0u;
        if (!nba_player_animation_rom_step(
                &assets, (uint8_t)upper, (uint8_t)lower,
                (uint8_t)direction, (uint16_t)speed, alternate != 0u,
                (uint16_t)variant,
                &ua, &la, &up, &lp, &ur, &lr)) {
            puts("unsupported");
        } else {
            printf("%04x %04x %04x %04x %04x %04x\n",
                   ua, la, up, lp, ur, lr);
        }
    }
    nba_assets_free(&assets);
    return 0;
}
