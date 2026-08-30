#include "nba_tipoff.h"
#include <stdio.h>
#include <string.h>
#include "../tests/fixtures/inbound-cancel-recovery.h"

static int32_t native_fixed(uint16_t fraction,uint16_t integer) {
    return (int32_t)(int16_t)integer*256+(fraction>>8);
}

/* This binds the complete captured inputs used by F4F2-F58E. The adapter
 * does not run the earlier F43A steering prefix, CPU selector or NMI. The
 * fixture retains every original byte/register for independent auditing. */
static void bind_recovery(NbaTipoff *s,const InboundCancelWitness *v) {
    const uint16_t *g=v->globals,*a=v->actor,*b=v->ball;
    s->rng.state=g[0];s->inbound_timer_raw=g[1];s->live_state_raw=g[2];
    s->possession_actor=(int8_t)(int16_t)g[3];
    s->pass_actor_raw=(int16_t)g[4];s->pass_aux_raw=(int16_t)g[5];
    s->pass_receiver_raw=(int16_t)g[6];s->ball_activity_raw=g[7];
    s->inbound_state_raw=g[8];s->inbound_actor_raw=g[9];
    s->inbound_target_x_raw=(int16_t)g[10];s->inbound_target_y_raw=(int16_t)g[11];
    s->inbound_direction_raw=g[12];s->fouls.foul_event_raw_0964=g[13];
    s->dead_ball_raw_0968=g[14];s->fouls.free_throw_state_raw_0978=g[15];
    s->play_code=g[16];s->fouls.whistle_active_raw_09b6=g[17];
    s->inbound_transfer_raw=g[18];s->inbound_ready_raw=g[19];
    s->pass_active_raw=g[20];s->attached_ball_state_raw_09f6=g[21];
    s->fouls.substitution_request_raw_0a08=g[22];
    NbaTipoffActor *owner=&s->actors[a[0]];
    owner->x_fp=native_fixed(a[1],a[2]);owner->y_fp=native_fixed(a[3],a[4]);
    owner->z_fp=native_fixed(a[5],a[6]);
    owner->velocity_x=(int16_t)a[7];owner->velocity_y=(int16_t)a[8];
    owner->velocity_z=(int16_t)a[9];owner->controller_assignment_raw=(int8_t)(int16_t)a[10];
    owner->actor_status_raw_28=a[11];owner->upper_animation_resource_raw_2a=a[12];
    owner->lower_animation_resource_raw_2c=a[13];owner->animation_resources_valid=true;
    owner->animation_state=(uint8_t)a[14];owner->lower_animation_state=(uint8_t)a[15];
    owner->base_animation_state_raw_38=(uint8_t)a[16];
    owner->upper_animation_phase_raw=a[17];owner->rom_upper_animation_phase_raw_3a=a[17];
    owner->lower_animation_phase_raw=a[18];owner->rom_lower_animation_phase_raw_3c=a[18];
    owner->movement_magnitude_raw=a[19];owner->movement_direction=(uint8_t)a[20];
    owner->requested_direction=(uint8_t)a[21];owner->direction=(uint8_t)a[20];
    owner->target_x=(int16_t)a[23];owner->target_y=(int16_t)a[24];
    owner->control_mode=(uint8_t)a[25];owner->reaction_threshold=a[26];
    owner->assignment_base_raw=a[27];owner->assignment_current_raw=a[28];
    owner->behavior_flags_raw=a[29];owner->free_throw_launch_half_raw_a8=a[30];
    owner->movement_boost_timer=a[31];
    s->ball.x_fp=native_fixed(b[0],b[1]);s->ball.y_fp=native_fixed(b[2],b[3]);
    s->ball.z_fp=native_fixed(b[4],b[5]);s->ball.velocity_x=(int16_t)b[6];
    s->ball.velocity_y=(int16_t)b[7];s->ball.velocity_z=(int16_t)b[8];
}

