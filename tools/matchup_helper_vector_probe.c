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
static int slot(uint16_t pointer) {
    unsigned delta = (unsigned)(pointer - ACTOR_BASE);
    return pointer >= ACTOR_BASE && delta % ACTOR_STRIDE == 0u &&
           delta / ACTOR_STRIDE < 10u ? (int)(delta / ACTOR_STRIDE) : -1;
}
static void load_actor(NbaTipoffActor *actor, const uint8_t *raw,
                       unsigned base) {
    memset(actor, 0, sizeof(*actor));
    actor->x_fp = (int32_t)(int16_t)word(raw, base + 4u) * 256;
    actor->y_fp = (int32_t)(int16_t)word(raw, base + 8u) * 256;
    actor->control_mode = (uint8_t)word(raw, base + 0x5Eu);
    actor->movement_boost_timer = word(raw, base + 0x72u);
    actor->assignment_current_raw = word(raw, base + 0x74u);
    actor->assignment_alternate_raw = word(raw, base + 0x78u);
    actor->anchor_distance_raw = word(raw, base + 0x8Cu);
    actor->focal_distance_raw_8e = word(raw, base + 0x8Eu);
}
static void print_actor(const NbaTipoffActor *actor) {
    printf(" %04x %04x %04x %04x %04x %04x",
        actor->control_mode, actor->movement_boost_timer,
        actor->assignment_current_raw, actor->assignment_alternate_raw,
        actor->anchor_distance_raw, actor->focal_distance_raw_8e);
}
int main(void) {
    uint8_t raw[WRAM_SIZE];
    _setmode(_fileno(stdin), _O_BINARY);
    while (fread(raw, 1u, sizeof(raw), stdin) == sizeof(raw)) {
        NbaTipoff state;
        memset(&state, 0, sizeof(state));
        for (unsigned i = 0; i < 10u; ++i)
            load_actor(&state.actors[i], raw, ACTOR_BASE + i * ACTOR_STRIDE);
        state.team_context[0].anchor_x_raw_0a = (int16_t)word(raw, 0x46F5u);
        state.team_context[1].anchor_x_raw_0a = (int16_t)word(raw, 0x4775u);
        int current = slot(word(raw, 0x96u));
        int related = slot(word(raw, 0x9Au));
        uint16_t context = word(raw, 0x9Eu);
        int first = context <= WRAM_SIZE - 6u ? slot(word(raw, context + 4u)) : -1;
        uint16_t entry = word(raw, 0x00u);
        if (current < 0 || related < 0 || first < 0) return 4;
        uint8_t selected = 0xFFu;
        bool ok = nba_tipoff_replay_matchup_helper(
            &state, entry, (uint8_t)current, (uint8_t)related,
            (uint8_t)(first / 5), &selected);
        printf("%04x %04x", ok ? 1u : 0u, selected);
        for (unsigned i = 0; i < 10u; ++i) print_actor(&state.actors[i]);
        putchar('\n');
    }
    return ferror(stdin) ? 1 : 0;
}
