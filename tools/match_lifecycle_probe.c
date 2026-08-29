#include "nba_session.h"
#include "nba_tipoff.h"
#include "nba_rom.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition, code) do { if (!(condition)) return (code); } while (0)

int main(int argc, char **argv) {
    static const uint16_t regulation[4] = {10800u, 18000u, 28800u, 43200u};
    static const uint16_t overtime[4] = {7200u, 10800u, 14400u, 18000u};
    static const uint8_t lineup[5] = {2u, 0u, 1u, 3u, 4u};
    CHECK(argc == 3, 2);

    NbaAssetPack assets;
    CHECK(nba_assets_load(&assets, argv[1]), 3);
    NbaRom rom;
    CHECK(nba_rom_load_file(&rom, argv[2]), 14);
    for (uint16_t setting = 0; setting < 4u; ++setting) {
        CHECK(nba_rom_read16(&rom, 0x86u,
                  (uint16_t)(0xE38Au + setting * 2u)) == regulation[setting], 15);
        CHECK(nba_rom_read16(&rom, 0x86u,
                  (uint16_t)(0xE392u + setting * 2u)) == overtime[setting], 16);
        CHECK(nba_match_regulation_clock(setting) == regulation[setting], 4);
        CHECK(nba_match_overtime_clock(setting) == overtime[setting], 5);
        for (uint16_t period_index = 0; period_index < 2u; ++period_index) {
            NbaSession session;
            nba_session_init(&session);
            session.config.main_values[3] = setting;
            session.match.period_raw_0926 = period_index ? 4u : 0u;
            CHECK(session.match.timeouts_remaining[0] == 7u &&
                  session.match.timeouts_remaining[1] == 7u, 6);
            CHECK(memcmp(session.match.active_lineup[0], lineup, 5u) == 0 &&
                  memcmp(session.match.active_lineup[1], lineup, 5u) == 0, 7);
            CHECK(nba_match_period_clock(&session) ==
                  (period_index ? overtime[setting] : regulation[setting]), 8);

            /* Prove the production court initializer consumes the persistent
             * period and lineup rather than a probe-only duplicate. */
            session.match.active_lineup[0][0] = 5u;
            session.match.active_lineup[1][4] = 6u;
            NbaTipoff tipoff;
            CHECK(nba_tipoff_init(&tipoff, &assets, &session), 9);
            CHECK(tipoff.period_raw_0926 == session.match.period_raw_0926, 10);
            CHECK(tipoff.match_clock_raw_0928 ==
                  (period_index ? overtime[setting] : regulation[setting]), 11);
            CHECK(tipoff.actors[0].roster_slot == 5u &&
                  tipoff.actors[9].roster_slot == 6u, 12);
            CHECK(session.match.timeouts_remaining[0] == 7u &&
                  session.match.timeouts_remaining[1] == 7u, 13);
        }
    }
    nba_rom_free(&rom);
    puts("[MATCH LIFECYCLE A] regulation=4 overtime=4 period/timeout/lineup binding PASS");
    return 0;
}
