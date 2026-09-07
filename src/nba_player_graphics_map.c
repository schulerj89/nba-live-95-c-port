#include "nba_player_graphics_map.h"
#include "nba_player_lab.h"

/* `$86:D7B8-$D85D`, gameplay graphics: project each side's active roster
 * permutation through its carried long-address table and the regular
 * `$87:9C8F` statistics table. */
bool nba_player_graphics_map(const NbaPlayerGraphicsMapInput *input,
                             NbaPlayerGraphicsMap *output) {
    NbaPlayerGraphicsMap next;
    if (!input || !output) return false;
    for (unsigned side = 0; side < NBA_PLAYER_GRAPHICS_MAP_SIDES; ++side) {
        for (unsigned active = 0; active < NBA_PLAYER_GRAPHICS_MAP_ACTIVE;
             ++active) {
            unsigned roster = input->lineup[side][active];
            unsigned actor = side * NBA_PLAYER_GRAPHICS_MAP_ACTIVE + active;
            if (roster >= NBA_PLAYER_GRAPHICS_MAP_ROSTER) return false;
            next.active_roster_address[actor] =
                input->roster_address[side][roster];
            next.statistics_address[actor] = (uint16_t)(
                0x40EBu + 0x40u *
                (roster + side * NBA_PLAYER_GRAPHICS_MAP_ROSTER));
        }
    }
    *output = next;
    return true;
}

/* Host-only gameplay adapter; no direct native address. For `$86:D7B8-$D85D`,
 * the pack accessor is the portable writer of the two native twelve-entry
 * `$3471/$34A1` roster-address tables. */
bool nba_player_graphics_map_from_assets(
    const NbaAssetPack *assets,
    const uint8_t teams[NBA_PLAYER_GRAPHICS_MAP_SIDES],
    const uint8_t lineup[NBA_PLAYER_GRAPHICS_MAP_SIDES]
                        [NBA_PLAYER_GRAPHICS_MAP_ACTIVE],
    NbaPlayerGraphicsMap *output) {
    NbaPlayerGraphicsMapInput input = {0};
    if (!assets || !teams || !lineup || !output) return false;
    for (unsigned side = 0; side < NBA_PLAYER_GRAPHICS_MAP_SIDES; ++side) {
        for (unsigned roster = 0; roster < NBA_PLAYER_GRAPHICS_MAP_ROSTER;
             ++roster) {
            if (!nba_player_gameplay_roster_address(
                    assets, teams[side], (uint8_t)roster,
                    &input.roster_address[side][roster]))
                return false;
        }
        for (unsigned active = 0; active < NBA_PLAYER_GRAPHICS_MAP_ACTIVE;
             ++active)
            input.lineup[side][active] = lineup[side][active];
    }
    return nba_player_graphics_map(&input, output);
}
