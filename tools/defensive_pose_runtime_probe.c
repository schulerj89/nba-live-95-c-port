#include "nba_tipoff.h"
#include <stdio.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "defensive pose runtime line %d\n", __LINE__); \
        return 10; \
    } \
} while (0)

int main(int argc, char **argv) {
    NbaAssetPack pack = {0};
    NbaSession session;
    NbaTipoff game;
    if (argc != 2 || !nba_assets_load(&pack, argv[1])) return 2;
    nba_session_init(&session);
    CHECK(nba_tipoff_init(&game, &pack, &session));

    /* Exercise the production adapter, including its live assignment and
     * target refresh, rather than calling the portable helper a second time. */
    NbaTipoffActor *actor = &game.actors[0];
    NbaTipoffActor *paired = &game.actors[5];
    actor->assignment_base_raw = actor->assignment_current_raw = 10u;
    actor->control_mode = 4u;
    actor->x_fp = paired->x_fp = 0;
    actor->y_fp = paired->y_fp = 0;
    actor->z_fp = 0;
    actor->movement_magnitude_raw = paired->movement_magnitude_raw = 0u;
    actor->anchor_distance_raw = paired->anchor_distance_raw = 0u;
    actor->animation_state = 0u;
    game.live_state_raw = 0u;
    game.possession_actor = 5;
    game.pass_receiver_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    CHECK(nba_tipoff_replay_defensive_pose(&game, 0u));
    CHECK(actor->base_animation_state_raw_38 == 7u);
    CHECK(game.defensive_pose_count_raw_1868 == 1u);
    CHECK(actor->movement_direction == actor->assignment_direction);
    CHECK(actor->requested_direction == actor->assignment_direction);

    /* The wrapper must also preserve the native airborne early-return state. */
    actor->z_fp = 1 * 256;
    actor->base_animation_state_raw_38 = 0x55u;
    CHECK(nba_tipoff_replay_defensive_pose(&game, 0u));
    CHECK(actor->base_animation_state_raw_38 == 0x55u);
    CHECK(game.defensive_pose_count_raw_1868 == 1u);

    nba_assets_free(&pack);
    puts("defensive pose runtime binding passed");
    return 0;
}
