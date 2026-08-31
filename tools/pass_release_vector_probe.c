/* Replays `$86:A6B3-$A79F` through the production mode-15 executor. */
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

static int32_t fixed_position(const uint8_t *raw, unsigned address) {
    return (int32_t)(int16_t)word(raw, address) * 256 +
           (uint8_t)(word(raw, address + 2u) >> 8);
}

static void load_actor(NbaTipoffActor *actor, const uint8_t *raw,
                       unsigned base, const NbaAssetPack *assets) {
    actor->x_fp = fixed_position(raw, base + 4u);
    actor->y_fp = fixed_position(raw, base + 8u);
    actor->z_fp = (int32_t)(int16_t)word(raw, base + 0x0Cu) * 256;
    actor->velocity_x = (int16_t)word(raw, base + 0x0Eu);
    actor->velocity_y = (int16_t)word(raw, base + 0x10u);
    actor->velocity_z = (int16_t)word(raw, base + 0x12u);
    actor->controller_assignment_raw = (int8_t)(int16_t)word(raw, base + 0x16u);
    actor->actor_status_raw_28 = word(raw, base + 0x28u);
    actor->animation_state = (uint8_t)word(raw, base + 0x30u);
    actor->lower_animation_state = (uint8_t)word(raw, base + 0x32u);
    actor->upper_animation_phase_raw = word(raw, base + 0x3Au);
    actor->direction = (uint8_t)word(raw, base + 0x4Eu);
    actor->control_mode = (uint8_t)word(raw, base + 0x5Eu);
    actor->reaction_threshold = word(raw, base + 0x60u);
    actor->pass_band_raw = word(raw, base + 0x62u);
    actor->behavior_timer = word(raw, base + 0x64u);
    actor->pass_direction_raw = word(raw, base + 0x66u);
    actor->behavior_flags_raw = word(raw, base + 0x7Eu);
    actor->pass_family_raw = (int16_t)word(raw, base + 0xC0u);
    actor->pass_released_raw = false;
    actor->pass_release_threshold_raw = 0u;
    (void)nba_assets_gameplay_pass_release_threshold(
        assets, actor->animation_state, &actor->pass_release_threshold_raw);
}

static void print_actor(const NbaTipoffActor *actor) {
    printf(" %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x",
           actor->control_mode, actor->reaction_threshold,
           actor->behavior_timer, actor->actor_status_raw_28,
           actor->behavior_flags_raw, (uint16_t)actor->pass_family_raw,
           actor->upper_animation_phase_raw, actor->animation_state,
           actor->lower_animation_state, (uint16_t)actor->velocity_x,
           (uint16_t)actor->velocity_y, (uint16_t)actor->velocity_z);
}

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    NbaAssetPack assets = {0};
    if (!nba_assets_load(&assets, argv[1])) return 3;
    uint8_t raw[WRAM_SIZE];
    _setmode(_fileno(stdin), _O_BINARY);
    while (fread(raw, 1u, sizeof(raw), stdin) == sizeof(raw)) {
        NbaSession session;
        NbaTipoff state;
        nba_session_init(&session);
        /* Native home/context0 -> UI right; visitor/context1 -> UI left. */
        session.right_team = (uint8_t)word(raw, 0x46EBu);
        session.left_team = (uint8_t)word(raw, 0x476Bu);
        if (!nba_tipoff_init(&state, &assets, &session)) return 4;
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
            load_actor(&state.actors[i], raw,
                       ACTOR_BASE + i * ACTOR_STRIDE, &assets);
        unsigned slot = (word(raw, 0x0096u) - ACTOR_BASE) / ACTOR_STRIDE;
        state.live_state_raw = word(raw, 0x0936u);
        state.possession_actor = (int8_t)(int16_t)word(raw, 0x093Eu);
        state.handler_actor = state.possession_actor >= 0 ?
            (uint8_t)state.possession_actor : NBA_GAMEPLAY_NO_ACTOR;
        state.pass_actor_raw = (int16_t)word(raw, 0x0942u);
        state.pass_aux_raw = (int16_t)word(raw, 0x0944u);
        state.pass_receiver_raw = (int16_t)word(raw, 0x0946u);
        state.receiver_actor = state.pass_receiver_raw >= 0 ?
            (uint8_t)state.pass_receiver_raw : NBA_GAMEPLAY_NO_ACTOR;
        state.pass_active_raw = word(raw, 0x09C4u);
        state.inbound_transfer_raw = word(raw, 0x09B8u);
        state.ball.x_fp = fixed_position(raw, 0x3EEFu);
        state.ball.y_fp = fixed_position(raw, 0x3EF3u);
        state.ball.z_fp = fixed_position(raw, 0x3EF7u);
        state.ball.velocity_x = (int16_t)word(raw, 0x3EFBu);
        state.ball.velocity_y = (int16_t)word(raw, 0x3EFDu);
        state.ball.velocity_z = (int16_t)word(raw, 0x3EFFu);
        state.ball.owner_actor = state.possession_actor;
        state.ball.state = NBA_BALL_ATTACHED;
        state.cpu_vs_cpu = true;
        bool handled = slot < NBA_GAMEPLAY_ACTOR_COUNT &&
            nba_tipoff_update_rom_passer(&state, slot);
        printf("%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x",
               handled ? 1u : 0u, state.live_state_raw,
               (uint16_t)state.pass_actor_raw, (uint16_t)state.pass_aux_raw,
               (uint16_t)state.pass_receiver_raw, state.pass_active_raw,
               state.inbound_transfer_raw,
               (uint16_t)(state.ball.x_fp / 256),
               (uint16_t)(state.ball.y_fp / 256),
               (uint16_t)(state.ball.z_fp / 256),
               (uint16_t)state.ball.velocity_x,
               (uint16_t)state.ball.velocity_y,
               (uint16_t)state.ball.velocity_z,
               (uint16_t)state.ball.owner_actor, state.ball.state,
               (uint16_t)state.possession_actor);
        if (slot < NBA_GAMEPLAY_ACTOR_COUNT) print_actor(&state.actors[slot]);
        else { NbaTipoffActor empty = {0}; print_actor(&empty); }
        putchar('\n');
    }
    nba_assets_free(&assets);
    return ferror(stdin) ? 1 : 0;
}
