#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nba_tipoff.h"

#define FIELD_COUNT 154u

/* Host-only mode-fourteen replay helper; no direct native address. Rebuild
 * the represented signed 16.8 coordinate from native fraction/integer words. */
static int32_t fixed_from_words(uint16_t fraction,uint16_t integer){
    return (int32_t)(int16_t)integer*256+(fraction>>8);
}
/* Host-only mode-fourteen replay helper; no direct native address. Recover
 * the native fraction word from a production 16.8 coordinate. */
static uint16_t fixed_fraction_word(int32_t value){return (uint16_t)(((uint32_t)value&255u)<<8);}
/* Host-only mode-fourteen replay helper; no direct native address. Recover
 * the signed native integer coordinate word. */
static uint16_t fixed_integer_word(int32_t value){
    return (uint16_t)(int16_t)(value>=0?value/256:-(((-value)+255)/256));
}
/* Host-only mode-fourteen replay helper; no direct native address. Emit one
 * compact projected word in fixture order. */
static void emit(uint16_t value,unsigned *count){printf("%s%04x",*count?" ":"",value);++*count;}

/* Host-only mode-fourteen replay adapter; no direct native address. Map the
 * captured native slot/id5 -> player $446B to host actor5/context1/roster2. */
static bool load_case(NbaTipoff *t,NbaSession *s,const NbaAssetPack *assets,
                      const uint16_t in[]){
    unsigned slot=in[0];if(slot!=5u)return false;
    nba_session_init(s);memset(t,0,sizeof(*t));t->assets=assets;t->session=s;
    s->right_team=(uint8_t)in[115];s->left_team=(uint8_t)in[121];
    s->config.main_values[2]=in[38];s->config.options[5]=in[39];
    t->rng.state=in[1];t->shot_origin_x=(int16_t)in[2];t->shot_origin_y=(int16_t)in[3];
    t->catch_actor_record_raw_0910=in[4];t->shot_roster_low_raw_0914=in[5];t->shot_roster_bank_raw_0916=in[6];
    t->shot_bounce_timer_raw_091c=in[7];t->rim_raw_0920=in[8];t->shot_previous_actor_x_raw_0922=in[9];
    t->period_raw_0926=in[10];t->match_clock_raw_0928=in[11];t->free_throw_flight_timer_raw_0930=in[12];
    t->live_state_raw=in[13];t->camera_side_group_raw=(uint8_t)in[14];t->possession_actor=(int8_t)(int16_t)in[15];
    t->pass_actor_raw=(int16_t)in[16];t->pass_aux_raw=(int16_t)in[17];t->pass_receiver_raw=(int16_t)in[18];
    t->ball_activity_raw=in[19];t->rim_raw_094a=in[20];t->shot_value_raw=in[21];t->close_finish_timing_raw_094e=in[22];
    t->dead_ball_raw_0966=in[23];t->dead_ball_raw_0968=in[24];t->rim_raw_096a=in[25];t->dead_ball_raw_096c=in[26];
    t->fouls.free_throw_state_raw_0978=in[27];t->fouls.free_throw_sequence_raw_097a=in[28];
    t->free_throw_aim_x_raw_0980=in[29];t->free_throw_aim_y_raw_0982=in[30];t->inbound_transfer_raw=in[31];
    t->assistance_team_raw_09c0=in[32];t->pass_active_raw=in[33];t->shot_actor_raw_09c8=(int16_t)in[34];
    t->attached_ball_state_raw_09f6=in[35];t->shot_inner_veto_raw=in[36]!=0;t->rim_raw_13e7=in[37];
    t->shot_control_raw_17c3=in[40];t->rim_force_raw_1866=in[41];
    t->rim_effect.gate_raw_3f33=in[42];t->court_presentation.basket_x_3fef=in[43];
    t->rim_effect.reference_y_raw_3ff3=(int16_t)in[44];t->rim_effect.resource_raw_4015=in[45];
    t->rim_effect.effect_raw_401b=in[46];t->rim_effect.frame_raw_4025=in[47];t->rim_effect.timer_raw_402d=in[48];
    t->hud.shot_category_raw_4939=in[49];t->shot_display_actor_raw_493b=in[50];
    NbaTipoffActor *a=&t->actors[slot];a->roster_slot=2u;
    a->x_fp=fixed_from_words(in[52],in[53]);a->y_fp=fixed_from_words(in[54],in[55]);a->z_fp=fixed_from_words(in[56],in[57]);
    a->velocity_x=(int16_t)in[58];a->velocity_y=(int16_t)in[59];a->velocity_z=(int16_t)in[60];
    a->controller_assignment_raw=(int8_t)(int16_t)in[61];a->animation_upper_queue_cursor_raw_18=in[62];
    a->animation_lower_queue_cursor_raw_1a=in[63];for(unsigned q=0;q<3;++q){a->animation_upper_queue_raw_1c[q]=in[64+q];a->animation_lower_queue_raw_22[q]=in[67+q];}
    a->actor_status_raw_28=in[70];a->upper_animation_resource_raw_2a=in[71];a->lower_animation_resource_raw_2c=in[72];
    a->animation_state=(uint8_t)in[73];a->lower_animation_state=(uint8_t)in[74];a->base_animation_state_raw_38=(uint8_t)in[75];
    a->rom_upper_animation_phase_raw_3a=in[76];a->rom_lower_animation_phase_raw_3c=in[77];a->upper_animation_accumulator_raw_42=in[78];
    a->lower_animation_accumulator_raw_44=in[79];a->upper_animation_lock_raw_46=in[80];a->lower_animation_lock_raw_48=in[81];
    a->movement_speed_raw_4a=in[82];a->movement_magnitude_raw=in[83];a->movement_direction=(uint8_t)in[84];
    a->requested_direction=(uint8_t)in[85];a->direction=(uint8_t)in[86];a->special_contact_raw_56=(int16_t)in[87];
    a->mode13_variant_raw_58=in[88];a->contact_inhibit_raw_5a=in[89];a->control_mode=(uint8_t)in[90];
    a->contact_action_timer_raw_60=in[91];a->reaction_threshold=0xdeadu;a->behavior_timer=in[92];a->pass_direction_raw=in[93];
    a->animation_variant_raw_6c=in[94];a->team_group_raw_6e=in[95];a->movement_boost_timer=in[96];a->behavior_flags_raw=in[97];
    a->anchor_direction_raw=(uint8_t)in[98];a->assignment_distance=in[99];a->anchor_distance_raw=in[100];
    a->free_throw_launch_half_raw_a8=in[101];a->upper_phase_target_raw_b0=in[102];a->shot_modifier_raw_b2=in[103];
    a->mode13_baseline_velocity_x=(int16_t)in[104];a->mode13_baseline_velocity_y=(int16_t)in[105];a->animation_resources_valid=true;
    t->ball.x_fp=fixed_from_words(in[106],in[107]);t->ball.y_fp=fixed_from_words(in[108],in[109]);
    t->ball.z_fp=fixed_from_words(in[110],in[111]);t->ball.velocity_x=(int16_t)in[112];t->ball.velocity_y=(int16_t)in[113];
    t->ball.velocity_z=(int16_t)in[114];t->ball.owner_actor=(int8_t)(int16_t)in[15];t->ball.state=NBA_BALL_ATTACHED;
    for(unsigned side=0;side<2;++side){unsigned j=115+side*6;NbaGameplayTeamContext *c=&t->team_context[side];
        c->strategy_team_raw_00=in[j];c->anchor_x_fraction_raw_08=in[j+1];c->anchor_x_raw_0a=(int16_t)in[j+2];
        c->previous_dead_ball_actor_raw_43=in[j+3];c->previous_controller_actor_raw_45=(int16_t)in[j+4];c->match_clock_raw_47=in[j+5];}
    unsigned persistent=NBA_MATCH_ROSTER_SIZE+2u;for(unsigned p=0;p<5;++p){a->shot_statistics[p]=in[127+p];t->roster_shot_statistics[persistent][p]=in[127+p];}
    a->shot_stamina_raw_18=in[132];for(unsigned p=0;p<5;++p)t->controllers.record[0].reserved[p]=in[133+p];
    for(unsigned i=0;i<NBA_GAMEPLAY_ACTOR_COUNT;++i)t->actors[i].control_mode=(uint8_t)in[138+i];
    t->scratch_0046=in[148];t->scratch_0047=in[149];t->offense_side=1u;t->handler_actor=(uint8_t)slot;
    a->upper_state_snapshot_raw_34=in[150];a->lower_state_snapshot_raw_36=in[151];
    a->upper_phase_snapshot_raw_3e=in[152];a->lower_phase_snapshot_raw_40=in[153];
    NbaShotLaunchState *shot=&t->last_shot_launch;shot->owner=in[15];shot->last_owner=in[34];shot->display_shooter=in[50];
    shot->attempt_latch=in[20];shot->dead_0966=in[23];shot->height_0968=in[24];shot->initial_value=in[25];shot->dead_096c=in[26];
    shot->bounce_0920=in[8];shot->inner_veto=in[36];shot->live_state=in[13];shot->timeout_0930=in[12];shot->value=in[21];
    shot->display_value=in[49];shot->ball_record=in[4];shot->rng.state=in[1];shot->facing=in[84];shot->contact_inhibit=in[89];
    shot->assist_43=t->team_context[1].previous_dead_ball_actor_raw_43;shot->assist_45=(uint16_t)t->team_context[1].previous_controller_actor_raw_45;
    memcpy(shot->player_stats,t->roster_shot_statistics[persistent],sizeof(shot->player_stats));
    return true;
}

