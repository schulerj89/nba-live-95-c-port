#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nba_tipoff.h"

#define FIELD_COUNT 123u
#define FT_INPUT_COUNT 102u
#define FT_OUTPUT_COUNT 102u

/* Host-only CPU-logic mode-twelve fixture helper; no direct native address. Rebuild a
 * signed 16.8 coordinate from native integer and fraction words. */
static int32_t fixed_from_words(uint16_t fraction, uint16_t integer) {
    return (int32_t)(int16_t)integer * 256 + (fraction >> 8);
}

/* Host-only CPU-logic mode-twelve fixture helper; no direct native address. Recover a
 * native fraction word from the production 16.8 coordinate. */
static uint16_t fixed_fraction_word(int32_t value) {
    return (uint16_t)(((uint32_t)value & 255u) << 8);
}

/* Host-only CPU-logic mode-twelve fixture helper; no direct native address. Recover a
 * signed native integer coordinate word. */
static uint16_t fixed_integer_word(int32_t value) {
    int32_t integer = value >= 0 ? value / 256 : -(((-value) + 255) / 256);
    return (uint16_t)(int16_t)integer;
}

/* Host-only CPU-logic mode-twelve fixture helper; no direct native address. Emit one
 * compact projected word. */
static void emit(uint16_t value, unsigned *count) {
    printf("%s%04x", *count ? " " : "", value);
    ++*count;
}

/* Host-only CPU-logic mode-twelve fixture helper; no direct native address. Load every
 * represented field used by `$86:B769-$B978` and its verified children. */
static bool load_case(NbaTipoff *tipoff, NbaSession *session,
                      const NbaAssetPack *assets, const uint16_t in[]) {
    unsigned slot = in[0];
    if (slot >= NBA_GAMEPLAY_ACTOR_COUNT) return false;
    nba_session_init(session);
    session->right_team = 18u;
    session->left_team = 28u;
    session->config.main_values[2] = in[30];
    session->config.options[5] = in[31];
    memset(tipoff, 0, sizeof(*tipoff));
    tipoff->assets=assets;tipoff->session=session;
    NbaTipoffActor *a = &tipoff->actors[slot];
    static const uint8_t lineup[5]={2u,0u,1u,3u,4u};
    a->roster_slot=lineup[slot%5u];

    tipoff->rng.state=in[1]; tipoff->shot_origin_x=(int16_t)in[2];
    tipoff->shot_origin_y=(int16_t)in[3];
    tipoff->controller_record_raw_090c=in[4];
    tipoff->catch_actor_record_raw_0910=in[5];
    tipoff->shot_bounce_timer_raw_091c=in[6]; tipoff->rim_raw_0920=in[7];
    tipoff->shot_previous_actor_x_raw_0922=in[8]; tipoff->period_raw_0926=in[9];
    tipoff->match_clock_raw_0928=in[10]; tipoff->free_throw_flight_timer_raw_0930=in[11];
    tipoff->live_state_raw=in[12]; tipoff->camera_side_group_raw=(uint8_t)in[13];
    tipoff->possession_actor=(int8_t)(int16_t)in[14]; tipoff->ball_activity_raw=in[15];
    tipoff->rim_raw_094a=in[16]; tipoff->shot_value_raw=in[17];
    tipoff->dead_ball_raw_0966=in[18]; tipoff->dead_ball_raw_0968=in[19];
    tipoff->rim_raw_096a=in[20]; tipoff->dead_ball_raw_096c=in[21];
    tipoff->fouls.free_throw_state_raw_0978=in[22];
    tipoff->fouls.free_throw_sequence_raw_097a=in[23];
    tipoff->free_throw_aim_x_raw_0980=in[24]; tipoff->free_throw_aim_y_raw_0982=in[25];
    tipoff->assistance_team_raw_09c0=in[26]; tipoff->shot_actor_raw_09c8=(int16_t)in[27];
    tipoff->attached_ball_state_raw_09f6=in[28]; tipoff->shot_inner_veto_raw=in[29]!=0;
    tipoff->shot_control_raw_17c3=in[32];
    tipoff->shot_display_actor_raw_493b=in[33];
    tipoff->hud.shot_category_raw_4939=in[34];

    a->x_fp=fixed_from_words(in[35],in[36]); a->y_fp=fixed_from_words(in[37],in[38]);
    a->z_fp=fixed_from_words(in[39],in[40]); a->velocity_x=(int16_t)in[41];
    a->velocity_y=(int16_t)in[42]; a->velocity_z=(int16_t)in[43];
    a->controller_assignment_raw=(int8_t)(int16_t)in[44];
    a->animation_upper_queue_cursor_raw_18=in[45]; a->animation_lower_queue_cursor_raw_1a=in[46];
    for(unsigned n=0;n<3;++n){a->animation_upper_queue_raw_1c[n]=in[47+n];a->animation_lower_queue_raw_22[n]=in[50+n];}
    a->actor_status_raw_28=in[53]; a->upper_animation_resource_raw_2a=in[54];
    a->lower_animation_resource_raw_2c=in[55]; a->animation_state=(uint8_t)in[56];
    a->lower_animation_state=(uint8_t)in[57]; a->base_animation_state_raw_38=(uint8_t)in[58];
    a->rom_upper_animation_phase_raw_3a=in[59]; a->rom_lower_animation_phase_raw_3c=in[60];
    a->upper_animation_accumulator_raw_42=in[61]; a->lower_animation_accumulator_raw_44=in[62];
    a->upper_animation_lock_raw_46=in[63]; a->lower_animation_lock_raw_48=in[64];
    a->movement_speed_raw_4a=in[65]; a->movement_magnitude_raw=in[66];
    a->movement_direction=(uint8_t)in[67]; a->requested_direction=(uint8_t)in[68];
    a->direction=(uint8_t)in[69]; a->contact_inhibit_raw_5a=in[70];
    a->control_mode=(uint8_t)in[71]; a->reaction_threshold=in[72];
    a->behavior_timer=in[73]; a->animation_variant_raw_6c=in[74];
    a->team_group_raw_6e=in[75]; a->movement_boost_timer=in[76];
    a->behavior_flags_raw=in[77]; a->anchor_direction_raw=(uint8_t)in[78];
    a->assignment_distance=in[79]; a->anchor_distance_raw=in[80];
    a->free_throw_launch_half_raw_a8=in[81]; a->upper_phase_target_raw_b0=in[82];
    a->shot_modifier_raw_b2=in[83]; a->animation_resources_valid=true;
    a->exact_shot_animation=true;

    tipoff->ball.x_fp=fixed_from_words(in[84],in[85]);
    tipoff->ball.y_fp=fixed_from_words(in[86],in[87]);
    tipoff->ball.z_fp=fixed_from_words(in[88],in[89]);
    tipoff->ball.velocity_x=(int16_t)in[90]; tipoff->ball.velocity_y=(int16_t)in[91];
    tipoff->ball.velocity_z=(int16_t)in[92]; tipoff->ball.owner_actor=(int8_t)(int16_t)in[14];
    tipoff->ball.state=NBA_BALL_ATTACHED;
    for(unsigned side=0;side<2;++side){unsigned j=93+side*5;NbaGameplayTeamContext *c=&tipoff->team_context[side];
        c->strategy_team_raw_00=in[j];c->anchor_x_raw_0a=(int16_t)in[j+1];
        c->previous_dead_ball_actor_raw_43=in[j+2];c->previous_controller_actor_raw_45=(int16_t)in[j+3];c->match_clock_raw_47=in[j+4];}
    for(unsigned pad=0;pad<5;++pad)tipoff->controllers.record[pad].held=in[103+pad];
    for(unsigned p=0;p<5;++p)a->shot_statistics[p]=in[108+p];
    a->shot_stamina_raw_18=in[113];
    unsigned persistent=(slot/5u)*NBA_MATCH_ROSTER_SIZE+a->roster_slot;
    memcpy(tipoff->roster_shot_statistics[persistent],a->shot_statistics,
           sizeof(a->shot_statistics));
    for(unsigned p=0;p<5;++p)tipoff->controllers.record[0].reserved[p]=in[114+p];
    tipoff->shot_roster_low_raw_0914=in[119];tipoff->shot_roster_bank_raw_0916=in[120];
    tipoff->scratch_0046=in[121];tipoff->scratch_0047=in[122];

    NbaShotLaunchState *s=&tipoff->last_shot_launch;
    s->owner=in[14];s->last_owner=in[27];s->display_shooter=in[33];s->attempt_latch=in[16];
    s->dead_0966=in[18];s->height_0968=in[19];s->dead_096c=in[21];s->bounce_0920=in[7];
    s->inner_veto=in[29];s->live_state=in[12];s->timeout_0930=in[11];s->value=in[17];
    s->display_value=in[34];s->initial_value=in[20];s->ball_record=in[5];s->rng.state=in[1];
    s->facing=in[67];s->contact_inhibit=in[70];
    unsigned side=slot/5u;s->assist_43=tipoff->team_context[side].previous_dead_ball_actor_raw_43;
    s->assist_45=(uint16_t)tipoff->team_context[side].previous_controller_actor_raw_45;
    memcpy(s->player_stats,a->shot_statistics,sizeof(s->player_stats));
    tipoff->offense_side=(uint8_t)side; tipoff->handler_actor=(uint8_t)slot;
    return true;
}

