#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nba_tipoff.h"

#define FIELD_COUNT 144u

/* Host-only mode-thirteen replay helper; no direct native address. Rebuild
 * the represented signed 16.8 coordinate from native fraction/integer words. */
static int32_t fixed_from_words(uint16_t fraction,uint16_t integer){
    return (int32_t)(int16_t)integer*256+(fraction>>8);
}
/* Host-only mode-thirteen replay helper; no direct native address. Recover
 * the native fraction word from a production 16.8 coordinate. */
static uint16_t fixed_fraction_word(int32_t value){return (uint16_t)(((uint32_t)value&255u)<<8);}
/* Host-only mode-thirteen replay helper; no direct native address. Recover
 * the signed native integer coordinate word. */
static uint16_t fixed_integer_word(int32_t value){
    return (uint16_t)(int16_t)(value>=0?value/256:-(((-value)+255)/256));
}
/* Host-only mode-thirteen replay helper; no direct native address. Emit one
 * compact projected word in fixture order. */
static void emit(uint16_t value,unsigned *count){printf("%s%04x",*count?" ":"",value);++*count;}

/* Host-only mode-thirteen replay adapter; no direct native address. Map the
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
    a->contact_action_timer_raw_60=in[91];a->reaction_threshold=in[91];a->behavior_timer=in[92];a->pass_direction_raw=in[93];
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
    t->scratch_0046=in[138];t->scratch_0047=in[139];t->offense_side=1u;t->handler_actor=(uint8_t)slot;
    a->upper_state_snapshot_raw_34=in[140];a->lower_state_snapshot_raw_36=in[141];
    a->upper_phase_snapshot_raw_3e=in[142];a->lower_phase_snapshot_raw_40=in[143];
    NbaShotLaunchState *shot=&t->last_shot_launch;shot->owner=in[15];shot->last_owner=in[34];shot->display_shooter=in[50];
    shot->attempt_latch=in[20];shot->dead_0966=in[23];shot->height_0968=in[24];shot->initial_value=in[25];shot->dead_096c=in[26];
    shot->bounce_0920=in[8];shot->inner_veto=in[36];shot->live_state=in[13];shot->timeout_0930=in[12];shot->value=in[21];
    shot->display_value=in[49];shot->ball_record=in[4];shot->rng.state=in[1];shot->facing=in[84];shot->contact_inhibit=in[89];
    shot->assist_43=t->team_context[1].previous_dead_ball_actor_raw_43;shot->assist_45=(uint16_t)t->team_context[1].previous_controller_actor_raw_45;
    memcpy(shot->player_stats,t->roster_shot_statistics[persistent],sizeof(shot->player_stats));
    return true;
}

/* Host-only mode-thirteen replay adapter; no direct native address. Emit the
 * represented parent/child state projection after production `$86:A7DA`. */
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
    for(unsigned p=0;p<5;++p)E(t->controllers.record[0].reserved[p]);E(t->scratch_0046);E(t->scratch_0047);
    E(a->upper_state_snapshot_raw_34);E(a->lower_state_snapshot_raw_36);
    E(a->upper_phase_snapshot_raw_3e);E(a->lower_phase_snapshot_raw_40);
#undef E
    if(n!=FIELD_COUNT)fprintf(stderr,"field count %u\n",n);putchar('\n');
}

typedef enum {MODE13_OBSERVE,MODE13_LOSE_OWNER,MODE13_DEFER} Mode13Mutation;
typedef struct {Mode13Mutation mutation;unsigned seen;int32_t x_fp,z_fp;
    int16_t vx,vz;uint32_t upper_tick,lower_tick;uint16_t timer,activity;} Mode13Phase;

/* Host-only mode-thirteen caller observer; no direct native address. Sample
 * the post-`$87:AAB2/$85:963D` boundary before globals and late `$87:9244`. */
