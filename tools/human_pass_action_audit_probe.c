#define _CRT_SECURE_NO_WARNINGS
#include "nba_human_pass_action.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    NbaAssetPack assets = {0};
    if (argc != 2 || !nba_assets_load(&assets, argv[1])) return 2;
    unsigned mode, relative, z, vz, profile, lower, distance, boost, vx, vy, upper, lock;
    while (scanf("%x %x %x %x %x %x %x %x %x %x %x %x", &mode, &relative,
        &z, &vz, &profile, &lower, &distance, &boost, &vx, &vy, &upper, &lock) == 12) {
        NbaHumanPassActionState s = {0};
        s.passer_slot = 2; s.receiver_slot = 7;
        s.relative_51 = (uint16_t)relative; s.distance_4f = (uint16_t)distance;
        s.coarse_be = 6; s.profile_3e = (uint8_t)profile;
        s.extra[2].z = (uint16_t)z; s.extra[2].velocity_z = (uint16_t)vz;
        s.extra[2].boost = (uint16_t)boost; s.extra[2].magnitude = 0x4321;
        s.extra[2].flags = 0x8001; s.extra[2].family = 0x1234;
        s.common.actors[2].velocity_x = (int16_t)vx;
        s.common.actors[2].velocity_y = (int16_t)vy;
        s.common.actors[2].animation.lower_state = (uint16_t)lower;
        s.common.actors[2].animation.upper_state = (uint16_t)upper;
        s.common.actors[2].animation.upper_lock = (uint16_t)lock;
        s.descriptor_47 = 0x1234; s.descriptor_bank_49 = 0xabcd;
        NbaHumanPassActionState before = s;
        unsigned route = mode ? (nba_human_pass_action_grounded(&assets, &s) ? 3u : 4u)
                              : (unsigned)nba_human_pass_action_select(&assets, &s);
        printf("%u %u %u %u %u %u %u %u %u %u %u %u %u %u\n", route,
            memcmp(&before, &s, sizeof(s)) != 0, s.common.actors[7].timer,
            s.extra[2].family, s.extra[2].flags, s.extra[2].pass_direction,
            s.extra[2].magnitude, (uint16_t)s.common.actors[2].velocity_x,
            (uint16_t)s.common.actors[2].velocity_y, s.request_00,
            s.common.actors[2].animation.upper_state, s.common.actors[2].animation.upper_lock,
            s.descriptor_47, s.descriptor_bank_49);
    }
    nba_assets_free(&assets);
    return ferror(stdin) ? 1 : 0;
}
