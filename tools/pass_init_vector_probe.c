/* Replays `$86:AB2D-$AF65` through the production pass initializer. */
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
    int16_t integer = (int16_t)word(raw, address);
    uint8_t fraction = (uint8_t)(word(raw, address + 2u) >> 8);
    return (int32_t)integer * 256 + fraction;
}

static void load_actor(NbaTipoffActor *actor, const uint8_t *raw,
                       unsigned base) {
    actor->x_fp = fixed_position(raw, base + 4u);
    actor->y_fp = fixed_position(raw, base + 8u);
    actor->z_fp = (int32_t)(int16_t)word(raw, base + 0x0Cu) * 256;
    actor->velocity_x = (int16_t)word(raw, base + 0x0Eu);
    actor->velocity_y = (int16_t)word(raw, base + 0x10u);
    actor->velocity_z = (int16_t)word(raw, base + 0x12u);
    actor->control_mode = (uint8_t)word(raw, base + 0x5Eu);
    actor->reaction_threshold = word(raw, base + 0x60u);
    actor->pass_band_raw = word(raw, base + 0x62u);
    actor->pass_direction_raw = word(raw, base + 0x66u);
    actor->movement_boost_timer = word(raw, base + 0x72u);
    actor->behavior_flags_raw = word(raw, base + 0x7Eu);
    actor->movement_direction = (uint8_t)word(raw, base + 0x4Eu);
    actor->direction = (uint8_t)word(raw, base + 0x52u);
    actor->movement_magnitude_raw = word(raw, base + 0x4Cu);
    actor->animation_state = (uint8_t)word(raw, base + 0x30u);
    actor->lower_animation_state = (uint8_t)word(raw, base + 0x32u);
    actor->upper_animation_phase_raw = word(raw, base + 0x3Au);
    actor->pass_family_raw = (int16_t)word(raw, base + 0xC0u);
}

static void print_actor(const NbaTipoffActor *actor) {
    printf(" %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x",
           actor->control_mode, actor->reaction_threshold,
           actor->pass_band_raw, actor->pass_direction_raw,
           (uint16_t)actor->pass_family_raw, actor->movement_boost_timer,
           actor->behavior_flags_raw, (uint16_t)actor->velocity_x,
           (uint16_t)actor->velocity_y, (uint16_t)actor->velocity_z,
           actor->movement_magnitude_raw, actor->animation_state,
           actor->lower_animation_state, actor->upper_animation_phase_raw);
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
        if (!nba_tipoff_init(&state, &assets, &session)) return 4;
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
            load_actor(&state.actors[i], raw,
                       ACTOR_BASE + i * ACTOR_STRIDE);
        state.live_state_raw = word(raw, 0x0936u);
        state.inbound_layout_raw = (int16_t)word(raw, 0x0956u);
        state.inbound_transfer_raw = word(raw, 0x09B8u);
        state.pass_actor_raw = (int16_t)word(raw, 0x0942u);
        state.pass_receiver_raw = (int16_t)word(raw, 0x0946u);
        state.pass_active_raw = word(raw, 0x09C4u);
        state.pass_distance_raw = word(raw, 0x09DAu);
        state.cpu_vs_cpu = true;
        unsigned passer = word(raw, 0x00C2u);
        unsigned receiver = word(raw, 0x00AAu);
        bool accepted = passer < NBA_GAMEPLAY_ACTOR_COUNT &&
                        receiver < NBA_GAMEPLAY_ACTOR_COUNT &&
                        nba_tipoff_begin_rom_pass(&state, passer, receiver);
        printf("%04x %04x %04x %04x %04x %04x %04x",
               accepted ? 1u : 0u, state.live_state_raw,
               (uint16_t)state.pass_actor_raw,
               (uint16_t)state.pass_receiver_raw, state.pass_active_raw,
               state.inbound_transfer_raw, state.pass_distance_raw);
        if (passer < NBA_GAMEPLAY_ACTOR_COUNT) print_actor(&state.actors[passer]);
        else { NbaTipoffActor empty = {0}; print_actor(&empty); }
        if (receiver < NBA_GAMEPLAY_ACTOR_COUNT) print_actor(&state.actors[receiver]);
        else { NbaTipoffActor empty = {0}; print_actor(&empty); }
        putchar('\n');
    }
    nba_assets_free(&assets);
    return ferror(stdin) ? 1 : 0;
}
