/* Replays represented `$85:AF5C-$B128` offense normalization through C. */
#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nba_tipoff.h"

#define WRAM_SIZE 0x4B00u
#define ACTOR_BASE 0x34EBu

static uint16_t word(const uint8_t *raw, unsigned address) {
    return (uint16_t)(raw[address] | ((uint16_t)raw[address + 1u] << 8));
}
static int32_t fixed_position(const uint8_t *raw, unsigned address) {
    return (int32_t)(int16_t)word(raw, address) * 256 +
           (uint8_t)(word(raw, address + 2u) >> 8);
}

int main(void) {
    uint8_t raw[WRAM_SIZE];
    _setmode(_fileno(stdin), _O_BINARY);
    while (fread(raw, 1u, sizeof(raw), stdin) == sizeof(raw)) {
        NbaSession session;
        NbaTipoff state;
        memset(&session, 0, sizeof(session));
        memset(&state, 0, sizeof(state));
        state.session = &session;
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
            unsigned base = ACTOR_BASE + i * 0x100u;
            state.actors[i].x_fp = fixed_position(raw, base + 4u);
            state.actors[i].y_fp = fixed_position(raw, base + 8u);
            state.actors[i].control_mode = (uint8_t)word(raw, base + 0x5Eu);
            state.actors[i].anchor_direction_raw =
                (uint8_t)word(raw, base + 0x88u);
            state.actors[i].anchor_distance_raw = word(raw, base + 0x8Cu);
        }
        state.team_context[0].anchor_x_raw_0a =
            (int16_t)word(raw, 0x46F5u);
        state.team_context[1].anchor_x_raw_0a =
            (int16_t)word(raw, 0x4775u);
        state.live_state_raw = word(raw, 0x0936u);
        state.camera_side_group_raw = (uint8_t)word(raw, 0x093Au);
        state.possession_actor = (int8_t)(int16_t)word(raw, 0x093Eu);
        state.pass_receiver_raw = (int16_t)word(raw, 0x0946u);
        state.inbound_state_raw = word(raw, 0x0952u);
        state.role_focal_x_raw_0918 = (int16_t)word(raw, 0x0918u);
        state.role_focal_y_raw_091a = (int16_t)word(raw, 0x091Au);
        state.ball.x_fp = fixed_position(raw, 0x3EEFu);
        state.ball.y_fp = fixed_position(raw, 0x3EF3u);
        state.ball.velocity_x = (int16_t)word(raw, 0x3EFBu);
        state.ball.velocity_y = (int16_t)word(raw, 0x3EFDu);
        nba_tipoff_refresh_offense_roles_end_frame(&state);
        unsigned offense = (state.live_state_raw == 0x82u ?
            state.inbound_state_raw : state.camera_side_group_raw) != 0u;
        printf("%04x %04x %04x %04x %04x",
               (uint16_t)state.role_focal_x_raw_0918,
               (uint16_t)state.role_focal_y_raw_091a,
               state.role_near_orientation_raw_09d4,
               state.role_ownerless_raw_09d8,
               state.role_nearest_offense_raw_09de);
        for (unsigned i = 0; i < 5u; ++i) {
            const NbaTipoffActor *actor = &state.actors[offense * 5u + i];
            printf(" %04x %04x %04x", actor->control_mode,
                   actor->anchor_direction_raw, actor->anchor_distance_raw);
        }
        putchar('\n');
    }
    return ferror(stdin) ? 1 : 0;
}
