#include <stdio.h>
#include <stdint.h>
#include "nba_tipoff.h"
#include "nba_player_lab.h"

/* Production-sequence guard for `$86:E545-$E592` followed by the next
 * `$87:AD5B-$AEC2` cadence and `$87:B649/$B66A` attachment. This is an
 * unforced CPU game: it observes natural bases 9/11 without injecting state. */
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    NbaAssetPack pack;
    if (!nba_assets_load(&pack, argv[1])) return 3;
    NbaSession session;
    NbaTipoff game;
    NbaInput input = {0};
    nba_session_init(&session);
    if (!nba_tipoff_init(&game, &pack, &session)) {
        nba_assets_free(&pack);
        return 4;
    }

    uint8_t previous_base[10] = {0};
    uint16_t previous_upper[10] = {0}, previous_lower[10] = {0};
    unsigned dynamic = 0, reversals = 0, preserved = 0;
    unsigned rebuilt_pair_differences = 0, rebuilt_offset_differences = 0;
    for (unsigned frame = 1; frame <= 20000u; ++frame) {
        nba_tipoff_update(&game, &input);
        int owner = game.ball.owner_actor;
        if (owner >= 0 && owner < 10 && game.possession_actor == owner &&
            game.live_state_raw < 0x80u &&
            game.fouls.free_throw_state_raw_0978 == 0u) {
            NbaTipoffActor *actor = &game.actors[owner];
            uint8_t base = actor->base_animation_state_raw_38;
            if (actor->control_mode == 11u && (base == 9u || base == 11u)) {
                ++dynamic;
                if (!actor->animation_resources_valid) return 5;
                uint16_t mirror = actor->direction < 3u ? 0x8000u : 0u;
                int16_t x = 0, y = 0, z = 0;
                if (!nba_player_ball_attachment_offsets(
                        &pack, actor->upper_animation_resource_raw_2a,
                        actor->lower_animation_resource_raw_2c, mirror,
                        &x, &y, &z)) return 6;
                int32_t actual_x = game.ball.x_fp - actor->x_fp;
                int32_t actual_y = game.ball.y_fp - actor->y_fp;
                if (actual_x != (int32_t)x * 256 ||
                    actual_y != (int32_t)y * 256) {
                    fprintf(stderr,
                        "dynamic dribble attachment mismatch frame=%u actor=%d "
                        "base=%u resources=%04x/%04x actual=%ld/%ld expected=%d/%d\n",
                        frame, owner, base,
                        actor->upper_animation_resource_raw_2a,
                        actor->lower_animation_resource_raw_2c,
                        (long)actual_x, (long)actual_y, x * 256, y * 256);
                    return 7;
                }

                uint16_t rebuilt_upper = 0, rebuilt_lower = 0;
                if (!nba_player_animation_resources_for_appearance(
                        &pack, actor->animation_state,
                        actor->lower_animation_state, actor->direction,
                        actor->upper_animation_tick, actor->lower_animation_tick,
                        actor->free_throw_launch_half_raw_a8 != 0u,
                        actor->animation_variant_raw_6c,
                        &rebuilt_upper, &rebuilt_lower)) return 8;
                if (rebuilt_upper != actor->upper_animation_resource_raw_2a ||
                    rebuilt_lower != actor->lower_animation_resource_raw_2c) {
                    ++rebuilt_pair_differences;
                    int16_t rx = 0, ry = 0, rz = 0;
                    if (!nba_player_ball_attachment_offsets(
                            &pack, rebuilt_upper, rebuilt_lower, mirror,
                            &rx, &ry, &rz)) return 9;
                    if (rx != x || ry != y) ++rebuilt_offset_differences;
                }
                if ((previous_base[owner] == 9u && base == 11u) ||
                    (previous_base[owner] == 11u && base == 9u)) {
                    ++reversals;
                    if (previous_upper[owner] ==
                            actor->upper_animation_resource_raw_2a &&
                        previous_lower[owner] ==
                            actor->lower_animation_resource_raw_2c)
                        ++preserved;
                }
            }
        }
        for (unsigned actor = 0; actor < 10u; ++actor) {
            previous_base[actor] = game.actors[actor].base_animation_state_raw_38;
            previous_upper[actor] = game.actors[actor].upper_animation_resource_raw_2a;
            previous_lower[actor] = game.actors[actor].lower_animation_resource_raw_2c;
        }
    }
    nba_assets_free(&pack);
    printf("[DYNAMIC DRIBBLE ATTACHMENT] frames=20000 dynamic=%u "
           "reversals=%u preserved=%u rebuilt_pairs=%u rebuilt_offsets=%u\n",
           dynamic, reversals, preserved, rebuilt_pair_differences,
           rebuilt_offset_differences);
    if (dynamic == 0u || reversals == 0u || preserved == 0u ||
        rebuilt_pair_differences == 0u || rebuilt_offset_differences == 0u)
        return 10;
    return 0;
}
