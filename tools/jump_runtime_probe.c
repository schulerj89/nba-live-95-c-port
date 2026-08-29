/* Binding/continuation test: runs production game code, no captured art/RAM. */
#include "nba_tipoff.h"
#include "nba_player_lab.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do{if(!(c)){fprintf(stderr,"jump binding line %d\n",__LINE__);return 10;}}while(0)
int main(int argc,char **argv) {
    NbaAssetPack pack={0};NbaSession session;NbaTipoff t;NbaInput input={0};
    if(argc!=2 || !nba_assets_load(&pack,argv[1]))return 2;
    nba_session_init(&session);CHECK(nba_tipoff_init(&t,&pack,&session));
    CHECK(t.tip_toss_countdown_raw_09f2==120 && t.ball.velocity_z==600);
    for(unsigned f=1;f<=120;++f) {
        nba_tipoff_update(&t,&input);
        CHECK(t.tip_toss_countdown_raw_09f2==120-f);
        CHECK(t.ball.z_fp==80*256 && t.ball.velocity_z==600 && t.tip_contact_actor==-1);
    }
    unsigned launched=0,airborne=0;bool landing=false;
    for(unsigned f=121;f<=1000;++f) {
        nba_tipoff_update(&t,&input);
        if(f==122)CHECK(t.ball.z_fp==80*256+576+552 && t.ball.velocity_z==552);
        for(unsigned slot=0;slot<10;++slot) {
            NbaTipoffActor *a=&t.actors[slot];
            if(a->velocity_z==528 || a->velocity_z==600)launched|=1u<<slot;
            if(a->z_fp>0)airborne|=1u<<slot;
            if((airborne&(1u<<slot)) && a->z_fp==0 && a->velocity_z==0)landing=true;
        }
        NbaGameplayTelemetry tele; nba_tipoff_capture_telemetry(&t,&input,&tele);
        for(unsigned slot=0;slot<10;++slot) {
            CHECK(tele.actors[slot].world_z_fp==t.actors[slot].z_fp);
            CHECK(tele.actors[slot].velocity_z==t.actors[slot].velocity_z);
        }
    }
    CHECK(launched && airborne && landing && t.tip_contact_actor>=0 &&
          t.jump_decision_calls>0 && t.tip_reach_mask!=0u &&
          t.tip_possession_frame>t.tip_contact_frame);
    printf("runtime jump launches=%u calls=%u rejected=%u contact=%u possession=%u\n",
        t.jump_launches,t.jump_decision_calls,t.jump_rejected_contexts,t.tip_contact_frame,t.tip_possession_frame);
    /* Prove binding consumes real packed ratings and shared scratch, with
     * both sign alternatives, then applies the returned channel request. */
    for(unsigned sign=0;sign<2;++sign) {
        CHECK(nba_tipoff_init(&t,&pack,&session));
        NbaTipoffActor *a=&t.actors[0];
        t.ball.z_fp=100*256;t.ball.velocity_z=-100;t.rng.state=1;
        t.scratch_0046=(uint16_t)(sign<<15);a->focal_distance_raw_8e=10;
        CHECK(nba_tipoff_jump_reach(&t,0));
        NbaJumpReachResult expected;CHECK(nba_jump_reach_decide(&pack,&t.last_jump_input,&expected));
        CHECK(expected.velocity_z==(uint16_t)a->velocity_z && expected.rng==t.rng.state);
        CHECK(t.last_jump_input.raw_0046==(sign<<15));
        CHECK(a->animation_state==0x32 && a->lower_animation_state==0x32 && a->animation_resources_valid);
    }
    /* BD1F: actual live low-ball request; current negative upper lock may
     * refuse pose13, in which case velocities must not be zeroed. */
    for(unsigned locked=0;locked<2;++locked) {
        CHECK(nba_tipoff_init(&t,&pack,&session));NbaTipoffActor *a=&t.actors[0];
        t.live_state_raw=0;t.ball.z_fp=72*256;t.rng.state=0;a->focal_distance_raw_8e=10;
        a->assignment_direction=0;t.actors[a->assignment_current_raw>>1].assignment_direction=0;
        a->velocity_x=123;a->upper_animation_lock_raw_46=locked?0x8000:0;
        CHECK(nba_tipoff_jump_reach(&t,0));CHECK(a->velocity_x==(locked?123:0));
        CHECK(a->animation_state==(locked?0:0x13));
    }
    nba_assets_free(&pack);puts("jump runtime bindings passed");return 0;
}
