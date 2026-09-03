#include "nba_tipoff.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    uint64_t digest;
    unsigned camera_changes;
    unsigned actor_motion_frames;
    unsigned owner_changes;
    unsigned pass_frames;
    unsigned shot_frames;
    unsigned score_changes;
    unsigned dead_recoveries;
    unsigned resource_changes;
    unsigned render_changes;
} ScenarioResult;

static void mix16(uint64_t *hash, uint16_t value) {
    *hash ^= (uint8_t)value; *hash *= 1099511628211ull;
    *hash ^= (uint8_t)(value >> 8); *hash *= 1099511628211ull;
}

static void mix32(uint64_t *hash, uint32_t value) {
    mix16(hash, (uint16_t)value); mix16(hash, (uint16_t)(value >> 16));
}

static uint64_t renderer_hash(const NbaRenderer *renderer) {
    const uint8_t *bytes = (const uint8_t *)renderer->pixels;
    uint64_t hash = 1469598103934665603ull;
    for (size_t i = 0; i < sizeof(renderer->pixels); ++i) {
        hash ^= bytes[i]; hash *= 1099511628211ull;
    }
    return hash;
}

static void hash_gameplay_state(uint64_t *hash, const NbaTipoff *game) {
    mix32(hash, (uint32_t)game->frame);
    mix16(hash, game->live_state_raw);
    mix16(hash, (uint16_t)(int16_t)game->possession_actor);
    mix16(hash, game->match_clock_raw_0928);
    mix16(hash, game->play_code);
    mix16(hash, (uint16_t)game->play_step_raw);
    mix16(hash, game->ball.state);
    mix32(hash, (uint32_t)game->ball.x_fp);
    mix32(hash, (uint32_t)game->ball.y_fp);
    mix32(hash, (uint32_t)game->ball.z_fp);
    mix16(hash, (uint16_t)game->camera_x);
    mix16(hash, (uint16_t)game->camera_y);
    mix16(hash, game->session->score[0]);
    mix16(hash, game->session->score[1]);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        const NbaTipoffActor *actor = &game->actors[i];
        mix32(hash, (uint32_t)actor->x_fp);
        mix32(hash, (uint32_t)actor->y_fp);
        mix16(hash, actor->control_mode);
        mix16(hash, actor->requested_direction);
        mix16(hash, actor->upper_animation_resource_raw_2a);
        mix16(hash, actor->lower_animation_resource_raw_2c);
        mix16(hash, actor->rom_upper_animation_phase_raw_3a);
        mix16(hash, actor->rom_lower_animation_phase_raw_3c);
    }
}

