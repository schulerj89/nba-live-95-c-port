#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nba_tipoff.h"

#define SIZE 0x4B00u
#define ACTOR_BASE 0x34EBu

static uint16_t word(const uint8_t *raw, unsigned at) {
    return (uint16_t)(raw[at] | (uint16_t)raw[at + 1u] << 8);
}
static int32_t fixed(const uint8_t *raw, unsigned integer_at) {
    return (int32_t)(int16_t)word(raw, integer_at) * 256 +
           (word(raw, integer_at - 2u) >> 8);
}
static void load_actor(NbaTipoffActor *a, const uint8_t *raw, unsigned base,
                       uint8_t roster) {
    a->roster_slot = roster;
    a->x_fp = fixed(raw, base + 4u); a->y_fp = fixed(raw, base + 8u);
    a->z_fp = fixed(raw, base + 0x0Cu);
    a->velocity_x = (int16_t)word(raw, base + 0x0Eu);
    a->velocity_y = (int16_t)word(raw, base + 0x10u);
    a->velocity_z = (int16_t)word(raw, base + 0x12u);
    a->controller_assignment_raw = (int8_t)(int16_t)word(raw, base + 0x16u);
    a->actor_status_raw_28 = word(raw, base + 0x28u);
    a->upper_animation_resource_raw_2a = word(raw, base + 0x2Au);
    a->lower_animation_resource_raw_2c = word(raw, base + 0x2Cu);
    a->animation_resources_valid = true;
    a->animation_state = (uint8_t)word(raw, base + 0x30u);
    a->lower_animation_state = (uint8_t)word(raw, base + 0x32u);
    a->base_animation_state_raw_38 = (uint8_t)word(raw, base + 0x38u);
    a->rom_upper_animation_phase_raw_3a = word(raw, base + 0x3Au);
    a->rom_lower_animation_phase_raw_3c = word(raw, base + 0x3Cu);
    a->upper_animation_accumulator_raw_42 = word(raw, base + 0x42u);
    a->lower_animation_accumulator_raw_44 = word(raw, base + 0x44u);
    a->upper_animation_lock_raw_46 = word(raw, base + 0x46u);
    a->lower_animation_lock_raw_48 = word(raw, base + 0x48u);
    a->movement_magnitude_raw = word(raw, base + 0x4Cu);
    a->direction = (uint8_t)word(raw, base + 0x4Eu);
    a->requested_direction = (uint8_t)word(raw, base + 0x50u);
    a->movement_direction = (uint8_t)word(raw, base + 0x52u);
    a->target_x = (int16_t)word(raw, base + 0x56u);
    a->target_y = (int16_t)word(raw, base + 0x58u);
    a->contact_inhibit_raw_5a = word(raw, base + 0x5Au);
    a->formation_timer_raw_5c = word(raw, base + 0x5Cu);
    a->control_mode = (uint8_t)word(raw, base + 0x5Eu);
    a->contact_action_timer_raw_60 = word(raw, base + 0x60u);
    a->behavior_timer = word(raw, base + 0x64u);
    a->pass_direction_raw = word(raw, base + 0x66u);
    a->animation_variant_raw_6c = word(raw, base + 0x6Cu);
    a->team_group_raw_6e = word(raw, base + 0x6Eu);
    a->movement_boost_timer = word(raw, base + 0x72u);
    a->assignment_distance = word(raw, base + 0x8Au);
    a->anchor_distance_raw = word(raw, base + 0x8Cu);
    a->anchor_direction_raw = (uint8_t)word(raw, base + 0x88u);
    a->behavior_flags_raw = word(raw, base + 0x7Eu);
    a->free_throw_launch_half_raw_a8 = word(raw, base + 0xA8u);
}

