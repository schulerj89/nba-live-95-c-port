/* Replay live `$86:D25A-$D3C5` ball-acquisition continuations. */
#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nba_tipoff.h"

#define WRAM_SIZE 0x4B00u
#define ACTOR_BASE 0x34EBu
#define ACTOR_STRIDE 0x100u

static uint16_t word(const uint8_t *raw, unsigned address) {
    return (uint16_t)(raw[address] | ((uint16_t)raw[address + 1u] << 8));
}

static int16_t sword(const uint8_t *raw, unsigned address) {
    return (int16_t)word(raw, address);
}

static int32_t fixed_position(const uint8_t *raw, unsigned base,
                              unsigned fraction, unsigned integer) {
    return (int32_t)sword(raw, base + integer) * 256 +
           (uint8_t)(word(raw, base + fraction) >> 8);
}

static void load_actor(NbaTipoffActor *actor, const uint8_t *raw,
                       unsigned base) {
    actor->x_fp = fixed_position(raw, base, 2u, 4u);
    actor->y_fp = fixed_position(raw, base, 6u, 8u);
    actor->z_fp = fixed_position(raw, base, 0x0Au, 0x0Cu);
    actor->velocity_x = sword(raw, base + 0x0Eu);
    actor->velocity_y = sword(raw, base + 0x10u);
    actor->velocity_z = sword(raw, base + 0x12u);
    actor->controller_assignment_raw = (int8_t)sword(raw, base + 0x16u);
    actor->actor_status_raw_28 = word(raw, base + 0x28u);
    actor->action_state = word(raw, base + 0x2Au);
    actor->animation_state = (uint8_t)word(raw, base + 0x30u);
    actor->lower_animation_state = (uint8_t)word(raw, base + 0x32u);
    actor->upper_animation_tick = word(raw, base + 0x34u);
    actor->lower_animation_tick = word(raw, base + 0x36u);
    actor->base_animation_state_raw_38 = (uint8_t)word(raw, base + 0x38u);
    actor->upper_animation_phase_raw = word(raw, base + 0x3Au);
    actor->direction = (uint8_t)word(raw, base + 0x4Eu);
    actor->requested_direction = (uint8_t)word(raw, base + 0x50u);
    actor->movement_direction = (uint8_t)word(raw, base + 0x52u);
    actor->special_contact_raw_56 = sword(raw, base + 0x56u);
    actor->contact_inhibit_raw_5a = word(raw, base + 0x5Au);
    actor->control_mode = (uint8_t)word(raw, base + 0x5Eu);
    actor->contact_action_timer_raw_60 = word(raw, base + 0x60u);
    actor->pass_band_raw = word(raw, base + 0x62u);
    actor->behavior_timer = word(raw, base + 0x64u);
    actor->pass_direction_raw = word(raw, base + 0x66u);
    actor->team_group_raw_6e = word(raw, base + 0x6Eu);
    actor->movement_boost_timer = word(raw, base + 0x72u);
    actor->assignment_current_raw = word(raw, base + 0x74u);
    actor->assignment_base_raw = word(raw, base + 0x76u);
    actor->assignment_alternate_raw = word(raw, base + 0x78u);
    actor->recovery_inhibit_raw = word(raw, base + 0x7Au);
    actor->behavior_flags_raw = word(raw, base + 0x7Eu);
    actor->saved_control_mode = (uint8_t)word(raw, base + 0x84u);
    actor->anchor_distance_raw = word(raw, base + 0x8Cu);
    actor->reaction_threshold = word(raw, base + 0x60u);
    actor->contact_height_raw_aa = word(raw, base + 0xAAu);
    actor->catcher_latch_raw_ae = word(raw, base + 0xAEu);
    actor->pass_family_raw = sword(raw, base + 0xC0u);
    actor->movement_magnitude_raw = word(raw, base + 0x4Cu);
}

static void load_context(NbaGameplayTeamContext *context,
                         const uint8_t *raw, unsigned base) {
    context->mode_raw_30 = word(raw, base + 0x30u);
    context->flags_raw_32 = word(raw, base + 0x32u);
    context->activity_raw_39 = word(raw, base + 0x39u);
    context->dead_ball_actor_raw_3f = word(raw, base + 0x3Fu);
    context->controller_actor_raw_41 = sword(raw, base + 0x41u);
    context->previous_dead_ball_actor_raw_43 = word(raw, base + 0x43u);
    context->previous_controller_actor_raw_45 = sword(raw, base + 0x45u);
    context->match_clock_raw_47 = word(raw, base + 0x47u);
}

