#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nba_tipoff.h"

#define WRAM_SIZE 0x4B00u
#define ACTOR_BASE 0x34EBu
#define ACTOR_STRIDE 0x100u

static uint16_t word(const uint8_t *raw, unsigned address) {
    return (uint16_t)(raw[address] | ((uint16_t)raw[address + 1u] << 8));
}
static int32_t fixed(const uint8_t *raw, unsigned address) {
    return (int32_t)(int16_t)word(raw, address) * 256 +
           (uint8_t)(word(raw, address + 2u) >> 8);
}
static void load_actor(NbaTipoffActor *a, const uint8_t *raw, unsigned base,
                       uint8_t roster) {
    a->roster_slot=roster;a->x_fp=fixed(raw,base+4);a->y_fp=fixed(raw,base+8);
    a->z_fp=fixed(raw,base+0x0c);a->velocity_x=(int16_t)word(raw,base+0x0e);
    a->velocity_y=(int16_t)word(raw,base+0x10);a->velocity_z=(int16_t)word(raw,base+0x12);
    a->controller_assignment_raw=(int8_t)(int16_t)word(raw,base+0x16);
    a->actor_status_raw_28=word(raw,base+0x28);a->animation_state=(uint8_t)word(raw,base+0x30);
    a->lower_animation_state=(uint8_t)word(raw,base+0x32);a->rom_upper_animation_phase_raw_3a=word(raw,base+0x3a);
    a->movement_magnitude_raw=word(raw,base+0x4c);a->direction=(uint8_t)word(raw,base+0x4e);
    a->requested_direction=(uint8_t)word(raw,base+0x50);a->special_contact_raw_56=(int16_t)word(raw,base+0x56);
    a->mode13_variant_raw_58=word(raw,base+0x58);a->contact_inhibit_raw_5a=word(raw,base+0x5a);
    a->control_mode=(uint8_t)word(raw,base+0x5e);a->reaction_threshold=word(raw,base+0x60);
    a->behavior_timer=word(raw,base+0x64);a->pass_direction_raw=word(raw,base+0x66);
    a->animation_variant_raw_6c=word(raw,base+0x6c);a->team_group_raw_6e=word(raw,base+0x6e);
    a->movement_boost_timer=word(raw,base+0x72);a->behavior_flags_raw=word(raw,base+0x7e);
    a->assignment_direction=(uint8_t)word(raw,base+0x86);a->anchor_direction_raw=(uint8_t)word(raw,base+0x88);
    a->assignment_distance=word(raw,base+0x8a);a->anchor_distance_raw=word(raw,base+0x8c);
    a->mode13_baseline_velocity_x=(int16_t)word(raw,base+0xba);
    a->mode13_baseline_velocity_y=(int16_t)word(raw,base+0xbc);
    a->upper_animation_resource_raw_2a=word(raw,base+0x2a);
    a->lower_animation_resource_raw_2c=word(raw,base+0x2c);
    a->animation_resources_valid=true;
}
static void print_actor(const NbaTipoffActor *a) {
    printf(" %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x",
        (uint16_t)(a->x_fp >> 8),(uint16_t)(a->y_fp >> 8),(uint16_t)(a->z_fp >> 8),
        (uint16_t)a->velocity_x,(uint16_t)a->velocity_y,(uint16_t)a->velocity_z,
        a->control_mode,a->reaction_threshold,a->behavior_flags_raw,a->animation_state,
        a->lower_animation_state,a->direction,a->requested_direction,
        (uint16_t)a->special_contact_raw_56,a->mode13_variant_raw_58,
        a->contact_inhibit_raw_5a,a->pass_direction_raw,
        (uint16_t)a->mode13_baseline_velocity_x,(uint16_t)a->mode13_baseline_velocity_y);
}
int main(int argc,char **argv){
    if(argc!=2)return 2;NbaAssetPack assets={0};if(!nba_assets_load(&assets,argv[1]))return 3;
    uint8_t raw[WRAM_SIZE];_setmode(_fileno(stdin),_O_BINARY);
    while(fread(raw,1,WRAM_SIZE,stdin)==WRAM_SIZE){
        NbaSession session;NbaTipoff s;nba_session_init(&session);
        /* Native home/context0 -> UI right; visitor/context1 -> UI left. */
        session.right_team=(uint8_t)word(raw,0x46eb);session.left_team=(uint8_t)word(raw,0x476b);
        if(!nba_tipoff_init(&s,&assets,&session))return 4;
        for(unsigned i=0;i<10;i++)load_actor(&s.actors[i],raw,ACTOR_BASE+i*ACTOR_STRIDE,
            (uint8_t)word(raw,(i<5?0x46f9:0x4779)+(i%5)*2));
        unsigned ptr=word(raw,0x96),slot=ptr>=ACTOR_BASE?(ptr-ACTOR_BASE)/ACTOR_STRIDE:10;
        s.rng.state=word(raw,0x07f6);s.live_state_raw=word(raw,0x0936);
        s.camera_side_group_raw=(uint8_t)word(raw,0x093a);s.offense_side=s.camera_side_group_raw?1:0;
        s.possession_actor=(int8_t)(int16_t)word(raw,0x093e);s.handler_actor=s.possession_actor>=0?(uint8_t)s.possession_actor:0xff;
        s.pass_actor_raw=(int16_t)word(raw,0x0942);s.pass_aux_raw=(int16_t)word(raw,0x0944);
        s.pass_receiver_raw=(int16_t)word(raw,0x0946);s.receiver_actor=s.pass_receiver_raw>=0?(uint8_t)s.pass_receiver_raw:0xff;
        s.ball_activity_raw=word(raw,0x0948);s.shot_value_raw=word(raw,0x094c);
        s.rim_raw_096a=word(raw,0x096a);s.inbound_transfer_raw=word(raw,0x09b8);
        s.pass_active_raw=word(raw,0x09c4);s.shot_actor_raw_09c8=(int16_t)word(raw,0x09c8);
        s.rim_force_raw_1866=word(raw,0x1866);s.ball.x_fp=fixed(raw,0x3eef);
        s.court_presentation.basket_x_3fef=word(raw,0x3fef);
        s.ball.y_fp=fixed(raw,0x3ef3);s.ball.z_fp=fixed(raw,0x3ef7);
        s.ball.velocity_x=(int16_t)word(raw,0x3ef9);s.ball.velocity_y=(int16_t)word(raw,0x3efb);
        s.ball.velocity_z=(int16_t)word(raw,0x3efd);s.ball.owner_actor=s.possession_actor;
        s.ball.state=s.possession_actor>=0?NBA_BALL_ATTACHED:NBA_BALL_LOOSE;s.cpu_vs_cpu=true;
        bool ok=false;if(slot<10){uint8_t mode=s.actors[slot].control_mode;
            if(mode==16)ok=nba_tipoff_replay_passive_mode(&s,(uint8_t)slot);
            else if(mode==14)ok=nba_tipoff_replay_mode14_close_finish(&s,(uint8_t)slot);
            else ok=nba_tipoff_replay_close_finish_start(&s,(uint8_t)slot);}
        printf("%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x",
            ok?1:0,s.rng.state,s.live_state_raw,(uint16_t)s.possession_actor,s.ball_activity_raw,
            s.shot_value_raw,s.rim_raw_096a,s.rim_force_raw_1866,(uint16_t)s.pass_actor_raw,
            (uint16_t)s.pass_aux_raw,(uint16_t)s.pass_receiver_raw,s.pass_active_raw,s.inbound_transfer_raw,
            (uint16_t)s.shot_actor_raw_09c8,(uint16_t)(s.ball.x_fp >> 8),(uint16_t)(s.ball.y_fp >> 8),
            (uint16_t)(s.ball.z_fp >> 8),(uint16_t)s.ball.velocity_x,(uint16_t)s.ball.velocity_y);
        printf(" %04x %04x",(uint16_t)s.ball.velocity_z,(uint16_t)s.ball.owner_actor);
        if(slot<10)print_actor(&s.actors[slot]);putchar('\n');
    }
    nba_assets_free(&assets);return ferror(stdin)?1:0;
}
