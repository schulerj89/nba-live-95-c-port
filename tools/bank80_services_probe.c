#include "nba_game.h"
#include "nba_snes_ppu.h"
#include "nba_spc.h"
#include <stdio.h>
#include <string.h>

static int input_probe(void) {
    NbaInput input;
    memset(&input, 0, sizeof(input));
    nba_game_input_update(&input, NBA_BTN_A | NBA_BTN_LEFT);
    if (input.held != (NBA_BTN_A | NBA_BTN_LEFT) || input.pressed != input.held ||
        input.released != 0u) return 1;
    nba_game_input_update(&input, NBA_BTN_A | NBA_BTN_START);
    if (input.held != (NBA_BTN_A | NBA_BTN_START) ||
        input.pressed != NBA_BTN_START || input.released != NBA_BTN_LEFT) return 2;
    nba_game_input_update(&input, NBA_BTN_A | NBA_BTN_START);
    if (input.pressed != 0u || input.released != 0u) return 3;
    nba_game_input_update(&input, NBA_BTN_DEBUG_F10);
    if (input.pressed != NBA_BTN_DEBUG_F10 ||
        input.released != (NBA_BTN_A | NBA_BTN_START)) return 4;
    nba_game_input_update(&input, 0u);
    return input.held == 0u && input.pressed == 0u &&
           input.released == NBA_BTN_DEBUG_F10 ? 0 : 5;
}

int main(void) {
    int input = input_probe();
    bool ppu = nba_snes_mode1_self_test();
    bool spc = nba_spc_self_test();
    printf("BANK80_SERVICES input=%s ppu=%s spc=%s\n",
           input == 0 ? "PASS" : "FAIL", ppu ? "PASS" : "FAIL",
           spc ? "PASS" : "FAIL");
    return input == 0 && ppu && spc ? 0 : 1;
}
