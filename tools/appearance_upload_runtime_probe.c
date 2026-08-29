#include <stdio.h>
#include "nba_tipoff.h"
static uint16_t iw(int32_t fp){return (uint16_t)(fp>>8);}
int main(int argc,char **argv){
 if(argc!=2)return 2;NbaAssetPack p={0};NbaSession s;NbaTipoff g;
 if(!nba_assets_load(&p,argv[1]))return 3;nba_session_init(&s);
 if(!nba_tipoff_init(&g,&p,&s))return 4;
 printf("%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
  (uint16_t)g.possession_actor,(uint16_t)(int16_t)(int8_t)g.camera_side_group_raw,g.dead_ball_raw_097e,
  g.catch_actor_record_raw_0910,g.inbound_state_raw,(uint16_t)g.inbound_layout_raw,
  g.live_state_raw,g.inbound_timer_raw,g.inbound_actor_raw,iw(g.ball.x_fp),iw(g.ball.y_fp),
  iw(g.ball.z_fp),g.dead_ball_raw_0968,g.attached_ball_state_raw_09f6,
  (uint16_t)g.ball.velocity_z,(uint16_t)g.ball.velocity_x,(uint16_t)g.ball.velocity_y,
  (uint16_t)g.role_focal_x_raw_0918,(uint16_t)g.role_focal_y_raw_091a);
 nba_assets_free(&p);return 0;
}