static void observe_mode_thirteen_after_common(const NbaTipoff *observed,
        const char *boundary,void *raw){
    if(strcmp(boundary,"actors.end")!=0)return;
    Mode13Phase *phase=raw;NbaTipoff *t=(NbaTipoff *)observed;NbaTipoffActor *a=&t->actors[0];
    ++phase->seen;phase->x_fp=a->x_fp;phase->z_fp=a->z_fp;phase->vx=a->velocity_x;
    phase->vz=a->velocity_z;phase->upper_tick=a->upper_animation_tick;
    phase->lower_tick=a->lower_animation_tick;phase->timer=a->contact_action_timer_raw_60;
    phase->activity=t->ball_activity_raw;
    if(phase->mutation==MODE13_LOSE_OWNER)t->possession_actor=1;
    if(phase->mutation==MODE13_DEFER)t->rim_raw_13e7|=0x10u;
    t->rim_force_raw_1866=0x7777u;
    t->ball_activity_raw=0x2222u;
}

/* Host-only mode-thirteen caller fixture; no direct native address. Prepare
 * an isolated live state for `$87:AAB2 -> $85:963D -> globals -> $87:9244`. */
static bool prepare_mode_thirteen_scheduler(const NbaAssetPack *assets,
        NbaSession *session,NbaTipoff *t){
    nba_session_init(session);session->right_team=18u;session->left_team=28u;
    if(!nba_tipoff_init(t,assets,session))return false;
    for(unsigned i=0;i<NBA_GAMEPLAY_ACTOR_COUNT;++i){NbaTipoffActor *a=&t->actors[i];
        a->control_mode=0u;a->controller_assignment_raw=-1;a->velocity_x=a->velocity_y=a->velocity_z=0;
        a->x_fp=(int32_t)(-300+(int)i*65)*256;a->y_fp=(int32_t)(-180+(int)i*37)*256;a->z_fp=0;}
    t->simulation_tick=1u;t->tip_contact_actor=0;t->tip_possession_frame=1u;t->phase=NBA_TIPOFF_LIVE;
    t->live_state_raw=0u;t->camera_side_group_raw=0u;t->possession_actor=0;t->handler_actor=0u;
    t->ball.owner_actor=0;t->ball.state=NBA_BALL_ATTACHED;t->ball_activity_raw=1u;
    t->cpu_play_state=NBA_CPU_PLAY_ATTACK;t->team_context[0].anchor_x_raw_0a=-336;
    NbaTipoffActor *a=&t->actors[0];a->control_mode=13u;a->team_group_raw_6e=0u;
    a->x_fp=100*256+0x35;a->y_fp=0;a->z_fp=1*256+0x23;a->velocity_x=0x0100;
    a->velocity_y=0;a->velocity_z=0x0030;a->mode13_baseline_velocity_x=0x0100;
    a->mode13_baseline_velocity_y=0;a->animation_state=0x18u;a->lower_animation_state=0x1fu;
    a->base_animation_state_raw_38=3u;a->contact_action_timer_raw_60=0x28u;a->reaction_threshold=0x28u;
    a->special_contact_raw_56=1;a->movement_direction=4u;a->requested_direction=6u;a->direction=2u;
    a->rom_upper_animation_phase_raw_3a=0u;a->rom_lower_animation_phase_raw_3c=0u;
    a->upper_animation_accumulator_raw_42=0x05ffu;a->lower_animation_accumulator_raw_44=0x05ffu;
    a->upper_animation_lock_raw_46=1u;a->lower_animation_lock_raw_48=1u;
    a->upper_animation_resource_raw_2a=0x01fau;a->lower_animation_resource_raw_2c=0x07c8u;
    a->animation_resources_valid=true;return true;
}

/* Host-only mode-thirteen caller support; no direct native address. Clone the
 * animation resource in memory and make one raw-direction-8 descriptor pair
 * valid so the `$87:AEC3` preserve-+$52 branch has a focused executable case. */
