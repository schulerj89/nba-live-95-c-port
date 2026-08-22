#ifndef NBA_TEAM_SELECT_H
#define NBA_TEAM_SELECT_H

#include "nba_assets.h"
#include "nba_renderer.h"
#include "nba_session.h"

#define NBA_REGULAR_TEAM_COUNT 27
#define NBA_TEAM_COUNT 29
#define NBA_TEAM_RANK_COUNT 5
#define NBA_TEAM_TRANSITION_FRAMES 176
#define NBA_TEAM_PLATE_ANIM_START_FRAME 172
#define NBA_TEAM_PLATE_ANIM_PERIOD 56

/* Live/Ghidra addresses for the Exhibition Team Select scene. */
#define SNES_ADDR_TEAM_SELECT_SCENE       0x82809A
#define SNES_ADDR_TEAM_SELECT_FRAME       0x82838E
#define SNES_ADDR_TEAM_SELECT_SIDE_INPUT  0x8283BC
#define SNES_ADDR_TEAM_SELECT_DPAD_INPUT  0x828406
#define SNES_ADDR_TEAM_SELECT_REDRAW      0x8285D1
#define SNES_ADDR_TEAM_SELECT_PLATE_ANIM  0x828933
#define SNES_ADDR_TEAM_SELECT_PLATE_DATA  0x828968
#define SNES_ADDR_TEAM_FRAME_DIVIDER      0x8789D5
#define SNES_WRAM_TEAM_LEFT               0x7E16FB
#define SNES_WRAM_TEAM_RIGHT              0x7E16FD
#define SNES_WRAM_TEAM_ACTIVE_SIDE        0x7E16B5
#define SNES_WRAM_TEAM_SELECTOR           0x7E1693
#define SNES_ROM_TEAM_RANKINGS            0x80D9AF

typedef enum {
    NBA_TEAM_RANK_SCORING = 0,
    NBA_TEAM_RANK_REBOUNDS,
    NBA_TEAM_RANK_BALL_CONTROL,
    NBA_TEAM_RANK_DEFENSE,
    NBA_TEAM_RANK_OVERALL
} NbaTeamRankCategory;

typedef enum {
    NBA_TEAM_SIDE_LEFT = 0,
    NBA_TEAM_SIDE_RIGHT = 1
} NbaTeamSide;

/* $7E:1693: the two team-name rows precede the five ranking rows. */
typedef enum {
    NBA_TEAM_SELECT_LEFT_NAME = 0,
    NBA_TEAM_SELECT_RIGHT_NAME,
    NBA_TEAM_SELECT_SCORING,
    NBA_TEAM_SELECT_REBOUNDS,
    NBA_TEAM_SELECT_BALL_CONTROL,
    NBA_TEAM_SELECT_DEFENSE,
    NBA_TEAM_SELECT_OVERALL,
    NBA_TEAM_SELECT_POSITION_COUNT
} NbaTeamSelectPosition;

typedef enum {
    NBA_TEAM_SOUND_NONE = 0,
    NBA_TEAM_SOUND_SIDE,
    NBA_TEAM_SOUND_CATEGORY,
    NBA_TEAM_SOUND_CHANGE
} NbaTeamSelectSound;

typedef struct {
    const char *name;
    const char *nickname;
    uint8_t rank[NBA_TEAM_RANK_COUNT];
} NbaTeamRecord;

typedef struct {
    const NbaAssetPack *assets;
    NbaSession *session;
    uint32_t *outgoing_pixels;
    NbaTeamSide active_side;
    NbaTeamSelectPosition selector;
    int transition_frame;
    int steady_frame;
    bool is_initialized;
} NbaTeamSelect;

extern const NbaTeamRecord nba_team_records[NBA_TEAM_COUNT];
const NbaTeamRecord *nba_team_record(uint8_t team);
bool nba_team_select_init(NbaTeamSelect *screen, const NbaAssetPack *assets,
                          NbaSession *session, const uint32_t *outgoing_pixels);
void nba_team_select_shutdown(NbaTeamSelect *screen);
NbaTeamSelectSound nba_team_select_update(NbaTeamSelect *screen,
                                           const NbaInput *input);
void nba_team_select_render(const NbaTeamSelect *screen, NbaRenderer *renderer);

#endif
