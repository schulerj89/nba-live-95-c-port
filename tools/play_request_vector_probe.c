#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nba_tipoff.h"

#define SIZE 0x4B00u
static uint16_t word(const uint8_t *raw, unsigned at) {
    return (uint16_t)(raw[at] | (uint16_t)raw[at + 1u] << 8);
}

int main(int argc, char **argv) {
    NbaAssetPack assets = {0};
    uint8_t raw[SIZE];
    if (argc != 2 || !nba_assets_load(&assets, argv[1])) return 2;
    _setmode(_fileno(stdin), _O_BINARY);
    while (fread(raw, 1u, SIZE, stdin) == SIZE) {
        NbaSession session;
        NbaTipoff state;
        memset(&session, 0, sizeof(session));
        memset(&state, 0, sizeof(state));
        state.assets = &assets;
        state.session = &session;
        session.left_team = (uint8_t)word(raw, 0x46EBu);
        session.right_team = (uint8_t)word(raw, 0x476Bu);
        session.score[0] = word(raw, 0x4711u);
        session.score[1] = word(raw, 0x4791u);
        /* DP $9E is the active team context at B120. $093A is a persistent
         * free-ball/camera proxy and can legitimately name the other side. */
        state.offense_side = word(raw, 0x009Eu) == 0x476Bu;
        state.live_state_raw = word(raw, 0x0936u);
        state.period_raw_0926 = word(raw, 0x0926u);
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
        for (unsigned side = 0; side < 2u; ++side) {
            unsigned base = side ? 0x476Bu : 0x46EBu;
            state.team_context[side].strategy_team_raw_00 = word(raw, base);
            state.team_context[side].score_raw_26 = word(raw, base + 0x26u);
            state.team_context[side].strategy_raw_2e = word(raw, base + 0x2Eu);
            state.team_context[side].mode_raw_30 = word(raw, base + 0x30u);
            state.team_context[side].activity_raw_39 = raw[base + 0x39u];
            state.team_context[side].play_selection_raw_56 = word(raw, base + 0x56u);
        }
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
            state.actors[i].behavior_flags_raw = word(raw, 0x3569u + i * 0x100u);
        nba_tipoff_update_play_control_end_frame(&state);
        printf("%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x "
               "%04x %04x %04x %04x %04x %04x",
               state.play_request_raw, state.play_code,
               (uint16_t)state.play_step_raw, (uint16_t)state.play_countdown_raw,
               state.play_mirror_raw, state.play_event_wait_raw,
               state.special_actor_raw, state.play_cycle_raw,
               (uint16_t)state.play_selector_raw[0],
               (uint16_t)state.play_selector_raw[1],
               (uint16_t)state.play_selector_raw[2], state.play_hold_raw,
               state.rng.state, state.live_state_raw,
               state.team_context[0].mode_raw_30,
               state.team_context[1].mode_raw_30);
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
            printf(" %04x", state.actors[i].behavior_flags_raw);
        putchar('\n');
    }
    nba_assets_free(&assets);
    return ferror(stdin) ? 1 : 0;
}