/* Host-only mode-fourteen replay adapter; no direct native address. Emit the
 * represented parent/child state projection after production `$86:B154`. */
static void emit_case(const NbaTipoff *t,const uint16_t in[]){
    unsigned n=0,slot=in[0];const NbaTipoffActor *a=&t->actors[slot];
#define E(v) emit((uint16_t)(v),&n)
    E(slot);E(t->rng.state);E(t->shot_origin_x);E(t->shot_origin_y);E(t->catch_actor_record_raw_0910);E(t->shot_roster_low_raw_0914);E(t->shot_roster_bank_raw_0916);
    E(t->shot_bounce_timer_raw_091c);E(t->rim_raw_0920);E(t->shot_previous_actor_x_raw_0922);E(t->period_raw_0926);E(t->match_clock_raw_0928);
    E(t->free_throw_flight_timer_raw_0930);E(t->live_state_raw);E(t->camera_side_group_raw);E((int16_t)t->possession_actor);E(t->pass_actor_raw);E(t->pass_aux_raw);E(t->pass_receiver_raw);
    E(t->ball_activity_raw);E(t->rim_raw_094a);E(t->shot_value_raw);E(t->close_finish_timing_raw_094e);E(t->dead_ball_raw_0966);E(t->dead_ball_raw_0968);E(t->rim_raw_096a);E(t->dead_ball_raw_096c);
    E(t->fouls.free_throw_state_raw_0978);E(t->fouls.free_throw_sequence_raw_097a);E(t->free_throw_aim_x_raw_0980);E(t->free_throw_aim_y_raw_0982);E(t->inbound_transfer_raw);
    E(t->assistance_team_raw_09c0);E(t->pass_active_raw);E(t->shot_actor_raw_09c8);E(t->attached_ball_state_raw_09f6);E(t->shot_inner_veto_raw);E(t->rim_raw_13e7);
    E(t->session->config.main_values[2]);E(t->session->config.options[5]);E(t->shot_control_raw_17c3);E(t->rim_force_raw_1866);E(t->rim_effect.gate_raw_3f33);
    E(t->court_presentation.basket_x_3fef);E(t->rim_effect.reference_y_raw_3ff3);E(t->rim_effect.resource_raw_4015);E(t->rim_effect.effect_raw_401b);E(t->rim_effect.frame_raw_4025);E(t->rim_effect.timer_raw_402d);
    E(t->hud.shot_category_raw_4939);E(t->shot_display_actor_raw_493b);E(in[51]);E(fixed_fraction_word(a->x_fp));E(fixed_integer_word(a->x_fp));E(fixed_fraction_word(a->y_fp));E(fixed_integer_word(a->y_fp));E(fixed_fraction_word(a->z_fp));E(fixed_integer_word(a->z_fp));
    E(a->velocity_x);E(a->velocity_y);E(a->velocity_z);E((int16_t)a->controller_assignment_raw);E(a->animation_upper_queue_cursor_raw_18);E(a->animation_lower_queue_cursor_raw_1a);
    for(unsigned q=0;q<3;++q)E(a->animation_upper_queue_raw_1c[q]);for(unsigned q=0;q<3;++q)E(a->animation_lower_queue_raw_22[q]);
    E(a->actor_status_raw_28);E(a->upper_animation_resource_raw_2a);E(a->lower_animation_resource_raw_2c);E(a->animation_state);E(a->lower_animation_state);E(a->base_animation_state_raw_38);
    E(a->rom_upper_animation_phase_raw_3a);E(a->rom_lower_animation_phase_raw_3c);E(a->upper_animation_accumulator_raw_42);E(a->lower_animation_accumulator_raw_44);E(a->upper_animation_lock_raw_46);E(a->lower_animation_lock_raw_48);
    E(a->movement_speed_raw_4a);E(a->movement_magnitude_raw);E(a->movement_direction);E(a->requested_direction);E(a->direction);E(a->special_contact_raw_56);E(a->mode13_variant_raw_58);E(a->contact_inhibit_raw_5a);
    E(a->control_mode);E(a->contact_action_timer_raw_60);E(a->behavior_timer);E(a->pass_direction_raw);E(a->animation_variant_raw_6c);E(a->team_group_raw_6e);E(a->movement_boost_timer);E(a->behavior_flags_raw);
    E(a->anchor_direction_raw);E(a->assignment_distance);E(a->anchor_distance_raw);E(a->free_throw_launch_half_raw_a8);E(a->upper_phase_target_raw_b0);E(a->shot_modifier_raw_b2);E(a->mode13_baseline_velocity_x);E(a->mode13_baseline_velocity_y);
    E(fixed_fraction_word(t->ball.x_fp));E(fixed_integer_word(t->ball.x_fp));E(fixed_fraction_word(t->ball.y_fp));E(fixed_integer_word(t->ball.y_fp));E(fixed_fraction_word(t->ball.z_fp));E(fixed_integer_word(t->ball.z_fp));E(t->ball.velocity_x);E(t->ball.velocity_y);E(t->ball.velocity_z);
    for(unsigned side=0;side<2;++side){const NbaGameplayTeamContext *c=&t->team_context[side];E(c->strategy_team_raw_00);E(c->anchor_x_fraction_raw_08);E(c->anchor_x_raw_0a);E(c->previous_dead_ball_actor_raw_43);E(c->previous_controller_actor_raw_45);E(c->match_clock_raw_47);}
    unsigned persistent=NBA_MATCH_ROSTER_SIZE+2u;for(unsigned p=0;p<5;++p)E(t->roster_shot_statistics[persistent][p]);E(a->shot_stamina_raw_18);
    for(unsigned p=0;p<5;++p)E(t->controllers.record[0].reserved[p]);
    for(unsigned i=0;i<NBA_GAMEPLAY_ACTOR_COUNT;++i)E(t->actors[i].control_mode);
    E(t->scratch_0046);E(t->scratch_0047);
    E(a->upper_state_snapshot_raw_34);E(a->lower_state_snapshot_raw_36);
    E(a->upper_phase_snapshot_raw_3e);E(a->lower_phase_snapshot_raw_40);
#undef E
    if(n!=FIELD_COUNT)fprintf(stderr,"field count %u\n",n);putchar('\n');
}