/* Host-only CPU-logic mode-twelve fixture helper; no direct native address. Emit the
 * exact compact projection after production calls `$86:B769-$B978`. */
static void emit_case(const NbaTipoff *t, const uint16_t in[]) {
    unsigned n=0,slot=in[0];const NbaTipoffActor *a=&t->actors[slot];
    emit((uint16_t)slot,&n);emit(t->rng.state,&n);emit((uint16_t)t->shot_origin_x,&n);
    emit((uint16_t)t->shot_origin_y,&n);emit(t->controller_record_raw_090c,&n);emit(t->catch_actor_record_raw_0910,&n);
    emit(t->shot_bounce_timer_raw_091c,&n);emit(t->rim_raw_0920,&n);emit(t->shot_previous_actor_x_raw_0922,&n);
    emit(t->period_raw_0926,&n);emit(t->match_clock_raw_0928,&n);emit(t->free_throw_flight_timer_raw_0930,&n);
    emit(t->live_state_raw,&n);emit(t->camera_side_group_raw,&n);emit((uint16_t)(int16_t)t->possession_actor,&n);
    emit(t->ball_activity_raw,&n);emit(t->rim_raw_094a,&n);emit(t->shot_value_raw,&n);
    emit(t->dead_ball_raw_0966,&n);emit(t->dead_ball_raw_0968,&n);emit(t->rim_raw_096a,&n);
    emit(t->dead_ball_raw_096c,&n);emit(t->fouls.free_throw_state_raw_0978,&n);
    emit(t->fouls.free_throw_sequence_raw_097a,&n);emit(t->free_throw_aim_x_raw_0980,&n);
    emit(t->free_throw_aim_y_raw_0982,&n);emit(t->assistance_team_raw_09c0,&n);
    emit((uint16_t)t->shot_actor_raw_09c8,&n);emit(t->attached_ball_state_raw_09f6,&n);
    emit(t->shot_inner_veto_raw,&n);emit(t->session->config.main_values[2],&n);emit(t->session->config.options[5],&n);
    emit(t->shot_control_raw_17c3,&n);emit(t->shot_display_actor_raw_493b,&n);emit(t->hud.shot_category_raw_4939,&n);
    emit(fixed_fraction_word(a->x_fp),&n);emit(fixed_integer_word(a->x_fp),&n);
    emit(fixed_fraction_word(a->y_fp),&n);emit(fixed_integer_word(a->y_fp),&n);
    emit(fixed_fraction_word(a->z_fp),&n);emit(fixed_integer_word(a->z_fp),&n);
    emit((uint16_t)a->velocity_x,&n);emit((uint16_t)a->velocity_y,&n);emit((uint16_t)a->velocity_z,&n);
    emit((uint16_t)(int16_t)a->controller_assignment_raw,&n);emit(a->animation_upper_queue_cursor_raw_18,&n);
    emit(a->animation_lower_queue_cursor_raw_1a,&n);
    for(unsigned q=0;q<3;++q)emit(a->animation_upper_queue_raw_1c[q],&n);
    for(unsigned q=0;q<3;++q)emit(a->animation_lower_queue_raw_22[q],&n);
    emit(a->actor_status_raw_28,&n);emit(a->upper_animation_resource_raw_2a,&n);emit(a->lower_animation_resource_raw_2c,&n);
    emit(a->animation_state,&n);emit(a->lower_animation_state,&n);emit(a->base_animation_state_raw_38,&n);
    emit(a->rom_upper_animation_phase_raw_3a,&n);emit(a->rom_lower_animation_phase_raw_3c,&n);
    emit(a->upper_animation_accumulator_raw_42,&n);emit(a->lower_animation_accumulator_raw_44,&n);
    emit(a->upper_animation_lock_raw_46,&n);emit(a->lower_animation_lock_raw_48,&n);
    emit(a->movement_speed_raw_4a,&n);emit(a->movement_magnitude_raw,&n);emit(a->movement_direction,&n);
    emit(a->requested_direction,&n);emit(a->direction,&n);emit(a->contact_inhibit_raw_5a,&n);
    emit(a->control_mode,&n);emit(a->reaction_threshold,&n);emit(a->behavior_timer,&n);
    emit(a->animation_variant_raw_6c,&n);emit(a->team_group_raw_6e,&n);emit(a->movement_boost_timer,&n);
    emit(a->behavior_flags_raw,&n);emit(a->anchor_direction_raw,&n);emit(a->assignment_distance,&n);
    emit(a->anchor_distance_raw,&n);emit(a->free_throw_launch_half_raw_a8,&n);emit(a->upper_phase_target_raw_b0,&n);
    emit(a->shot_modifier_raw_b2,&n);
    emit(fixed_fraction_word(t->ball.x_fp),&n);emit(fixed_integer_word(t->ball.x_fp),&n);
    emit(fixed_fraction_word(t->ball.y_fp),&n);emit(fixed_integer_word(t->ball.y_fp),&n);
    emit(fixed_fraction_word(t->ball.z_fp),&n);emit(fixed_integer_word(t->ball.z_fp),&n);
    emit((uint16_t)t->ball.velocity_x,&n);emit((uint16_t)t->ball.velocity_y,&n);emit((uint16_t)t->ball.velocity_z,&n);
    for(unsigned side=0;side<2;++side){const NbaGameplayTeamContext *c=&t->team_context[side];
        emit(c->strategy_team_raw_00,&n);emit((uint16_t)c->anchor_x_raw_0a,&n);emit(c->previous_dead_ball_actor_raw_43,&n);
        emit((uint16_t)c->previous_controller_actor_raw_45,&n);emit(c->match_clock_raw_47,&n);}
    for(unsigned pad=0;pad<5;++pad)emit(t->controllers.record[pad].held,&n);
    unsigned persistent=(slot/5u)*NBA_MATCH_ROSTER_SIZE+a->roster_slot;
    for(unsigned p=0;p<5;++p)emit(t->roster_shot_statistics[persistent][p],&n);
    emit(a->shot_stamina_raw_18,&n);
    for(unsigned p=0;p<5;++p)emit(t->controllers.record[0].reserved[p],&n);
    emit(t->shot_roster_low_raw_0914,&n);emit(t->shot_roster_bank_raw_0916,&n);
    emit(t->scratch_0046,&n);emit(t->scratch_0047,&n);
    if(n!=FIELD_COUNT)fprintf(stderr,"field count %u\n",n);putchar('\n');
}

