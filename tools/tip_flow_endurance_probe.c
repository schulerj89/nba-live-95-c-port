#include "nba_tipoff.h"
#include <stdio.h>
int main(int argc,char **argv) {
    NbaAssetPack pack={0};NbaSession session;NbaTipoff game;NbaInput input={0};
    if(argc!=2||!nba_assets_load(&pack,argv[1]))return 2;
    nba_session_init(&session);if(!nba_tipoff_init(&game,&pack,&session))return 3;
    unsigned dead=0;
    for(unsigned frame=1;frame<=63800;++frame) {
        unsigned old_ball=game.ball.state;bool had_mode12=false;
        for(unsigned i=0;i<10;++i)if(game.actors[i].control_mode==12)had_mode12=true;
        nba_tipoff_update(&game,&input);
        if(old_ball==NBA_BALL_ATTACHED && game.ball.state==NBA_BALL_SHOT &&
           game.ball_activity_raw==0xffff && had_mode12 &&
           (game.shot_actor_raw_09c8<0 || game.shot_actor_raw_09c8>=10 ||
            game.rim_raw_096a<1 || game.rim_raw_096a>3))
            printf("SHOT-LATCH diagnostic f%u actor=%d latch=%u FT=%u Z=%d state=%u\n",frame,
                game.shot_actor_raw_09c8,game.rim_raw_096a,game.fouls.free_throw_state_raw_0978,
                game.ball.z_fp/256,game.live_state_raw);
        dead=game.live_state_raw>=0x80?dead+1:0;
        if(dead>2400 || frame==35126 || frame==36000) {
            printf("f%u state=%x owner=%d ball=%d,%d,%d mode=%u inbound=%u ready=%u timer=%u transfer=%u FT=%u foul=%u whistle=%u scores=%u/%u clock=%u\n",
                frame,game.live_state_raw,game.possession_actor,game.ball.x_fp/256,game.ball.y_fp/256,game.ball.z_fp/256,
                game.ball.state,game.inbound_actor_raw,game.inbound_ready_raw,game.inbound_timer_raw,game.inbound_transfer_raw,
                game.fouls.free_throw_state_raw_0978,game.fouls.shooting_foul_raw_09bc,game.fouls.whistle_active_raw_09b6,
                session.score[0],session.score[1],game.match_clock_raw_0928);
            for(unsigned i=0;i<10;++i)printf("actor%u x/y=%d/%d mode=%u vx/y=%d/%d target=%d/%d flag=%x phase=%u family=%d\n",i,
                game.actors[i].x_fp/256,game.actors[i].y_fp/256,game.actors[i].control_mode,game.actors[i].velocity_x,
                game.actors[i].velocity_y,game.actors[i].target_x,game.actors[i].target_y,game.actors[i].behavior_flags_raw,
                game.actors[i].upper_animation_phase_raw,game.actors[i].pass_family_raw);
            if(dead>2400)return 4;
        }
    }
    puts("TIP FLOW endurance PASS: 63800 frames, no long dead-ball stall");nba_assets_free(&pack);return 0;
}