typedef enum {
    MODE14_OBSERVE,
    MODE14_LOSE_OWNER,
    MODE14_ORDER
} Mode14Mutation;
typedef struct {
    Mode14Mutation mutation;
    unsigned seen;
    int32_t x_fp, z_fp;
    int16_t vx, vz, attach_x, attach_y, attach_z;
    uint32_t upper_tick, lower_tick;
    uint16_t timer, reaction, activity, upper_resource, lower_resource;
    uint8_t upper_state;
    uint16_t upper_lock;
    bool attachment_valid;
} Mode14Phase;

/* Host-only mode-fourteen caller observer; no direct native address. Sample
 * the post-`$87:AAB2/$85:963D` boundary before globals and late `$87:9244`. */
static void observe_mode_fourteen_after_common(const NbaTipoff *observed,
        const char *boundary,void *raw){
    if(strcmp(boundary,"actors.end")!=0)return;
    Mode14Phase *phase=raw;NbaTipoff *t=(NbaTipoff *)observed;NbaTipoffActor *a=&t->actors[0];
    ++phase->seen;phase->x_fp=a->x_fp;phase->z_fp=a->z_fp;phase->vx=a->velocity_x;
    phase->vz=a->velocity_z;phase->upper_tick=a->upper_animation_tick;
    phase->lower_tick=a->lower_animation_tick;phase->timer=a->contact_action_timer_raw_60;
    phase->reaction=a->reaction_threshold;phase->activity=t->ball_activity_raw;
    phase->upper_resource=a->upper_animation_resource_raw_2a;
    phase->lower_resource=a->lower_animation_resource_raw_2c;
    phase->upper_state=a->animation_state;phase->upper_lock=a->upper_animation_lock_raw_46;
    phase->attachment_valid=nba_player_ball_attachment_point_offsets(t->assets,
        phase->upper_resource,phase->lower_resource,a->actor_status_raw_28,0u,
        &phase->attach_x,&phase->attach_y,&phase->attach_z);
    if(phase->mutation==MODE14_LOSE_OWNER)t->possession_actor=1;
    t->rim_force_raw_1866=0x7777u;t->close_finish_timing_raw_094e=0x6666u;
    t->ball_activity_raw=0x2222u;
}

