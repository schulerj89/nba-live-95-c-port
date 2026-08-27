#include "nba_tipoff.h"
#include <stdio.h>

#define CHECK(condition,code) do { if(!(condition)) { printf("tip possession failure %d\n",code); return code; } } while(0)
int main(int argc,char **argv) {
    NbaAssetPack pack={0};NbaSession session;NbaTipoff t;NbaInput input={0};
    if(argc!=2||!nba_assets_load(&pack,argv[1]))return 2;
    nba_session_init(&session);
    CHECK(nba_tipoff_init(&t,&pack,&session),3);
    while(t.frame<400 && !t.tip_possession_frame)nba_tipoff_update(&t,&input);
    CHECK(t.tip_contact_frame && t.tip_possession_frame>t.tip_contact_frame,4);
    CHECK(t.ball.owner_actor>=0 && t.live_state_raw==0 && t.phase==NBA_TIPOFF_LIVE,5);
    printf("TIP actual contact=%u possession=%u owner=%d\n",t.tip_contact_frame,t.tip_possession_frame,t.ball.owner_actor);
    CHECK(nba_tipoff_init(&t,&pack,&session),6);
    while(t.tip_contact_actor<0 && t.frame<400)nba_tipoff_update(&t,&input);
    CHECK(t.tip_contact_actor>=0,7);
    /* Hold the physical ball above every player across the old frame220
     * ownership gate. No clock/owner/phase reset may occur there. */
    while(t.frame<260) {
        t.ball.x_fp=0;t.ball.y_fp=0;t.ball.z_fp=200*256;
        t.ball.velocity_x=t.ball.velocity_y=t.ball.velocity_z=0;
        nba_tipoff_update(&t,&input);
        CHECK(t.ball.owner_actor<0 && !t.tip_possession_frame && t.live_state_raw==0x81,8);
    }
    /* Then let the normal contact sweep collect a low stationary ball. */
    for(unsigned n=0;n<10 && !t.tip_possession_frame;++n) {
        unsigned receiver=(unsigned)t.pass_receiver_raw;
        CHECK(receiver<10,9);
        t.ball.x_fp=t.actors[receiver].x_fp;t.ball.y_fp=t.actors[receiver].y_fp;
        t.ball.z_fp=30*256;t.ball.velocity_x=t.ball.velocity_y=t.ball.velocity_z=0;
        t.actors[receiver].contact_inhibit_raw_5a=0;
        nba_tipoff_update(&t,&input);
    }
    CHECK(t.tip_possession_frame>260 && t.ball.owner_actor>=0,10);
    for(unsigned side=0;side<2;++side)for(unsigned bit=0;bit<2;++bit)for(unsigned whistle=0;whistle<2;++whistle) {
        CHECK(nba_tipoff_init(&t,&pack,&session),11);
        t.frame=42;t.rng.state=bit?0x8000:1;t.ball.z_fp=61*256+0x77;
        t.camera_side_group_raw=(uint8_t)(side*5);
        nba_tipoff_replay_ball_acquisition(&t,(uint8_t)(side*5));
        unsigned receiver=side*5+3+bit;
        CHECK(t.tip_contact_frame==42 && !t.tip_possession_frame && t.ball.owner_actor<0 &&
            t.live_state_raw==0x81 && t.pass_actor_raw==10 && t.pass_receiver_raw==(int)receiver,12);
        CHECK(t.ball.z_fp==61*256+0x77 && t.ball.velocity_z==0,13);
        uint16_t rng=t.rng.state,clock=t.match_clock_raw_0928;
        t.frame=444;t.fouls.whistle_active_raw_09b6=(uint16_t)whistle;
        nba_tipoff_replay_ball_acquisition(&t,(uint8_t)receiver);
        CHECK(t.tip_possession_frame==444 && t.ball.owner_actor==(int)receiver && t.possession_actor==(int)receiver,14);
        CHECK(t.pass_actor_raw==-1 && t.pass_receiver_raw==-1 && t.pass_aux_raw==-1 && t.inbound_transfer_raw==0,15);
        CHECK(t.live_state_raw==(whistle?0x81:0) && t.ball.velocity_z==0 && t.rng.state==rng && t.match_clock_raw_0928==clock,16);
        CHECK(t.actors[receiver].control_mode==11 && t.actors[receiver].reaction_threshold==0,17);
    }
    puts("TIP POSSESSION binding PASS: natural/delayed physical contact, no frame220 award, both sides/bits/whistles, preserved RNG/clock");
    nba_assets_free(&pack);return 0;
}