typedef enum {
    MODE_TWELVE_OBSERVE_ONLY = 0,
    MODE_TWELVE_LOSE_OWNER,
    MODE_TWELVE_DEFER_BEHAVIOR
} ModeTwelveMutation;

typedef struct {
    unsigned actor;
    ModeTwelveMutation mutation;
    unsigned seen;
    int32_t x_fp;
    int16_t velocity_x;
    uint32_t upper_tick;
    uint32_t lower_tick;
    uint16_t upper_phase;
    uint16_t lower_phase;
    uint16_t upper_accumulator;
    uint16_t lower_accumulator;
    uint16_t activity;
} ModeTwelvePhase;

/* Host-only CPU-logic test observer; no direct native address. Sample the
 * real actors.end boundary and publish post-common globals consumed by the
 * later `$87:9244 -> $86:B769-$B978` behavior pass. */
static void observe_mode_twelve_after_common(const NbaTipoff *observed,
                                              const char *boundary,
                                              void *raw_context) {
    ModeTwelvePhase *context=raw_context;
    if(strcmp(boundary,"actors.end")!=0)return;
    NbaTipoff *tipoff=(NbaTipoff *)observed;
    NbaTipoffActor *actor=&tipoff->actors[context->actor];
    ++context->seen;
    context->x_fp=actor->x_fp;
    context->velocity_x=actor->velocity_x;
    context->upper_tick=actor->upper_animation_tick;
    context->lower_tick=actor->lower_animation_tick;
    context->upper_phase=actor->rom_upper_animation_phase_raw_3a;
    context->lower_phase=actor->rom_lower_animation_phase_raw_3c;
    context->upper_accumulator=actor->upper_animation_accumulator_raw_42;
    context->lower_accumulator=actor->lower_animation_accumulator_raw_44;
    context->activity=tipoff->ball_activity_raw;
    if(context->mutation==MODE_TWELVE_LOSE_OWNER){
        tipoff->possession_actor=1;
        actor->animation_state=8u;
        actor->lower_animation_state=8u;
        actor->base_animation_state_raw_38=8u;
    }else if(context->mutation==MODE_TWELVE_DEFER_BEHAVIOR){
        tipoff->rim_raw_13e7|=0x0010u;
    }
}

