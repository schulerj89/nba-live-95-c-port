#include <stdio.h>
#include "nba_shot_action.h"

int main(int argc, char **argv) {
    NbaAssetPack assets;
    if (argc != 2 || !nba_assets_load(&assets, argv[1])) return 2;
    uint16_t v[58];
    while (scanf_s("%hx", &v[0]) == 1) {
        for (unsigned i=1;i<58;++i) if (scanf_s("%hx",&v[i])!=1) return 3;
        NbaShotAction s={0};
        NbaPlayerAnimationChannels *c=&s.animation;
        c->upper_queue_cursor=v[21]; c->lower_queue_cursor=v[22];
        c->upper_state=v[23]; c->lower_state=v[24]; c->base_state=v[25];
        c->upper_phase=v[26]; c->lower_phase=v[27];
        c->upper_accumulator=v[28]; c->lower_accumulator=v[29];
        c->upper_lock=v[30]; c->lower_lock=v[31];
        for(unsigned i=0;i<3;++i){c->upper_queue[i]=v[32+i];c->lower_queue[i]=v[35+i];}
        s.velocity_x=(int16_t)v[38]; s.velocity_y=(int16_t)v[39]; s.velocity_z=(int16_t)v[40];
        s.speed=v[41];s.mode=v[42];s.flags=v[43];s.timer=v[44];s.status=v[45];
        s.behavior_timer=v[46];s.activity=v[47];s.bounce_count=v[48];s.bounce_timer=v[49];
        NbaSpecialShotFrame frame={(int16_t)v[8],(int16_t)v[9],(int16_t)v[10],(int16_t)v[11],
            v[12],v[13],v[14],v[15],v[16],v[17],v[18],v[19],v[20]!=0,v[1]!=0,v[2]!=0};
        NbaSpecialShotBall ball={v[50],v[51],v[52],v[53],v[54],v[55],v[56],(int16_t)v[57]};
        int result;
        if(v[0]==0) {
            NbaSpecialShotSelection in={v[3],v[4],v[5],v[6],v[12],v[7],v[1]!=0,v[2]!=0};
            if(!nba_special_shot_select(&assets,&s,&frame.direction_66,&in))return 4;
            result=s.mode==17;
        } else if(v[0]==1) {
            result=nba_special_shot_step(&assets,&s,&frame,&ball);
            if(result==NBA_SPECIAL_SHOT_ERROR)return 4;
        } else return 3;
        printf("%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x",
            c->upper_queue_cursor,c->lower_queue_cursor,c->upper_state,c->lower_state,c->base_state,
            c->upper_phase,c->lower_phase,c->upper_accumulator,c->lower_accumulator,c->upper_lock,c->lower_lock);
        for(unsigned i=0;i<3;++i)printf(" %04x",c->upper_queue[i]);
        for(unsigned i=0;i<3;++i)printf(" %04x",c->lower_queue[i]);
        printf(" %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x",
            (uint16_t)s.velocity_x,(uint16_t)s.velocity_y,(uint16_t)s.velocity_z,s.speed,s.mode,s.flags,
            s.timer,s.status,s.behavior_timer,s.activity,s.bounce_count,s.bounce_timer);
        printf(" %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x\n",
            frame.facing,frame.direction_66,ball.x,ball.y,ball.z,ball.previous_actor_x,
            ball.live_state,ball.attachment_state,ball.height_latch,(uint16_t)ball.velocity_z,result);
    }
    nba_assets_free(&assets);
    return 0;
}
