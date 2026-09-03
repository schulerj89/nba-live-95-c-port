#define _CRT_SECURE_NO_WARNINGS
#include "nba_court_presentation.h"
#include <stdio.h>

int main(void) {
    int camera_x;
    unsigned scroll_y, right, line;
    while (scanf("%d %u %u %u", &camera_x, &scroll_y, &right, &line) == 4) {
        NbaCourtPresentation state = {0};
        state.window_y_087e = (uint16_t)scroll_y;
        state.window_left_0880 = (uint16_t)right;
        uint8_t edge;
        bool enabled = nba_court_goal_scanline(&state, (int16_t)camera_x, line, &edge);
        printf("%u %u\n", enabled ? 1u : 0u, edge);
    }
    return ferror(stdin) ? 1 : 0;
}
