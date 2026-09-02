#include "nba_player_lab.h"
#include <stdio.h>

/* Native AF1E entry -> ordered B348 arguments; ordinary ball resource only. */
int main(int argc, char **argv) {
    NbaAssetPack assets;
    if (argc != 2 || !nba_assets_load(&assets, argv[1])) return 2;
    unsigned v[18];
    for (;;) {
        for (unsigned i = 0; i < 18; ++i) {
            if (scanf_s("%x", &v[i]) != 1) { nba_assets_free(&assets); return i ? 3 : 0; }
            if (v[i] > 65535u) return 4;
        }
        if (v[13] >= 0x082cu) return 5;
        NbaPlayerSpritePoseInput pose = {
            (uint16_t)v[0], (uint16_t)v[1], (uint16_t)v[2], (uint16_t)v[3],
            (uint16_t)v[4], (uint16_t)v[5], (uint16_t)v[6], (uint16_t)v[7],
            (uint16_t)v[8], (int16_t)v[9], (int16_t)v[10]
        };
        int16_t table_order;
        if (!nba_player_ball_draw_order(&assets, pose.upper_d6, &table_order) ||
            (uint16_t)table_order != v[11]) return 6;
        NbaPlayerSpriteBallInput ball = {
            table_order, (uint16_t)v[12], (uint16_t)v[15], (int16_t)v[16], (int16_t)v[17]
        };
        NbaPlayerSpritePoseComposition result;
        if (!nba_player_compose_sprite_pose_with_ball(&assets, &pose, &ball, &result)) return 7;
        printf("%04x", result.count);
        for (unsigned i = 0; i < result.count; ++i) {
            NbaPlayerSpriteSubmission *p = &result.parts[i];
            printf(" %04x %04x %04x %04x", p->resource, p->attribute,
                   (uint16_t)p->x, (uint16_t)p->y);
        }
        putchar('\n');
    }
}