/* Host-only CPU-logic fixture helper; no direct native address. Prepare an
 * isolated post-tip live state for the public mode-twelve scheduler caller. */
static bool prepare_mode_twelve_scheduler_case(const NbaAssetPack *assets,
                                                NbaSession *session,
                                                NbaTipoff *tipoff) {
    nba_session_init(session);
    session->right_team=18u;
    session->left_team=28u;
    if(!nba_tipoff_init(tipoff,assets,session))return false;
    for(unsigned slot=0;slot<NBA_GAMEPLAY_ACTOR_COUNT;++slot){
        NbaTipoffActor *actor=&tipoff->actors[slot];
        actor->control_mode=0u;
        actor->controller_assignment_raw=-1;
        actor->velocity_x=actor->velocity_y=actor->velocity_z=0;
        actor->x_fp=(int32_t)(-300+(int)slot*65)*256;
        actor->y_fp=(int32_t)(-180+(int)slot*37)*256;
        actor->z_fp=0;
    }
    tipoff->simulation_tick=1u;
    tipoff->tip_contact_actor=0;
    tipoff->tip_possession_frame=1u;
    tipoff->phase=NBA_TIPOFF_LIVE;
    tipoff->live_state_raw=0u;
    tipoff->camera_side_group_raw=5u;
    tipoff->possession_actor=0;
    tipoff->handler_actor=0u;
    tipoff->ball.owner_actor=0;
    tipoff->ball.state=NBA_BALL_ATTACHED;
    tipoff->ball_activity_raw=1u;
    tipoff->cpu_play_state=NBA_CPU_PLAY_ATTACK;
    tipoff->team_context[0].anchor_x_raw_0a=-336;
    NbaTipoffActor *actor=&tipoff->actors[0];
    actor->control_mode=12u;
    actor->team_group_raw_6e=0u;
    actor->x_fp=100*256+0x35;
    actor->y_fp=0;
    actor->velocity_x=0x0100;
    actor->animation_state=0x16u;
    actor->lower_animation_state=0x16u;
    actor->base_animation_state_raw_38=3u;
    actor->rom_upper_animation_phase_raw_3a=0u;
    actor->rom_lower_animation_phase_raw_3c=0u;
    actor->upper_animation_accumulator_raw_42=0x05ffu;
    actor->lower_animation_accumulator_raw_44=0x05ffu;
    actor->upper_animation_lock_raw_46=1u;
    actor->lower_animation_lock_raw_48=0xffffu;
    actor->upper_animation_resource_raw_2a=0x0094u;
    actor->lower_animation_resource_raw_2c=0x073bu;
    actor->animation_resources_valid=true;
    actor->exact_shot_animation=true;
    return true;
}

/* Host-only CPU-logic production caller test; no direct native address.
 * Exercise `$87:AAB2 -> $85:963D -> globals -> $87:9244 ->
 * $86:B769-$B978` through complete nba_tipoff_update passes, including lost
 * ownership restoration and the one pending odd-frame behavior exception. */