/* Host-only mode-fourteen caller fixture; no direct native address. Prepare
 * an isolated live state for `$87:AAB2 -> $85:963D -> globals -> $87:9244`. */
static bool prepare_mode_fourteen_scheduler(const NbaAssetPack *assets,
        NbaSession *session,NbaTipoff *t){
    nba_session_init(session);session->right_team=18u;session->left_team=28u;
    if(!nba_tipoff_init(t,assets,session))return false;
    for(unsigned i=0;i<NBA_GAMEPLAY_ACTOR_COUNT;++i){NbaTipoffActor *a=&t->actors[i];
        a->control_mode=0u;a->controller_assignment_raw=-1;a->velocity_x=a->velocity_y=a->velocity_z=0;
        a->x_fp=(int32_t)(-300+(int)i*65)*256;a->y_fp=(int32_t)(-180+(int)i*37)*256;a->z_fp=0;}
    t->simulation_tick=1u;t->tip_contact_actor=0;t->tip_possession_frame=1u;t->phase=NBA_TIPOFF_LIVE;
    t->live_state_raw=0u;t->camera_side_group_raw=0u;t->possession_actor=0;t->handler_actor=0u;
    t->pass_actor_raw=0;t->pass_aux_raw=-1;t->pass_receiver_raw=0;
    t->ball.owner_actor=0;t->ball.state=NBA_BALL_ATTACHED;t->ball_activity_raw=1u;
    t->cpu_play_state=NBA_CPU_PLAY_ATTACK;t->team_context[0].anchor_x_raw_0a=-336;
    NbaTipoffActor *a=&t->actors[0];a->control_mode=14u;a->team_group_raw_6e=0u;
    a->x_fp=100*256+0x35;a->y_fp=0;a->z_fp=1*256+0x23;a->velocity_x=0x0100;
    a->velocity_y=0;a->velocity_z=0x0030;a->mode13_baseline_velocity_x=0x0100;
    a->mode13_baseline_velocity_y=0;a->animation_state=0x18u;a->lower_animation_state=0x1fu;
    a->base_animation_state_raw_38=3u;a->contact_action_timer_raw_60=0x28u;
    a->reaction_threshold=0xdeadu;a->special_contact_raw_56=1;a->movement_direction=4u;
    a->requested_direction=6u;a->direction=2u;a->rom_upper_animation_phase_raw_3a=0u;
    a->rom_lower_animation_phase_raw_3c=0u;a->upper_animation_accumulator_raw_42=0u;
    a->lower_animation_accumulator_raw_44=0u;a->upper_animation_lock_raw_46=1u;
    a->lower_animation_lock_raw_48=1u;a->animation_upper_queue_cursor_raw_18=0xffffu;
    a->animation_lower_queue_cursor_raw_1a=0xffffu;a->upper_animation_resource_raw_2a=0x01fau;
    a->lower_animation_resource_raw_2c=0x07c8u;a->animation_resources_valid=true;
    return true;
}

