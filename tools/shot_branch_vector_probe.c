#include <stdio.h>
#include "nba_shot_action.h"

int main(int argc, char **argv) {
    NbaAssetPack assets;
    if (argc != 2 || !nba_assets_load(&assets, argv[1])) return 2;
    uint16_t v[45];
    while (scanf_s("%hx", &v[0]) == 1) {
        for (unsigned i=1; i<45; ++i)
            if (scanf_s("%hx", &v[i]) != 1) return 3;
        NbaShotAction s = {0};
        NbaPlayerAnimationChannels *c = &s.animation;
        c->upper_queue_cursor=v[5]; c->lower_queue_cursor=v[6];
        c->upper_state=v[7]; c->lower_state=v[8]; c->base_state=v[9];
        c->upper_phase=v[10]; c->lower_phase=v[11];
        c->upper_accumulator=v[12]; c->lower_accumulator=v[13];
        c->upper_lock=v[14]; c->lower_lock=v[15];
        for(unsigned i=0;i<3;++i) {c->upper_queue[i]=v[16+i]; c->lower_queue[i]=v[19+i];}
        s.velocity_x=(int16_t)v[22]; s.velocity_y=(int16_t)v[23];
        s.velocity_z=(int16_t)v[24]; s.mode=v[25]; s.flags=v[26];
        s.timer=v[27]; s.status=v[28]; s.behavior_timer=v[29]; s.activity=v[30];
        NbaShotCancelBall ball={v[31],v[32],v[33],v[34],(int16_t)v[35]};
        NbaShotSidestepInput step={(int16_t)v[38],(int16_t)v[39],(int16_t)v[40],
                                   v[36],v[37],v[41],v[42]};
        unsigned decision=0;
        switch(v[0]) {
            case 0: nba_shot_action_sidestep(&s,&step); break;
            case 1: nba_shot_action_restore(&s,v[2],v[3]); break;
            case 2: if(!nba_shot_action_cancel(&assets,&s,&ball,v[1]!=0))return 4; break;
            case 3: nba_shot_action_windup_button(&s,(int16_t)v[43],v[41],v[44]); break;
            case 4: decision=nba_shot_action_owner_gate(&s,v[4]!=0); break;
            case 5: decision=nba_shot_action_release_facing(step.x,step.y,step.basket_x); break;
            default: return 3;
        }
        printf("%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x",
            c->upper_queue_cursor,c->lower_queue_cursor,c->upper_state,c->lower_state,c->base_state,
            c->upper_phase,c->lower_phase,c->upper_accumulator,c->lower_accumulator,c->upper_lock,c->lower_lock);
        for(unsigned i=0;i<3;++i)printf(" %04x",c->upper_queue[i]);
        for(unsigned i=0;i<3;++i)printf(" %04x",c->lower_queue[i]);
        printf(" %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x\n",
            (uint16_t)s.velocity_x,(uint16_t)s.velocity_y,(uint16_t)s.velocity_z,s.mode,s.flags,
            s.timer,s.status,s.behavior_timer,s.activity,ball.live_state,ball.ball_z,
            ball.attachment_state,ball.height_latch,(uint16_t)ball.ball_velocity_z,decision);
    }
    nba_assets_free(&assets);
    return 0;
}