static bool recovery_contract(const NbaAssetPack *pack) {
    for(unsigned i=0;i<sizeof(inbound_cancel_witnesses)/sizeof(inbound_cancel_witnesses[0]);++i) {
        const InboundCancelWitness *v=&inbound_cancel_witnesses[i];
        NbaSession session;NbaTipoff s={0};nba_session_init(&session);
        s.assets=pack;s.session=&session;bind_recovery(&s,v);
        /* Host scheduling and caches are binding sentinels, not ROM inputs.
         * In particular stale receiver_actor=4 must not replace raw0946. */
        s.simulation_tick=2;s.receiver_actor=4;s.handler_actor=(uint8_t)s.possession_actor;
        s.ball.state=NBA_BALL_ATTACHED;s.ball.owner_actor=s.possession_actor;
        NbaTipoffActor *a=&s.actors[s.possession_actor];
        int32_t x=a->x_fp,y=a->y_fp,z=a->z_fp;
        nba_tipoff_replay_inbound_continuation(&s);
        uint16_t actual[10]={s.dead_ball_raw_0968,s.attached_ball_state_raw_09f6,
            a->behavior_flags_raw,(uint16_t)a->velocity_x,(uint16_t)a->velocity_y,
            s.inbound_ready_raw,s.fouls.whistle_active_raw_09b6,s.fouls.foul_event_raw_0964,
            s.inbound_transfer_raw,a->direction};
        if(memcmp(actual,v->expected,sizeof(actual))!=0 ||
           s.pass_receiver_raw!=(int16_t)v->globals[6] || s.receiver_actor!=4 ||
           s.possession_actor!=(int8_t)v->globals[3] || s.inbound_actor_raw!=v->globals[9] ||
           s.inbound_timer_raw!=v->globals[1] || s.rng.state!=v->globals[0] ||
           a->x_fp!=x || a->y_fp!=y || a->z_fp!=z) {
            fprintf(stderr,"INBOUND CANCEL native projection FAIL case=%u frame=%u transfer=%x expected=%x receiver=%d\n",
                i+1,v->frame,s.inbound_transfer_raw,v->expected[8],s.pass_receiver_raw);return false;
        }
    }
    /* Full production scheduler regression: an attached host ball must not
     * return early before F57F. This is a binding extension of native case1,
     * not an exact native whole-frame oracle. Other actors use normal init. */
    NbaSession session;NbaTipoff s;NbaInput input={0};nba_session_init(&session);
    if(!nba_tipoff_init(&s,pack,&session))return false;
    bind_recovery(&s,&inbound_cancel_witnesses[0]);
    s.simulation_tick=1;s.tip_contact_actor=s.possession_actor;s.tip_possession_frame=1;
    s.phase=NBA_TIPOFF_LIVE;s.cpu_play_state=NBA_CPU_PLAY_PASS;
    s.handler_actor=(uint8_t)s.possession_actor;s.receiver_actor=4;
    s.ball.state=NBA_BALL_ATTACHED;s.ball.owner_actor=s.possession_actor;
    nba_tipoff_update(&s,&input);
    if(s.inbound_transfer_raw!=0 || s.pass_receiver_raw!=-1 ||
       s.inbound_ready_raw!=1 || s.live_state_raw!=0x82 || s.possession_actor!=3) {
        fprintf(stderr,"INBOUND CANCEL production binding FAIL transfer=%u receiver=%d ready=%u live=%x owner=%d\n",
            s.inbound_transfer_raw,s.pass_receiver_raw,s.inbound_ready_raw,s.live_state_raw,s.possession_actor);return false;
    }
    puts("INBOUND CANCEL recovery PASS: four controlled native projections + attached whole-update binding");
    return true;
}

int main(int argc,char **argv) {
    NbaAssetPack pack={0};NbaSession session;NbaTipoff game;NbaInput input={0};
    if((argc!=2 && argc!=3) ||
       (argc==3 && strcmp(argv[2],"--recovery-only")!=0) ||
       !nba_assets_load(&pack,argv[1]))return 2;
    if(!recovery_contract(&pack))return 6;
    if(argc==3 && strcmp(argv[2],"--recovery-only")==0){nba_assets_free(&pack);return 0;}
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
        if(dead>2400) {
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
    /* A native correctness fix can change which random contacts occur in
     * this finite journey. Cancellation remains mandatory above through the
     * captured production replay; natural occurrence is telemetry only. */
    if(highest_period<1u || post_restart_live<600u) {
        fprintf(stderr,"TIP FLOW lifecycle coverage missing: period=%u post=%u canceled=%u recovered=%u\n",
            highest_period,post_restart_live,canceled_receiver_seen,
            canceled_transfer_recovered);
        return 5;
    }
    printf("TIP FLOW endurance PASS: 63800 frames period=%u post_restart_live=%u natural_canceled=%u natural_recovered=%u; deterministic recovery required\n",
        highest_period,post_restart_live,canceled_receiver_seen,canceled_transfer_recovered);
    nba_assets_free(&pack);return 0;
}