static bool mode_twelve_scheduler_cases(const NbaAssetPack *assets) {
    NbaSession session;
    NbaTipoff tipoff;
    if(!prepare_mode_twelve_scheduler_case(assets,&session,&tipoff))return false;
    NbaTipoffActor *actor=&tipoff.actors[0];
    int32_t initial_x=actor->x_fp;
    uint32_t initial_upper_tick=actor->upper_animation_tick;
    uint32_t initial_lower_tick=actor->lower_animation_tick;
    uint16_t initial_upper_phase=actor->rom_upper_animation_phase_raw_3a;
    uint16_t initial_lower_phase=actor->rom_lower_animation_phase_raw_3c;
    ModeTwelvePhase ordinary={.actor=0u,.mutation=MODE_TWELVE_OBSERVE_ONLY};
    tipoff.differential_observer=observe_mode_twelve_after_common;
    tipoff.differential_context=&ordinary;
    nba_tipoff_update(&tipoff,NULL);
    actor=&tipoff.actors[0];
    if(ordinary.seen!=1u || ordinary.x_fp!=initial_x+0x0200 ||
       ordinary.velocity_x!=0x0100 || ordinary.activity!=1u ||
       ordinary.upper_tick!=initial_upper_tick+1u ||
       ordinary.lower_tick!=initial_lower_tick+1u ||
       ordinary.upper_phase!=(uint16_t)(initial_upper_phase+1u) ||
       ordinary.lower_phase!=initial_lower_phase ||
       actor->x_fp!=ordinary.x_fp || actor->upper_animation_tick!=ordinary.upper_tick ||
       actor->lower_animation_tick!=ordinary.lower_tick ||
       tipoff.ball_activity_raw!=3u || actor->control_mode!=12u){
        fprintf(stderr,"[CPU MODE TWELVE SCHEDULER] ordinary seen=%u x=%ld/%ld "
                "ticks=%lu/%lu phases=%04x/%04x activity=%04x/%04x mode=%u\n",
            ordinary.seen,(long)ordinary.x_fp,(long)(initial_x+0x0200),
            (unsigned long)ordinary.upper_tick,(unsigned long)ordinary.lower_tick,
            ordinary.upper_phase,ordinary.lower_phase,ordinary.activity,
            tipoff.ball_activity_raw,actor->control_mode);
        return false;
    }

    if(!prepare_mode_twelve_scheduler_case(assets,&session,&tipoff))return false;
    actor=&tipoff.actors[0];
    initial_x=actor->x_fp;
    initial_upper_tick=actor->upper_animation_tick;
    initial_lower_tick=actor->lower_animation_tick;
    ModeTwelvePhase restore={.actor=0u,.mutation=MODE_TWELVE_LOSE_OWNER};
    tipoff.differential_observer=observe_mode_twelve_after_common;
    tipoff.differential_context=&restore;
    nba_tipoff_update(&tipoff,NULL);
    actor=&tipoff.actors[0];
    if(restore.seen!=1u || restore.x_fp!=initial_x+0x0200 ||
       restore.activity!=1u || restore.upper_tick!=initial_upper_tick+1u ||
       restore.lower_tick!=initial_lower_tick+1u || actor->control_mode!=2u ||
       actor->base_animation_state_raw_38!=8u || actor->reaction_threshold!=0u ||
       actor->x_fp!=restore.x_fp){
        fprintf(stderr,"[CPU MODE TWELVE SCHEDULER] restore seen=%u x=%ld/%ld "
                "ticks=%lu/%lu mode=%u base=%u timer=%04x activity=%04x\n",
            restore.seen,(long)restore.x_fp,(long)(initial_x+0x0200),
            (unsigned long)restore.upper_tick,(unsigned long)restore.lower_tick,
            actor->control_mode,actor->base_animation_state_raw_38,
            actor->reaction_threshold,tipoff.ball_activity_raw);
        return false;
    }
    tipoff.differential_observer=NULL;
    uint32_t restored_upper_tick=actor->upper_animation_tick;
    uint32_t restored_lower_tick=actor->lower_animation_tick;
    int32_t restored_x=actor->x_fp;
    nba_tipoff_update(&tipoff,NULL);
    actor=&tipoff.actors[0];
    if(tipoff.simulation_tick!=3u || actor->x_fp!=restored_x ||
       actor->upper_animation_tick!=restored_upper_tick ||
       actor->lower_animation_tick!=restored_lower_tick ||
       actor->control_mode!=2u || actor->base_animation_state_raw_38!=8u ||
       actor->reaction_threshold!=0u)return false;
    nba_tipoff_update(&tipoff,NULL);
    actor=&tipoff.actors[0];
    if(tipoff.simulation_tick!=4u || actor->x_fp!=restored_x+0x0200 ||
       actor->upper_animation_tick!=restored_upper_tick+1u ||
       actor->lower_animation_tick!=restored_lower_tick+1u ||
       actor->reaction_threshold==0u)return false;

    if(!prepare_mode_twelve_scheduler_case(assets,&session,&tipoff))return false;
    actor=&tipoff.actors[0];
    initial_x=actor->x_fp;
    initial_upper_tick=actor->upper_animation_tick;
    initial_lower_tick=actor->lower_animation_tick;
    ModeTwelvePhase pending={.actor=0u,.mutation=MODE_TWELVE_DEFER_BEHAVIOR};
    tipoff.differential_observer=observe_mode_twelve_after_common;
    tipoff.differential_context=&pending;
    nba_tipoff_update(&tipoff,NULL);
    actor=&tipoff.actors[0];
    if(pending.seen!=1u || pending.x_fp!=initial_x+0x0200 ||
       pending.activity!=1u || pending.upper_tick!=initial_upper_tick+1u ||
       pending.lower_tick!=initial_lower_tick+1u ||
       tipoff.actor_behavior_pending!=1u || tipoff.ball_activity_raw!=1u ||
       actor->x_fp!=pending.x_fp)return false;
    tipoff.differential_observer=NULL;
    uint32_t pending_upper_tick=actor->upper_animation_tick;
    uint32_t pending_lower_tick=actor->lower_animation_tick;
    int32_t pending_x=actor->x_fp;
    nba_tipoff_update(&tipoff,NULL);
    actor=&tipoff.actors[0];
    if(tipoff.simulation_tick!=3u || tipoff.actor_behavior_pending!=0u ||
       tipoff.ball_activity_raw!=3u || actor->x_fp!=pending_x ||
       actor->upper_animation_tick!=pending_upper_tick ||
       actor->lower_animation_tick!=pending_lower_tick ||
       actor->control_mode!=12u)return false;
    return true;
}

/* Host-only CPU-logic free-throw caller fixture helper; no direct native address. Load
 * the represented `$87:9F11-$9F75` inputs while retaining ROM-backed player
 * data for the uncaptured native player record at $416B. */
