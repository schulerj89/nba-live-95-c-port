#include <stdio.h>
#include "nba_shot_action.h"
int main(int argc,char **argv) {
    NbaAssetPack assets;
    if(argc!=2 || !nba_assets_load(&assets,argv[1])) return 2;
    uint16_t v[47];
    while(scanf_s("%hx",&v[0])==1) {
        for(unsigned i=1;i<47;++i) if(scanf_s("%hx",&v[i])!=1)return 3;
        NbaShotAction s={0};
        NbaPlayerAnimationChannels *c=&s.animation;
        c->upper_queue_cursor=v[6]; c->lower_queue_cursor=v[7];
        c->upper_state=v[8]; c->lower_state=v[9]; c->base_state=v[10];
        c->upper_phase=v[11]; c->lower_phase=v[12];
        c->upper_accumulator=v[13]; c->lower_accumulator=v[14];
        c->upper_lock=v[15]; c->lower_lock=v[16];
        for(unsigned i=0;i<3;++i){c->upper_queue[i]=v[17+i];c->lower_queue[i]=v[20+i];}
        s.velocity_x=(int16_t)v[23];s.velocity_y=(int16_t)v[24];s.velocity_z=(int16_t)v[25];
        s.speed=v[26];s.mode=v[27];s.flags=v[28];s.timer=v[29];s.status=v[30];
        s.behavior_timer=v[31];s.activity=v[32];s.bounce_count=v[33];s.bounce_timer=v[34];
        NbaShotGateInput g={(int16_t)v[35],(int16_t)v[36],(int16_t)v[37],(int16_t)v[38],
            (int16_t)v[39],v[40],v[41],v[42],v[43],v[44],(int16_t)v[45]};
        unsigned gate=0,stage=0;
        switch(v[0]) {
            case 0: if(!nba_shot_action_start(&assets,&s,v[2]!=0,v[1]!=0))return 4;break;
            case 1: gate=nba_shot_action_gate(&g);break;
            case 2: nba_shot_action_restore(&s,v[4],v[5]);break;
            case 3: nba_shot_action_clear(&s);break;
            case 4: if(!nba_shot_action_jump(&assets,&s,v[3]!=0,v[1]!=0))return 4;break;
            case 5: stage=nba_shot_action_delay(&s.activity,v[46],v[3]!=0);break;
            default:return 3;
        }
        printf("%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x",
            c->upper_queue_cursor,c->lower_queue_cursor,c->upper_state,c->lower_state,c->base_state,
            c->upper_phase,c->lower_phase,c->upper_accumulator,c->lower_accumulator,c->upper_lock,c->lower_lock);
        for(unsigned i=0;i<3;++i)printf(" %04x",c->upper_queue[i]);
        for(unsigned i=0;i<3;++i)printf(" %04x",c->lower_queue[i]);
        printf(" %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x\n",
            (uint16_t)s.velocity_x,(uint16_t)s.velocity_y,(uint16_t)s.velocity_z,s.speed,s.mode,s.flags,
            s.timer,s.status,s.behavior_timer,s.activity,s.bounce_count,s.bounce_timer,gate,g.facing,stage);
    }
    nba_assets_free(&assets);return 0;
}
