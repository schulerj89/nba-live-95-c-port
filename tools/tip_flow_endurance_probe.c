#include "nba_tipoff.h"
#include <stdio.h>
int main(int argc,char **argv) {
    NbaAssetPack pack={0};NbaSession session;NbaTipoff game;NbaInput input={0};
    if(argc!=2||!nba_assets_load(&pack,argv[1]))return 2;
    nba_session_init(&session);if(!nba_tipoff_init(&game,&pack,&session))return 3;
    unsigned dead=0;
    uint16_t old_transfer=game.inbound_transfer_raw;
    int16_t old_receiver=game.pass_receiver_raw;
    uint16_t highest_period=game.period_raw_0926;
    unsigned post_restart_live=0;
    bool canceled_receiver_seen=false,canceled_transfer_recovered=false;
    for(unsigned frame=1;frame<=63800;++frame) {
        unsigned old_ball=game.ball.state;bool had_mode12=false;
        for(unsigned i=0;i<10;++i)if(game.actors[i].control_mode==12)had_mode12=true;
        nba_tipoff_update(&game,&input);
        if(old_receiver>=0 && game.pass_receiver_raw<0 &&
           game.live_state_raw==0x82u && game.inbound_transfer_raw!=0u)
            canceled_receiver_seen=true;
        if(canceled_receiver_seen && old_transfer!=0u &&
           game.inbound_transfer_raw==0u)
            canceled_transfer_recovered=true;
        old_transfer=game.inbound_transfer_raw;
        old_receiver=game.pass_receiver_raw;
        if(game.period_raw_0926>highest_period)
            highest_period=game.period_raw_0926;
        if(game.period_raw_0926>0u && game.live_state_raw<0x80u)
            ++post_restart_live;
        if(old_ball==NBA_BALL_ATTACHED && game.ball.state==NBA_BALL_SHOT &&
           game.ball_activity_raw==0xffff && had_mode12 &&
           (game.shot_actor_raw_09c8<0 || game.shot_actor_raw_09c8>=10 ||
            game.rim_raw_096a<1 || game.rim_raw_096a>3))
            printf("SHOT-LATCH diagnostic f%u actor=%d latch=%u FT=%u Z=%d state=%u\n",frame,
                game.shot_actor_raw_09c8,game.rim_raw_096a,game.fouls.free_throw_state_raw_0978,
                game.ball.z_fp/256,game.live_state_raw);
        dead=game.live_state_raw>=0x80?dead+1:0;
        if(dead>2400 || frame==35126 || frame==36000) {
            printf("f%u state=%x owner=%d ballowner=%d ball=%d,%d,%d vel=%d,%d,%d mode=%u pass=%d/%d activity=%u inbound=%u ready=%u timer=%u transfer=%u FT=%u foul=%u whistle=%u scores=%u/%u clock=%u\n",
                frame,game.live_state_raw,game.possession_actor,game.ball.owner_actor,
                game.ball.x_fp/256,game.ball.y_fp/256,game.ball.z_fp/256,
                game.ball.velocity_x,game.ball.velocity_y,game.ball.velocity_z,
                game.ball.state,game.pass_actor_raw,game.pass_receiver_raw,game.ball_activity_raw,
                game.inbound_actor_raw,game.inbound_ready_raw,game.inbound_timer_raw,game.inbound_transfer_raw,
                game.fouls.free_throw_state_raw_0978,game.fouls.shooting_foul_raw_09bc,game.fouls.whistle_active_raw_09b6,
                session.score[0],session.score[1],game.match_clock_raw_0928);
            for(unsigned i=0;i<10;++i)printf("actor%u x/y=%d/%d mode=%u vx/y=%d/%d target=%d/%d flag=%x phase=%u family=%d\n",i,
                game.actors[i].x_fp/256,game.actors[i].y_fp/256,game.actors[i].control_mode,game.actors[i].velocity_x,
                game.actors[i].velocity_y,game.actors[i].target_x,game.actors[i].target_y,game.actors[i].behavior_flags_raw,
                game.actors[i].upper_animation_phase_raw,game.actors[i].pass_family_raw);
            if(dead>2400)return 4;
        }
    }
    if(highest_period<1u || post_restart_live<600u ||
       !canceled_receiver_seen || !canceled_transfer_recovered) {
        fprintf(stderr,"TIP FLOW lifecycle coverage missing: period=%u post=%u canceled=%u recovered=%u\n",
            highest_period,post_restart_live,canceled_receiver_seen,
            canceled_transfer_recovered);
        return 5;
    }
    puts("TIP FLOW endurance PASS: 63800 frames, period restart + canceled transfer recovered");nba_assets_free(&pack);return 0;
}
