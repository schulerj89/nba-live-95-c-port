/* Replays the request-zero `$85:B24C-$B353` play-control cadence. */
#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nba_tipoff.h"

#define WRAM_SIZE 0x4B00u
static uint16_t word(const uint8_t *raw, unsigned address) {
    return (uint16_t)(raw[address] | ((uint16_t)raw[address + 1u] << 8));
}

int main(int argc, char **argv) {
    NbaAssetPack assets;
    uint8_t raw[WRAM_SIZE];
    if (argc != 2 || !nba_assets_load(&assets, argv[1])) return 2;
    _setmode(_fileno(stdin), _O_BINARY);
    while (fread(raw, 1u, sizeof(raw), stdin) == sizeof(raw)) {
        NbaSession session;
        NbaTipoff state;
        memset(&session, 0, sizeof(session));
        memset(&state, 0, sizeof(state));
        state.assets = &assets;
        state.session = &session;
        state.offense_side = word(raw, 0x093Au) != 0u;
        state.live_state_raw = word(raw, 0x0936u);
        state.play_request_raw = word(raw, 0x0994u);
        state.play_code = word(raw, 0x0996u);
        state.play_step_raw = (int16_t)word(raw, 0x0998u);
        state.play_countdown_raw = (int16_t)word(raw, 0x099Au);
        state.play_mirror_raw = word(raw, 0x099Cu);
        state.play_event_wait_raw = word(raw, 0x099Eu);
        state.special_actor_raw = word(raw, 0x09A2u);
        state.play_cycle_raw = word(raw, 0x09A4u);
        state.play_selector_raw[0] = (int16_t)word(raw, 0x09AAu);
        state.play_selector_raw[1] = (int16_t)word(raw, 0x09ACu);
        state.play_selector_raw[2] = (int16_t)word(raw, 0x09AEu);
        state.play_hold_raw = word(raw, 0x09D0u);
        state.rng.state = word(raw, 0x07F6u);
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
            unsigned base = 0x34EBu + i * 0x100u;
            state.actors[i].controller_assignment_raw =
                (int8_t)(int16_t)word(raw, base + 0x16u);
            state.actors[i].behavior_flags_raw =
                word(raw, 0x3569u + i * 0x100u);
        }
        nba_tipoff_update_play_control_end_frame(&state);
        printf("%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x "
               "%04x %04x %04x %04x",
               state.play_request_raw, state.play_code,
               (uint16_t)state.play_step_raw,
               (uint16_t)state.play_countdown_raw, state.play_mirror_raw,
               state.play_event_wait_raw, state.special_actor_raw,
               state.play_cycle_raw, (uint16_t)state.play_selector_raw[0],
               (uint16_t)state.play_selector_raw[1],
               (uint16_t)state.play_selector_raw[2], state.play_hold_raw,
               state.rng.state, state.live_state_raw);
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
            printf(" %04x", state.actors[i].behavior_flags_raw);
        putchar('\n');
    }
    nba_assets_free(&assets);
    return ferror(stdin) ? 1 : 0;
}
