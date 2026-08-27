/* Independent Mesen entry states -> production C; no oracle logic here. */
#include <stdio.h>
#include <stdint.h>
#include "nba_player_lab.h"
#include "nba_gameplay_ball.h"

int main(int argc, char **argv) {
    NbaAssetPack assets;
    if (argc != 2 || !nba_assets_load(&assets, argv[1])) return 2;
    unsigned v[27];
    while (scanf_s("%x", &v[0]) == 1) {
        for (unsigned i=1;i<27;++i) if (scanf_s("%x", &v[i]) != 1) return 3;
        if (v[0] == 7) {
            uint8_t facing=(uint8_t)v[6];
            uint8_t pose=nba_gameplay_owner_latched_pose((int16_t)v[1],
                (uint16_t)v[2],(uint16_t)v[3],(uint16_t)v[4],(uint8_t)v[5],&facing);
            printf("%04x %04x\n",pose,facing);
            continue;
        }
        if (v[0] != 6) return 4;
        NbaPlayerAnimationChannels c = {
            (uint16_t)v[9], (uint16_t)v[10], (uint16_t)v[11], (uint16_t)v[12],
            (uint16_t)v[13], (uint16_t)v[14], (uint16_t)v[15], (uint16_t)v[16],
            (uint16_t)v[17], (uint16_t)v[18], (uint16_t)v[19], {0}, {0}, (uint16_t)v[26]
        };
        for(unsigned i=0;i<3;++i) {
            c.upper_queue[i]=(uint16_t)v[20+i];
            c.lower_queue[i]=(uint16_t)v[23+i];
        }
        uint16_t rng=(uint16_t)v[8],ur=0,lr=0;
        if(!nba_player_animation_step_channels(&assets,&c,(uint16_t)v[4],
            (uint16_t)v[5],(uint16_t)v[6],v[3]!=0,(uint16_t)v[7],&rng,&ur,&lr)) {
            puts("unsupported");continue;
        }
        printf("%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x",
            c.upper_queue_cursor,c.lower_queue_cursor,c.upper_state,c.lower_state,
            c.base_state,c.upper_phase,c.lower_phase,c.upper_accumulator,
            c.lower_accumulator,c.upper_lock,c.lower_lock);
        for(unsigned i=0;i<3;++i)printf(" %04x",c.upper_queue[i]);
        for(unsigned i=0;i<3;++i)printf(" %04x",c.lower_queue[i]);
        printf(" %04x %04x %04x %04x %04x\n",0,rng,ur,lr,c.upper_phase_target);
    }
    nba_assets_free(&assets);return 0;
}
