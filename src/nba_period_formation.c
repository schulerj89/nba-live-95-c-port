#include "nba_period_formation.h"
#include "period_appearance.h"
#include "period_support.h"
#include "period_render_tail.h"
#include <string.h>

enum {PARENT=0,CHILD,AFTER_APPEARANCE,BRIDGE,ROLES,RENDER,FINISHED};
static void channels(NbaPeriodFormationState *s,unsigned i,NbaPlayerAnimationChannels *c,bool save){
 NbaPeriodRestartActor *p=&s->parent.actors[i];NbaPeriodFormationActorExtra *e=&s->actors[i];unsigned j;
#define COPY(owner,name) do{if(save)(owner)->name=c->name;else c->name=(owner)->name;}while(0)
 COPY(e,upper_queue_cursor);COPY(e,lower_queue_cursor);COPY(e,upper_state);COPY(e,lower_state);COPY(e,base_state);COPY(e,upper_phase_target);
 COPY(p,upper_phase);COPY(p,lower_phase);COPY(p,upper_accumulator);COPY(p,lower_accumulator);COPY(p,upper_lock);COPY(p,lower_lock);
 for(j=0;j<3;j++){COPY(e,upper_queue[j]);COPY(e,lower_queue[j]);}
#undef COPY
}
static bool appearance(NbaPeriodFormationState *s,const NbaAssetPack *assets,unsigned i){
 NbaPeriodAppearance a={0};NbaPeriodRestartActor *p=&s->parent.actors[i];NbaPeriodFormationActorExtra *e=&s->actors[i];
 channels(s,i,&a.channels,false);a.owner=s->parent.owner_093e;a.owner_pointer=s->parent.owner_pointer_0940;
 a.controller=(uint16_t)s->controllers.actor_assignment[i];a.velocity_x=(uint16_t)p->velocity_x;a.velocity_y=(uint16_t)p->velocity_y;a.z=(uint16_t)p->z;
 a.boost=p->boost_timer;a.speed=p->speed;a.direction=p->direction;a.display_direction=p->movement_direction;a.status=e->mirror_flags;
 a.alternate_lower=e->alternate_lower;a.variant=e->variant;a.catcher_latch=e->catcher_latch;a.delta=s->delta;a.rng=s->rng;a.upper_resource=e->upper_resource;a.lower_resource=e->lower_resource;
 if(!nba_period_appearance(assets,&a))return false;
 channels(s,i,&a.channels,true);p->direction=a.direction;e->mirror_flags=a.status;e->upper_resource=a.upper_resource;e->lower_resource=a.lower_resource;
 e->catcher_latch=a.catcher_latch;s->rng=a.rng;s->parent.owner_pointer_0940=a.owner_pointer;return true;
}
static bool assignment(NbaPeriodFormationState *s,const NbaAssetPack *assets){
 NbaPeriodAssignmentInput in={0};NbaPeriodAssignment out={0};unsigned side,i;uint32_t expected;
 for(side=0;side<2;side++){
  if(s->contexts[side].team>=29)return false;in.team[side]=(uint8_t)s->contexts[side].team;
  /* D7B8 consumes the carried full roster address table. The bounded child
   * derives the same ROM addresses: require that explicit state to agree. */
  for(i=0;i<12;i++)if(!nba_player_gameplay_roster_address(assets,in.team[side],(uint8_t)i,&expected)||expected!=s->roster_table[side][i])return false;
  for(i=0;i<5;i++){
   if(s->contexts[side].roster[i]>=12)return false;
   in.roster[side*5+i]=(uint8_t)s->contexts[side].roster[i];in.selector[side*5+i]=s->contexts[side].selector[i];
  }
 }
 if(!nba_period_assignment(assets,&in,&out))return false;
 for(i=0;i<10;i++){
  NbaPeriodFormationActorExtra *e=&s->actors[i];e->variant=out.actor[i].variant;e->current_assignment=out.actor[i].current;e->base_assignment=out.actor[i].base;e->alternate_assignment=out.actor[i].alternate;e->help=out.actor[i].help;e->role=out.actor[i].role;
  s->active_roster_pointer[i]=out.roster_pointer[i];s->statistic_pointer[i]=out.statistic_pointer[i];s->assignment_sort_slots[i]=out.keys[i];
 }
 for(side=0;side<2;side++)for(i=0;i<5;i++)s->contexts[side].order[i]=out.order[side][i];return true;
}
static bool object_sort(NbaPeriodFormationState *s){
 NbaPeriodObjectSort c={0};unsigned i;
 for(i=0;i<10;i++){c.x[i]=s->parent.actors[i].x;c.link[i]=s->parent.actors[i].list_link;}
 c.x[10]=s->parent.ball.x;c.link[10]=s->parent.ball.list_link;
 for(i=0;i<12;i++)c.object[i]=s->parent.object_list[i];
 if(!nba_period_object_sort(&c))return false;
 for(i=0;i<10;i++)s->parent.actors[i].list_link=c.link[i];s->parent.ball.list_link=c.link[10];
 for(i=0;i<12;i++)s->parent.object_list[i]=c.object[i];return true;
}
static bool attachment(NbaPeriodFormationState *s,const NbaAssetPack *assets){
 NbaPeriodAttachment a={0};unsigned i=s->parent.actor_0954;NbaPeriodRestartActor *p;NbaPeriodFormationActorExtra *e;if(i>=10)return false;
 p=&s->parent.actors[i];e=&s->actors[i];a.actor=(uint16_t)i;a.owner=s->parent.owner_093e;a.group=p->team_group;a.controllers=s->controllers;channels(s,i,&a.channels,false);
 a.pose.mirror_flags=e->mirror_flags;a.pose.direction=p->movement_direction;a.pose.upper_resource=e->upper_resource;a.pose.lower_resource=e->lower_resource;
 a.pose.upper_state=e->resolved_upper_state;a.pose.lower_state=e->resolved_lower_state;a.pose.upper_phase=e->resolved_upper_phase;a.pose.lower_phase=e->resolved_lower_phase;
 a.facing=p->direction;a.alternate_lower=e->alternate_lower;a.variant=e->variant;a.boost=p->boost_timer;a.x=p->x;a.y=p->y;a.z=p->z;
 a.ball_x=s->parent.ball.x;a.ball_y=s->parent.ball.y;a.ball_z=s->parent.ball.z;a.previous_ball_x=s->previous_ball_x;
 /* scratch47 is a write-only descriptor observer in this included install;
  * its incoming value is never consumed. CPU scratch is not gameplay state. */
 if(!nba_period_attachment(assets,&a))return false;
 s->controllers=a.controllers;channels(s,i,&a.channels,true);e->mirror_flags=a.pose.mirror_flags;e->upper_resource=a.pose.upper_resource;e->lower_resource=a.pose.lower_resource;
 e->resolved_upper_state=a.pose.upper_state;e->resolved_lower_state=a.pose.lower_state;e->resolved_upper_phase=a.pose.upper_phase;e->resolved_lower_phase=a.pose.lower_phase;p->movement_direction=a.pose.direction;
 s->parent.ball.x=a.ball_x;s->parent.ball.y=a.ball_y;s->parent.ball.z=a.ball_z;s->previous_ball_x=a.previous_ball_x;return true;
}
static bool bridge(NbaPeriodFormationState *s){
 NbaControllerState controllers=s->controllers;
 /* E1AC-E1E5. Reg already transferred inside E183 attachment. */
 if(s->input.period==0||s->input.period>=4){
  if(!nba_controller_transfer(&controllers,0,0)||!nba_controller_transfer(&controllers,5,5))return false;
 }
 s->predicted_x=s->parent.ball.x;s->predicted_y=s->parent.ball.y;s->controllers=controllers;return true;
}
static uint16_t distance(uint16_t x,uint16_t y){
 uint16_t t;if(x&0x8000u)x=(uint16_t)(0u-x);if(y&0x8000u)y=(uint16_t)(0u-y);
 if((uint16_t)(x-y)&0x8000u){t=x;x=y;y=t;}return (uint16_t)(x+(y>>2));
}
static bool roles_have_defined_nearest(const NbaPeriodFormationState *s){
 unsigned side,i;uint16_t x=(uint16_t)((s->parent.owner_093e&0x8000u)?s->predicted_x:s->parent.ball.x);
 uint16_t y=(uint16_t)((s->parent.owner_093e&0x8000u)?s->predicted_y:s->parent.ball.y);
 for(side=0;side<2;side++){
  bool found=false;for(i=side*5;i<side*5+5;i++){
   uint16_t d=distance((uint16_t)((uint16_t)s->parent.actors[i].x-x),(uint16_t)((uint16_t)s->parent.actors[i].y-y));
   if((uint16_t)(d-0x7fffu)&0x8000u)found=true;
  }
  if(!found)return false;
 }
 return true;
}
static void role_state(NbaPeriodFormationState *s,NbaPeriodRoleStateV2 *r,bool save){
 if(save)s->parent.actors[0].x=r->prefix.actors[0].x;else r->prefix.actors[0].x=s->parent.actors[0].x;
 if(save)s->parent.actors[0].y=r->prefix.actors[0].y;else r->prefix.actors[0].y=s->parent.actors[0].y;
 if(save)s->actors[0].current_assignment=r->prefix.actors[0].assignment_74;else r->prefix.actors[0].assignment_74=s->actors[0].current_assignment;
 if(save)s->actors[0].pair_direction=r->prefix.actors[0].direction_86;else r->prefix.actors[0].direction_86=s->actors[0].pair_direction;
 if(save)s->actors[0].pair_distance=r->prefix.actors[0].pair_distance_8a;else r->prefix.actors[0].pair_distance_8a=s->actors[0].pair_distance;
 if(save)s->parent.actors[0].focal_distance=r->prefix.actors[0].focal_distance_8e;else r->prefix.actors[0].focal_distance_8e=s->parent.actors[0].focal_distance;
 if(save)s->parent.actors[1].x=r->prefix.actors[1].x;else r->prefix.actors[1].x=s->parent.actors[1].x;
 if(save)s->parent.actors[1].y=r->prefix.actors[1].y;else r->prefix.actors[1].y=s->parent.actors[1].y;
 if(save)s->actors[1].current_assignment=r->prefix.actors[1].assignment_74;else r->prefix.actors[1].assignment_74=s->actors[1].current_assignment;
 if(save)s->actors[1].pair_direction=r->prefix.actors[1].direction_86;else r->prefix.actors[1].direction_86=s->actors[1].pair_direction;
 if(save)s->actors[1].pair_distance=r->prefix.actors[1].pair_distance_8a;else r->prefix.actors[1].pair_distance_8a=s->actors[1].pair_distance;
 if(save)s->parent.actors[1].focal_distance=r->prefix.actors[1].focal_distance_8e;else r->prefix.actors[1].focal_distance_8e=s->parent.actors[1].focal_distance;
 if(save)s->parent.actors[2].x=r->prefix.actors[2].x;else r->prefix.actors[2].x=s->parent.actors[2].x;
 if(save)s->parent.actors[2].y=r->prefix.actors[2].y;else r->prefix.actors[2].y=s->parent.actors[2].y;
 if(save)s->actors[2].current_assignment=r->prefix.actors[2].assignment_74;else r->prefix.actors[2].assignment_74=s->actors[2].current_assignment;
 if(save)s->actors[2].pair_direction=r->prefix.actors[2].direction_86;else r->prefix.actors[2].direction_86=s->actors[2].pair_direction;
 if(save)s->actors[2].pair_distance=r->prefix.actors[2].pair_distance_8a;else r->prefix.actors[2].pair_distance_8a=s->actors[2].pair_distance;
 if(save)s->parent.actors[2].focal_distance=r->prefix.actors[2].focal_distance_8e;else r->prefix.actors[2].focal_distance_8e=s->parent.actors[2].focal_distance;
 if(save)s->parent.actors[3].x=r->prefix.actors[3].x;else r->prefix.actors[3].x=s->parent.actors[3].x;
 if(save)s->parent.actors[3].y=r->prefix.actors[3].y;else r->prefix.actors[3].y=s->parent.actors[3].y;
 if(save)s->actors[3].current_assignment=r->prefix.actors[3].assignment_74;else r->prefix.actors[3].assignment_74=s->actors[3].current_assignment;
 if(save)s->actors[3].pair_direction=r->prefix.actors[3].direction_86;else r->prefix.actors[3].direction_86=s->actors[3].pair_direction;
 if(save)s->actors[3].pair_distance=r->prefix.actors[3].pair_distance_8a;else r->prefix.actors[3].pair_distance_8a=s->actors[3].pair_distance;
 if(save)s->parent.actors[3].focal_distance=r->prefix.actors[3].focal_distance_8e;else r->prefix.actors[3].focal_distance_8e=s->parent.actors[3].focal_distance;
 if(save)s->parent.actors[4].x=r->prefix.actors[4].x;else r->prefix.actors[4].x=s->parent.actors[4].x;
 if(save)s->parent.actors[4].y=r->prefix.actors[4].y;else r->prefix.actors[4].y=s->parent.actors[4].y;
 if(save)s->actors[4].current_assignment=r->prefix.actors[4].assignment_74;else r->prefix.actors[4].assignment_74=s->actors[4].current_assignment;
 if(save)s->actors[4].pair_direction=r->prefix.actors[4].direction_86;else r->prefix.actors[4].direction_86=s->actors[4].pair_direction;
 if(save)s->actors[4].pair_distance=r->prefix.actors[4].pair_distance_8a;else r->prefix.actors[4].pair_distance_8a=s->actors[4].pair_distance;
 if(save)s->parent.actors[4].focal_distance=r->prefix.actors[4].focal_distance_8e;else r->prefix.actors[4].focal_distance_8e=s->parent.actors[4].focal_distance;
 if(save)s->parent.actors[5].x=r->prefix.actors[5].x;else r->prefix.actors[5].x=s->parent.actors[5].x;
 if(save)s->parent.actors[5].y=r->prefix.actors[5].y;else r->prefix.actors[5].y=s->parent.actors[5].y;
 if(save)s->actors[5].current_assignment=r->prefix.actors[5].assignment_74;else r->prefix.actors[5].assignment_74=s->actors[5].current_assignment;
 if(save)s->actors[5].pair_direction=r->prefix.actors[5].direction_86;else r->prefix.actors[5].direction_86=s->actors[5].pair_direction;
 if(save)s->actors[5].pair_distance=r->prefix.actors[5].pair_distance_8a;else r->prefix.actors[5].pair_distance_8a=s->actors[5].pair_distance;
 if(save)s->parent.actors[5].focal_distance=r->prefix.actors[5].focal_distance_8e;else r->prefix.actors[5].focal_distance_8e=s->parent.actors[5].focal_distance;
 if(save)s->parent.actors[6].x=r->prefix.actors[6].x;else r->prefix.actors[6].x=s->parent.actors[6].x;
 if(save)s->parent.actors[6].y=r->prefix.actors[6].y;else r->prefix.actors[6].y=s->parent.actors[6].y;
 if(save)s->actors[6].current_assignment=r->prefix.actors[6].assignment_74;else r->prefix.actors[6].assignment_74=s->actors[6].current_assignment;
 if(save)s->actors[6].pair_direction=r->prefix.actors[6].direction_86;else r->prefix.actors[6].direction_86=s->actors[6].pair_direction;
 if(save)s->actors[6].pair_distance=r->prefix.actors[6].pair_distance_8a;else r->prefix.actors[6].pair_distance_8a=s->actors[6].pair_distance;
 if(save)s->parent.actors[6].focal_distance=r->prefix.actors[6].focal_distance_8e;else r->prefix.actors[6].focal_distance_8e=s->parent.actors[6].focal_distance;
 if(save)s->parent.actors[7].x=r->prefix.actors[7].x;else r->prefix.actors[7].x=s->parent.actors[7].x;
 if(save)s->parent.actors[7].y=r->prefix.actors[7].y;else r->prefix.actors[7].y=s->parent.actors[7].y;
 if(save)s->actors[7].current_assignment=r->prefix.actors[7].assignment_74;else r->prefix.actors[7].assignment_74=s->actors[7].current_assignment;
 if(save)s->actors[7].pair_direction=r->prefix.actors[7].direction_86;else r->prefix.actors[7].direction_86=s->actors[7].pair_direction;
 if(save)s->actors[7].pair_distance=r->prefix.actors[7].pair_distance_8a;else r->prefix.actors[7].pair_distance_8a=s->actors[7].pair_distance;
 if(save)s->parent.actors[7].focal_distance=r->prefix.actors[7].focal_distance_8e;else r->prefix.actors[7].focal_distance_8e=s->parent.actors[7].focal_distance;
 if(save)s->parent.actors[8].x=r->prefix.actors[8].x;else r->prefix.actors[8].x=s->parent.actors[8].x;
 if(save)s->parent.actors[8].y=r->prefix.actors[8].y;else r->prefix.actors[8].y=s->parent.actors[8].y;
 if(save)s->actors[8].current_assignment=r->prefix.actors[8].assignment_74;else r->prefix.actors[8].assignment_74=s->actors[8].current_assignment;
 if(save)s->actors[8].pair_direction=r->prefix.actors[8].direction_86;else r->prefix.actors[8].direction_86=s->actors[8].pair_direction;
 if(save)s->actors[8].pair_distance=r->prefix.actors[8].pair_distance_8a;else r->prefix.actors[8].pair_distance_8a=s->actors[8].pair_distance;
 if(save)s->parent.actors[8].focal_distance=r->prefix.actors[8].focal_distance_8e;else r->prefix.actors[8].focal_distance_8e=s->parent.actors[8].focal_distance;
 if(save)s->parent.actors[9].x=r->prefix.actors[9].x;else r->prefix.actors[9].x=s->parent.actors[9].x;
 if(save)s->parent.actors[9].y=r->prefix.actors[9].y;else r->prefix.actors[9].y=s->parent.actors[9].y;
 if(save)s->actors[9].current_assignment=r->prefix.actors[9].assignment_74;else r->prefix.actors[9].assignment_74=s->actors[9].current_assignment;
 if(save)s->actors[9].pair_direction=r->prefix.actors[9].direction_86;else r->prefix.actors[9].direction_86=s->actors[9].pair_direction;
 if(save)s->actors[9].pair_distance=r->prefix.actors[9].pair_distance_8a;else r->prefix.actors[9].pair_distance_8a=s->actors[9].pair_distance;
 if(save)s->parent.actors[9].focal_distance=r->prefix.actors[9].focal_distance_8e;else r->prefix.actors[9].focal_distance_8e=s->parent.actors[9].focal_distance;
 if(save)s->contexts[0].opponent_pointer=r->prefix.contexts[0].opponent_02;else r->prefix.contexts[0].opponent_02=s->contexts[0].opponent_pointer;
 if(save)s->contexts[0].first_actor_pointer=r->prefix.contexts[0].first_actor_04;else r->prefix.contexts[0].first_actor_04=s->contexts[0].first_actor_pointer;
 if(save)s->contexts[1].opponent_pointer=r->prefix.contexts[1].opponent_02;else r->prefix.contexts[1].opponent_02=s->contexts[1].opponent_pointer;
 if(save)s->contexts[1].first_actor_pointer=r->prefix.contexts[1].first_actor_04;else r->prefix.contexts[1].first_actor_04=s->contexts[1].first_actor_pointer;
 if(save)s->parent.ball.x=r->prefix.ball_x;else r->prefix.ball_x=s->parent.ball.x;
 if(save)s->parent.ball.y=r->prefix.ball_y;else r->prefix.ball_y=s->parent.ball.y;
 if(save)s->parent.ball_pointer_0910=r->prefix.ball_pointer_0910;else r->prefix.ball_pointer_0910=s->parent.ball_pointer_0910;
 if(save)s->predicted_x=r->prefix.predicted_x_0918;else r->prefix.predicted_x_0918=s->predicted_x;
 if(save)s->predicted_y=r->prefix.predicted_y_091a;else r->prefix.predicted_y_091a=s->predicted_y;
 if(save)s->parent.camera_093a=r->prefix.camera_093a;else r->prefix.camera_093a=s->parent.camera_093a;
 if(save)s->parent.owner_093e=r->prefix.owner_093e;else r->prefix.owner_093e=s->parent.owner_093e;
 if(save)s->role_cadence=r->prefix.cadence_09d2;else r->prefix.cadence_09d2=s->role_cadence;
 if(save)s->role_rebuild=r->prefix.rebuild_09d6;else r->prefix.rebuild_09d6=s->role_rebuild;
 if(save)s->assignment_sort_slots[0]=r->prefix.nearest_09da;else r->prefix.nearest_09da=s->assignment_sort_slots[0];
 if(save)s->delta=r->prefix.delta_00c6;else r->prefix.delta_00c6=s->delta;
 if(save)s->parent.list_cursor=r->prefix.pair_009a;else r->prefix.pair_009a=s->parent.list_cursor;
 if(save)s->parent.ready_09ba=r->prefix.ready_09ba;else r->prefix.ready_09ba=s->parent.ready_09ba;
 if(save)s->parent.dead_ball_x_09b0=r->prefix.dead_x_09b0;else r->prefix.dead_x_09b0=s->parent.dead_ball_x_09b0;
 if(save)s->parent.dead_ball_y_09b2=r->prefix.dead_y_09b2;else r->prefix.dead_y_09b2=s->parent.dead_ball_y_09b2;
 if(save)s->parent.actors[0].id=r->actors[0].id_00;else r->actors[0].id_00=s->parent.actors[0].id;
 if(save)s->parent.actors[0].mode=r->actors[0].mode_5e;else r->actors[0].mode_5e=s->parent.actors[0].mode;
 if(save)s->parent.actors[0].action_timer=r->actors[0].reaction_60;else r->actors[0].reaction_60=s->parent.actors[0].action_timer;
 if(save)s->parent.actors[0].behavior_timer=r->actors[0].behavior_64;else r->actors[0].behavior_64=s->parent.actors[0].behavior_timer;
 if(save)s->parent.actors[0].team_group=r->actors[0].team_6e;else r->actors[0].team_6e=s->parent.actors[0].team_group;
 if(save)s->parent.actors[0].boost_timer=r->actors[0].boost_72;else r->actors[0].boost_72=s->parent.actors[0].boost_timer;
 if(save)s->actors[0].base_assignment=r->actors[0].base_76;else r->actors[0].base_76=s->actors[0].base_assignment;
 if(save)s->actors[0].alternate_assignment=r->actors[0].alternate_78;else r->actors[0].alternate_78=s->actors[0].alternate_assignment;
 if(save)s->parent.actors[0].behavior_flags=r->actors[0].clear_7e;else r->actors[0].clear_7e=s->parent.actors[0].behavior_flags;
 if(save)s->actors[0].saved_mode=r->actors[0].saved_84;else r->actors[0].saved_84=s->actors[0].saved_mode;
 if(save)s->actors[0].anchor_distance=r->actors[0].anchor_distance_8c;else r->actors[0].anchor_distance_8c=s->actors[0].anchor_distance;
 if(save)s->parent.actors[1].id=r->actors[1].id_00;else r->actors[1].id_00=s->parent.actors[1].id;
 if(save)s->parent.actors[1].mode=r->actors[1].mode_5e;else r->actors[1].mode_5e=s->parent.actors[1].mode;
 if(save)s->parent.actors[1].action_timer=r->actors[1].reaction_60;else r->actors[1].reaction_60=s->parent.actors[1].action_timer;
 if(save)s->parent.actors[1].behavior_timer=r->actors[1].behavior_64;else r->actors[1].behavior_64=s->parent.actors[1].behavior_timer;
 if(save)s->parent.actors[1].team_group=r->actors[1].team_6e;else r->actors[1].team_6e=s->parent.actors[1].team_group;
 if(save)s->parent.actors[1].boost_timer=r->actors[1].boost_72;else r->actors[1].boost_72=s->parent.actors[1].boost_timer;
 if(save)s->actors[1].base_assignment=r->actors[1].base_76;else r->actors[1].base_76=s->actors[1].base_assignment;
 if(save)s->actors[1].alternate_assignment=r->actors[1].alternate_78;else r->actors[1].alternate_78=s->actors[1].alternate_assignment;
 if(save)s->parent.actors[1].behavior_flags=r->actors[1].clear_7e;else r->actors[1].clear_7e=s->parent.actors[1].behavior_flags;
 if(save)s->actors[1].saved_mode=r->actors[1].saved_84;else r->actors[1].saved_84=s->actors[1].saved_mode;
 if(save)s->actors[1].anchor_distance=r->actors[1].anchor_distance_8c;else r->actors[1].anchor_distance_8c=s->actors[1].anchor_distance;
 if(save)s->parent.actors[2].id=r->actors[2].id_00;else r->actors[2].id_00=s->parent.actors[2].id;
 if(save)s->parent.actors[2].mode=r->actors[2].mode_5e;else r->actors[2].mode_5e=s->parent.actors[2].mode;
 if(save)s->parent.actors[2].action_timer=r->actors[2].reaction_60;else r->actors[2].reaction_60=s->parent.actors[2].action_timer;
 if(save)s->parent.actors[2].behavior_timer=r->actors[2].behavior_64;else r->actors[2].behavior_64=s->parent.actors[2].behavior_timer;
 if(save)s->parent.actors[2].team_group=r->actors[2].team_6e;else r->actors[2].team_6e=s->parent.actors[2].team_group;
 if(save)s->parent.actors[2].boost_timer=r->actors[2].boost_72;else r->actors[2].boost_72=s->parent.actors[2].boost_timer;
 if(save)s->actors[2].base_assignment=r->actors[2].base_76;else r->actors[2].base_76=s->actors[2].base_assignment;
 if(save)s->actors[2].alternate_assignment=r->actors[2].alternate_78;else r->actors[2].alternate_78=s->actors[2].alternate_assignment;
 if(save)s->parent.actors[2].behavior_flags=r->actors[2].clear_7e;else r->actors[2].clear_7e=s->parent.actors[2].behavior_flags;
 if(save)s->actors[2].saved_mode=r->actors[2].saved_84;else r->actors[2].saved_84=s->actors[2].saved_mode;
 if(save)s->actors[2].anchor_distance=r->actors[2].anchor_distance_8c;else r->actors[2].anchor_distance_8c=s->actors[2].anchor_distance;
 if(save)s->parent.actors[3].id=r->actors[3].id_00;else r->actors[3].id_00=s->parent.actors[3].id;
 if(save)s->parent.actors[3].mode=r->actors[3].mode_5e;else r->actors[3].mode_5e=s->parent.actors[3].mode;
 if(save)s->parent.actors[3].action_timer=r->actors[3].reaction_60;else r->actors[3].reaction_60=s->parent.actors[3].action_timer;
 if(save)s->parent.actors[3].behavior_timer=r->actors[3].behavior_64;else r->actors[3].behavior_64=s->parent.actors[3].behavior_timer;
 if(save)s->parent.actors[3].team_group=r->actors[3].team_6e;else r->actors[3].team_6e=s->parent.actors[3].team_group;
 if(save)s->parent.actors[3].boost_timer=r->actors[3].boost_72;else r->actors[3].boost_72=s->parent.actors[3].boost_timer;
 if(save)s->actors[3].base_assignment=r->actors[3].base_76;else r->actors[3].base_76=s->actors[3].base_assignment;
 if(save)s->actors[3].alternate_assignment=r->actors[3].alternate_78;else r->actors[3].alternate_78=s->actors[3].alternate_assignment;
 if(save)s->parent.actors[3].behavior_flags=r->actors[3].clear_7e;else r->actors[3].clear_7e=s->parent.actors[3].behavior_flags;
 if(save)s->actors[3].saved_mode=r->actors[3].saved_84;else r->actors[3].saved_84=s->actors[3].saved_mode;
 if(save)s->actors[3].anchor_distance=r->actors[3].anchor_distance_8c;else r->actors[3].anchor_distance_8c=s->actors[3].anchor_distance;
 if(save)s->parent.actors[4].id=r->actors[4].id_00;else r->actors[4].id_00=s->parent.actors[4].id;
 if(save)s->parent.actors[4].mode=r->actors[4].mode_5e;else r->actors[4].mode_5e=s->parent.actors[4].mode;
 if(save)s->parent.actors[4].action_timer=r->actors[4].reaction_60;else r->actors[4].reaction_60=s->parent.actors[4].action_timer;
 if(save)s->parent.actors[4].behavior_timer=r->actors[4].behavior_64;else r->actors[4].behavior_64=s->parent.actors[4].behavior_timer;
 if(save)s->parent.actors[4].team_group=r->actors[4].team_6e;else r->actors[4].team_6e=s->parent.actors[4].team_group;
 if(save)s->parent.actors[4].boost_timer=r->actors[4].boost_72;else r->actors[4].boost_72=s->parent.actors[4].boost_timer;
 if(save)s->actors[4].base_assignment=r->actors[4].base_76;else r->actors[4].base_76=s->actors[4].base_assignment;
 if(save)s->actors[4].alternate_assignment=r->actors[4].alternate_78;else r->actors[4].alternate_78=s->actors[4].alternate_assignment;
 if(save)s->parent.actors[4].behavior_flags=r->actors[4].clear_7e;else r->actors[4].clear_7e=s->parent.actors[4].behavior_flags;
 if(save)s->actors[4].saved_mode=r->actors[4].saved_84;else r->actors[4].saved_84=s->actors[4].saved_mode;
 if(save)s->actors[4].anchor_distance=r->actors[4].anchor_distance_8c;else r->actors[4].anchor_distance_8c=s->actors[4].anchor_distance;
 if(save)s->parent.actors[5].id=r->actors[5].id_00;else r->actors[5].id_00=s->parent.actors[5].id;
 if(save)s->parent.actors[5].mode=r->actors[5].mode_5e;else r->actors[5].mode_5e=s->parent.actors[5].mode;
 if(save)s->parent.actors[5].action_timer=r->actors[5].reaction_60;else r->actors[5].reaction_60=s->parent.actors[5].action_timer;
 if(save)s->parent.actors[5].behavior_timer=r->actors[5].behavior_64;else r->actors[5].behavior_64=s->parent.actors[5].behavior_timer;
 if(save)s->parent.actors[5].team_group=r->actors[5].team_6e;else r->actors[5].team_6e=s->parent.actors[5].team_group;
 if(save)s->parent.actors[5].boost_timer=r->actors[5].boost_72;else r->actors[5].boost_72=s->parent.actors[5].boost_timer;
 if(save)s->actors[5].base_assignment=r->actors[5].base_76;else r->actors[5].base_76=s->actors[5].base_assignment;
 if(save)s->actors[5].alternate_assignment=r->actors[5].alternate_78;else r->actors[5].alternate_78=s->actors[5].alternate_assignment;
 if(save)s->parent.actors[5].behavior_flags=r->actors[5].clear_7e;else r->actors[5].clear_7e=s->parent.actors[5].behavior_flags;
 if(save)s->actors[5].saved_mode=r->actors[5].saved_84;else r->actors[5].saved_84=s->actors[5].saved_mode;
 if(save)s->actors[5].anchor_distance=r->actors[5].anchor_distance_8c;else r->actors[5].anchor_distance_8c=s->actors[5].anchor_distance;
 if(save)s->parent.actors[6].id=r->actors[6].id_00;else r->actors[6].id_00=s->parent.actors[6].id;
 if(save)s->parent.actors[6].mode=r->actors[6].mode_5e;else r->actors[6].mode_5e=s->parent.actors[6].mode;
 if(save)s->parent.actors[6].action_timer=r->actors[6].reaction_60;else r->actors[6].reaction_60=s->parent.actors[6].action_timer;
 if(save)s->parent.actors[6].behavior_timer=r->actors[6].behavior_64;else r->actors[6].behavior_64=s->parent.actors[6].behavior_timer;
 if(save)s->parent.actors[6].team_group=r->actors[6].team_6e;else r->actors[6].team_6e=s->parent.actors[6].team_group;
 if(save)s->parent.actors[6].boost_timer=r->actors[6].boost_72;else r->actors[6].boost_72=s->parent.actors[6].boost_timer;
 if(save)s->actors[6].base_assignment=r->actors[6].base_76;else r->actors[6].base_76=s->actors[6].base_assignment;
 if(save)s->actors[6].alternate_assignment=r->actors[6].alternate_78;else r->actors[6].alternate_78=s->actors[6].alternate_assignment;
 if(save)s->parent.actors[6].behavior_flags=r->actors[6].clear_7e;else r->actors[6].clear_7e=s->parent.actors[6].behavior_flags;
 if(save)s->actors[6].saved_mode=r->actors[6].saved_84;else r->actors[6].saved_84=s->actors[6].saved_mode;
 if(save)s->actors[6].anchor_distance=r->actors[6].anchor_distance_8c;else r->actors[6].anchor_distance_8c=s->actors[6].anchor_distance;
 if(save)s->parent.actors[7].id=r->actors[7].id_00;else r->actors[7].id_00=s->parent.actors[7].id;
 if(save)s->parent.actors[7].mode=r->actors[7].mode_5e;else r->actors[7].mode_5e=s->parent.actors[7].mode;
 if(save)s->parent.actors[7].action_timer=r->actors[7].reaction_60;else r->actors[7].reaction_60=s->parent.actors[7].action_timer;
 if(save)s->parent.actors[7].behavior_timer=r->actors[7].behavior_64;else r->actors[7].behavior_64=s->parent.actors[7].behavior_timer;
 if(save)s->parent.actors[7].team_group=r->actors[7].team_6e;else r->actors[7].team_6e=s->parent.actors[7].team_group;
 if(save)s->parent.actors[7].boost_timer=r->actors[7].boost_72;else r->actors[7].boost_72=s->parent.actors[7].boost_timer;
 if(save)s->actors[7].base_assignment=r->actors[7].base_76;else r->actors[7].base_76=s->actors[7].base_assignment;
 if(save)s->actors[7].alternate_assignment=r->actors[7].alternate_78;else r->actors[7].alternate_78=s->actors[7].alternate_assignment;
 if(save)s->parent.actors[7].behavior_flags=r->actors[7].clear_7e;else r->actors[7].clear_7e=s->parent.actors[7].behavior_flags;
 if(save)s->actors[7].saved_mode=r->actors[7].saved_84;else r->actors[7].saved_84=s->actors[7].saved_mode;
 if(save)s->actors[7].anchor_distance=r->actors[7].anchor_distance_8c;else r->actors[7].anchor_distance_8c=s->actors[7].anchor_distance;
 if(save)s->parent.actors[8].id=r->actors[8].id_00;else r->actors[8].id_00=s->parent.actors[8].id;
 if(save)s->parent.actors[8].mode=r->actors[8].mode_5e;else r->actors[8].mode_5e=s->parent.actors[8].mode;
 if(save)s->parent.actors[8].action_timer=r->actors[8].reaction_60;else r->actors[8].reaction_60=s->parent.actors[8].action_timer;
 if(save)s->parent.actors[8].behavior_timer=r->actors[8].behavior_64;else r->actors[8].behavior_64=s->parent.actors[8].behavior_timer;
 if(save)s->parent.actors[8].team_group=r->actors[8].team_6e;else r->actors[8].team_6e=s->parent.actors[8].team_group;
 if(save)s->parent.actors[8].boost_timer=r->actors[8].boost_72;else r->actors[8].boost_72=s->parent.actors[8].boost_timer;
 if(save)s->actors[8].base_assignment=r->actors[8].base_76;else r->actors[8].base_76=s->actors[8].base_assignment;
 if(save)s->actors[8].alternate_assignment=r->actors[8].alternate_78;else r->actors[8].alternate_78=s->actors[8].alternate_assignment;
 if(save)s->parent.actors[8].behavior_flags=r->actors[8].clear_7e;else r->actors[8].clear_7e=s->parent.actors[8].behavior_flags;
 if(save)s->actors[8].saved_mode=r->actors[8].saved_84;else r->actors[8].saved_84=s->actors[8].saved_mode;
 if(save)s->actors[8].anchor_distance=r->actors[8].anchor_distance_8c;else r->actors[8].anchor_distance_8c=s->actors[8].anchor_distance;
 if(save)s->parent.actors[9].id=r->actors[9].id_00;else r->actors[9].id_00=s->parent.actors[9].id;
 if(save)s->parent.actors[9].mode=r->actors[9].mode_5e;else r->actors[9].mode_5e=s->parent.actors[9].mode;
 if(save)s->parent.actors[9].action_timer=r->actors[9].reaction_60;else r->actors[9].reaction_60=s->parent.actors[9].action_timer;
 if(save)s->parent.actors[9].behavior_timer=r->actors[9].behavior_64;else r->actors[9].behavior_64=s->parent.actors[9].behavior_timer;
 if(save)s->parent.actors[9].team_group=r->actors[9].team_6e;else r->actors[9].team_6e=s->parent.actors[9].team_group;
 if(save)s->parent.actors[9].boost_timer=r->actors[9].boost_72;else r->actors[9].boost_72=s->parent.actors[9].boost_timer;
 if(save)s->actors[9].base_assignment=r->actors[9].base_76;else r->actors[9].base_76=s->actors[9].base_assignment;
 if(save)s->actors[9].alternate_assignment=r->actors[9].alternate_78;else r->actors[9].alternate_78=s->actors[9].alternate_assignment;
 if(save)s->parent.actors[9].behavior_flags=r->actors[9].clear_7e;else r->actors[9].clear_7e=s->parent.actors[9].behavior_flags;
 if(save)s->actors[9].saved_mode=r->actors[9].saved_84;else r->actors[9].saved_84=s->actors[9].saved_mode;
 if(save)s->actors[9].anchor_distance=r->actors[9].anchor_distance_8c;else r->actors[9].anchor_distance_8c=s->actors[9].anchor_distance;
 if(save)s->input.anchor_x[0]=r->contexts[0].anchor_0a;else r->contexts[0].anchor_0a=s->input.anchor_x[0];
 if(save)s->contexts[0].order[0]=(uint8_t)r->contexts[0].order_49[0];else r->contexts[0].order_49[0]=s->contexts[0].order[0];
 if(save)s->contexts[0].order[1]=(uint8_t)r->contexts[0].order_49[1];else r->contexts[0].order_49[1]=s->contexts[0].order[1];
 if(save)s->contexts[0].order[2]=(uint8_t)r->contexts[0].order_49[2];else r->contexts[0].order_49[2]=s->contexts[0].order[2];
 if(save)s->contexts[0].order[3]=(uint8_t)r->contexts[0].order_49[3];else r->contexts[0].order_49[3]=s->contexts[0].order[3];
 if(save)s->contexts[0].order[4]=(uint8_t)r->contexts[0].order_49[4];else r->contexts[0].order_49[4]=s->contexts[0].order[4];
 if(save)s->input.anchor_x[1]=r->contexts[1].anchor_0a;else r->contexts[1].anchor_0a=s->input.anchor_x[1];
 if(save)s->contexts[1].order[0]=(uint8_t)r->contexts[1].order_49[0];else r->contexts[1].order_49[0]=s->contexts[1].order[0];
 if(save)s->contexts[1].order[1]=(uint8_t)r->contexts[1].order_49[1];else r->contexts[1].order_49[1]=s->contexts[1].order[1];
 if(save)s->contexts[1].order[2]=(uint8_t)r->contexts[1].order_49[2];else r->contexts[1].order_49[2]=s->contexts[1].order[2];
 if(save)s->contexts[1].order[3]=(uint8_t)r->contexts[1].order_49[3];else r->contexts[1].order_49[3]=s->contexts[1].order[3];
 if(save)s->contexts[1].order[4]=(uint8_t)r->contexts[1].order_49[4];else r->contexts[1].order_49[4]=s->contexts[1].order[4];
 if(save)s->ball_assignment=r->ball_assignment_74;else r->ball_assignment_74=s->ball_assignment;
 if(save)s->ball_alternate_assignment=r->ball_alternate_78;else r->ball_alternate_78=s->ball_alternate_assignment;
 if(save)s->ball_anchor_distance=r->ball_anchor_distance_8c;else r->ball_anchor_distance_8c=s->ball_anchor_distance;
 if(save)s->parent.live_0936=r->live_0936;else r->live_0936=s->parent.live_0936;
 if(save)s->parent.receiver_0946=r->receiver_0946;else r->receiver_0946=s->parent.receiver_0946;
 if(save)s->parent.side_0952=r->side_0952;else r->side_0952=s->parent.side_0952;
 if(save)s->parent.actor_0954=r->inbound_0954;else r->inbound_0954=s->parent.actor_0954;
 if(save)s->rng=r->rng_07f6;else r->rng_07f6=s->rng;
 if(save)s->assignment_sort_slots[2]=r->nearest_offense_09de;else r->nearest_offense_09de=s->assignment_sort_slots[2];
 if(save)s->assignment_sort_slots[4]=r->nearest_anchor_09e2;else r->nearest_anchor_09e2=s->assignment_sort_slots[4];
}