static bool load_free_throw_caller_case(NbaTipoff *tipoff,
                                        NbaSession *session,
                                        const NbaAssetPack *assets,
                                        const uint16_t input[]) {
    const uint16_t *v=input+1;
    unsigned slot=v[0];
    if(slot>=NBA_GAMEPLAY_ACTOR_COUNT)return false;
    nba_session_init(session);
    session->right_team=18u;
    session->left_team=28u;
    if(!nba_tipoff_init(tipoff,assets,session))return false;
    tipoff->rng.state=input[0];
    tipoff->shot_origin_x=(int16_t)v[1];tipoff->shot_origin_y=(int16_t)v[2];
    tipoff->controller_record_raw_090c=v[3];tipoff->catch_actor_record_raw_0910=v[4];
    tipoff->shot_roster_low_raw_0914=v[5];tipoff->shot_roster_bank_raw_0916=v[6];
    tipoff->shot_bounce_timer_raw_091c=v[7];tipoff->rim_raw_0920=v[8];
    tipoff->shot_previous_actor_x_raw_0922=v[9];tipoff->free_throw_flight_timer_raw_0930=v[10];
    tipoff->live_state_raw=v[11];tipoff->camera_side_group_raw=(uint8_t)v[12];
    tipoff->possession_actor=(int8_t)(int16_t)v[13];tipoff->ball_activity_raw=v[14];
    tipoff->rim_raw_094a=v[15];tipoff->shot_value_raw=v[16];
    tipoff->dead_ball_raw_0966=v[17];tipoff->dead_ball_raw_0968=v[18];
    tipoff->rim_raw_096a=v[19];tipoff->dead_ball_raw_096c=v[20];
    tipoff->free_throw_resolution_raw_0972=v[21];
    tipoff->fouls.free_throw_state_raw_0978=v[22];
    tipoff->fouls.free_throw_sequence_raw_097a=v[23];tipoff->rim_raw_097c=v[24];
    tipoff->free_throw_aim_x_raw_0980=v[25];tipoff->free_throw_aim_y_raw_0982=v[26];
    tipoff->assistance_team_raw_09c0=v[27];tipoff->shot_actor_raw_09c8=(int16_t)v[28];
    tipoff->attached_ball_state_raw_09f6=v[29];tipoff->shot_inner_veto_raw=v[30]!=0;
    session->config.options[5]=v[31];tipoff->shot_control_raw_17c3=v[32];
    tipoff->free_throw_upload_raw_180b=v[33];tipoff->free_throw_upload_raw_180c=v[34];
    tipoff->hud.shot_category_raw_4939=v[35];tipoff->shot_display_actor_raw_493b=v[36];

    NbaTipoffActor *a=&tipoff->actors[slot];
    a->x_fp=fixed_from_words(v[37],v[38]);a->y_fp=fixed_from_words(v[39],v[40]);
    a->z_fp=fixed_from_words(v[41],v[42]);a->velocity_x=(int16_t)v[43];
    a->velocity_y=(int16_t)v[44];a->velocity_z=(int16_t)v[45];
    a->controller_assignment_raw=(int8_t)(int16_t)v[46];
    a->animation_upper_queue_cursor_raw_18=v[47];a->animation_lower_queue_cursor_raw_1a=v[48];
    for(unsigned n=0;n<3;++n){a->animation_upper_queue_raw_1c[n]=v[49+n];a->animation_lower_queue_raw_22[n]=v[52+n];}
    a->actor_status_raw_28=v[55];a->upper_animation_resource_raw_2a=v[56];
    a->lower_animation_resource_raw_2c=v[57];a->animation_state=(uint8_t)v[58];
    a->lower_animation_state=(uint8_t)v[59];a->base_animation_state_raw_38=(uint8_t)v[60];
    a->rom_upper_animation_phase_raw_3a=v[61];a->rom_lower_animation_phase_raw_3c=v[62];
    a->upper_animation_accumulator_raw_42=v[63];a->lower_animation_accumulator_raw_44=v[64];
    a->upper_animation_lock_raw_46=v[65];a->lower_animation_lock_raw_48=v[66];
    a->movement_speed_raw_4a=v[67];a->movement_magnitude_raw=v[68];
    a->movement_direction=(uint8_t)v[69];a->requested_direction=(uint8_t)v[70];
    a->direction=(uint8_t)v[71];a->contact_inhibit_raw_5a=v[72];
    a->control_mode=(uint8_t)v[73];a->reaction_threshold=v[74];a->behavior_timer=v[75];
    a->animation_variant_raw_6c=v[76];a->team_group_raw_6e=v[77];
    a->movement_boost_timer=v[78];a->behavior_flags_raw=v[79];
    a->anchor_direction_raw=(uint8_t)v[80];a->assignment_distance=v[81];
    a->anchor_distance_raw=v[82];a->free_throw_launch_half_raw_a8=v[83];
    a->upper_phase_target_raw_b0=v[84];a->shot_modifier_raw_b2=v[85];
    a->animation_resources_valid=true;a->exact_shot_animation=true;

    tipoff->ball.x_fp=fixed_from_words(v[86],v[87]);
    tipoff->ball.y_fp=fixed_from_words(v[88],v[89]);
    tipoff->ball.z_fp=fixed_from_words(v[90],v[91]);
    tipoff->ball.velocity_x=(int16_t)v[92];tipoff->ball.velocity_y=(int16_t)v[93];
    tipoff->ball.velocity_z=(int16_t)v[94];
    tipoff->ball.owner_actor=(int8_t)(int16_t)v[13];
    tipoff->ball.state=tipoff->ball.owner_actor>=0?NBA_BALL_ATTACHED:NBA_BALL_SHOT;
    NbaGameplayTeamContext *context=&tipoff->team_context[0];
    context->strategy_team_raw_00=v[95];context->anchor_x_raw_0a=(int16_t)v[96];
    context->previous_dead_ball_actor_raw_43=v[97];
    context->previous_controller_actor_raw_45=(int16_t)v[98];
    context->match_clock_raw_47=v[99];
    tipoff->match_clock_raw_0928=v[100];
    tipoff->handler_actor=(uint8_t)slot;tipoff->offense_side=0u;
    tipoff->possession_team=0;

    NbaShotLaunchState *shot=&tipoff->last_shot_launch;
    shot->owner=v[13];shot->last_owner=v[28];shot->display_shooter=v[36];
    shot->attempt_latch=v[15];shot->dead_0966=v[17];shot->height_0968=v[18];
    shot->initial_value=v[19];shot->dead_096c=v[20];shot->bounce_0920=v[8];
    shot->inner_veto=v[30];shot->live_state=v[11];shot->timeout_0930=v[10];
    shot->value=v[16];shot->display_value=v[35];shot->ball_record=v[4];
    shot->rng.state=input[0];shot->facing=v[69];shot->contact_inhibit=v[72];
    shot->assist_43=v[97];shot->assist_45=v[98];
    unsigned persistent=a->roster_slot;
    memcpy(shot->player_stats,tipoff->roster_shot_statistics[persistent],
           sizeof(shot->player_stats));
    return true;
}

