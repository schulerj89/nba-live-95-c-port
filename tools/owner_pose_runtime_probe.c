#include <stdio.h>
#include "nba_tipoff.h"

/* Unforced dispatcher observation. No seed, roster, clock or shot injection. */
static int exercise(const NbaAssetPack *pack,bool reverse,unsigned *total_specials) {
    NbaSession session;NbaTipoff game;NbaInput input={0};
    nba_session_init(&session);
    if(reverse) {session.left_team=18;session.right_team=3;}
    if(!nba_tipoff_init(&game,pack,&session))return 3;
    unsigned states[2]={0},first[2]={0},serial=0,specials=0;
    int special_actor=-1;unsigned selected_frame=0,pose=0;bool airborne=false;
    for(unsigned frame=1;frame<=200000;++frame) {
        uint16_t prior_phase=special_actor<0?0:game.actors[special_actor].rom_upper_animation_phase_raw_3a;
        nba_tipoff_update(&game,&input);
        if(special_actor>=0) {
            airborne|=game.actors[special_actor].z_fp>0;
            if(game.ball.owner_actor!=special_actor) {
                if(!airborne || prior_phase<3 || game.ball.owner_actor!=-1 ||
                   game.ball.state!=NBA_BALL_SHOT || game.actors[special_actor].animation_state!=pose ||
                   game.shot_actor_raw_09c8!=special_actor || game.shot_value_raw!=2)return 6;
                printf("[NATURAL SPECIAL] teams=%u/%u actor=%d selected=%u released=%u pose=%02x\n",
                    session.left_team,session.right_team,special_actor,selected_frame,frame,pose);
                special_actor=-1;++specials;
            } else if(frame-selected_frame>=120)return 7;
        }
        for(unsigned i=0;i<10;++i) {
            const NbaTipoffActor *a=&game.actors[i];
            if(a->animation_state==13 || a->animation_state==18) {
                unsigned which=a->animation_state==18;
                ++states[which];if(!first[which])first[which]=frame;
                if(!a->animation_resources_valid ||
                   a->rom_upper_animation_phase_raw_3a>=(which?8:2))return 4;
                NbaGameplayTelemetry telemetry={0};
                nba_tipoff_capture_telemetry(&game,&input,&telemetry);
                if(telemetry.actors[i].animation_phase_target_raw_b0!=a->upper_phase_target_raw_b0)return 11;
            }
        }
        if(game.shot_selection_serial!=serial) {
            serial=game.shot_selection_serial;
            if(game.shot_selection_inputs[6]==17) {
                special_actor=game.shot_selection_inputs[7];selected_frame=frame;
                if(special_actor>=10 || game.ball.owner_actor!=special_actor)return 8;
                pose=game.actors[special_actor].animation_state;airborne=false;
                if(pose!=0x14 && pose!=0x15)return 9;
            }
        }
    }
    printf("[OWNER POSE RUNTIME] teams=%u/%u state13=%u first=%u state18=%u first=%u selectors=%u specials=%u score=%u-%u\n",
        session.left_team,session.right_team,states[0],first[0],states[1],first[1],serial,specials,session.score[0],session.score[1]);
    *total_specials+=specials;
    return states[0] && states[1] && special_actor<0 ? 0:5;
}

int main(int argc,char **argv) {
    NbaAssetPack pack;if(argc!=2 || !nba_assets_load(&pack,argv[1]))return 2;
    unsigned specials=0;
    int result=exercise(&pack,false,&specials);
    if(!result)result=exercise(&pack,true,&specials);
    nba_assets_free(&pack);
    if(!result && !specials)result=10;
    if(result)fprintf(stderr,"owner pose/natural-special runtime check %d failed\n",result);
    return result;
}