static bool render(NbaPeriodFormationState *s){
 NbaPeriodRenderTail r={0};unsigned i;
 for(i=0;i<10;i++){r.x[i]=s->parent.actors[i].x;r.y[i]=s->parent.actors[i].y;r.depth[i]=s->actors[i].depth;r.collision.x[i]=r.x[i];r.collision.link[i]=s->parent.actors[i].list_link;}
 r.x[10]=s->parent.ball.x;r.y[10]=s->parent.ball.y;r.depth[10]=s->ball_depth;r.collision.x[10]=r.x[10];r.collision.link[10]=s->parent.ball.list_link;
 r.x[11]=s->extra_draw_x;r.y[11]=s->extra_draw_y;r.depth[11]=s->extra_draw_depth;
 for(i=0;i<12;i++){r.draw_order[i]=s->draw_order[i];r.collision.object[i]=s->parent.object_list[i];}
 r.camera_y=s->camera_y;r.leading_sentinel=s->leading_sentinel;r.frame_low=s->frame_low;r.frame_high=s->frame_high;
 if(!nba_period_render_tail(&r))return false;
 for(i=0;i<10;i++){s->actors[i].depth=r.depth[i];s->parent.actors[i].list_link=r.collision.link[i];}
 s->ball_depth=r.depth[10];s->extra_draw_depth=r.depth[11];s->parent.ball.list_link=r.collision.link[10];
 for(i=0;i<12;i++){s->draw_order[i]=r.draw_order[i];s->parent.object_list[i]=r.collision.object[i];}
 s->frame_low=r.frame_low;s->frame_high=r.frame_high;return true;
}
static NbaPeriodFormationBoundary boundary(NbaPeriodFormationKind k,uint32_t pc,uint16_t actor,NbaPeriodFormationRefusal reason){
 NbaPeriodFormationBoundary b={0};b.kind=k;b.source_pc=pc;b.actor=actor;b.refusal=reason;return b;
}
bool nba_period_formation_begin(NbaPeriodFormation *w,const NbaPeriodFormationState *s){
 unsigned i;if(!w||!s||s->leading_sentinel)return false;
 /* Actor/ball IDs are outputs here: parent writes them before any child
  * consumes them. Do not demand invented initial values for those words. */
 for(i=0;i<2;i++)if(s->contexts[i].first_actor_pointer!=(i?0x39eb:0x34eb)||s->contexts[i].opponent_pointer!=(i?0x46eb:0x476b))return false;
 memset(w,0,sizeof(*w));if(!nba_period_restart_begin(&w->parent,&s->input))return false;w->valid=true;return true;
}
NbaPeriodFormationBoundary nba_period_formation_advance(NbaPeriodFormation *w,NbaPeriodFormationState *s,const NbaAssetPack *assets){
 NbaPeriodFormationBoundary b={0};NbaPeriodRestartBoundary p;NbaPeriodFormationRefusal refusal=NBA_PERIOD_FORMATION_NO_REFUSAL;
 if(!w||!s||!assets||!w->valid)return b;if(w->waiting)return w->boundary;
 for(;;){
  if(w->phase==PARENT){
   p=nba_period_restart_advance(&w->parent,&s->parent);if(p.kind==NBA_PERIOD_INVALID||p.kind==NBA_PERIOD_DONE)return b;
   b=boundary(NBA_PERIOD_FORMATION_CHECKPOINT,p.source_pc,p.actor,refusal);w->phase=CHILD;break;
  }
  if(w->phase==AFTER_APPEARANCE){if(!nba_period_restart_resume(&w->parent))return b;w->phase=PARENT;continue;}
  if(w->phase==CHILD){
   p=w->parent.boundary;
   if(p.kind==NBA_PERIOD_APPEARANCE){
    if(!appearance(s,assets,p.actor))refusal=NBA_PERIOD_FORMATION_APPEARANCE_DOMAIN;
    else{b=boundary(NBA_PERIOD_FORMATION_CHECKPOINT,p.source_pc+4,p.actor,refusal);w->phase=AFTER_APPEARANCE;break;}
   }else if(p.kind==NBA_PERIOD_APPEARANCE_GEOMETRY){if(!assignment(s,assets))refusal=NBA_PERIOD_FORMATION_ASSIGNMENT_DOMAIN;}
   else if(p.kind==NBA_PERIOD_OBJECT_SORT){if(!object_sort(s))refusal=NBA_PERIOD_FORMATION_SORT_DOMAIN;}
   else if(p.kind==NBA_PERIOD_CONTROLLER){
    if(!attachment(s,assets))refusal=NBA_PERIOD_FORMATION_ATTACHMENT_DOMAIN;
    else{b=boundary(NBA_PERIOD_FORMATION_CHECKPOINT,0x86e1a4,p.actor,refusal);w->phase=BRIDGE;break;}
   }else if(p.kind==NBA_PERIOD_OPENING){w->phase=BRIDGE;continue;}
   if(refusal){b=boundary(NBA_PERIOD_FORMATION_REFUSED,p.source_pc,p.actor,refusal);break;}
   if(!nba_period_restart_resume(&w->parent))return b;w->phase=PARENT;continue;
  }
  if(w->phase==BRIDGE){
   if(!bridge(s))b=boundary(NBA_PERIOD_FORMATION_REFUSED,0x86e1ac,UINT16_MAX,NBA_PERIOD_FORMATION_CONTROLLER_DOMAIN);
   else{b=boundary(NBA_PERIOD_FORMATION_CHECKPOINT,0x86e1e5,UINT16_MAX,refusal);w->phase=ROLES;}break;
  }
  if(w->phase==ROLES){
   NbaPeriodRoleStateV2 r={0};NbaPeriodRolesV2 rw;NbaPeriodRoleBoundaryV2 rb;
   if(!roles_have_defined_nearest(s)){b=boundary(NBA_PERIOD_FORMATION_REFUSED,0x86e1e5,UINT16_MAX,NBA_PERIOD_FORMATION_ROLE_CARRIED_NEAREST);break;}
   /* Zeroed unprojected CPU scratch is dead: each consumed value is written
    * by the source prefix before use, and carried92 cases are refused above. */
   role_state(s,&r,false);
   if(!nba_period_roles_v2_begin(&rw,&r)){b=boundary(NBA_PERIOD_FORMATION_REFUSED,0x86e1e5,UINT16_MAX,NBA_PERIOD_FORMATION_ROLE_DOMAIN);break;}
   rb=nba_period_roles_v2_advance(&rw,&r);
   if(rb.kind==NBA_PERIOD_ROLES_V2_FIRST_RETURN){if(!nba_period_roles_v2_resume(&rw))return b;rb=nba_period_roles_v2_advance(&rw,&r);}
   role_state(s,&r,true);
   if(rb.kind!=NBA_PERIOD_ROLES_V2_COMPLETE){b=boundary(NBA_PERIOD_FORMATION_ROLE_STOP,rb.source_pc,UINT16_MAX,refusal);b.role=rb;break;}
   b=boundary(NBA_PERIOD_FORMATION_CHECKPOINT,0x86e1f7,UINT16_MAX,refusal);w->phase=RENDER;break;
  }
  if(w->phase==RENDER){
   if(!render(s))b=boundary(NBA_PERIOD_FORMATION_REFUSED,0x86e1f7,UINT16_MAX,NBA_PERIOD_FORMATION_RENDER_DOMAIN);
   else{b=boundary(NBA_PERIOD_FORMATION_COMPLETE,0x86e207,UINT16_MAX,refusal);w->phase=FINISHED;}break;
  }
  return b;
 }
 w->boundary=b;w->waiting=true;return b;
}
bool nba_period_formation_resume(NbaPeriodFormation *w){
 if(!w||!w->valid||!w->waiting||w->boundary.kind!=NBA_PERIOD_FORMATION_CHECKPOINT)return false;w->waiting=false;return true;
}
