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

static int32_t position_fp(const uint8_t *raw, unsigned base,
                           unsigned integer_offset) {
    return (int32_t)(int16_t)word(raw, base + integer_offset) * 256 +
           (word(raw, base + integer_offset - 2u) >> 8);
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
        state.possession_actor = (int8_t)(int16_t)word(raw, 0x093Eu);
        state.live_state_raw = word(raw, 0x0936u);
        state.ball_activity_raw = word(raw, 0x0948u);
        state.inbound_actor_raw = word(raw, 0x0954u);
        state.inbound_target_x_raw = (int16_t)word(raw, 0x0958u);
        state.inbound_target_y_raw = (int16_t)word(raw, 0x095Au);
        state.dead_ball_raw_0968 = word(raw, 0x0968u);
        state.rim_raw_097c = word(raw, 0x097Cu);
        state.play_code = word(raw, 0x0996u);
        state.play_step_raw = (int16_t)word(raw, 0x0998u);
        state.play_mirror_raw = word(raw, 0x099Cu);
        state.special_actor_raw = word(raw, 0x09A2u);
        state.formation_override_raw_005c = word(raw, 0x005Cu);
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
            unsigned base = 0x34EBu + i * 0x100u;
            NbaTipoffActor *actor = &state.actors[i];
            actor->x_fp = position_fp(raw, base, 4u);
            actor->y_fp = position_fp(raw, base, 8u);
            actor->z_fp = position_fp(raw, base, 0x0Cu);
            actor->velocity_x = (int16_t)word(raw, base + 0x0Eu);
            actor->velocity_y = (int16_t)word(raw, base + 0x10u);
            actor->controller_assignment_raw =
                (int8_t)(int16_t)word(raw, base + 0x16u);
            actor->target_x = (int16_t)word(raw, base + 0x56u);
            actor->target_y = (int16_t)word(raw, base + 0x58u);
            actor->formation_timer_raw_5c = word(raw, base + 0x5Cu);
            actor->team_group_raw_6e = word(raw, base + 0x6Eu);
            actor->movement_boost_timer = word(raw, base + 0x72u);
            actor->behavior_flags_raw = word(raw, base + 0x7Eu);
            actor->roster_slot = (uint8_t)word(
                raw, (i < 5u ? 0x46F9u : 0x4779u) + (i % 5u) * 2u);
        }
        uint8_t slot = (uint8_t)word(raw, 0x00C2u);
        uint8_t direction = (uint8_t)word(raw, 0x00B2u);
        bool ran = nba_tipoff_replay_formation_route(&state, slot, &direction);
        printf("%04x", ran ? 1u : 0u);
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
            printf(" %04x %04x %04x %04x %04x %04x",
                   (uint16_t)state.actors[i].target_x,
                   (uint16_t)state.actors[i].target_y,
                   state.actors[i].behavior_flags_raw,
                   (uint16_t)state.actors[i].velocity_x,
                   (uint16_t)state.actors[i].velocity_y,
                   state.actors[i].movement_boost_timer);
        putchar('\n');
    }
    nba_assets_free(&assets);
    return ferror(stdin) ? 1 : 0;
}
