#include <stdio.h>
#include <string.h>
#include "nba_tipoff.h"

/* Binding tests: independent ROM replay verifies helpers; this probe checks
 * that the live dispatcher supplies their real configuration/roster inputs
 * and commits their outputs before the later actor and score writers. */
static int exercise(const NbaAssetPack *pack,unsigned enabled) {
    NbaSession session;NbaTipoff game;NbaInput input={0};
    nba_session_init(&session);session.config.rules[11]=(uint16_t)enabled;
    session.config.options[6]=1;
    if(!nba_tipoff_init(&game,pack,&session))return 1;
    unsigned updates=0,makes=0,assisted_makes=0;
    for(unsigned frame=1;frame<=16000;++frame) {
        if(enabled && frame==221) {
            /* Clearly controlled C integration fixture: enter the late-clock
             * boundary with a three-point deficit; do not force a basket. */
            game.match_clock_raw_0928=7199;
            session.score[0]=97;session.score[1]=100;
        }
        NbaTipoff before=game;
        uint16_t scores[2]={session.score[0],session.score[1]};
        NbaShotFatigue expected=before.fatigue;
        NbaShotClock clock={before.live_state_raw,before.period_raw_0926,
            before.match_clock_raw_0928,before.rim_raw_092c,
            before.shot_clock_mirror_raw_09c6,before.dead_clock_enabled_raw_0a04,
            expected.timer,before.free_throw_flight_timer_raw_0930,
            session.config.rules[8],before.elapsed_clock_raw_13f9,before.elapsed_shot_clock_raw_13f7};
        if(frame>NBA_TIPOFF_BREAK_FRAME)nba_shot_clock_step(&clock);
        expected.timer=clock.fatigue_timer;
        if(frame>=NBA_TIPOFF_BREAK_FRAME && !(frame&1)) {
            expected.live_state=before.live_state_raw;
            expected.enabled=(uint16_t)enabled;
            expected.quarter=session.config.main_values[3];
            for(unsigned i=0;i<10;++i) {
                expected.active_roster[i]=(uint16_t)((i<5?0:12)+before.actors[i].roster_slot);
                expected.boost[i]=before.actors[i].movement_boost_timer;
            }
            if(!nba_shot_fatigue_step(pack,&expected))return 2;
        }
        nba_tipoff_update(&game,&input);
        if(memcmp(&expected,&game.fatigue,sizeof(expected)))return 3;
        if(game.fatigue.playing_seconds[0]!=before.fatigue.playing_seconds[0])++updates;
        for(unsigned i=0;i<24;++i) {
            if((i%12)>=5 && (game.fatigue.stamina[i]!=0x7FFF || game.fatigue.playing_seconds[i]))return 4;
            if(!enabled && game.fatigue.stamina[i]!=0x7FFF)return 5;
        }
        if(frame>=NBA_TIPOFF_BREAK_FRAME)for(unsigned i=0;i<10;++i)
            if(game.actors[i].shot_stamina_raw_18!=game.fatigue.stamina[game.fatigue.active_roster[i]])return 6;
        if(scores[0]!=session.score[0] || scores[1]!=session.score[1]) {
            NbaShotMomentum expected_make={0};
            for(unsigned i=0;i<10;++i) {
                expected_make.made_run[i]=before.actors[i].shot_modifier_raw_b2;
                expected_make.defensive_run[i]=before.actors[i].defensive_run_raw_b4;
                expected_make.team_group[i]=before.actors[i].team_group_raw_6e;
            }
            if(!nba_shot_momentum_make(&expected_make,(uint16_t)game.shot_actor_raw_09c8,
                session.config.options[6],clock.clock,scores[0],scores[1]))return 7;
            for(unsigned i=0;i<10;++i)
                if(expected_make.made_run[i]!=game.actors[i].shot_modifier_raw_b2 ||
                   expected_make.defensive_run[i]!=game.actors[i].defensive_run_raw_b4)return 8;
            if(expected_make.assistance_team!=game.assistance_team_raw_09c0)return 9;
            if(game.assistance_team_raw_09c0!=0xFFFF)++assisted_makes;
            ++makes;
        }
    }
    if(!updates || !makes || (enabled && !assisted_makes))return 10;
    printf("[SHOT STATE RUNTIME] fatigue=%u updates=%u made=%u assisted=%u roster/clock/score binding PASS\n",enabled,updates,makes,assisted_makes);
    return 0;
}
int main(int argc,char **argv) {
    NbaAssetPack pack;if(argc!=2 || !nba_assets_load(&pack,argv[1]))return 20;
    int result=exercise(&pack,0);if(!result)result=exercise(&pack,1);
    nba_assets_free(&pack);
    if(result)fprintf(stderr,"shot-state runtime check %d failed\n",result);
    return result;
}
