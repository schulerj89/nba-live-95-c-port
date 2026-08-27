/* Replays `$87:B832-$B952` pose-point composition through production C. */
#include <stdio.h>
#include "nba_assets.h"
#include "nba_player_lab.h"

int main(int argc, char **argv) {
    NbaAssetPack assets;
    if (argc != 2 || !nba_assets_load(&assets, argv[1])) return 2;
    unsigned upper, lower, flags, selector;
    while (scanf_s("%x %x %x %x", &upper, &lower, &flags, &selector) == 4) {
        int16_t x, y, z;
        if (!nba_player_ball_attachment_point_offsets(
                &assets, (uint16_t)upper, (uint16_t)lower,
                (uint16_t)flags, (uint8_t)selector, &x, &y, &z))
            puts("unsupported");
        else
            printf("%04x %04x %04x\n", (uint16_t)x, (uint16_t)y,
                   (uint16_t)z);
    }
    nba_assets_free(&assets);
    return 0;
}
