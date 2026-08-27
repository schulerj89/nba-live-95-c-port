#include <stdio.h>
#include "nba_player_lab.h"

int main(int argc, char **argv) {
    NbaAssetPack assets;
    if (argc != 2 || !nba_assets_load(&assets, argv[1])) return 2;
    unsigned op, v[20];
    while (scanf_s("%x", &op) == 1) {
        unsigned count = op == 0 ? 9u : 20u;
        if (op > 1u) return 3;
        for (unsigned i=0;i<count;++i) if (scanf_s("%x", &v[i]) != 1) return 3;
        if (op == 0) {
            NbaPlayerAnimationChannels c = {0};
            c.upper_state=(uint16_t)v[0]; c.lower_state=(uint16_t)v[1];
            c.upper_phase=(uint16_t)v[2]; c.lower_phase=(uint16_t)v[3];
            NbaPlayerResolvedPose p = {0};
            p.mirror_flags=(uint16_t)v[7]; p.direction=(uint16_t)v[8];
            if (!nba_player_resolve_pose(&assets,&c,(uint16_t)v[4],v[5]!=0,(uint16_t)v[6],&p)) return 4;
            printf("%04x %04x %04x %04x %04x %04x %04x %04x\n",p.mirror_flags,
                p.upper_resource,p.lower_resource,p.upper_state,p.lower_state,
                p.upper_phase,p.lower_phase,p.direction);
        } else {
            uint8_t teams[10], roster[10];
            for(unsigned i=0;i<10;++i) { teams[i]=(uint8_t)v[i*2]; roster[i]=(uint8_t)v[i*2+1]; }
            NbaPlayerAppearanceSetup setup;
            if(!nba_player_appearance_setup(&assets,teams,roster,&setup)) return 4;
            printf("%06x",(unsigned)setup.upload_address);
            for(unsigned i=0;i<10;++i) {
                NbaPlayerAppearance *p=&setup.players[i];
                printf(" %04x %04x %04x %04x %04x",p->palette_offset,
                    p->alternate_lower,p->upper_variant,p->head_resource,p->dirty);
            }
            puts("");
        }
    }
    nba_assets_free(&assets);
    return 0;
}
