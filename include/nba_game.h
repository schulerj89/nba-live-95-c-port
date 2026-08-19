#ifndef NBA_GAME_H
#define NBA_GAME_H

#include "nba_types.h"
#include "nba_rom.h"
#include "nba_renderer.h"

typedef enum {
    NBA_STATE_BOOT_RESET = 0,
    NBA_STATE_NINTENDO_LICENSE,
    NBA_STATE_NBA_LEGAL_NOTICE,
    NBA_STATE_EA_INTRO,
    NBA_STATE_MAIN_MENU
} NbaGameState;

typedef struct {
    NbaRom rom;
    NbaRenderer renderer;
    NbaInput input;
    NbaGameState state;
    uint32_t frame_count;
    float state_timer;
    bool is_initialized;
} NbaGame;

bool nba_game_init(NbaGame *game, const char *rom_path);
void nba_game_shutdown(NbaGame *game);
void nba_game_input_update(NbaInput *input, uint16_t raw_buttons);
void nba_game_tick(NbaGame *game, float delta_time);
void nba_game_render(NbaGame *game);
void nba_game_render_nba_legal_notice(NbaRenderer *ren);
void nba_game_render_ea_intro(NbaRenderer *ren, float timer);

#endif /* NBA_GAME_H */