static int run_scenario(const NbaAssetPack *assets, uint8_t away,
                        uint8_t home, uint16_t seed, unsigned frames,
                        ScenarioResult *result) {
    NbaSession session;
    nba_session_init(&session);
    session.left_team = away;
    session.right_team = home;
    NbaTipoff game;
    if (!nba_tipoff_init(&game, assets, &session)) return 10;
    /* Preserve the retired scaffold only as an explicit controlled test
     * input so this pre-expiry gameplay trajectory remains comparable. */
    game.match_clock_raw_0928 = 43200u;
    game.rng.state = seed;
    NbaInput input = {0};
    NbaRenderer renderer;
    nba_renderer_init(&renderer);
    memset(result, 0, sizeof(*result));
    result->digest = 1469598103934665603ull;
    int16_t previous_camera_x = game.camera_x;
    int16_t previous_camera_y = game.camera_y;
    int8_t previous_owner = game.possession_actor;
    uint16_t previous_score[2] = {session.score[0], session.score[1]};
    uint16_t previous_resource[NBA_GAMEPLAY_ACTOR_COUNT][2];
    int32_t previous_xy[NBA_GAMEPLAY_ACTOR_COUNT][2];
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        previous_resource[i][0] = game.actors[i].upper_animation_resource_raw_2a;
        previous_resource[i][1] = game.actors[i].lower_animation_resource_raw_2c;
        previous_xy[i][0] = game.actors[i].x_fp;
        previous_xy[i][1] = game.actors[i].y_fp;
    }
    bool was_dead = game.live_state_raw >= 0x80u;
    uint64_t previous_render = 0;
    unsigned long_dead = 0;

    for (unsigned frame = 1; frame <= frames; ++frame) {
        /* This is a sustained gameplay/render trajectory probe, not a match
         * lifecycle probe. Keep it within one continuous period; expiry,
         * quarter restarts and final state have dedicated native witnesses. */
        if (game.match_clock_raw_0928 < 6000u)
            game.match_clock_raw_0928 = 43200u;
        nba_tipoff_update(&game, &input);
        hash_gameplay_state(&result->digest, &game);
        if (game.possession_actor < -1 ||
            game.possession_actor >= NBA_GAMEPLAY_ACTOR_COUNT) return 11;
        if (game.ball.owner_actor < -1 ||
            game.ball.owner_actor >= NBA_GAMEPLAY_ACTOR_COUNT) return 12;
        if (game.camera_x != previous_camera_x ||
            game.camera_y != previous_camera_y) ++result->camera_changes;
        previous_camera_x = game.camera_x;
        previous_camera_y = game.camera_y;
        if (game.possession_actor != previous_owner) ++result->owner_changes;
        previous_owner = game.possession_actor;
        if (game.ball.state == NBA_BALL_PASS) ++result->pass_frames;
        if (game.ball.state == NBA_BALL_SHOT) ++result->shot_frames;
        if (session.score[0] != previous_score[0] ||
            session.score[1] != previous_score[1]) ++result->score_changes;
        previous_score[0] = session.score[0];
        previous_score[1] = session.score[1];
        bool dead = game.live_state_raw >= 0x80u;
        if (was_dead && !dead) ++result->dead_recoveries;
        was_dead = dead;
        long_dead = dead ? long_dead + 1u : 0u;
        if (long_dead > 2400u) {
            const NbaTipoffActor *inbound =
                game.inbound_actor_raw < NBA_GAMEPLAY_ACTOR_COUNT ?
                &game.actors[game.inbound_actor_raw] : NULL;
            fprintf(stderr,"GAMEPLAY85 dead stall frame=%u live=%x period=%u clock=%u owner=%d ball_owner=%d inbound=%u layout=%d ready=%u transfer=%u ft=%u ball=%d,%d actor=%d,%d velocity=%d,%d target=%d,%d mode=%u\n",
                frame,game.live_state_raw,game.period_raw_0926,
                game.match_clock_raw_0928,game.possession_actor,
                game.ball.owner_actor,game.inbound_actor_raw,
                game.inbound_layout_raw,
                game.inbound_ready_raw,game.inbound_transfer_raw,
                game.fouls.free_throw_state_raw_0978,
                game.ball.x_fp / 256,game.ball.y_fp / 256,
                inbound ? inbound->x_fp / 256 : 0,
                inbound ? inbound->y_fp / 256 : 0,
                inbound ? inbound->velocity_x : 0,
                inbound ? inbound->velocity_y : 0,
                game.inbound_target_x_raw,game.inbound_target_y_raw,
                inbound ? inbound->control_mode : 0u);
            return 13;
        }
        bool moved = false;
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
            const NbaTipoffActor *actor = &game.actors[i];
            if (actor->x_fp != previous_xy[i][0] ||
                actor->y_fp != previous_xy[i][1]) moved = true;
            previous_xy[i][0] = actor->x_fp;
            previous_xy[i][1] = actor->y_fp;
            if (actor->upper_animation_resource_raw_2a !=
                    previous_resource[i][0] ||
                actor->lower_animation_resource_raw_2c !=
                    previous_resource[i][1]) ++result->resource_changes;
            previous_resource[i][0] = actor->upper_animation_resource_raw_2a;
            previous_resource[i][1] = actor->lower_animation_resource_raw_2c;
        }
        if (moved) ++result->actor_motion_frames;
        if ((frame % 120u) == 0u) {
            nba_tipoff_render(&game, &renderer);
            uint64_t rendered = renderer_hash(&renderer);
            mix32(&result->digest, (uint32_t)rendered);
            mix32(&result->digest, (uint32_t)(rendered >> 32));
            if (previous_render != 0u && rendered != previous_render)
                ++result->render_changes;
            previous_render = rendered;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    NbaAssetPack assets = {0};
    if (!nba_assets_load(&assets, argv[1])) return 3;
    const uint8_t teams[][2] = {{0,18}, {18,3}, {26,8}};
    const uint16_t seeds[] = {0x1357u, 0x4A91u, 0xBEEFu};
    /* C-only integration digests, re-reviewed after native team/rank and
     * factory configuration corrections plus the $85:C39C inbound fix.
     * All three 16,000-frame paths retain scoring, both-team motion, multiple
     * possessions, dead-ball recoveries and changing resources/renders. The
     * exact ROM fixtures and these semantic guards are separate from hashes.
     * No native trajectory is rebaselined by these C-only controls. */
    const uint64_t expected[] = {
        0xbe9fb0edcea0d524ull,
        0x2a8877c0056dc49dull,
        0xfddf2d5e8ba68e5aull
    };
    ScenarioResult total = {0};
    int code = 0;
    int digest_code = 0;
    for (unsigned scenario = 0; scenario < 3u; ++scenario) {
        ScenarioResult result;
        code = run_scenario(&assets, teams[scenario][0], teams[scenario][1],
                            seeds[scenario], 16000u, &result);
        if (code != 0) break;
        if (result.digest != expected[scenario]) digest_code = 14;
        printf("GAMEPLAY85 scenario=%u teams=%u/%u digest=%016llx expected=%016llx camera=%u motion=%u owner=%u pass=%u shot=%u score=%u dead_recovery=%u resources=%u renders=%u\n",
            scenario, teams[scenario][0], teams[scenario][1],
            (unsigned long long)result.digest,
            (unsigned long long)expected[scenario], result.camera_changes,
            result.actor_motion_frames, result.owner_changes,
            result.pass_frames, result.shot_frames, result.score_changes,
            result.dead_recoveries, result.resource_changes,
            result.render_changes);
        total.camera_changes += result.camera_changes;
        total.actor_motion_frames += result.actor_motion_frames;
        total.owner_changes += result.owner_changes;
        total.pass_frames += result.pass_frames;
        total.shot_frames += result.shot_frames;
        total.score_changes += result.score_changes;
        total.dead_recoveries += result.dead_recoveries;
        total.resource_changes += result.resource_changes;
        total.render_changes += result.render_changes;
    }
    if (code == 0 && digest_code != 0) code = digest_code;
    nba_assets_free(&assets);
    if (code == 0 && (total.camera_changes < 1000u ||
        total.actor_motion_frames < 10000u || total.owner_changes < 10u ||
        total.pass_frames == 0u || total.shot_frames == 0u ||
        total.score_changes == 0u || total.dead_recoveries == 0u ||
        total.resource_changes < 100u || total.render_changes < 100u))
        code = 20;
    printf("GAMEPLAY85_ENDURANCE %s frames=48000 code=%d\n",
           code == 0 ? "PASS" : "FAIL", code);
    return code == 0 ? 0 : 1;
}