static bool mode_thirteen_direction_eight_case(const NbaAssetPack *assets){
    const NbaAssetItem *source=nba_assets_get(assets,NBA_ASSET_PLAYER_ANIMATIONS);
    if(!source || !source->data || source->size<80u)return false;
    uint8_t *copy=malloc(source->size);if(!copy)return false;
    memcpy(copy,source->data,source->size);NbaAssetPack altered=*assets;
    bool replaced=false;for(uint32_t i=0;i<altered.item_count;++i)
        if(altered.items[i].id==NBA_ASSET_PLAYER_ANIMATIONS){
            altered.items[i].data=copy;replaced=true;break;
        }
    if(!replaced){free(copy);return false;}
    uint32_t bank_offset=(uint32_t)copy[20]|((uint32_t)copy[21]<<8)|
        ((uint32_t)copy[22]<<16)|((uint32_t)copy[23]<<24);
    if(bank_offset>source->size || source->size-bank_offset<0x8000u){free(copy);return false;}
    uint8_t *bank=copy+bank_offset;uint16_t supported_upper=0xffffu,supported_lower=0xffffu;
    for(uint16_t upper=0;upper<0x30u && supported_upper==0xffffu;++upper)
        for(uint16_t lower=0;lower<0x30u;++lower){
            NbaPlayerAnimationChannels channels={0};NbaPlayerResolvedPose pose={0};
            channels.upper_state=upper;channels.lower_state=lower;
            if(nba_player_resolve_pose(assets,&channels,0u,false,0u,&pose)){
                supported_upper=upper;supported_lower=lower;break;
            }
        }
    if(supported_upper==0xffffu){free(copy);return false;}
    const uint32_t tables[2]={0x42fcu,0x4218u};
    const uint16_t states[2]={supported_upper,supported_lower};
    for(unsigned i=0;i<2u;++i){
        uint32_t entry=tables[i]+states[i]*2u;
        uint16_t descriptor=(uint16_t)(bank[entry]|((uint16_t)bank[entry+1u]<<8));
        if(descriptor<0x8000u || descriptor>0xffe6u){free(copy);return false;}
        uint32_t raw=(uint32_t)descriptor-0x8000u;
        bank[raw+24u]=bank[raw+8u];bank[raw+25u]=bank[raw+9u];
    }
    NbaPlayerAnimationChannels supported={0};NbaPlayerResolvedPose resolved={0};
    supported.upper_state=supported_upper;supported.lower_state=supported_lower;
    resolved.direction=2u;resolved.mirror_flags=0x9234u;
    if(!nba_player_resolve_pose(&altered,&supported,8u,false,0u,&resolved) ||
       resolved.direction!=2u || resolved.mirror_flags!=0x1234u){
        fprintf(stderr,"altered direction8 resolved=%u displayed=%u status=%04x\n",
            resolved.upper_resource,resolved.direction,resolved.mirror_flags);
        free(copy);return false;
    }
    NbaSession session;NbaTipoff t;
    if(!prepare_mode_thirteen_scheduler(assets,&session,&t)){free(copy);return false;}
    t.assets=&altered;
    NbaTipoffActor *a=&t.actors[0];a->movement_direction=8u;a->direction=2u;
    a->animation_state=(uint8_t)supported_upper;
    a->lower_animation_state=(uint8_t)supported_lower;a->z_fp=0;
    a->free_throw_launch_half_raw_a8=0u;
    a->actor_status_raw_28=0x9234u;
    bool ok=nba_tipoff_replay_mode13_close_finish(&t,0u) &&
        a->direction==2u && a->actor_status_raw_28==0x1234u;
    if(!ok)fprintf(stderr,"direction8 displayed=%u status=%04x\n",
        a->direction,a->actor_status_raw_28);
    free(copy);return ok;
}

/* Host-only mode-thirteen production caller test; no direct native address.
 * Prove one animation/common-physics pass, intervening globals, late parent,
 * owner-loss restore/next due, and the retained odd-frame dispatch. */
