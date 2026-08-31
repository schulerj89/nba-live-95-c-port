#include "nba_tipoff.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* C initialization coverage, not a native trajectory oracle. Optional owned
 * snapshots permit byte-exact before/after comparisons with matching layouts. */
int main(int argc, char **argv) {
    if (argc != 2 && argc != 3) return 2;
    NbaAssetPack assets = {0};
    if (!nba_assets_load(&assets, argv[1])) return 3;
    FILE *states = NULL;
    if (argc == 3) {
        states = fopen(argv[2], "wb");
        if (!states) { nba_assets_free(&assets); return 4; }
    }
    unsigned failed = 0, passed = 0;
    const size_t offset = offsetof(NbaTipoff, frame);
    const size_t bytes = sizeof(NbaTipoff) - offset;
    printf("INITIALIZATION_RECORD offset=%zu bytes=%zu\n", offset, bytes);
    for (unsigned away = 0; away < 29u; ++away) {
        for (unsigned home = 0; home < 29u; ++home) {
            NbaSession session;
            nba_session_init(&session);
            session.left_team = (uint8_t)away;
            session.right_team = (uint8_t)home;
            session.config.main_values[3] = (uint16_t)((away + home) % 4u);
            NbaSession before = session;
            NbaTipoff game;
            bool initialized = nba_tipoff_init(&game, &assets, &session);
            if (!initialized) {
                ++failed;
                printf("INITIALIZATION_FAIL away=%u home=%u\n", away, home);
                continue;
            }
            if (memcmp(&before, &session, sizeof(session)) != 0 ||
                game.team_context[0].strategy_team_raw_00 != home ||
                game.team_context[1].strategy_team_raw_00 != away ||
                game.match_clock_raw_0928 != nba_match_period_clock(&session)) {
                nba_assets_free(&assets);
                if (states) fclose(states);
                return 5;
            }
            if (states) {
                const uint8_t identity[2] = {(uint8_t)away, (uint8_t)home};
                if (fwrite(identity, 1, sizeof(identity), states) != sizeof(identity) ||
                    fwrite((const uint8_t *)&game + offset, 1, bytes, states) != bytes) {
                    fclose(states); nba_assets_free(&assets); return 6;
                }
            }
            ++passed;
        }
    }
    if (states && fclose(states)) { nba_assets_free(&assets); return 7; }
    nba_assets_free(&assets);
    printf("MATCH INITIALIZATION %s: pairs=%u passed=%u failed=%u; "
           "four quarter settings, canonical teams and unchanged sessions\n",
           failed ? "FAIL" : "PASS", passed + failed, passed, failed);
    return failed ? 1 : 0;
}
