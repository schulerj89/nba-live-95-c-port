#ifndef NBA_PLAYER_LAB_H
#define NBA_PLAYER_LAB_H

#include "nba_assets.h"
#include "nba_renderer.h"
#include "nba_team_select.h"

#define NBA_PLAYER_ROSTER_SIZE 12
#define NBA_PLAYER_ANIMATION_STATES 57
#define SNES_ROM_TEAM_ROSTER_TABLE 0x84E640
#define SNES_ADDR_PLAYER_POINTERS 0x86D7B8
#define SNES_ADDR_PLAYER_APPEARANCE 0x86D85E
#define SNES_ADDR_PLAYER_SORT 0x86D73E

typedef struct {
    bool is_active;
    uint8_t team;
    uint8_t player;
    uint8_t animation_state;
    uint8_t direction;
    uint32_t animation_tick;
    bool animation_paused;
    bool has_assets;
} NbaPlayerLab;

void nba_player_lab_init(NbaPlayerLab *lab, const NbaAssetPack *assets);
void nba_player_lab_toggle(NbaPlayerLab *lab, const NbaAssetPack *assets);
void nba_player_lab_update(NbaPlayerLab *lab, const NbaAssetPack *assets,
                           const NbaInput *input);
void nba_player_lab_render(const NbaPlayerLab *lab, const NbaAssetPack *assets,
                           NbaRenderer *renderer);
void nba_player_lab_print(const NbaPlayerLab *lab, const NbaAssetPack *assets);

/* Shared ROM-sprite compositor used by Player Lab and live court scenes.
 * origin_x/origin_y are the ROM's lower-body attachment point; scale 1 is
 * native SNES size and scale 2 is the enlarged laboratory view. */
bool nba_player_sprite_render(NbaRenderer *renderer, const NbaAssetPack *assets,
                              uint8_t team, uint8_t roster_slot, uint8_t side,
                              uint8_t upper_state, uint8_t direction,
                              uint32_t animation_tick, int origin_x,
                              int origin_y, int scale);
bool nba_player_sprite_render_split(NbaRenderer *renderer,
                                    const NbaAssetPack *assets, uint8_t team,
                                    uint8_t roster_slot, uint8_t side,
                                    uint8_t upper_state, uint8_t lower_state,
                                    uint8_t direction, uint32_t upper_tick,
                                    uint32_t lower_tick, int origin_x,
                                    int origin_y, int scale);
bool nba_player_animation_resources(const NbaAssetPack *assets,
                                    uint8_t upper_state, uint8_t lower_state,
                                    uint8_t direction, uint32_t upper_tick,
                                    uint32_t lower_tick,
                                    uint16_t *upper_resource,
                                    uint16_t *lower_resource);
bool nba_player_gameplay_shot_ratings(const NbaAssetPack *assets,
                                      uint8_t team, uint8_t roster_slot,
                                      uint8_t *two_point, uint8_t *three_point);

#endif