static bool mode_thirteen_scheduler_cases(const NbaAssetPack *assets){
    NbaSession session;NbaTipoff t;
    if(!prepare_mode_thirteen_scheduler(assets,&session,&t))return false;
    NbaTipoffActor *a=&t.actors[0];int32_t initial_x=a->x_fp;
    uint32_t initial_upper=a->upper_animation_tick,initial_lower=a->lower_animation_tick;
    Mode13Phase ordinary={.mutation=MODE13_OBSERVE};t.differential_observer=observe_mode_thirteen_after_common;t.differential_context=&ordinary;
    nba_tipoff_update(&t,NULL);a=&t.actors[0];
    if(ordinary.seen!=1u || ordinary.x_fp!=initial_x+0x0200 ||
       ordinary.upper_tick!=initial_upper+1u || ordinary.lower_tick!=initial_lower+1u ||
       ordinary.timer!=0x28u || a->x_fp!=ordinary.x_fp || a->z_fp!=ordinary.z_fp ||
       a->contact_action_timer_raw_60!=0x26u || t.ball_activity_raw!=1u ||
       t.rim_force_raw_1866!=0x34ebu || t.close_finish_timing_raw_094e!=0x1eu){
        fprintf(stderr,"ordinary seen=%u x=%ld/%ld ticks=%lu/%lu timer=%04x/%04x z=%ld/%ld activity=%04x rim=%04x timing=%04x mode=%u\n",
            ordinary.seen,(long)ordinary.x_fp,(long)(initial_x+0x200),(unsigned long)ordinary.upper_tick,
            (unsigned long)ordinary.lower_tick,ordinary.timer,a->contact_action_timer_raw_60,
            (long)ordinary.z_fp,(long)a->z_fp,t.ball_activity_raw,t.rim_force_raw_1866,
            t.close_finish_timing_raw_094e,a->control_mode);return false;}

    if(!prepare_mode_thirteen_scheduler(assets,&session,&t))return false;a=&t.actors[0];initial_x=a->x_fp;
    Mode13Phase lost={.mutation=MODE13_LOSE_OWNER};t.differential_observer=observe_mode_thirteen_after_common;t.differential_context=&lost;
    nba_tipoff_update(&t,NULL);a=&t.actors[0];
    if(lost.seen!=1u || lost.x_fp!=initial_x+0x0200 || a->x_fp!=lost.x_fp ||
       a->control_mode!=1u || a->contact_action_timer_raw_60!=0u ||
       t.rim_force_raw_1866!=0u || t.pass_actor_raw!=-1 || t.pass_receiver_raw!=-1){
        fprintf(stderr,"lost seen=%u x=%ld/%ld mode=%u timer=%04x rim=%04x pass=%d/%d\n",
            lost.seen,(long)lost.x_fp,(long)(initial_x+0x200),a->control_mode,
            a->contact_action_timer_raw_60,t.rim_force_raw_1866,t.pass_actor_raw,t.pass_receiver_raw);return false;}
    t.differential_observer=NULL;int32_t restored_x=a->x_fp;uint32_t restored_upper=a->upper_animation_tick;
    nba_tipoff_update(&t,NULL);a=&t.actors[0];if(t.simulation_tick!=3u || a->x_fp!=restored_x || a->upper_animation_tick!=restored_upper){fprintf(stderr,"lost odd tick=%lu x=%ld/%ld upper=%lu/%lu\n",(unsigned long)t.simulation_tick,(long)a->x_fp,(long)restored_x,(unsigned long)a->upper_animation_tick,(unsigned long)restored_upper);return false;}
    nba_tipoff_update(&t,NULL);a=&t.actors[0];if(t.simulation_tick!=4u || a->x_fp==restored_x || a->upper_animation_tick!=restored_upper+1u){fprintf(stderr,"lost due tick=%lu x=%ld/%ld upper=%lu/%lu\n",(unsigned long)t.simulation_tick,(long)a->x_fp,(long)restored_x,(unsigned long)a->upper_animation_tick,(unsigned long)restored_upper);return false;}

    if(!prepare_mode_thirteen_scheduler(assets,&session,&t))return false;a=&t.actors[0];initial_x=a->x_fp;
    Mode13Phase pending={.mutation=MODE13_DEFER};t.differential_observer=observe_mode_thirteen_after_common;t.differential_context=&pending;
    nba_tipoff_update(&t,NULL);a=&t.actors[0];
    if(pending.seen!=1u || pending.x_fp!=initial_x+0x0200 || t.actor_behavior_pending!=1u ||
       a->contact_action_timer_raw_60!=0x28u || t.ball_activity_raw!=0x2222u)return false;
    t.differential_observer=NULL;int32_t pending_x=a->x_fp;uint32_t pending_upper=a->upper_animation_tick;
    nba_tipoff_update(&t,NULL);a=&t.actors[0];
    bool ok=t.simulation_tick==3u && t.actor_behavior_pending==0u && a->x_fp==pending_x &&
        a->upper_animation_tick==pending_upper && a->contact_action_timer_raw_60==0x26u &&
        t.ball_activity_raw==1u && t.rim_force_raw_1866==0x34ebu;
    if(!ok)fprintf(stderr,"pending odd tick=%lu pending=%u x=%ld/%ld upper=%lu/%lu timer=%04x activity=%04x rim=%04x\n",
        (unsigned long)t.simulation_tick,t.actor_behavior_pending,(long)a->x_fp,(long)pending_x,
        (unsigned long)a->upper_animation_tick,(unsigned long)pending_upper,
        a->contact_action_timer_raw_60,t.ball_activity_raw,t.rim_force_raw_1866);
    if(!ok)return false;

    return ok && mode_thirteen_direction_eight_case(assets);
}