/* Host-only mode-fourteen scheduler order test; no direct native address.
 * Force the locked action descriptor to complete. Its unlock must precede
 * common locomotion, which maps base state 3 to airborne-owner state 12. */
static bool mode_fourteen_animation_order_case(const NbaAssetPack *assets){
    NbaSession session;NbaTipoff t;
    if(!prepare_mode_fourteen_scheduler(assets,&session,&t))return false;
    NbaTipoffActor *a=&t.actors[0];
    NbaPlayerAnimationChannels channels={
        a->animation_upper_queue_cursor_raw_18,a->animation_lower_queue_cursor_raw_1a,
        a->animation_state,a->lower_animation_state,a->base_animation_state_raw_38,
        a->rom_upper_animation_phase_raw_3a,a->rom_lower_animation_phase_raw_3c,
        a->upper_animation_accumulator_raw_42,a->lower_animation_accumulator_raw_44,
        a->upper_animation_lock_raw_46,a->lower_animation_lock_raw_48,{0},{0},
        a->upper_phase_target_raw_b0};
    memcpy(channels.upper_queue,a->animation_upper_queue_raw_1c,sizeof(channels.upper_queue));
    memcpy(channels.lower_queue,a->animation_lower_queue_raw_22,sizeof(channels.lower_queue));
    NbaPlayerAnimationChannels before=channels;
    uint16_t rng=t.rng.state,before_rng=rng,upper=0u,lower=0u;
    bool found=false;
    for(unsigned step=0;step<512u;++step){
        before=channels;before_rng=rng;
        if(!nba_player_animation_step_channels(assets,&channels,a->direction,0u,
                0x200u,false,0u,&rng,&upper,&lower))return false;
        if(before.upper_lock!=0u && channels.upper_lock==0u){found=true;break;}
    }
    if(!found)return false;
    a->animation_state=(uint8_t)before.upper_state;
    a->lower_animation_state=(uint8_t)before.lower_state;
    a->base_animation_state_raw_38=(uint8_t)before.base_state;
    a->rom_upper_animation_phase_raw_3a=before.upper_phase;
    a->rom_lower_animation_phase_raw_3c=before.lower_phase;
    a->upper_animation_accumulator_raw_42=before.upper_accumulator;
    a->lower_animation_accumulator_raw_44=before.lower_accumulator;
    a->upper_animation_lock_raw_46=before.upper_lock;
    a->lower_animation_lock_raw_48=before.lower_lock;
    a->animation_upper_queue_cursor_raw_18=before.upper_queue_cursor;
    a->animation_lower_queue_cursor_raw_1a=before.lower_queue_cursor;
    a->upper_phase_target_raw_b0=before.upper_phase_target;
    memcpy(a->animation_upper_queue_raw_1c,before.upper_queue,sizeof(before.upper_queue));
    memcpy(a->animation_lower_queue_raw_22,before.lower_queue,sizeof(before.lower_queue));
    t.rng.state=before_rng;
    Mode14Phase phase={.mutation=MODE14_ORDER};
    t.differential_observer=observe_mode_fourteen_after_common;t.differential_context=&phase;
    nba_tipoff_update(&t,NULL);
    if(phase.seen!=1u || phase.upper_state!=12u || phase.upper_lock!=0u ||
       phase.x_fp!=100*256+0x35+0x0200){
        fprintf(stderr,"order seen=%u state=%u lock=%04x x=%ld\n",phase.seen,
            phase.upper_state,phase.upper_lock,(long)phase.x_fp);return false;}
    return true;
}

/* Host-only mode-fourteen pass-creation test; no direct native address.
 * Exercise the real mode-11 owner flow through `$85:B50E` selection and
 * `$86:AB2D-$AF65`, then prove the first common pass consumes initialized +$60. */
