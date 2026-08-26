/* Replay player/player work observed inside `$86:D652-$D6EE`. */
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

static int32_t fixed_position(const uint8_t *raw, unsigned fraction,
                              unsigned integer) {
    (void)fraction;
    /* The contact routines read the integer coordinate words directly.
     * Do not let the host's rounded fixed-point accessor promote a captured
     * subpixel value into the next court coordinate. */
    return (int32_t)(int16_t)word(raw, integer) * 256;
}

static void load_actor(NbaTipoffActor *actor, const uint8_t *raw,
                       unsigned base) {
    actor->x_fp = fixed_position(raw, base + 2u, base + 4u);
    actor->y_fp = fixed_position(raw, base + 6u, base + 8u);
    actor->z_fp = fixed_position(raw, base + 0x0Au, base + 0x0Cu);
    actor->velocity_x = (int16_t)word(raw, base + 0x0Eu);
    actor->velocity_y = (int16_t)word(raw, base + 0x10u);
    actor->velocity_z = (int16_t)word(raw, base + 0x12u);
    actor->controller_assignment_raw = (int8_t)(int16_t)word(raw, base + 0x16u);
    actor->actor_status_raw_28 = word(raw, base + 0x28u);
    actor->animation_state = (uint8_t)word(raw, base + 0x30u);
    actor->lower_animation_state = (uint8_t)word(raw, base + 0x32u);
    actor->upper_animation_tick = word(raw, base + 0x34u);
    actor->lower_animation_tick = word(raw, base + 0x36u);
    actor->upper_animation_phase_raw = word(raw, base + 0x3Au);
    actor->movement_magnitude_raw = word(raw, base + 0x4Cu);
    actor->direction = (uint8_t)word(raw, base + 0x4Eu);
    actor->special_contact_raw_56 = (int16_t)word(raw, base + 0x56u);
    actor->contact_inhibit_raw_5a = word(raw, base + 0x5Au);
    actor->control_mode = (uint8_t)word(raw, base + 0x5Eu);
    actor->contact_action_timer_raw_60 = word(raw, base + 0x60u);
    actor->pass_direction_raw = word(raw, base + 0x66u);
    actor->team_group_raw_6e = word(raw, base + 0x6Eu);
    actor->movement_boost_timer = word(raw, base + 0x72u);
    actor->assignment_current_raw = word(raw, base + 0x74u);
    actor->recovery_inhibit_raw = word(raw, base + 0x7Au);
    actor->behavior_flags_raw = word(raw, base + 0x7Eu);
    actor->anchor_distance_raw = word(raw, base + 0x8Cu);
}

static void print_actor(const NbaTipoffActor *actor) {
    printf(" %04x %04x %04x %04x %04x %04x %04x %04x %04x "
           "%04x %04x %04x %04x %04x %04x %04x %04x",
           (uint16_t)actor->velocity_x, (uint16_t)actor->velocity_y,
           (uint16_t)actor->velocity_z, actor->actor_status_raw_28,
           actor->animation_state, actor->lower_animation_state,
           actor->upper_animation_phase_raw,
           (uint16_t)actor->special_contact_raw_56,
           actor->contact_inhibit_raw_5a, actor->control_mode,
           actor->contact_action_timer_raw_60, actor->pass_direction_raw,
           actor->movement_boost_timer, actor->recovery_inhibit_raw,
           actor->behavior_flags_raw, actor->movement_magnitude_raw,
           actor->direction);
}

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    NbaAssetPack assets;
    memset(&assets, 0, sizeof(assets));
    if (!nba_assets_load(&assets, argv[1])) return 3;
    uint8_t raw[WRAM_SIZE];
    _setmode(_fileno(stdin), _O_BINARY);
    while (fread(raw, 1u, sizeof(raw), stdin) == sizeof(raw)) {
        NbaSession session;
        NbaTipoff state;
        nba_session_init(&session);
        session.left_team = (uint8_t)word(raw, 0x46EBu);
        session.right_team = (uint8_t)word(raw, 0x476Bu);
        for (unsigned i = 0; i < 6u; ++i)
            session.config.rules[i] = (uint8_t)word(raw, 0x17D1u + i * 2u);
        if (!nba_tipoff_init(&state, &assets, &session)) return 4;
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
            load_actor(&state.actors[i], raw, ACTOR_BASE + i * ACTOR_STRIDE);
        state.rng.state = word(raw, 0x07F6u);
        state.period_raw_0926 = word(raw, 0x0926u);
        state.live_state_raw = word(raw, 0x0936u);
        state.camera_side_group_raw = (uint8_t)word(raw, 0x093Au);
        state.possession_actor = (int8_t)(int16_t)word(raw, 0x093Eu);
        state.ball.owner_actor = state.possession_actor;
        state.inbound_actor_raw = word(raw, 0x0954u);
        state.ball_activity_raw = word(raw, 0x0948u);
        state.pass_receiver_raw = (int16_t)word(raw, 0x0946u);
        state.shot_actor_raw_09c8 = (int16_t)word(raw, 0x09C8u);
        state.rim_raw_13e7 = word(raw, 0x13E7u);
        state.fouls.foul_event_raw_0964 = word(raw, 0x0964u);
        state.fouls.free_throw_state_raw_0978 = word(raw, 0x0978u);
        state.fouls.free_throw_sequence_raw_097a = word(raw, 0x097Au);
        state.fouls.whistle_active_raw_09b6 = word(raw, 0x09B6u);
        state.fouls.shooting_foul_raw_09bc = word(raw, 0x09BCu);
        state.fouls.team_fouls[0] = word(raw, 0x4713u);
        state.fouls.team_fouls[1] = word(raw, 0x4793u);
        state.team_pose_contact_count_raw[0] = word(raw, 0x473Bu);
        state.team_pose_contact_count_raw[1] = word(raw, 0x47BBu);
        state.cpu_vs_cpu = true;
        uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];
        unsigned order_count = 0u;
        for (unsigned at = 0x34D3u; at + 1u < WRAM_SIZE; at += 2u) {
            uint16_t pointer = word(raw, at);
            if (pointer == 0u) break;
            if (pointer == 0x3EEBu) continue;
            if (pointer < ACTOR_BASE ||
                (pointer - ACTOR_BASE) % ACTOR_STRIDE != 0u) return 5;
            unsigned slot = (pointer - ACTOR_BASE) / ACTOR_STRIDE;
            if (slot >= NBA_GAMEPLAY_ACTOR_COUNT ||
                order_count >= NBA_GAMEPLAY_ACTOR_COUNT) return 6;
            order[order_count++] = (uint8_t)slot;
        }
        nba_tipoff_replay_player_contact_order(&state, order, order_count);
        printf("%04x %04x %04x %04x %04x %04x %04x %04x %04x",
               state.rng.state, state.live_state_raw,
               (uint16_t)state.possession_actor,
               (uint16_t)state.pass_receiver_raw, state.rim_raw_13e7,
               state.fouls.foul_event_raw_0964,
               state.fouls.free_throw_state_raw_0978,
               state.fouls.shooting_foul_raw_09bc,
               state.fouls.whistle_active_raw_09b6);
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
            print_actor(&state.actors[i]);
        putchar('\n');
    }
    nba_assets_free(&assets);
    return ferror(stdin) ? 1 : 0;
}
