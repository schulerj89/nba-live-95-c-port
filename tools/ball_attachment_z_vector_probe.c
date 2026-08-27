/* Replays `$87:B66A-$B67B` actor-relative ball Z composition. */
#include <stdio.h>
#include "nba_assets.h"
#include "nba_player_lab.h"

int main(int argc, char **argv) {
    NbaAssetPack assets;
    if (argc != 2 || !nba_assets_load(&assets, argv[1])) return 2;
    unsigned upper, lower, flags, actor_z;
    while (scanf_s("%x %x %x %x", &upper, &lower, &flags, &actor_z) == 4) {
        int16_t x, y, z;
        if (!nba_player_ball_attachment_point_offsets(
                &assets, (uint16_t)upper, (uint16_t)lower,
                (uint16_t)flags, 0u, &x, &y, &z))
            puts("unsupported");
        else
            printf("%04x\n", (uint16_t)(actor_z + (uint16_t)z));
    }
    nba_assets_free(&assets);
    return 0;
}