static bool mode_fourteen_special_pass_creation_case(const NbaAssetPack *assets){
    NbaSession session;NbaTipoff t;
    if(!prepare_mode_fourteen_scheduler(assets,&session,&t))return false;
    for(unsigned i=0;i<NBA_GAMEPLAY_ACTOR_COUNT;++i){
        t.actors[i].control_mode=2u;t.actors[i].velocity_x=0;
        t.actors[i].velocity_y=0;t.actors[i].velocity_z=0;
    }
    NbaTipoffActor *passer=&t.actors[0],*receiver=&t.actors[1];
    passer->control_mode=11u;passer->controller_assignment_raw=-1;
    passer->reaction_threshold=0u;passer->recovery_inhibit_raw=1u;
    passer->assignment_base_raw=0xffffu;passer->x_fp=0;passer->y_fp=0;
    passer->z_fp=0;passer->movement_direction=0u;
    receiver->x_fp=100*256;receiver->y_fp=0;receiver->z_fp=0;
    receiver->velocity_x=0x0040;receiver->velocity_y=-0x0020;
    receiver->contact_action_timer_raw_60=0xbeefu;receiver->reaction_threshold=0xaaaau;
    t.special_actor_raw=1u;t.possession_actor=0;t.ball.owner_actor=0;
    t.team_context[0].anchor_x_raw_0a=336;
    t.actors[5].x_fp=50*256;t.actors[5].y_fp=0;
    t.rim_raw_092c=1000u;t.match_clock_raw_0928=1000u;
    t.pass_actor_raw=-1;t.pass_receiver_raw=-1;
    bool dispatched=nba_tipoff_replay_normal_actor(&t,0u);
    if(!dispatched || t.pass_receiver_raw!=1 ||
       receiver->control_mode!=14u ||
       receiver->contact_action_timer_raw_60==0xbeefu ||
       receiver->contact_action_timer_raw_60!=receiver->reaction_threshold ||
       receiver->mode13_baseline_velocity_x!=0x0040 ||
       receiver->mode13_baseline_velocity_y!=-0x0020){
        fprintf(stderr,"special transition dispatched=%u receiver=%d mode=%u timer=%04x reaction=%04x baseline=%d,%d passer_mode=%u play=%u\n",
            dispatched,t.pass_receiver_raw,receiver->control_mode,
            receiver->contact_action_timer_raw_60,receiver->reaction_threshold,
            receiver->mode13_baseline_velocity_x,receiver->mode13_baseline_velocity_y,
            passer->control_mode,t.cpu_play_state);return false;}
    uint16_t initialized=receiver->contact_action_timer_raw_60;
    passer->control_mode=0u;t.possession_actor=-1;t.ball.owner_actor=-1;
    t.ball.state=NBA_BALL_LOOSE;t.ball.x_fp=-300*256;t.ball.y_fp=-180*256;
    t.ball.z_fp=100*256;t.cpu_play_state=NBA_CPU_PLAY_ATTACK;
    nba_tipoff_update(&t,NULL);receiver=&t.actors[1];
    bool ok=initialized>=0x28u && receiver->control_mode==14u &&
        receiver->contact_action_timer_raw_60==(uint16_t)(initialized-2u) &&
        receiver->reaction_threshold==receiver->contact_action_timer_raw_60;
    if(!ok)fprintf(stderr,"special pass timer=%04x now=%04x reaction=%04x mode=%u\n",
        initialized,receiver->contact_action_timer_raw_60,
        receiver->reaction_threshold,receiver->control_mode);
    return ok;
}

/* Host-only mode-fourteen acquisition-order test; no direct native address.
 * A real post-common pass collision must install `$093E` and defer B154 to
 * the next odd frame, where it runs without a second physics/animation pass. */
static bool mode_fourteen_pass_acquisition_case(const NbaAssetPack *assets){
    NbaSession session;NbaTipoff t;
    if(!prepare_mode_fourteen_scheduler(assets,&session,&t))return false;
    NbaTipoffActor *a=&t.actors[0];int32_t initial_x=a->x_fp;
    uint32_t initial_upper=a->upper_animation_tick;
    t.possession_actor=-1;t.ball.owner_actor=-1;t.ball.state=NBA_BALL_PASS;
    t.cpu_play_state=NBA_CPU_PLAY_PASS;t.pass_actor_raw=1;t.pass_aux_raw=-1;
    t.pass_receiver_raw=0;t.ball_activity_raw=1u;
    t.ball.x_fp=initial_x+0x0200;t.ball.y_fp=a->y_fp;
    t.ball.z_fp=a->z_fp+8*256;t.ball.velocity_x=0;t.ball.velocity_y=0;
    t.ball.velocity_z=0;
    nba_tipoff_update(&t,NULL);a=&t.actors[0];
    if(t.possession_actor!=0 || a->control_mode!=14u ||
       (t.rim_raw_13e7&0x0010u)==0u || t.actor_behavior_pending!=1u ||
       a->contact_action_timer_raw_60!=0x28u || a->x_fp!=initial_x+0x0200 ||
       a->upper_animation_tick!=initial_upper+1u){
        fprintf(stderr,"acquire owner=%d mode=%u event=%04x pending=%u timer=%04x x=%ld tick=%lu\n",
            t.possession_actor,a->control_mode,t.rim_raw_13e7,t.actor_behavior_pending,
            a->contact_action_timer_raw_60,(long)a->x_fp,
            (unsigned long)a->upper_animation_tick);return false;}
    int32_t acquired_x=a->x_fp;uint32_t acquired_upper=a->upper_animation_tick;
    nba_tipoff_update(&t,NULL);a=&t.actors[0];
    bool ok=t.simulation_tick==3u && t.actor_behavior_pending==0u &&
        a->x_fp==acquired_x && a->upper_animation_tick==acquired_upper &&
        a->contact_action_timer_raw_60==0x26u;
    if(!ok)fprintf(stderr,"acquire odd tick=%lu pending=%u timer=%04x x=%ld/%ld upper=%lu/%lu\n",
        (unsigned long)t.simulation_tick,t.actor_behavior_pending,
        a->contact_action_timer_raw_60,(long)a->x_fp,(long)acquired_x,
        (unsigned long)a->upper_animation_tick,(unsigned long)acquired_upper);
    return ok;
}