static void print_actor(const NbaTipoffActor *actor) {
    printf(" %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x "
           "%04x %04x %04x",
           (uint16_t)actor->velocity_x, (uint16_t)actor->velocity_y,
           (uint16_t)actor->velocity_z, actor->movement_magnitude_raw,
           actor->control_mode, actor->contact_action_timer_raw_60,
           actor->contact_height_raw_aa,
           actor->behavior_timer, actor->movement_boost_timer,
           actor->assignment_current_raw, actor->recovery_inhibit_raw,
           actor->behavior_flags_raw, actor->catcher_latch_raw_ae);
}

static void print_word(uint16_t value) {
    printf("%04x ", value);
}

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3) return 2;
    int continuation = argc == 3 && strcmp(argv[2], "continuation") == 0;
    int wrapper = argc == 3 && strcmp(argv[2], "tip-wrapper") == 0;
    NbaAssetPack assets;
    memset(&assets, 0, sizeof(assets));
    if (!nba_assets_load(&assets, argv[1])) return 3;
    uint8_t raw[WRAM_SIZE];
    _setmode(_fileno(stdin), _O_BINARY);
    while (fread(raw, 1u, sizeof(raw), stdin) == sizeof(raw)) {
        uint16_t pointer = word(raw, (continuation || wrapper) ? 0x009Au : 0x0096u);
        if (pointer < ACTOR_BASE ||
            (pointer - ACTOR_BASE) % ACTOR_STRIDE != 0u) return 4;
        unsigned catcher = (pointer - ACTOR_BASE) / ACTOR_STRIDE;
        if (catcher >= NBA_GAMEPLAY_ACTOR_COUNT) return 5;

        NbaSession session;
        NbaTipoff state;
        nba_session_init(&session);
        if (!nba_tipoff_init(&state, &assets, &session)) return 6;
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
            load_actor(&state.actors[i], raw, ACTOR_BASE + i * ACTOR_STRIDE);
        load_context(&state.team_context[0], raw, 0x46EBu);
        load_context(&state.team_context[1], raw, 0x476Bu);
        state.rng.state = word(raw, 0x07F6u);
        state.match_clock_raw_0928 = word(raw, 0x0928u);
        state.rim_raw_092c = word(raw, 0x092Cu);
        state.free_throw_flight_timer_raw_0930 = word(raw, 0x0930u);
        state.live_state_raw = word(raw, 0x0936u);
        state.camera_side_group_raw = (uint8_t)word(raw, 0x093Au);
        state.possession_actor = (int8_t)sword(raw, 0x093Eu);
        state.possession_team = state.possession_actor >= 0 ?
            (int8_t)(state.possession_actor / 5) : -1;
        state.offense_side = state.camera_side_group_raw != 0u ? 1u : 0u;
        state.pass_actor_raw = sword(raw, 0x0942u);
        state.pass_aux_raw = sword(raw, 0x0944u);
        state.pass_receiver_raw = sword(raw, 0x0946u);
        state.ball_activity_raw = word(raw, 0x0948u);
        state.shot_value_raw = word(raw, 0x094Cu);
        state.inbound_state_raw = word(raw, 0x0952u);
        state.inbound_actor_raw = word(raw, 0x0954u);
        state.rim_raw_0962 = word(raw, 0x0962u);
        state.dead_ball_raw_0968 = word(raw, 0x0968u);
        state.rim_raw_096a = word(raw, 0x096Au);
        state.dead_ball_raw_096c = word(raw, 0x096Cu);
        state.rim_raw_097c = word(raw, 0x097Cu);
        state.dead_ball_raw_097e = word(raw, 0x097Eu);
        state.free_throw_resolution_raw_0972 = word(raw, 0x0972u);
        state.play_request_raw = word(raw, 0x0994u);
        state.play_code = word(raw, 0x0996u);
        state.special_actor_raw = word(raw, 0x09A2u);
        state.play_aux_selector_raw_09a6 = sword(raw, 0x09A6u);
        state.play_selector_raw[0] = sword(raw, 0x09AAu);
        state.play_selector_raw[1] = sword(raw, 0x09ACu);
        state.play_selector_raw[2] = sword(raw, 0x09AEu);
        state.inbound_transfer_raw = word(raw, 0x09B8u);
        state.inbound_ready_raw = word(raw, 0x09BAu);
        state.pass_active_raw = word(raw, 0x09C4u);
        state.shot_actor_raw_09c8 = sword(raw, 0x09C8u);
        state.role_rebuild_raw_09d6 = word(raw, 0x09D6u);
        state.pass_distance_raw = word(raw, 0x09DAu);
        state.inbound_timer_raw = word(raw, 0x092Eu);
        state.rim_raw_13e7 = word(raw, 0x13E7u);
        state.tip_event_bits_raw_13e9 = word(raw, 0x13E9u);
        state.fouls.whistle_active_raw_09b6 = word(raw, 0x09B6u);
        state.rim_force_raw_1866 = word(raw, 0x1866u);
        state.catch_actor_record_raw_0910 = word(raw, 0x0910u);
        state.catch_context_record_raw_0912 = word(raw, 0x0912u);
        state.ball.x_fp = fixed_position(raw, 0x3EEBu, 2u, 4u);
        state.ball.y_fp = fixed_position(raw, 0x3EEBu, 6u, 8u);
        state.ball.z_fp = fixed_position(raw, 0x3EEBu, 0x0Au, 0x0Cu);
        state.ball.velocity_x = sword(raw, 0x3EF9u);
        state.ball.velocity_y = sword(raw, 0x3EFBu);
        state.ball.velocity_z = sword(raw, 0x3EFDu);
        state.ball.owner_actor = state.possession_actor;
        state.handler_actor = (uint8_t)catcher;
        state.cpu_vs_cpu = true;

        if (wrapper) {
            state.tip_contact_actor = (int8_t)catcher;
            if (!nba_tipoff_select_tip_receiver(&state)) return 7;
        } else if (continuation)
            nba_tipoff_replay_ball_acquisition(&state, (uint8_t)catcher);
        else
            nba_tipoff_replay_ball_acquisition_core(&state, (uint8_t)catcher);
        const uint16_t globals[] = {
            state.rng.state, state.live_state_raw,
            (uint16_t)state.possession_actor, state.camera_side_group_raw,
            (uint16_t)state.pass_actor_raw, (uint16_t)state.pass_aux_raw,
            (uint16_t)state.pass_receiver_raw, state.pass_active_raw,
            state.pass_distance_raw, state.rim_raw_092c,
            state.free_throw_flight_timer_raw_0930,
            state.role_rebuild_raw_09d6, state.play_request_raw,
            state.inbound_state_raw, state.inbound_actor_raw,
            state.inbound_timer_raw, state.inbound_ready_raw,
            state.inbound_transfer_raw, state.ball_activity_raw,
            state.shot_value_raw, (uint16_t)state.shot_actor_raw_09c8,
            state.rim_raw_0962, state.rim_raw_096a,
            state.rim_raw_097c, state.rim_raw_13e7,
            (uint16_t)(state.ball.x_fp >> 8),
            (uint16_t)(state.ball.y_fp >> 8),
            (uint16_t)(state.ball.z_fp >> 8),
            (uint16_t)state.ball.velocity_x,
            (uint16_t)state.ball.velocity_y,
            (uint16_t)state.ball.velocity_z,
            state.team_context[0].dead_ball_actor_raw_3f,
            (uint16_t)state.team_context[0].controller_actor_raw_41,
            state.team_context[0].previous_dead_ball_actor_raw_43,
            (uint16_t)state.team_context[0].previous_controller_actor_raw_45,
            state.team_context[1].dead_ball_actor_raw_3f,
            (uint16_t)state.team_context[1].controller_actor_raw_41,
            state.team_context[1].previous_dead_ball_actor_raw_43,
            (uint16_t)state.team_context[1].previous_controller_actor_raw_45
        };
        for (unsigned i = 0; i < sizeof(globals) / sizeof(globals[0]); ++i)
            print_word(globals[i]);
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
            print_actor(&state.actors[i]);
        if (wrapper) {
            printf(" %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x",
                state.tip_winner_group_raw_0932,state.tip_event_bits_raw_13e9,
                state.tip_event.timer_140f,state.tip_event.active_148f,
                state.tip_event.enabled_14a7,state.tip_event.duration_1477,
                state.tip_event.address_14bf,state.tip_event.kind_1430,state.tip_event.bank_1448,
                (unsigned)(state.ball.z_fp & 255u),state.rim_raw_094a);
            for (unsigned i=0;i<10;++i)
                printf(" %04x %04x %04x %04x",state.actors[i].reaction_threshold,
                    state.actors[i].contact_inhibit_raw_5a,
                    (uint16_t)state.actors[i].pass_family_raw,state.actors[i].pass_band_raw);
        }
        putchar('\n');
    }
    nba_assets_free(&assets);
    return ferror(stdin) ? 1 : 0;
}
