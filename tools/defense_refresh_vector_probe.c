/* Replays the represented `$85:BC07-$C0F5` defense refresh through the
 * production C routine. Input is one raw $0000-$4AFF WRAM image per call. */
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
    memset(actor, 0, sizeof(*actor));
    actor->x_fp = fixed_position(raw, base + 4u);
    actor->y_fp = fixed_position(raw, base + 8u);
    actor->control_mode = (uint8_t)word(raw, base + 0x5Eu);
    actor->saved_control_mode = (uint8_t)word(raw, base + 0x84u);
    actor->behavior_timer = word(raw, base + 0x64u);
    actor->movement_boost_timer = word(raw, base + 0x72u);
    actor->assignment_current_raw = word(raw, base + 0x74u);
    actor->assignment_base_raw = word(raw, base + 0x76u);
    actor->assignment_alternate_raw = word(raw, base + 0x78u);
    actor->team_group_raw_6e = word(raw, base + 0x6Eu);
    actor->help_request_raw_80 = word(raw, base + 0x80u);
    actor->assignment_direction = (uint8_t)word(raw, base + 0x86u);
    actor->anchor_direction_raw = (uint8_t)word(raw, base + 0x88u);
    actor->assignment_distance = word(raw, base + 0x8Au);
    actor->pair_distance = actor->assignment_distance;
    actor->anchor_distance_raw = word(raw, base + 0x8Cu);
    actor->focal_distance_raw_8e = word(raw, base + 0x8Eu);
    actor->assignment_role_raw_92 = (uint8_t)word(raw, base + 0x92u);
}

static void load_context(NbaGameplayTeamContext *context, const uint8_t *raw,
                         unsigned base) {
    memset(context, 0, sizeof(*context));
    context->anchor_x_raw_0a = (int16_t)word(raw, base + 0x0Au);
    context->mode_raw_30 = word(raw, base + 0x30u);
    context->flags_raw_32 = word(raw, base + 0x32u);
    context->help_distance_raw_4e = word(raw, base + 0x4Eu);
    memcpy(context->actor_order_raw_49, raw + base + 0x49u, 5u);
}

static void print_actor(const NbaTipoffActor *actor) {
    printf(" %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x",
           actor->control_mode, actor->saved_control_mode,
           actor->behavior_timer, actor->movement_boost_timer,
           actor->assignment_current_raw, actor->assignment_direction,
           actor->anchor_direction_raw, actor->assignment_distance,
           actor->anchor_distance_raw, actor->focal_distance_raw_8e,
           actor->assignment_role_raw_92);
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
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
            load_actor(&state.actors[i], raw,
                       ACTOR_BASE + i * ACTOR_STRIDE);
        load_context(&state.team_context[0], raw, 0x46EBu);
        load_context(&state.team_context[1], raw, 0x476Bu);
        state.live_state_raw = word(raw, 0x0936u);
        state.camera_side_group_raw = (uint8_t)word(raw, 0x093Au);
        state.possession_actor = (int8_t)(int16_t)word(raw, 0x093Eu);
        state.pass_receiver_raw = (int16_t)word(raw, 0x0946u);
        state.inbound_state_raw = word(raw, 0x0952u);
        state.role_focal_x_raw_0918 = (int16_t)word(raw, 0x0918u);
        state.role_focal_y_raw_091a = (int16_t)word(raw, 0x091Au);
        state.role_rebuild_raw_09d6 = word(raw, 0x09D6u);
        state.role_cadence_raw_09d2 = word(raw, 0x09D2u);
        state.role_ownerless_raw_09d8 = word(raw, 0x09D8u);
        state.pass_distance_raw = word(raw, 0x09DAu);
        state.ball.x_fp = fixed_position(raw, 0x3EEFu);
        state.ball.y_fp = fixed_position(raw, 0x3EF3u);
        state.ball.velocity_x = (int16_t)word(raw, 0x3EFBu);
        state.ball.velocity_y = (int16_t)word(raw, 0x3EFDu);

        nba_tipoff_refresh_defense_roles_end_frame(&state);
        printf("%04x %04x %04x", state.role_rebuild_raw_09d6,
               state.role_ownerless_raw_09d8, state.pass_distance_raw);
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
            print_actor(&state.actors[i]);
        putchar('\n');
    }
    return ferror(stdin) ? 1 : 0;
}
