#define _CRT_SECURE_NO_WARNINGS
#include "nba_player_lab.h"
#include <stdio.h>

int main(int argc, char **argv) {
    NbaAssetPack pack = {0};
    if (argc != 2 || !nba_assets_load(&pack, argv[1])) return 2;
    static const uint8_t directions[6] = {3u, 7u, 4u, 0u, 2u, 6u};
    while (1) {
        unsigned jersey[10];
        if (scanf("%x", &jersey[0]) != 1) break;
        for (unsigned i = 1; i < 10u; ++i)
            if (scanf("%x", &jersey[i]) != 1) return 3;
        for (unsigned actor = 0; actor < 10u; ++actor) {
            for (unsigned view = 0; view < 6u; ++view) {
                uint8_t tile[32];
                if (!nba_player_compose_jersey_number(
                        &pack, (uint8_t)jersey[actor], directions[view],
                        actor < 5u ? 0u : 1u, tile)) return 4;
                for (unsigned byte = 0; byte < 32u; ++byte)
                    printf("%02x", tile[byte]);
            }
        }
        putchar('\n');
    }
    nba_assets_free(&pack);
    return 0;
}
