#ifndef NBA_PLAYER_INTRO_H
#define NBA_PLAYER_INTRO_H

#include "nba_assets.h"
#include "nba_renderer.h"
#include "nba_session.h"

/* Live ROM paths proven by the targeted Mesen trace and Ghidra listings. */
#define SNES_ADDR_INTRO_PRESENTATION_LOOP 0x87BE92
#define SNES_ADDR_INTRO_PORTRAIT_PREPARE  0x87BD7F
#define SNES_ADDR_INTRO_GRAPHICS_DISPATCH 0x80C62B
#define SNES_ADDR_INTRO_GRAPHICS_EXPAND   0x80BD1B
#define SNES_ADDR_INTRO_PALETTE_COPY      0x81A1E7

#define NBA_PLAYER_INTRO_TRANSITION_FRAMES 180
#define NBA_PLAYER_INTRO_MATCHUP_FRAMES    300
#define NBA_PLAYER_INTRO_RATINGS_FRAMES    300
/* Measured between roster changes: 2580->3015, 3015->3450, etc. */
#define NBA_PLAYER_INTRO_CARD_FRAMES       434
#define NBA_PLAYER_INTRO_STARTERS_PER_TEAM 5
#define NBA_PLAYER_INTRO_CARD_COUNT         10

typedef enum {
    NBA_PLAYER_INTRO_TRANSITION = 0,
    NBA_PLAYER_INTRO_MATCHUP,
    NBA_PLAYER_INTRO_RATINGS,
    NBA_PLAYER_INTRO_LINEUPS,
    NBA_PLAYER_INTRO_COMPLETE
} NbaPlayerIntroPhase;

typedef struct {
    const NbaAssetPack *assets;
    NbaSession *session;
    uint32_t *outgoing_pixels;
    NbaPlayerIntroPhase phase;
    int phase_frame;
    int lineup_card;
    bool is_initialized;
} NbaPlayerIntro;

bool nba_player_intro_init(NbaPlayerIntro *screen, const NbaAssetPack *assets,
                           NbaSession *session, const uint32_t *outgoing_pixels);
void nba_player_intro_shutdown(NbaPlayerIntro *screen);
void nba_player_intro_update(NbaPlayerIntro *screen, const NbaInput *input);
void nba_player_intro_render(const NbaPlayerIntro *screen, NbaRenderer *renderer);

#endif
