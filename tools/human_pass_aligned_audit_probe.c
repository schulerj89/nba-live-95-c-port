#define _CRT_SECURE_NO_WARNINGS
#include "nba_human_pass_aligned.h"
#include <stdio.h>

static unsigned value(void) { unsigned v; if (scanf("%x", &v) != 1 || v > 65535u) return 0x10000u; return v; }
int main(int argc, char **argv) {
    NbaAssetPack assets={0};
    if(argc!=2 || !nba_assets_load(&assets,argv[1]))return 2;
    unsigned mode;
    while(scanf("%x",&mode)==1) {
        NbaHumanPassAlignedState s={0};
        s.action.passer_slot=0;s.action.receiver_slot=1;
        s.action.common.actors[0].animation.upper_state=5;
        if(mode==0) {
            NbaHumanPassLaneInput lane={0};
            for(unsigned i=0;i<11;++i) {
                lane.actors[i].x=(uint16_t)value();lane.actors[i].y=(uint16_t)value();lane.actors[i].team=(uint16_t)value();
            }
            for(unsigned i=0;i<13;++i)lane.order[i]=(uint16_t)value();
            lane.source_slot=(uint16_t)value();lane.receiver_slot=(uint16_t)value();
            lane.source_cursor=(uint16_t)value();lane.receiver_cursor=(uint16_t)value();
            uint16_t result=0x1234;bool ok=nba_human_pass_lane_obstructed(&lane,&result);
            printf("%u %u\n",ok?1u:0u,result);
        } else if(mode==1) {
            s.fine_c0=(uint16_t)value();s.action.common.actors[0].movement_direction=(uint16_t)value();s.action.relative_51=(uint16_t)value();
            s.action.common.actors[1].mode=14;
            bool ok=nba_human_pass_aligned_choose(&s);
            printf("%u %u %u\n",ok?1u:0u,s.action.request_00,s.family_ae);
        } else if(mode==2) {
            s.action.relative_51=(uint16_t)value();s.action.request_00=(uint16_t)value();s.action.distance_4f=(uint16_t)value();
            s.action.common.live_0936=(uint16_t)value();s.action.common.actors[0].velocity_x=(int16_t)value();s.action.common.actors[0].velocity_y=(int16_t)value();
            s.action.extra[0].z=(uint16_t)value();s.family_ae=(uint16_t)value();
            unsigned route=(unsigned)nba_human_pass_aligned_install(&assets,&s);
            printf("%u %u %u %u\n",route,s.action.request_00,s.action.extra[0].family,s.action.common.actors[0].animation.upper_state);
        } else if(mode==3) {
            s.action.common.live_0936=(uint16_t)value();s.band_b2=(uint16_t)value();s.receiver_anchor_8c=(uint16_t)value();
            s.action.extra[1].z=(uint16_t)value();s.action.extra[1].velocity_z=(uint16_t)value();s.action.relative_51=1;
            unsigned route=(unsigned)nba_human_pass_aligned_prepare(&assets,&s);
            printf("%u %u %u\n",route,s.action.request_00,s.action.extra[0].family);
        } else if(mode==4) {
            s.action.common.actors[1].mode=(uint16_t)value();unsigned blocked=value();s.options_07f6=(uint16_t)value();s.action.common.live_0936=(uint16_t)value();
            s.layout_0956=(uint16_t)value();s.action.profile_3e=(uint8_t)value();s.action.distance_4f=(uint16_t)value();s.passer_anchor_8a=(uint16_t)value();
            s.fine_c0=4;s.action.common.actors[0].movement_direction=2;
            s.lane.source_slot=0;s.lane.receiver_slot=1;s.lane.source_cursor=1;s.lane.receiver_cursor=2;
            s.lane.order[0]=s.lane.order[12]=0xffff;
            for(unsigned i=0;i<11;++i)s.lane.order[i+1]=(uint16_t)i;
            s.lane.actors[1].x=s.lane.actors[1].y=100;
            s.lane.actors[2].x=s.lane.actors[2].y=blocked?50u:999u;s.lane.actors[2].team=1;
            s.scratch_aa=0x1234;
            bool ok=nba_human_pass_aligned_choose(&s);
            printf("%u %u %u %u\n",ok?1u:0u,s.action.request_00,s.family_ae,s.scratch_aa);
        } else return 2;
    }
    nba_assets_free(&assets);return ferror(stdin)?1:0;
}
