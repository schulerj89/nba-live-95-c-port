#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include "nba_tipoff.h"

#define SIZE 0x4B00u
static uint16_t word(const uint8_t *r,unsigned a){return (uint16_t)(r[a]|(uint16_t)r[a+1]<<8);}
static int32_t fixed(const uint8_t *r,unsigned a){return (int32_t)(int16_t)word(r,a)*256;}

static void output(const NbaTipoff *s){
    printf("%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x "
           "%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x "
           "%04x %04x %04x",
        s->live_state_raw,s->camera_side_group_raw,(uint16_t)s->possession_actor,
        s->ball_activity_raw,s->inbound_state_raw,s->inbound_actor_raw,
        (uint16_t)s->inbound_layout_raw,s->fouls.foul_event_raw_0964,
        s->dead_ball_raw_0966,s->dead_ball_raw_0968,s->rim_raw_096a,
        s->dead_ball_raw_096c,s->fouls.free_throw_state_raw_0978,
        s->fouls.free_throw_sequence_raw_097a,s->rim_raw_097c,s->rim_raw_092c,
        s->inbound_timer_raw,(uint16_t)s->dead_ball_x_raw_09b0,
        (uint16_t)s->dead_ball_y_raw_09b2,s->fouls.shooting_foul_raw_09bc,
        s->shot_clock_mirror_raw_09c6,s->role_rebuild_raw_09d6,s->rim_raw_13e7,
        s->team_context[0].dead_ball_actor_raw_3f,
        s->team_context[1].dead_ball_actor_raw_3f,
        (uint16_t)s->ball.velocity_x,(uint16_t)s->ball.velocity_y);
    for(unsigned i=0;i<10u;++i)printf(" %04x",s->actors[i].control_mode);
    putchar('\n');
}

int main(int argc,char **argv){
    NbaAssetPack assets={0};static uint8_t raw[SIZE];
    if(argc!=2||!nba_assets_load(&assets,argv[1]))return 2;
    _setmode(_fileno(stdin),_O_BINARY);
    while(fread(raw,1,SIZE,stdin)==SIZE){
        NbaSession session;NbaTipoff s;nba_session_init(&session);
        session.left_team=(uint8_t)word(raw,0x46EB);session.right_team=(uint8_t)word(raw,0x476B);
        session.config.rules[2]=(uint8_t)word(raw,0x17D5);
        session.config.rules[5]=(uint8_t)word(raw,0x17DB);
        if(!nba_tipoff_init(&s,&assets,&session))return 3;
        s.live_state_raw=word(raw,0x0936);s.camera_side_group_raw=(uint8_t)word(raw,0x093A);
        s.possession_actor=(int8_t)(int16_t)word(raw,0x093E);
        s.ball_activity_raw=word(raw,0x0948);s.inbound_state_raw=word(raw,0x0952);
        s.inbound_actor_raw=word(raw,0x0954);s.inbound_layout_raw=(int16_t)word(raw,0x0956);
        s.fouls.foul_event_raw_0964=word(raw,0x0964);s.dead_ball_raw_0966=word(raw,0x0966);
        s.dead_ball_raw_0968=word(raw,0x0968);s.rim_raw_096a=word(raw,0x096A);
        s.dead_ball_raw_096c=word(raw,0x096C);s.fouls.free_throw_state_raw_0978=word(raw,0x0978);
        s.fouls.free_throw_sequence_raw_097a=word(raw,0x097A);s.rim_raw_097c=word(raw,0x097C);
        s.rim_raw_092c=word(raw,0x092C);s.inbound_timer_raw=word(raw,0x092E);
        s.dead_ball_x_raw_09b0=(int16_t)word(raw,0x09B0);s.dead_ball_y_raw_09b2=(int16_t)word(raw,0x09B2);
        s.dead_ball_dispatch_busy_raw_09b4=word(raw,0x09B4);
        s.fouls.whistle_active_raw_09b6=word(raw,0x09B6);
        s.fouls.shooting_foul_raw_09bc=word(raw,0x09BC);
        s.shot_clock_mirror_raw_09c6=word(raw,0x09C6);s.role_rebuild_raw_09d6=word(raw,0x09D6);
        s.deferred_shot_foul_phase_raw_0a02=word(raw,0x0A02);s.rim_raw_13e7=word(raw,0x13E7);
        s.fouls.offender_actor_raw=(int8_t)(int16_t)word(raw,0x492D);
        s.fouls.victim_actor_raw=(int8_t)(int16_t)word(raw,0x492F);
        s.rng.state=word(raw,0x07F6);
        for(unsigned side=0;side<2;++side){unsigned c=side?0x476B:0x46EB;
            s.team_context[side].dead_ball_actor_raw_3f=word(raw,c+0x3F);}
        for(unsigned i=0;i<10;++i){unsigned a=0x34EB+i*0x100;
            s.actors[i].x_fp=fixed(raw,a+4);s.actors[i].y_fp=fixed(raw,a+8);
            s.actors[i].z_fp=fixed(raw,a+0x0C);s.actors[i].velocity_x=(int16_t)word(raw,a+0x0E);
            s.actors[i].velocity_y=(int16_t)word(raw,a+0x10);s.actors[i].control_mode=(uint8_t)word(raw,a+0x5E);}
        s.ball.x_fp=fixed(raw,0x3EEF);s.ball.y_fp=fixed(raw,0x3EF3);s.ball.z_fp=fixed(raw,0x3EF7);
        s.ball.velocity_x=(int16_t)word(raw,0x3EF9);s.ball.velocity_y=(int16_t)word(raw,0x3EFB);
        s.ball.owner_actor=s.possession_actor;
        nba_tipoff_replay_violation_dispatch(&s);output(&s);
    }
    nba_assets_free(&assets);return ferror(stdin)?1:0;
}