/* Host-only mode-fourteen production caller test; no direct native address.
 * Prove pre-common animation, one common physics pass, intervening globals,
 * canonical +$60 ownership, late parent, restore/next due and odd dispatch. */
static bool mode_fourteen_scheduler_cases(const NbaAssetPack *assets){
    NbaSession session;NbaTipoff t;
    if(!prepare_mode_fourteen_scheduler(assets,&session,&t))return false;
    NbaTipoffActor *a=&t.actors[0];int32_t initial_x=a->x_fp;
    uint32_t initial_upper=a->upper_animation_tick,initial_lower=a->lower_animation_tick;
    Mode14Phase ordinary={.mutation=MODE14_OBSERVE};
    t.differential_observer=observe_mode_fourteen_after_common;t.differential_context=&ordinary;
    nba_tipoff_update(&t,NULL);a=&t.actors[0];
    int16_t ball_x=(int16_t)(t.ball.x_fp>>8),ball_y=(int16_t)(t.ball.y_fp>>8),
        ball_z=(int16_t)(t.ball.z_fp>>8);
    if(ordinary.seen!=1u || ordinary.x_fp!=initial_x+0x0200 ||
       ordinary.upper_tick!=initial_upper+1u || ordinary.lower_tick!=initial_lower+1u ||
       ordinary.timer!=0x28u || ordinary.reaction!=0x28u || !ordinary.attachment_valid ||
       a->x_fp!=ordinary.x_fp || a->z_fp!=ordinary.z_fp ||
       a->contact_action_timer_raw_60!=0x26u || a->reaction_threshold!=0x26u ||
       t.ball_activity_raw!=0x2222u || t.rim_force_raw_1866!=0x34ebu ||
       t.close_finish_timing_raw_094e!=0x1eu || t.live_state_raw!=2u ||
       ball_x!=(int16_t)((a->x_fp>>8)+ordinary.attach_x) ||
       ball_y!=(int16_t)((a->y_fp>>8)+ordinary.attach_y) ||
       ball_z!=(int16_t)((a->z_fp>>8)+ordinary.attach_z)){
        fprintf(stderr,"ordinary seen=%u x=%ld/%ld ticks=%lu/%lu timer=%04x/%04x reaction=%04x/%04x attach=%u ball=%d,%d,%d offset=%d,%d,%d activity=%04x rim=%04x timing=%04x mode=%u\n",
            ordinary.seen,(long)ordinary.x_fp,(long)(initial_x+0x200),
            (unsigned long)ordinary.upper_tick,(unsigned long)ordinary.lower_tick,
            ordinary.timer,a->contact_action_timer_raw_60,ordinary.reaction,a->reaction_threshold,
            ordinary.attachment_valid,ball_x,ball_y,ball_z,ordinary.attach_x,
            ordinary.attach_y,ordinary.attach_z,t.ball_activity_raw,t.rim_force_raw_1866,
            t.close_finish_timing_raw_094e,a->control_mode);return false;}

    if(!prepare_mode_fourteen_scheduler(assets,&session,&t))return false;a=&t.actors[0];initial_x=a->x_fp;
    Mode14Phase lost={.mutation=MODE14_LOSE_OWNER};
    t.differential_observer=observe_mode_fourteen_after_common;t.differential_context=&lost;
    nba_tipoff_update(&t,NULL);a=&t.actors[0];
    if(lost.seen!=1u || lost.x_fp!=initial_x+0x0200 || a->x_fp!=lost.x_fp ||
       a->control_mode!=1u || a->contact_action_timer_raw_60!=0u ||
       t.rim_force_raw_1866!=0u || t.pass_actor_raw!=-1 || t.pass_receiver_raw!=-1){
        fprintf(stderr,"lost seen=%u x=%ld/%ld mode=%u timer=%04x rim=%04x pass=%d/%d\n",
            lost.seen,(long)lost.x_fp,(long)(initial_x+0x200),a->control_mode,
            a->contact_action_timer_raw_60,t.rim_force_raw_1866,t.pass_actor_raw,t.pass_receiver_raw);return false;}
    t.differential_observer=NULL;int32_t restored_x=a->x_fp;uint32_t restored_upper=a->upper_animation_tick;
    nba_tipoff_update(&t,NULL);a=&t.actors[0];
    if(t.simulation_tick!=3u || a->x_fp!=restored_x || a->upper_animation_tick!=restored_upper){
        fprintf(stderr,"lost odd tick=%lu x=%ld/%ld upper=%lu/%lu\n",(unsigned long)t.simulation_tick,
            (long)a->x_fp,(long)restored_x,(unsigned long)a->upper_animation_tick,
            (unsigned long)restored_upper);return false;}
    nba_tipoff_update(&t,NULL);a=&t.actors[0];
    if(t.simulation_tick!=4u || a->x_fp==restored_x || a->upper_animation_tick!=restored_upper+1u){
        fprintf(stderr,"lost due tick=%lu x=%ld/%ld upper=%lu/%lu\n",(unsigned long)t.simulation_tick,
            (long)a->x_fp,(long)restored_x,(unsigned long)a->upper_animation_tick,
            (unsigned long)restored_upper);return false;}

    return mode_fourteen_animation_order_case(assets) &&
        mode_fourteen_special_pass_creation_case(assets) &&
        mode_fourteen_pass_acquisition_case(assets);
}

