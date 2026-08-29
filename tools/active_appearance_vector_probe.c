#define _CRT_SECURE_NO_WARNINGS
#include "nba_player_lab.h"
#include <stdio.h>

int main(void) {
    NbaPlayerActiveAppearanceInput input;
    while (1) {
        unsigned value;
        if (scanf("%x", &value) != 1) break;
        input.lineup_selector[0] = (uint8_t)value;
        for (unsigned i = 1; i < 10u; ++i) {
            if (scanf("%x", &value) != 1) return 2;
            input.lineup_selector[i] = (uint8_t)value;
        }
        for (unsigned i = 0; i < 10u; ++i) {
            if (scanf("%x", &value) != 1) return 2;
            input.appearance_a[i] = (uint8_t)value;
        }
        for (unsigned i = 0; i < 10u; ++i) {
            if (scanf("%x", &value) != 1) return 2;
            input.appearance_b[i] = (uint8_t)value;
        }
        for (unsigned i = 0; i < 10u; ++i) {
            if (scanf("%x", &value) != 1) return 2;
            input.upper_variant[i] = (uint8_t)value;
        }
        NbaPlayerActiveAppearance output;
        if (!nba_player_build_active_appearance(&input, &output)) return 3;
        for (unsigned i = 0; i < 10u; ++i)
            printf("%04x%c", output.assignment_base[i], i == 9u ? ' ' : ' ');
        for (unsigned i = 0; i < 10u; ++i)
            printf("%04x ", output.assignment_alternate[i]);
        for (unsigned i = 0; i < 10u; ++i)
            printf("%04x ", output.upper_variant[i]);
        for (unsigned i = 0; i < 10u; ++i)
            printf("%04x ", output.help_request[i]);
        for (unsigned i = 0; i < 5u; ++i)
            printf("%04x %04x%c", output.sorted_key[1][i],
                   output.sorted_actor_offset[1][i], i == 4u ? '\n' : ' ');
    }
    return 0;
}
