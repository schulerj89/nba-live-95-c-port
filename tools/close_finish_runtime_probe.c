#include <stdint.h>
#include <stdio.h>
#include "nba_tipoff.h"

static int fail(const char *message) {
    fprintf(stderr, "[CLOSE FINISH RUNTIME] %s\n", message);
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    NbaAssetPack assets = {0};
    if (!nba_assets_load(&assets, argv[1])) return 3;
    NbaSession session;
    NbaTipoff state;
    nba_session_init(&session);
    if (!nba_tipoff_init(&state, &assets, &session)) return 4;

    NbaTipoffActor *actor = &state.actors[0];
    actor->control_mode = 16u;
    actor->reaction_threshold = 24u;
    actor->animation_state = 40u;
    actor->lower_animation_state = 40u;
    actor->direction = 3u;
    actor->actor_status_raw_28 = 0u;
    if (!nba_player_animation_resources_for_appearance(
            &assets, 40u, 40u, 3u, 0u, 0u, false,
            actor->animation_variant_raw_6c,
            &actor->upper_animation_resource_raw_2a,
            &actor->lower_animation_resource_raw_2c))
        return fail("could not resolve hold pose resources");
    actor->animation_resources_valid = true;
    state.court_presentation.basket_x_3fef = 336u;
    int16_t ox, oy, oz;
    if (!nba_player_ball_attachment_point_offsets(
            &assets, actor->upper_animation_resource_raw_2a,
            actor->lower_animation_resource_raw_2c,
            actor->actor_status_raw_28, 1u, &ox, &oy, &oz))
        return fail("could not resolve hold attachment");
    if (!nba_tipoff_replay_passive_mode(&state, 0u) ||
        actor->reaction_threshold != 22u || actor->behavior_flags_raw != 1u ||
        (int16_t)(actor->x_fp >> 8) != (int16_t)(336 - ox) ||
        (int16_t)(actor->y_fp >> 8) != (int16_t)-oy ||
        (int16_t)(actor->z_fp >> 8) != (int16_t)(80 - oz))
        return fail("basket-relative post-shot hold changed");

    actor->control_mode = 16u;
    actor->reaction_threshold = 0u;
    if (!nba_tipoff_replay_passive_mode(&state, 0u) ||
        actor->control_mode != 7u || actor->reaction_threshold != 0xB4u ||
        actor->behavior_flags_raw != 0u)
        return fail("post-shot hold expiry changed");

    actor->control_mode = 14u;
    actor->reaction_threshold = 2u;
    actor->pass_direction_raw = 0x1Bu;
    actor->special_contact_raw_56 = 6;
    state.possession_actor = 0;
    state.ball.x_fp = 335 * 256;
    state.ball.y_fp = 2 * 256;
    if (!nba_tipoff_replay_mode14_close_finish(&state, 0u) ||
        actor->control_mode != 7u || actor->reaction_threshold != 0xB4u ||
        state.possession_actor != -1 || state.shot_actor_raw_09c8 != 0)
        return fail("terminal close-finish continuation changed");

    nba_assets_free(&assets);
    puts("[CLOSE FINISH RUNTIME] PASS: hold, expiry, terminal continuation");
    return 0;
}
