#include "nba_menu_input.h"

uint16_t nba_menu_input_native_buttons(uint32_t host_buttons) {
    uint16_t native = 0;
    for (unsigned bit = 0; bit < 12u; ++bit)
        if (host_buttons & (1u << bit)) native |= (uint16_t)(0x8000u >> bit);
    return native;
}

/* `$81:AB58-$AC03`, recomp bank81-with-repeat.c, decoded independently in
 * docs/setup-config-native-contract.md. This NMI-owned producer scans five
 * connected type1 controller records backwards and queues whole held words.
 * Release deliberately preserves delay/speed/pending; it clears previous,
 * fast and $1846 only. Changed nonzero words also preserve fast until release.
 * These quirks matter when buttons change without an intervening empty frame. */
void nba_menu_input_produce(NbaMenuInput *state, const uint16_t held[5],
                            const uint8_t controller_type[5]) {
    if (!state || !held || !controller_type) return;
    for (int index = NBA_MENU_CONTROLLER_COUNT - 1; index >= 0; --index) {
        NbaMenuControllerInput *c = &state->controller[index];
        if ((controller_type[index] & 15u) != 1u) continue;
        if (held[index] != 0u) {
            if (held[index] != c->previous) {
                c->previous = c->pending = held[index];
                c->delay = 32u;
                c->speed = 12u;
                continue;
            }
            uint16_t remaining = (uint16_t)(c->delay - 1u);
            if (!(remaining & 0x8000u)) {
                if (remaining != 0u) {
                    c->delay = remaining;
                } else {
                    uint16_t speed = c->speed;
                    if (state->accelerate) {
                        speed = (uint16_t)(speed - 1u);
                        if (speed < 5u) { c->fast = 1u; speed = 5u; }
                    }
                    c->speed = c->delay = speed;
                    c->pending = c->previous;
                }
                continue;
            }
            c->pending = 0u;
            c->delay = 32u;
            c->speed = 12u;
        }
        c->fast = c->previous = c->auxiliary = 0u;
    }
}

/* `$81:AC04-$AC52` returns the first queued complete word, clears only its
 * pending slot and publishes its byte offset. No bit-priority decoding occurs:
 * a simultaneous LEFT+RIGHT is $0300, which the menu dispatcher ignores. */
uint16_t nba_menu_input_consume(NbaMenuInput *state) {
    if (!state) return 0u;
    for (unsigned index = 0; index < NBA_MENU_CONTROLLER_COUNT; ++index) {
        uint16_t pending = state->controller[index].pending;
        if (pending || index == NBA_MENU_CONTROLLER_COUNT - 1u) {
            state->controller[index].pending = 0u;
            state->selected_offset = (uint16_t)(index * 2u);
            return pending;
        }
    }
    return 0u;
}
