/* C1 diagnostic: includes the actual implementation to exercise its private
 * contact child and the public actor loop. No alternative gameplay body. */
#define _CRT_SECURE_NO_WARNINGS
#include "../src/nba_tipoff.c"
#include <stdio.h>
#include <stdlib.h>

static NbaTipoff game;
static NbaSession session;
static unsigned recovery_stage;
static void observe(const NbaTipoff *t,const char *stage,void *unused) {
    (void)unused;
    if(t->simulation_tick==41908u && strcmp(stage,"actors.end")==0) {
        const NbaTipoffActor *a=&t->actors[0];
        printf("RECOVERY_STAGE %u %u %u %u %u %u\n",a->control_mode,
            a->contact_action_timer_raw_60,a->contact_inhibit_raw_5a,
            a->behavior_flags_raw,a->actor_status_raw_28,a->behavior_timer);
        recovery_stage=a->control_mode==11u && !a->contact_action_timer_raw_60 &&
            !a->contact_inhibit_raw_5a && !a->behavior_flags_raw &&
            !a->actor_status_raw_28 && a->behavior_timer==0x2fu;
    }
}

static void controlled(void) {
    for (unsigned bank=0;bank<3u;++bank)
    for (unsigned n=1;n<=256u;++n)
    for (unsigned owner=0;owner<2u;++owner)
    for (unsigned boost=0;boost<2u;++boost)
    for (unsigned mi=0;mi<3u;++mi) {
        static const uint8_t modes[3]={15,10,14};
        unsigned seed=(bank==0u?0u:bank==1u?0x3000u:0xB000u)+n;
        memset(&game,0,sizeof(game));memset(&session,0,sizeof(session));
        game.session=&session;game.cpu_vs_cpu=true;
        game.possession_actor=owner?9:0;game.possession_team=owner?1:0;
        game.ball.owner_actor=game.possession_actor;
        game.ball.state=NBA_BALL_ATTACHED;
        game.ball.velocity_x=71;game.ball.velocity_y=72;game.ball.velocity_z=73;
        game.live_state_raw=0;game.pass_actor_raw=0;game.pass_receiver_raw=4;
        game.pass_aux_raw=2;game.pass_active_raw=0x1234;
        game.rim_raw_094a=0x47;game.catch_actor_record_raw_0910=0x55AA;
        game.fouls.whistle_active_raw_09b6=1; /* original C4FE early return */
        game.rng.state=(uint16_t)seed;
        NbaTipoffActor *v=&game.actors[0],*h=&game.actors[5];
        v->control_mode=modes[mi];v->saved_control_mode=0x25;
        v->pass_band_raw=12;v->pass_family_raw=5;v->pass_released_raw=false;
        v->contact_action_timer_raw_60=0x123;v->contact_inhibit_raw_5a=0x44;
        v->special_contact_raw_56=0x333;v->pass_direction_raw=0x66;
        v->movement_boost_timer=boost?5:0;v->controller_assignment_raw=-1;
        h->controller_assignment_raw=-1;
        h->velocity_x=boost?1600:640;h->velocity_y=boost?800:-320;
        h->movement_magnitude_raw=boost?1800:720;
        bool accepted=cpu_try_player_knockdown_contact(&game,0,5);
        printf("C %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
            seed,owner,boost,modes[mi],accepted?1u:0u,game.rng.state,
            (uint16_t)(int16_t)game.possession_actor,v->control_mode,v->animation_state,
            (uint16_t)v->velocity_z,(uint16_t)game.ball.velocity_z,
            (uint16_t)game.pass_actor_raw,(uint16_t)game.pass_receiver_raw,
            game.pass_active_raw,game.catch_actor_record_raw_0910,
            h->contact_inhibit_raw_5a,v->contact_action_timer_raw_60,
            (uint16_t)v->special_contact_raw_56,v->saved_control_mode,
            v->pass_band_raw,(uint16_t)v->pass_family_raw,v->pass_released_raw?1u:0u,
            (uint16_t)game.pass_aux_raw,game.rim_raw_094a,
            game.deferred_shot_foul_phase_raw_0a02);
    }
}

static int runtime(const char *pack) {
    NbaAssetPack assets={0};NbaInput input={0};
    if(!nba_assets_load(&assets,pack))return 3;
    nba_session_init(&session);
    if(!nba_tipoff_init(&game,&assets,&session))return 4;
    game.differential_observer=observe;
    unsigned seen=0,steps=0,recovered=0;
    for(unsigned frame=1;frame<=42000u;++frame) {
        NbaTipoffActor before=game.actors[0];
        nba_tipoff_update(&game,&input);
        NbaTipoffActor *a=&game.actors[0];
        if(frame>=41865u && frame<=41920u)
            printf("R %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
              frame,a->control_mode,a->animation_state,(uint16_t)(int16_t)game.possession_actor,
              (uint16_t)game.pass_actor_raw,(uint16_t)game.pass_receiver_raw,
              game.pass_active_raw,a->pass_released_raw?1u:0u,a->contact_action_timer_raw_60,
              a->contact_inhibit_raw_5a,a->behavior_flags_raw,a->actor_status_raw_28,
              a->behavior_timer,game.rng.state,game.catch_actor_record_raw_0910);
        if(frame==41876u) {
            if(before.control_mode!=15u || a->control_mode!=8u ||
               a->animation_state!=0x35u || game.possession_actor!=0 ||
               game.pass_actor_raw!=0 || game.pass_receiver_raw!=4 ||
               game.pass_active_raw!=1u || a->pass_released_raw ||
               a->contact_action_timer_raw_60!=30u || game.rng.state!=98u ||
               game.player_contact_routine_raw!=0x86BFBAu)return 5;
            ++seen;
        }
        if(frame>41876u && frame<41908u && !(frame&1u)) {
            if(a->control_mode!=8u || game.possession_actor!=0 ||
               game.pass_actor_raw!=0 || game.pass_receiver_raw!=4 ||
               game.pass_active_raw!=1u || a->pass_released_raw ||
               a->contact_action_timer_raw_60!=30u-(frame-41876u))return 6;
            ++steps;
        }
        if(frame==41908u) {
            if(a->control_mode!=11u || game.possession_actor!=0 ||
               game.pass_actor_raw!=0 || game.pass_receiver_raw!=4 ||
               game.pass_active_raw!=1u || a->pass_released_raw ||
               a->contact_action_timer_raw_60 || a->contact_inhibit_raw_5a ||
               !recovery_stage)return 7;
            /* The later real mode11 behavior pass runs after actors.end;
             * do not confuse its new flags/timer/RNG with C6AD's return. */
            ++recovered;
        }
    }
    printf("PASS_RUNTIME %u %u %u\n",seen,steps,recovered);
    nba_assets_free(&assets);return seen==1u&&steps==15u&&recovered==1u?0:8;
}
int main(int argc,char **argv) {
    if(argc==2 && strcmp(argv[1],"--controlled")==0){controlled();return 0;}
    if(argc==2)return runtime(argv[1]);
    return 2;
}
