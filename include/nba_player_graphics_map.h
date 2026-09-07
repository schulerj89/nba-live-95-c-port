#ifndef NBA_PLAYER_GRAPHICS_MAP_H
#define NBA_PLAYER_GRAPHICS_MAP_H

#include "nba_assets.h"
#include <stdbool.h>
#include <stdint.h>

enum {
    NBA_PLAYER_GRAPHICS_MAP_SIDES = 2,
    NBA_PLAYER_GRAPHICS_MAP_ACTIVE = 5,
    NBA_PLAYER_GRAPHICS_MAP_ROSTER = 12,
    NBA_PLAYER_GRAPHICS_MAP_ACTORS = 10
};

typedef struct {
    uint16_t lineup[NBA_PLAYER_GRAPHICS_MAP_SIDES]
                   [NBA_PLAYER_GRAPHICS_MAP_ACTIVE];
    uint32_t roster_address[NBA_PLAYER_GRAPHICS_MAP_SIDES]
                           [NBA_PLAYER_GRAPHICS_MAP_ROSTER];
} NbaPlayerGraphicsMapInput;

typedef struct {
    uint32_t active_roster_address[NBA_PLAYER_GRAPHICS_MAP_ACTORS];
    uint16_t statistics_address[NBA_PLAYER_GRAPHICS_MAP_ACTORS];
} NbaPlayerGraphicsMap;

/* `$86:D7B8-$D85D`: select ten active long roster-record addresses into
 * `$3449-$3470` and their 24-record statistics addresses into `$3435-$3448`.
 * CPU/DP/stack scratch is excluded; invalid host roster indices reject
 * atomically instead of reading beyond the native source tables. */
bool nba_player_graphics_map(const NbaPlayerGraphicsMapInput *input,
                             NbaPlayerGraphicsMap *output);

/* Host-only gameplay adapter; no direct native address. Rebuild both
 * twelve-entry `$3471/$34A1` source tables from exact pack roster addresses
 * before the supported `$86:D7B8-$D85D` selection. */
bool nba_player_graphics_map_from_assets(
    const NbaAssetPack *assets,
    const uint8_t teams[NBA_PLAYER_GRAPHICS_MAP_SIDES],
    const uint8_t lineup[NBA_PLAYER_GRAPHICS_MAP_SIDES]
                        [NBA_PLAYER_GRAPHICS_MAP_ACTIVE],
    NbaPlayerGraphicsMap *output);

#endif