int main(int argc, char **argv) {
    NbaAssetPack assets = {0}; static uint8_t raw[SIZE];
    if (argc != 2 || !nba_assets_load(&assets, argv[1])) return 2;
    _setmode(_fileno(stdin), _O_BINARY);
    while (fread(raw, 1u, SIZE, stdin) == SIZE) {
        NbaSession session; NbaTipoff state;
        nba_session_init(&session);
        session.left_team = (uint8_t)word(raw, 0x46EBu);
        session.right_team = (uint8_t)word(raw, 0x476Bu);
        session.config.main_values[2] = (uint8_t)word(raw, 0x17AFu);
        session.config.rules[8] = (uint8_t)word(raw, 0x17E1u);
        if (!nba_tipoff_init(&state, &assets, &session)) return 3;
        for (unsigned i = 0; i < 10u; ++i)
            load_actor(&state.actors[i], raw, ACTOR_BASE + i * 0x100u,
                (uint8_t)word(raw, (i < 5u ? 0x46F9u : 0x4779u) +
                                   (i % 5u) * 2u));
        state.rng.state = word(raw, 0x07F6u);
        state.match_clock_raw_0928 = word(raw, 0x0928u);
        state.rim_raw_092c = word(raw, 0x092Cu);
        state.live_state_raw = word(raw, 0x0936u);
        state.camera_side_group_raw = (uint8_t)word(raw, 0x093Au);
        state.possession_actor = (int8_t)(int16_t)word(raw, 0x093Eu);
        state.ball_activity_raw = word(raw, 0x0948u);
        state.shot_value_raw = word(raw, 0x094Cu);
        state.inbound_actor_raw = word(raw, 0x0954u);
        state.inbound_target_x_raw = (int16_t)word(raw, 0x0958u);
        state.inbound_target_y_raw = (int16_t)word(raw, 0x095Au);
        state.dead_ball_raw_0968 = word(raw, 0x0968u);
        state.rim_raw_097c = word(raw, 0x097Cu);
        state.play_code = word(raw, 0x0996u);
        state.play_step_raw = (int16_t)word(raw, 0x0998u);
        state.play_mirror_raw = word(raw, 0x099Cu);
        state.special_actor_raw = word(raw, 0x09A2u);
        state.play_cycle_raw = word(raw, 0x09A4u);
        state.play_hold_raw = word(raw, 0x09D0u);
        state.shot_actor_raw_09c8 = (int16_t)word(raw, 0x09C8u);
        state.formation_override_raw_005c = word(raw, 0x005Cu);
        for (unsigned side = 0; side < 2u; ++side) {
            unsigned context = side ? 0x476Bu : 0x46EBu;
            state.team_context[side].anchor_x_raw_0a =
                (int16_t)word(raw, context + 0x0Au);
            state.mode11_context_raw_3b[side] = word(raw, context + 0x3Bu);
        }
        for (unsigned control = 0; control < 5u; ++control) {
            unsigned record = 0x47EBu + control * 0x40u;
            state.mode11_control_group_raw[control] =
                (int16_t)word(raw, record);
            state.mode11_control_flags_raw[control] = word(raw, record + 8u);
        }
        state.offense_side = word(raw, 0x009Eu) == 0x476Bu;
        unsigned slot = word(raw, 0x00C2u);
        uint8_t outcome = nba_tipoff_replay_mode11_dispatch(
            &state, (uint8_t)slot);
        printf("%04x %04x %04x %04x %04x", outcome, state.rng.state,
               state.live_state_raw, state.shot_value_raw,
               (uint16_t)state.shot_actor_raw_09c8);
        for (unsigned i = 0; i < 10u; ++i) {
            NbaTipoffActor *a = &state.actors[i];
            printf(" %04x %04x %04x %04x %04x %04x %04x %04x %04x",
                   (uint16_t)a->target_x, (uint16_t)a->target_y,
                   a->behavior_flags_raw, (uint16_t)a->velocity_x,
                   (uint16_t)a->velocity_y, (uint16_t)a->velocity_z,
                   a->control_mode, a->animation_state,
                   a->lower_animation_state);
        }
        putchar('\n');
    }
    nba_assets_free(&assets);
    return ferror(stdin) ? 1 : 0;
}