/* Host-only shot-table pack contract; no direct native address. Prove the
 * eight-range mode-fourteen pack and both prior launch/close-finish layouts. */
static bool mode_fourteen_pack_contract(const NbaAssetPack *assets,int kind){
    const NbaAssetItem *item=nba_assets_get(assets,NBA_ASSET_GAMEPLAY_SHOT_TABLES);
    if(!item)return false;
    bool mode14=item->size==640u && item->width==8u && item->height==0u &&
        item->flags==0x869eb2u;
    bool mode13=item->size==620u && item->width==7u && item->height==0u &&
        item->flags==0x869eb2u;
    bool legacy=item->size==528u && item->width==5u && item->height==0u &&
        item->flags==0x869eb2u;
    NbaShotLaunchInput input={0};NbaShotLaunchState state={0};
    input.special_entry=true;input.controller=0;input.shot_control_17c3=1u;
    input.basket_x=100;input.actor_x=0;state.owner=0u;
    nba_gameplay_rng_seed(&state.rng,0x9146u);
    bool launch=nba_shot_launch(assets,&input,&state);
    uint8_t turn=0u;uint16_t animation=0u,queue0=0u,queue1=0u,queue6=0u;
    bool close=nba_shot_close_finish_turn(assets,false,1u,&turn) &&
        nba_shot_close_finish_turn(assets,true,28u,&turn) &&
        !nba_shot_close_finish_turn(assets,false,30u,&turn) &&
        nba_shot_close_finish_landing(assets,0u,&animation) &&
        nba_shot_close_finish_landing(assets,6u,&animation) &&
        !nba_shot_close_finish_landing(assets,1u,&animation) &&
        !nba_shot_close_finish_landing(assets,8u,&animation);
    bool receiver=nba_shot_special_receiver_lower_queue(assets,0u,&queue0) &&
        nba_shot_special_receiver_lower_queue(assets,1u,&queue1) &&
        nba_shot_special_receiver_lower_queue(assets,6u,&queue6) &&
        !nba_shot_special_receiver_lower_queue(assets,7u,&queue6) &&
        queue0!=queue1;
    if(kind==1)return mode14 && launch && close && receiver;
    if(kind==2)return mode13 && launch && close && !receiver;
    if(kind==3)return legacy && launch && !close && !receiver;
    return mode14 && !launch && !close && !receiver;
}

/* Host-only mode-fourteen executable fixture; no direct native address.
 * Replay compact native witnesses through the production parent. */
int main(int argc,char **argv){
    bool stale=argc==3 && strcmp(argv[2],"--stale-ball-owner")==0;
    bool self_test=argc==3 && strcmp(argv[2],"--self-test")==0;
    int pack_contract=argc==3 && strcmp(argv[2],"--pack-new")==0 ? 1 :
        argc==3 && strcmp(argv[2],"--pack-mode13")==0 ? 2 :
        argc==3 && strcmp(argv[2],"--pack-old")==0 ? 3 :
        argc==3 && strcmp(argv[2],"--pack-reject")==0 ? 4 : 0;
    if(argc<2 || argc>3 || (argc==3 && !stale && !self_test && !pack_contract)){fprintf(stderr,"usage: %s <asset-pack> [--stale-ball-owner|--self-test|--pack-new|--pack-mode13|--pack-old|--pack-reject]\n",argv[0]);return 2;}
    NbaAssetPack assets={0};if(!nba_assets_load(&assets,argv[1]))return 3;
    if(self_test){bool ok=mode_fourteen_scheduler_cases(&assets);nba_assets_free(&assets);
        if(ok)puts("[CPU MODE FOURTEEN CALLER] PASS: animation/common physics, globals, canonical timer, late parent, restore and odd dispatch");return ok?0:4;}
    if(pack_contract){bool ok=mode_fourteen_pack_contract(&assets,pack_contract);nba_assets_free(&assets);
        if(ok)puts("[CPU MODE FOURTEEN PACK] PASS");return ok?0:4;}
    _setmode(_fileno(stdin),_O_BINARY);uint16_t in[FIELD_COUNT];
    while(fread(in,sizeof(in),1,stdin)==1){NbaSession session;NbaTipoff tipoff;
        if(!load_case(&tipoff,&session,&assets,in)){nba_assets_free(&assets);return 4;}
        if(stale){tipoff.ball.owner_actor=0;tipoff.ball.state=NBA_BALL_ATTACHED;}
        if(!nba_tipoff_replay_mode14_close_finish(&tipoff,(uint8_t)in[0])){nba_assets_free(&assets);return 4;}
        emit_case(&tipoff,in);
    }
    bool ok=feof(stdin);nba_assets_free(&assets);return ok?0:5;
}