/* Host-only CPU-logic free-throw caller fixture helper; no direct native address. Emit
 * the caller-owned gameplay projection after `$87:9F11-$9F75`. */
static void emit_free_throw_caller_case(const NbaTipoff *t,unsigned slot){
    unsigned n=0;const NbaTipoffActor *a=&t->actors[slot];
    emit(t->rng.state,&n);emit((uint16_t)slot,&n);emit((uint16_t)t->shot_origin_x,&n);emit((uint16_t)t->shot_origin_y,&n);
    emit(t->controller_record_raw_090c,&n);emit(t->catch_actor_record_raw_0910,&n);
    emit(t->shot_roster_low_raw_0914,&n);emit(t->shot_roster_bank_raw_0916,&n);
    emit(t->shot_bounce_timer_raw_091c,&n);emit(t->rim_raw_0920,&n);emit(t->shot_previous_actor_x_raw_0922,&n);
    emit(t->free_throw_flight_timer_raw_0930,&n);emit(t->live_state_raw,&n);emit(t->camera_side_group_raw,&n);
    emit((uint16_t)(int16_t)t->possession_actor,&n);emit(t->ball_activity_raw,&n);emit(t->rim_raw_094a,&n);
    emit(t->shot_value_raw,&n);emit(t->dead_ball_raw_0966,&n);emit(t->dead_ball_raw_0968,&n);
    emit(t->rim_raw_096a,&n);emit(t->dead_ball_raw_096c,&n);emit(t->free_throw_resolution_raw_0972,&n);
    emit(t->fouls.free_throw_state_raw_0978,&n);emit(t->fouls.free_throw_sequence_raw_097a,&n);
    emit(t->rim_raw_097c,&n);emit(t->free_throw_aim_x_raw_0980,&n);emit(t->free_throw_aim_y_raw_0982,&n);
    emit(t->assistance_team_raw_09c0,&n);emit((uint16_t)t->shot_actor_raw_09c8,&n);
    emit(t->attached_ball_state_raw_09f6,&n);emit(t->shot_inner_veto_raw,&n);
    emit(t->session->config.options[5],&n);emit(t->shot_control_raw_17c3,&n);
    emit(t->free_throw_upload_raw_180b,&n);emit(t->free_throw_upload_raw_180c,&n);
    emit(t->hud.shot_category_raw_4939,&n);emit(t->shot_display_actor_raw_493b,&n);
    emit(fixed_fraction_word(a->x_fp),&n);emit(fixed_integer_word(a->x_fp),&n);
    emit(fixed_fraction_word(a->y_fp),&n);emit(fixed_integer_word(a->y_fp),&n);
    emit(fixed_fraction_word(a->z_fp),&n);emit(fixed_integer_word(a->z_fp),&n);
    emit((uint16_t)a->velocity_x,&n);emit((uint16_t)a->velocity_y,&n);emit((uint16_t)a->velocity_z,&n);
    emit((uint16_t)(int16_t)a->controller_assignment_raw,&n);emit(a->animation_upper_queue_cursor_raw_18,&n);
    emit(a->animation_lower_queue_cursor_raw_1a,&n);
    for(unsigned q=0;q<3;++q)emit(a->animation_upper_queue_raw_1c[q],&n);
    for(unsigned q=0;q<3;++q)emit(a->animation_lower_queue_raw_22[q],&n);
    emit(a->actor_status_raw_28,&n);emit(a->upper_animation_resource_raw_2a,&n);emit(a->lower_animation_resource_raw_2c,&n);
    emit(a->animation_state,&n);emit(a->lower_animation_state,&n);emit(a->base_animation_state_raw_38,&n);
    emit(a->rom_upper_animation_phase_raw_3a,&n);emit(a->rom_lower_animation_phase_raw_3c,&n);
    emit(a->upper_animation_accumulator_raw_42,&n);emit(a->lower_animation_accumulator_raw_44,&n);
    emit(a->upper_animation_lock_raw_46,&n);emit(a->lower_animation_lock_raw_48,&n);
    emit(a->movement_speed_raw_4a,&n);emit(a->movement_magnitude_raw,&n);emit(a->movement_direction,&n);
    emit(a->requested_direction,&n);emit(a->direction,&n);emit(a->contact_inhibit_raw_5a,&n);
    emit(a->control_mode,&n);emit(a->reaction_threshold,&n);emit(a->behavior_timer,&n);
    emit(a->animation_variant_raw_6c,&n);emit(a->team_group_raw_6e,&n);emit(a->movement_boost_timer,&n);
    emit(a->behavior_flags_raw,&n);emit(a->anchor_direction_raw,&n);emit(a->assignment_distance,&n);
    emit(a->anchor_distance_raw,&n);emit(a->free_throw_launch_half_raw_a8,&n);emit(a->upper_phase_target_raw_b0,&n);
    emit(a->shot_modifier_raw_b2,&n);emit(fixed_fraction_word(t->ball.x_fp),&n);emit(fixed_integer_word(t->ball.x_fp),&n);
    emit(fixed_fraction_word(t->ball.y_fp),&n);emit(fixed_integer_word(t->ball.y_fp),&n);
    emit(fixed_fraction_word(t->ball.z_fp),&n);emit(fixed_integer_word(t->ball.z_fp),&n);
    emit((uint16_t)t->ball.velocity_x,&n);emit((uint16_t)t->ball.velocity_y,&n);emit((uint16_t)t->ball.velocity_z,&n);
    const NbaGameplayTeamContext *context=&t->team_context[0];
    emit(context->strategy_team_raw_00,&n);emit((uint16_t)context->anchor_x_raw_0a,&n);
    emit(context->previous_dead_ball_actor_raw_43,&n);emit((uint16_t)context->previous_controller_actor_raw_45,&n);
    emit(context->match_clock_raw_47,&n);
    emit(t->match_clock_raw_0928,&n);
    if(n!=FT_OUTPUT_COUNT)fprintf(stderr,"FT caller field count %u\n",n);putchar('\n');
}