/* Host-only shot-table pack contract; no direct native address. Prove the
 * legacy five-range launch payload remains usable while close-finish reads
 * require the exact seven-range extension and reject malformed directories. */
static bool mode_thirteen_pack_contract(const NbaAssetPack *assets,int kind){
    const NbaAssetItem *item=nba_assets_get(assets,NBA_ASSET_GAMEPLAY_SHOT_TABLES);
    if(!item)return false;
    bool extended=item->size==620u && item->width==7u && item->height==0u &&
        item->flags==0x869eb2u;
    bool legacy=item->size==528u && item->width==5u && item->height==0u &&
        item->flags==0x869eb2u;
    NbaShotLaunchInput input={0};NbaShotLaunchState state={0};
    input.special_entry=true;input.controller=0;input.shot_control_17c3=1u;
    input.basket_x=100;input.actor_x=0;state.owner=0u;
    nba_gameplay_rng_seed(&state.rng,0x9146u);
    bool launch=nba_shot_launch(assets,&input,&state);
    uint8_t turn=0u;uint16_t animation=0u;
    bool close=nba_shot_close_finish_turn(assets,false,1u,&turn) &&
        nba_shot_close_finish_turn(assets,true,28u,&turn) &&
        !nba_shot_close_finish_turn(assets,false,30u,&turn) &&
        nba_shot_close_finish_landing(assets,0u,&animation) &&
        nba_shot_close_finish_landing(assets,6u,&animation) &&
        !nba_shot_close_finish_landing(assets,1u,&animation) &&
        !nba_shot_close_finish_landing(assets,8u,&animation);
    if(kind==1)return extended && launch && close;
    if(kind==2)return legacy && launch && !close;
    return extended && !launch && !close;
}

/* Host-only mode-thirteen executable fixture; no direct native address.
 * Replay compact native witnesses through the production parent. */
int main(int argc,char **argv){
    bool stale=argc==3 && strcmp(argv[2],"--stale-ball-owner")==0;
    bool self_test=argc==3 && strcmp(argv[2],"--self-test")==0;
    int pack_contract=argc==3 && strcmp(argv[2],"--pack-new")==0 ? 1 :
        argc==3 && strcmp(argv[2],"--pack-old")==0 ? 2 :
        argc==3 && strcmp(argv[2],"--pack-reject")==0 ? 3 : 0;
    if(argc<2 || argc>3 || (argc==3 && !stale && !self_test && !pack_contract)){fprintf(stderr,"usage: %s <asset-pack> [--stale-ball-owner|--self-test|--pack-new|--pack-old|--pack-reject]\n",argv[0]);return 2;}
    NbaAssetPack assets={0};if(!nba_assets_load(&assets,argv[1]))return 3;
    if(self_test){bool ok=mode_thirteen_scheduler_cases(&assets);nba_assets_free(&assets);
        if(ok)puts("[CPU MODE THIRTEEN CALLER] PASS: common physics, globals, late parent, restore and odd dispatch");return ok?0:4;}
    if(pack_contract){bool ok=mode_thirteen_pack_contract(&assets,pack_contract);nba_assets_free(&assets);
        if(ok)puts("[CPU MODE THIRTEEN PACK] PASS");return ok?0:4;}
    _setmode(_fileno(stdin),_O_BINARY);uint16_t in[FIELD_COUNT];
    while(fread(in,sizeof(in),1,stdin)==1){NbaSession session;NbaTipoff tipoff;
        if(!load_case(&tipoff,&session,&assets,in)){nba_assets_free(&assets);return 4;}
        if(stale){tipoff.ball.owner_actor=0;tipoff.ball.state=NBA_BALL_ATTACHED;}
        if(!nba_tipoff_replay_mode13_close_finish(&tipoff,(uint8_t)in[0])){nba_assets_free(&assets);return 4;}
        emit_case(&tipoff,in);
    }
    bool ok=feof(stdin);nba_assets_free(&assets);return ok?0:5;
}
