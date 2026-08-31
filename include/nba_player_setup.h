#ifndef NBA_PLAYER_SETUP_H
#define NBA_PLAYER_SETUP_H

#include "nba_assets.h"
#include "nba_renderer.h"
#include "nba_session.h"
#include "nba_team_select.h"

/* Live/Ghidra addresses for Team Select confirmation and Player Setup. */
#define SNES_ADDR_TEAM_SELECT_CONFIRM       0x828553
#define SNES_ADDR_PLAYER_SETUP_TRANSITION   0x81C41E
#define SNES_ADDR_MENU_TRANSITION_SCRIPT    0x80E95B
#define SNES_ADDR_PLAYER_SETUP_POSITION     0x81A7D0
#define SNES_ADDR_PLAYER_SETUP_PALETTE      0x81B546
#define SNES_ADDR_PLAYER_SETUP_REDRAW       0x81B62C
#define SNES_ADDR_PLAYER_SETUP_OBJECT_BUILD 0x81B719

/* Start at Team Select-relative frame 1650 and settled evidence at 1850. */
#define NBA_PLAYER_SETUP_TRANSITION_FRAMES 200

typedef enum {
    NBA_PLAYER_SETUP_SOUND_NONE = 0,
    NBA_PLAYER_SETUP_SOUND_MOVE,
    NBA_PLAYER_SETUP_SOUND_CONFIRM
} NbaPlayerSetupSound;

typedef struct {
    const NbaAssetPack *assets;
    NbaSession *session;
    uint32_t *outgoing_pixels;
    uint8_t *scene_vram;
    NbaTeamSide player_one_side;
    uint16_t controller_selection; /* native pad0 $166D: 0 left/1 neutral/2 right */
    int transition_frame;
    int steady_frame;
    bool confirm_requested;
    bool is_initialized;
} NbaPlayerSetup;

bool nba_player_setup_init(NbaPlayerSetup *screen, const NbaAssetPack *assets,
                           NbaSession *session, const uint32_t *outgoing_pixels);
void nba_player_setup_shutdown(NbaPlayerSetup *screen);
NbaPlayerSetupSound nba_player_setup_update(NbaPlayerSetup *screen,
                                             const NbaInput *input);
void nba_player_setup_render(const NbaPlayerSetup *screen, NbaRenderer *renderer);

#endif
