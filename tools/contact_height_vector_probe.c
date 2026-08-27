/* Replays the resource-derived body height written by `$87:A60D-$A6B2`. */
#include <stdio.h>
#include "nba_assets.h"
#include "nba_player_lab.h"

int main(int argc, char **argv) {
    NbaAssetPack assets;
    if (argc != 2 || !nba_assets_load(&assets, argv[1])) return 2;
    unsigned upper, lower;
    while (scanf_s("%x %x", &upper, &lower) == 2) {
        uint16_t height;
        if (!nba_player_animation_contact_height_from_resources(
                &assets, (uint16_t)upper, (uint16_t)lower, &height))
            puts("unsupported");
        else
            printf("%04x\n", height);
    }
    nba_assets_free(&assets);
    return 0;
}
