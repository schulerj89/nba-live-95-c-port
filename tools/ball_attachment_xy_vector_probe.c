/* Replays `$87:B649-$B669` actor-relative ball X/Y composition. */
#include <stdio.h>
#include "nba_assets.h"
#include "nba_player_lab.h"

int main(int argc, char **argv) {
    NbaAssetPack assets;
    if (argc != 2 || !nba_assets_load(&assets, argv[1])) return 2;
    unsigned upper, lower, flags, actor_x, actor_y;
    while (scanf_s("%x %x %x %x %x", &upper, &lower, &flags,
                   &actor_x, &actor_y) == 5) {
        int16_t x, y, z;
        if (!nba_player_ball_attachment_point_offsets(
                &assets, (uint16_t)upper, (uint16_t)lower,
                (uint16_t)flags, 0u, &x, &y, &z)) {
            puts("unsupported");
        } else {
            printf("%04x %04x\n", (uint16_t)(actor_x + (uint16_t)x),
                   (uint16_t)(actor_y + (uint16_t)y));
        }
    }
    nba_assets_free(&assets);
    return 0;
}