/* Host-only CPU-logic executable fixture entry; no direct native address. Replay the
 * compact parent witnesses through the production implementation. */
int main(int argc,char **argv){
    bool stale_mirror=argc==3 && strcmp(argv[2],"--stale-mirror")==0;
    bool stale_ball_owner=argc==3 && strcmp(argv[2],"--stale-ball-owner")==0;
    bool self_test=argc==3 && strcmp(argv[2],"--self-test")==0;
    bool ft_prep=argc==3 && strcmp(argv[2],"--ft-prep")==0;
    bool ft_state9=argc==3 && strcmp(argv[2],"--ft-state9")==0;
    if(argc<2 || argc>3 || (argc==3 && !stale_mirror && !stale_ball_owner &&
            !self_test && !ft_prep && !ft_state9)){
        fprintf(stderr,"usage: %s <asset-pack> [--stale-mirror|--stale-ball-owner|--self-test|--ft-prep|--ft-state9]\n",argv[0]);return 2;}
    NbaAssetPack assets={0};if(!nba_assets_load(&assets,argv[1]))return 3;
    if(self_test){NbaSession session;NbaTipoff tipoff;nba_session_init(&session);
        session.config.options[5]=0x6a5cu;
        bool ok=nba_tipoff_init(&tipoff,&assets,&session) &&
                mode_twelve_scheduler_cases(&assets) &&
                session.config.options[5]==0x6a5cu;
        nba_assets_free(&assets);
        if(ok)puts("[CPU MODE TWELVE CALLER] PASS: public scheduler, free-throw prep/body, assistance, ownership and handoff");
        return ok?0:4;}
    if(ft_prep||ft_state9){
        _setmode(_fileno(stdin),_O_BINARY);uint16_t in[FT_INPUT_COUNT];
        if(fread(in,sizeof(in),1,stdin)!=1){nba_assets_free(&assets);return 5;}
        NbaSession session;NbaTipoff tipoff;
        if(!load_free_throw_caller_case(&tipoff,&session,&assets,in) ||
           !nba_tipoff_replay_free_throw_shooter(&tipoff,(uint8_t)in[1],ft_prep)){
            nba_assets_free(&assets);return 4;}
        emit_free_throw_caller_case(&tipoff,in[1]);
        bool ok=fread(in,sizeof(in),1,stdin)==0 && feof(stdin);
        nba_assets_free(&assets);return ok?0:5;
    }
    _setmode(_fileno(stdin),_O_BINARY);uint16_t in[FIELD_COUNT];
    while(fread(in,sizeof(in),1,stdin)==1){NbaSession session;NbaTipoff tipoff;
        if(!load_case(&tipoff,&session,&assets,in)){nba_assets_free(&assets);return 4;}
        unsigned slot=in[0];NbaTipoffActor *actor=&tipoff.actors[slot];
        unsigned persistent=(slot/5u)*NBA_MATCH_ROSTER_SIZE+actor->roster_slot;
        if(stale_mirror)for(unsigned p=0;p<5;++p)actor->shot_statistics[p]=(uint16_t)(0x7000u+p);
        if(stale_ball_owner){tipoff.ball.owner_actor=(int8_t)((slot+1u)%NBA_GAMEPLAY_ACTOR_COUNT);
            tipoff.ball.state=NBA_BALL_ATTACHED;}
        if(!nba_tipoff_replay_mode_twelve(&tipoff,(uint8_t)slot) ||
           memcmp(actor->shot_statistics,tipoff.roster_shot_statistics[persistent],
                  sizeof(actor->shot_statistics))!=0){nba_assets_free(&assets);return 4;}
        emit_case(&tipoff,in);}
    bool ok=feof(stdin);nba_assets_free(&assets);return ok?0:5;
}
