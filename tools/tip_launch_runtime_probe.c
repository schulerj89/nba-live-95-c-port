#include "nba_tipoff.h"
#include <stdio.h>
int main(int argc,char **argv) {
    NbaAssetPack pack={0};NbaSession session;NbaTipoff t;NbaInput input={0};
    if(argc!=2||!nba_assets_load(&pack,argv[1]))return 2;
    nba_session_init(&session);if(!nba_tipoff_init(&t,&pack,&session))return 3;
    t.frame=200;t.simulation_tick=200;t.tip_contact_actor=5;t.tip_contact_frame=200;
    t.tip_winner_group_raw_0932=5;t.camera_side_group_raw=5;t.pass_receiver_raw=8;
    t.actors[5].pass_family_raw=-1;t.actors[5].pass_band_raw=12;t.actors[5].control_mode=11;
    t.actors[8].control_mode=10;t.ball.x_fp=0x55;t.ball.y_fp=0x66;t.ball.z_fp=61*256+0x77;
    if(!nba_tipoff_launch_tip_ball(&t))return 4;
    if(t.ball.x_fp!=-9*256+0x55 || t.ball.y_fp!=4*256+0x66 || t.ball.z_fp!=61*256+0x77 ||
       t.ball.velocity_x!=-1109 || t.ball.velocity_y!=597 || t.ball.velocity_z!=0 ||
       t.actors[8].reaction_threshold!=39 || t.actors[5].control_mode!=1 ||
       t.actors[0].contact_inhibit_raw_5a!=20 || t.actors[5].contact_inhibit_raw_5a!=20 ||
       t.ball.owner_actor!=-1 || t.possession_actor!=-1)return 5;
    int32_t x=t.ball.x_fp,z=t.ball.z_fp;
    nba_tipoff_update(&t,&input);
    if(t.ball.x_fp!=x || t.ball.z_fp!=z)return 6;
    nba_tipoff_update(&t,&input);
    if(t.ball.x_fp>=x || t.ball.z_fp>=z || t.ball.velocity_z>=0)return 7;
    puts("TIP LAUNCH binding PASS: native launch, fraction preservation, height restore, inhibits, detached physics cadence");
    nba_assets_free(&pack);return 0;
}
