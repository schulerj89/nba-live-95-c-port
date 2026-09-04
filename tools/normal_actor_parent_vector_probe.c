#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nba_tipoff.h"

#define SIZE 0x4B00u
#define ACTOR_BASE 0x34EBu
static uint16_t word(const uint8_t *r,unsigned a){return (uint16_t)(r[a]|(uint16_t)r[a+1]<<8);}
static int32_t fixed(const uint8_t *r,unsigned a){return (int32_t)(int16_t)word(r,a)*256+(word(r,a-2)>>8);}
static void load_actor(NbaTipoffActor *a,const uint8_t *r,unsigned b,uint8_t roster){
    a->roster_slot=roster;a->x_fp=fixed(r,b+4);a->y_fp=fixed(r,b+8);a->z_fp=fixed(r,b+12);
    a->velocity_x=(int16_t)word(r,b+14);a->velocity_y=(int16_t)word(r,b+16);a->velocity_z=(int16_t)word(r,b+18);
    a->controller_assignment_raw=(int8_t)(int16_t)word(r,b+0x16);a->actor_status_raw_28=word(r,b+0x28);
    a->animation_state=(uint8_t)word(r,b+0x30);a->lower_animation_state=(uint8_t)word(r,b+0x32);
    a->base_animation_state_raw_38=(uint8_t)word(r,b+0x38);a->movement_magnitude_raw=word(r,b+0x4c);
    a->movement_direction=(uint8_t)word(r,b+0x4e);a->requested_direction=(uint8_t)word(r,b+0x50);
    a->direction=(uint8_t)word(r,b+0x52);a->target_x=(int16_t)word(r,b+0x56);a->target_y=(int16_t)word(r,b+0x58);
    a->contact_inhibit_raw_5a=word(r,b+0x5a);a->formation_timer_raw_5c=word(r,b+0x5c);
    a->control_mode=(uint8_t)word(r,b+0x5e);a->reaction_threshold=word(r,b+0x60);a->pass_band_raw=word(r,b+0x62);
    a->behavior_timer=word(r,b+0x64);a->pass_direction_raw=word(r,b+0x66);a->animation_variant_raw_6c=word(r,b+0x6c);
    a->team_group_raw_6e=word(r,b+0x6e);a->movement_boost_timer=word(r,b+0x72);
    a->assignment_base_raw=word(r,b+0x74);a->assignment_current_raw=word(r,b+0x76);a->behavior_flags_raw=word(r,b+0x7e);
    a->assignment_direction=(uint8_t)word(r,b+0x86);a->anchor_direction_raw=(uint8_t)word(r,b+0x88);
    a->assignment_distance=word(r,b+0x8a);a->anchor_distance_raw=word(r,b+0x8c);a->assignment_role_raw_92=(uint8_t)word(r,b+0x92);
    a->recovery_inhibit_raw=word(r,b+0x7a);a->free_throw_launch_half_raw_a8=word(r,b+0xa8);
}
static void print_actor(const NbaTipoffActor *a){
    printf(" %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x",
      (uint16_t)a->target_x,(uint16_t)a->target_y,a->control_mode,a->reaction_threshold,a->behavior_flags_raw,
      (uint16_t)a->velocity_x,(uint16_t)a->velocity_y,a->movement_magnitude_raw,a->movement_direction,a->requested_direction,
      a->direction,a->base_animation_state_raw_38,a->animation_state,a->lower_animation_state);
}
int main(int argc,char **argv){NbaAssetPack assets={0};static uint8_t raw[SIZE];
 if(argc!=2||!nba_assets_load(&assets,argv[1]))return 2;_setmode(_fileno(stdin),_O_BINARY);
 while(fread(raw,1,SIZE,stdin)==SIZE){NbaSession session;NbaTipoff s;nba_session_init(&session);
  /* Native home/context0 -> UI right; visitor/context1 -> UI left. */
  session.right_team=(uint8_t)word(raw,0x46eb);session.left_team=(uint8_t)word(raw,0x476b);
  if(!nba_tipoff_init(&s,&assets,&session))return 3;
  for(unsigned i=0;i<10;i++)load_actor(&s.actors[i],raw,ACTOR_BASE+i*0x100,(uint8_t)word(raw,(i<5?0x46f9:0x4779)+(i%5)*2));
  s.rng.state=word(raw,0x7f6);s.live_state_raw=word(raw,0x936);s.camera_side_group_raw=(uint8_t)word(raw,0x93a);
  s.possession_actor=(int8_t)(int16_t)word(raw,0x93e);s.pass_receiver_raw=(int16_t)word(raw,0x946);
  s.ball_activity_raw=word(raw,0x948);s.rim_raw_094a=word(raw,0x94a);
  s.inbound_state_raw=(int16_t)word(raw,0x952);s.inbound_actor_raw=word(raw,0x954);s.inbound_target_x_raw=(int16_t)word(raw,0x958);s.inbound_target_y_raw=(int16_t)word(raw,0x95a);
  s.dead_ball_raw_0968=word(raw,0x968);s.fouls.free_throw_state_raw_0978=word(raw,0x978);s.rim_raw_097c=word(raw,0x97c);
  s.play_code=(uint8_t)word(raw,0x996);s.play_step_raw=(int16_t)word(raw,0x998);s.play_mirror_raw=word(raw,0x99c);
  s.special_actor_raw=word(raw,0x9a2);s.play_cycle_raw=word(raw,0x9a4);s.cpu_play_state=0;
  s.role_ownerless_raw_09d8=word(raw,0x9d8);s.formation_override_raw_005c=word(raw,0x5c);
  s.collision_actor_b_raw=(int8_t)(int16_t)word(raw,0x492f);s.ball.x_fp=fixed(raw,0x3eef);s.ball.y_fp=fixed(raw,0x3ef3);
  s.ball.velocity_x=(int16_t)word(raw,0x3ef9);s.ball.velocity_y=(int16_t)word(raw,0x3efb);s.ball.owner_actor=s.possession_actor;
  for(unsigned side=0;side<2;side++){unsigned c=side?0x476b:0x46eb;s.team_context[side].anchor_x_raw_0a=(int16_t)word(raw,c+0xa);s.team_context[side].mode_raw_30=word(raw,c+0x30);s.team_context[side].flags_raw_32=word(raw,c+0x32);}
  unsigned slot=word(raw,0xc2);bool ok=nba_tipoff_replay_normal_actor(&s,(uint8_t)slot);
  printf("%04x %04x %04x %04x",ok?1:0,s.rng.state,s.special_actor_raw,s.play_step_raw);
  for(unsigned i=0;i<10;i++)print_actor(&s.actors[i]);putchar('\n');
 }
 nba_assets_free(&assets);return ferror(stdin)?1:0;}
