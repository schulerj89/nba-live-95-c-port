/* Production-initializer identity projection, not a complete match replay.
 * Expectations come only from an independently captured native first-court
 * snapshot. The probe takes UI team identities on stdin; it never supplies
 * its own expected actor/team mapping. Controller dispatch is separate. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_tipoff.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 2 && argc != 3) return 2;
    unsigned left, right, quarter;
    char trailing;
    if (scanf("%u %u %u", &left, &right, &quarter) != 3 ||
        left >= 29u || right >= 29u || quarter >= 4u ||
        scanf(" %c", &trailing) != EOF) return 2;
    NbaAssetPack pack = {0};
    NbaSession session;
    NbaTipoff *game = (NbaTipoff *)calloc(1u, sizeof(*game));
    if (!game) return 3;
    if (!nba_assets_load(&pack, argv[1])) { free(game); return 4; }
    nba_session_init(&session);
    session.left_team = (uint8_t)left;
    session.right_team = (uint8_t)right;
    session.config.main_values[3] = (uint16_t)quarter;
    if (!nba_tipoff_init(game, &pack, &session)) {
        nba_assets_free(&pack); free(game); return 5;
    }
    printf("FIRST_COURT_IDENTITY {\"context_teams\":[%u,%u],"
           "\"anchor_x\":[%u,%u],\"actor_groups\":[",
           game->team_context[0].strategy_team_raw_00,
           game->team_context[1].strategy_team_raw_00,
           (uint16_t)game->team_context[0].anchor_x_raw_0a,
           (uint16_t)game->team_context[1].anchor_x_raw_0a);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        printf("%s%u", i ? "," : "", game->actors[i].team_group_raw_6e);
    fputs("],\"active_roster\":[", stdout);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        printf("%s%u", i ? "," : "", game->actors[i].roster_slot);
    fputs("],\"assignment_roles\":[", stdout);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        printf("%s%u", i ? "," : "", game->actors[i].assignment_role_raw_92);
    fputs("],\"appearance_variants\":[", stdout);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        printf("%s%u", i ? "," : "", game->actors[i].animation_variant_raw_6c);
    fputs("],\"height_variants\":[", stdout);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        printf("%s%u", i ? "," : "", game->actors[i].free_throw_launch_half_raw_a8);
    fputs("],\"active_stamina_ratings\":[", stdout);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        /* The production fatigue sweep binds this persistent roster slot.
         * Read its stored rating; do not re-fetch a rating using a corrected
         * team formula that could conceal a broken initializer. */
        unsigned roster = (i < 5u ? 0u : 12u) + game->actors[i].roster_slot;
        printf("%s%u", i ? "," : "", game->fatigue.rating[roster]);
    }
    puts("]}");
    if (argc == 3) {
        /* Optional visual evidence:30 ordinary production updates with no
         * input, after the initializer projection. This is not a claimed
         * native frame alignment; native/C scheduler differences remain. */
        NbaRenderer renderer;
        NbaInput input = {0};
        nba_renderer_init(&renderer);
        for (unsigned frame = 0; frame < 30u; ++frame)
            nba_tipoff_update(game, &input);
        nba_tipoff_render(game, &renderer);
        if (!nba_renderer_save_bmp(&renderer, argv[2])) {
            nba_assets_free(&pack); free(game); return 6;
        }
    }
    nba_assets_free(&pack);
    free(game);
    return 0;
}
