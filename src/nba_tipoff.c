#include "nba_tipoff.h"
#include "nba_player_lab.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int16_t world_x, world_y;
    int16_t screen_x, screen_y;
    uint8_t direction;
} TipoffFormation;

/* $86:DDA7-$DF54 initializes these paired $100-byte actor records. The
 * settled jump-ball camera sends only eight actors through $80:AD92;
 * $87:A47A culls actors 4 and 9 before composition. */
static const TipoffFormation formation[10] = {
    {   8,   3, 139, 122, 6 }, { -16, -83,  29, 107, 0 },
    { -24,  80, 184, 150, 4 }, { 104, -56, 176,  84, 7 },
    {  96,  59,   0,   0, 5 }, {  -8,  -3, 117, 125, 2 },
    {  16,  83, 227, 140, 4 }, {  24, -80,  72,  98, 0 },
    {-104,  56,  80, 164, 3 }, { -96, -59,   0,   0, 0 },
};

/* Native $80:AD92 submission order at gameplay frames 89/91. The SNES gives
 * lower OAM indices priority, so the framebuffer composites it in reverse. */
static const uint8_t visible_submission[8] = { 8, 2, 6, 5, 0, 1, 7, 3 };

static uint16_t read_u16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t bgr555(uint16_t word) {
    uint32_t r = word & 31u, g = (word >> 5) & 31u, b = (word >> 10) & 31u;
    return 0xFF000000u | ((r << 3 | r >> 2) << 16) |
           ((g << 3 | g >> 2) << 8) | (b << 3 | b >> 2);
}

static uint8_t tile_pixel(const uint8_t *tile, int x, int y) {
    int bit = 7 - x;
    return (uint8_t)(((tile[y * 2] >> bit) & 1) |
        (((tile[y * 2 + 1] >> bit) & 1) << 1) |
        (((tile[16 + y * 2] >> bit) & 1) << 2) |
        (((tile[17 + y * 2] >> bit) & 1) << 3));
}

static int16_t fp_round(int32_t value) {
    return (int16_t)(value >= 0 ? (value + 128) >> 8 : -((-value + 128) >> 8));
}

static int mirror_court_x(int x) {
    return 280 - x;
}

static int basket_x_for_side(unsigned side) {
    return side ? 386 : -106;
}

static int basket_y_for_side(unsigned side) {
    return side ? -55 : 2;
}

static void cpu_begin_possession(NbaTipoff *tipoff, uint8_t offense_side);

static uint16_t actor_distance(int dx, int dy) {
    unsigned ax = (unsigned)(dx < 0 ? -dx : dx);
    unsigned ay = (unsigned)(dy < 0 ? -dy : dy);
    unsigned high = ax > ay ? ax : ay, low = ax > ay ? ay : ax;
    return (uint16_t)(high + (low >> 2));
}

/* `$87:B37C/$B3BD/$B47A/$B4DB` install independent resources and restart
 * only the animation channel whose state changed. */
static void actor_set_animation(NbaTipoffActor *actor, uint8_t upper,
                                uint8_t lower) {
    if (actor->animation_state != upper) {
        actor->animation_state = upper;
        actor->upper_animation_tick = 0u;
    }
    if (actor->lower_animation_state != lower) {
        actor->lower_animation_state = lower;
        actor->lower_animation_tick = 0u;
    }
}

static void cpu_set_role_targets(NbaTipoff *tipoff) {
    static const int16_t offense_shape[5][2] = {
        {292, 110}, {310, -70}, {185, 112}, {128, -150}, {132, -28}
    };
    unsigned offense_base = tipoff->offense_side ? 5u : 0u;
    unsigned defense_base = tipoff->offense_side ? 0u : 5u;
    bool attack_right = tipoff->offense_side != 0u;

    if (tipoff->cpu_play_state == NBA_CPU_PLAY_REBOUND) {
        int16_t ball_x = fp_round(tipoff->ball.x_fp);
        int16_t ball_y = fp_round(tipoff->ball.y_fp);
        for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
            tipoff->actors[actor].target_x = ball_x;
            tipoff->actors[actor].target_y = ball_y;
            bool preserve_recovery = actor == tipoff->handler_actor &&
                (tipoff->actors[actor].control_mode == 16u ||
                 tipoff->actors[actor].control_mode == 7u);
            if (!preserve_recovery)
                tipoff->actors[actor].control_mode =
                    actor / 5u == tipoff->offense_side ? 1u : 2u;
        }
        if (tipoff->handler_actor < NBA_GAMEPLAY_ACTOR_COUNT) {
            NbaTipoffActor *handler = &tipoff->actors[tipoff->handler_actor];
            if (handler->control_mode != 16u && handler->control_mode != 7u) {
                handler->control_mode = 16u;
                handler->reaction_threshold = 0x18u;
            }
        }
        return;
    }

    for (unsigned slot = 0; slot < 5; ++slot) {
        unsigned actor = offense_base + slot;
        int target_x = offense_shape[slot][0];
        int target_y = offense_shape[slot][1];
        if (!attack_right) target_x = mirror_court_x(target_x);
        if (tipoff->cpu_play_state >= NBA_CPU_PLAY_ATTACK) {
            int basket_x = attack_right ? 370 : -90;
            target_x = (target_x * 2 + basket_x) / 3;
        }
        tipoff->actors[actor].target_x = (int16_t)target_x;
        tipoff->actors[actor].target_y = (int16_t)target_y;
        /* Functional policy currently keeps the attacking five on mode 1.
         * The genuine controller-free ROM oracle proves mode `$0B` is also
         * used by CPU actors, so it must not be labeled human-only. */
        if (actor != tipoff->handler_actor ||
            (tipoff->actors[actor].control_mode != 16u &&
             tipoff->actors[actor].control_mode != 7u))
            tipoff->actors[actor].control_mode = 1u;
        tipoff->actors[actor].assignment_actor = (uint8_t)(defense_base + slot);
        tipoff->actors[actor].assignment_current_raw =
            (uint16_t)((defense_base + slot) * 2u);
    }

    /* `$85:BC43-$BC81` continuously resolves a defender's assigned actor;
     * defenders target the ball-side shoulder rather than a static spot. */
    int defend_offset = attack_right ? -18 : 18;
    for (unsigned slot = 0; slot < 5; ++slot) {
        unsigned actor = defense_base + slot;
        unsigned matchup = offense_base + slot;
        NbaTipoffActor *opponent = &tipoff->actors[matchup];
        tipoff->actors[actor].assignment_actor = (uint8_t)matchup;
        tipoff->actors[actor].assignment_current_raw = (uint16_t)(matchup * 2u);
        tipoff->actors[actor].target_x = (int16_t)(fp_round(opponent->x_fp) +
                                                   defend_offset);
        tipoff->actors[actor].target_y = fp_round(opponent->y_fp);
        tipoff->actors[actor].control_mode =
            matchup == tipoff->handler_actor ? 4u : 2u;
        int dx = fp_round(opponent->x_fp) - fp_round(tipoff->actors[actor].x_fp);
        int dy = fp_round(opponent->y_fp) - fp_round(tipoff->actors[actor].y_fp);
        uint16_t distance = 0u;
        uint8_t direction = nba_gameplay_target_direction(
            (int16_t)dx, (int16_t)dy, &distance);
        tipoff->actors[actor].assignment_direction = direction;
        tipoff->actors[actor].assignment_distance = distance;
        tipoff->actors[actor].pair_distance = distance;
        opponent->assignment_direction = direction < 8u ? direction ^ 4u : direction;
        opponent->pair_distance = distance;
    }

    NbaTipoffActor *handler = &tipoff->actors[tipoff->handler_actor];
    if (tipoff->cpu_play_state == NBA_CPU_PLAY_BREAK) {
        handler->target_x = (int16_t)(attack_right ? 80 : 200);
        handler->target_y = -80;
    } else if (tipoff->cpu_play_state == NBA_CPU_PLAY_DRIVE) {
        handler->target_x = (int16_t)(attack_right ? 128 : 152);
        handler->target_y = -150;
    } else if (tipoff->cpu_play_state == NBA_CPU_PLAY_ATTACK ||
               tipoff->cpu_play_state == NBA_CPU_PLAY_SHOT) {
        handler->target_x = (int16_t)(attack_right ? 350 : -70);
        handler->target_y = -55;
    }

    /* Actor `+$5E` is a local executor mode, not the global C play phase.
     * Preserve the proven CPU possession lifecycle from `$87:9244/$9BD0`:
     * handler 11, receiver 10, passer 15, shot 12, recovery 16. */
    if (tipoff->cpu_play_state == NBA_CPU_PLAY_PASS) {
        handler->control_mode = 15u;
        tipoff->actors[tipoff->receiver_actor].control_mode = 10u;
    } else if (tipoff->cpu_play_state == NBA_CPU_PLAY_SHOT) {
        if (tipoff->play_state_frame == 0u) {
            handler->control_mode = 12u;
        } else if (handler->control_mode != 16u && handler->control_mode != 7u) {
            /* `$86:9905` seeds +$60=$18 before mode 16's `$86:B0F7`. */
            handler->control_mode = 16u;
            handler->reaction_threshold = 0x18u;
        }
    } else {
        handler->control_mode = 11u;
    }
}

/* Proven passive behavior executors from `$87:9244/$87:9BD3`. Returning
 * true means the mode consumed this actor's scheduled pass. Modes 1-6 and
 * the active ball handlers remain in the explicitly provisional planner. */
static bool cpu_apply_passive_mode(NbaTipoffActor *actor) {
    if (actor->control_mode == 0u) { /* `$87:9C1B`: no-op wrapper. */
        actor->velocity_x = actor->velocity_y = 0;
        actor_set_animation(actor, 0u, 0u);
        return true;
    }
    if (actor->control_mode == 9u) { /* `$86:F0B7`: timed target override. */
        uint16_t remaining = (uint16_t)(actor->reaction_threshold - 2u);
        actor->reaction_threshold = remaining;
        if ((remaining & 0x8000u) != 0u) {
            actor->reaction_threshold = 0u;
            actor->behavior_timer = 0u;
            actor->control_mode = actor->saved_control_mode;
            return true;
        }
        return false;
    }
    if (actor->control_mode == 16u) { /* `$86:B0F7-$B153`: post-shot hold. */
        uint16_t remaining = (uint16_t)(actor->reaction_threshold - 2u);
        actor->reaction_threshold = remaining;
        actor->velocity_x = actor->velocity_y = 0;
        actor_set_animation(actor, 1u, 1u);
        if ((remaining & 0x8000u) != 0u) {
            /* `$86:B10A-$B122`: reset, mode 7, +$60=$B4. */
            actor->control_mode = 7u;
            actor->reaction_threshold = 0xB4u;
        }
        return true;
    }
    if (actor->control_mode == 7u) { /* `$86:994C-$99C3`: dead-ball hold. */
        uint16_t remaining = (uint16_t)(actor->reaction_threshold - 2u);
        actor->reaction_threshold = remaining;
        actor->velocity_x = actor->velocity_y = 0;
        actor_set_animation(actor, 3u, 3u);
        if ((remaining & 0x8000u) != 0u) {
            actor->reaction_threshold = 0u;
            actor->control_mode = 0u; /* `$86:9846` planner boundary. */
        }
        return true;
    }
    return false;
}

static bool cpu_active_decision_due(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    uint8_t mode = actor->control_mode;
    if (mode < 1u || mode > 6u) return true;
    uint8_t team = slot >= 5u ? tipoff->session->right_team :
                               tipoff->session->left_team;
    uint8_t profile_3f = 0u, profile_40 = 0u;
    (void)nba_player_gameplay_decision_profiles(
        tipoff->assets, team, actor->roster_slot,
        &profile_3f, &profile_40);
    int16_t actor_x = fp_round(actor->x_fp);
    /* Modes 2/4/6 compare signed byte actor +$04 against side context +$0A.
     * `$87:8EFE/$8F11` keeps DP $9E at $46EB for slots 0..4 and $476B for
     * slots 5..9; live values are stable anchors $B0 (-80) and $50 (+80).
     * This is deliberately not a ball-position or matchup comparison. */
    int8_t actor_x_low = (int8_t)(uint8_t)actor_x;
    int8_t side_anchor = slot < 5u ? -80 : 80;
    bool same_half = (int8_t)(actor_x_low ^ side_anchor) >= 0;
    if (mode == 1u || mode == 3u || mode == 5u)
        return nba_gameplay_decision_timer_step(
            &actor->reaction_threshold, profile_3f, 0x40u, false);
    /* Mode 2 reloads `$30 + profile[$40]`; modes 4/6 use `$20`.
     * All three add another `$20` when actor/related X signs match. */
    return nba_gameplay_decision_timer_step(
        &actor->reaction_threshold, profile_40,
        mode == 2u ? 0x30u : 0x20u, same_half);
}

static void cpu_move_actor(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    if (cpu_apply_passive_mode(actor)) return;
    /* `$85:B95C` seeds actor +$60; the mode-specific `$C8=$20` cadence above
     * replaces the former handcrafted per-slot/possession-frame delay. */
    /* `$85:963D-$985F` dispatches and integrates fractional words every
     * scheduled actor; the coordinate write slice is `$85:97CA-$985F`.
     * update. Integer coordinates change irregularly from subpixel carry;
     * there is no ROM evidence for the former odd/even slot shortcut. */
    int x = fp_round(actor->x_fp), y = fp_round(actor->y_fp);
    int dx = actor->target_x - x, dy = actor->target_y - y;
    bool decision_due = cpu_active_decision_due(tipoff, slot);
    /* Temporary parity guard until the live-covered mode-2 E7 target branch
     * replaces the remaining host target shape. `$85:A82C` itself is exact. */
    static const uint16_t first_move_start[10] = {
        100, 7, 2, 4, 2, 14, 5, 7, 2, 12
    };
    if (tipoff->possession_number == 0u &&
        tipoff->possession_frame < first_move_start[slot]) {
        actor->velocity_x = actor->velocity_y = 0;
        return;
    }
    uint8_t direction = actor->movement_direction;
    if (decision_due && actor->recovery_inhibit_raw == 0u) {
        direction = nba_gameplay_target_direction((int16_t)dx, (int16_t)dy,
                                                    NULL);
        actor->movement_direction = direction;
        if (direction < 8u) actor->requested_direction = direction;
    }
    uint8_t team = slot >= 5u ? tipoff->session->right_team :
                               tipoff->session->left_team;
    uint8_t profile_42 = 0x58u;
    (void)nba_player_gameplay_movement_profile(
        tipoff->assets, team, actor->roster_slot, &profile_42);
    /* `$85:A82C-$AB16` runs every actor dispatch. The +$60 decision timer
     * controls target refresh, not integer acceleration/integration cadence. */
    nba_gameplay_velocity_step(
        &actor->velocity_x, &actor->velocity_y,
        &actor->movement_boost_timer, direction, profile_42, 2u,
        tipoff->live_state_raw == 0x81u || fp_round(actor->z_fp) != 0,
        (int16_t)tipoff->possession_actor);
    actor->movement_magnitude_raw = actor_distance(
        actor->velocity_x, actor->velocity_y);
    /* `$8E86-$8E9C` turns `$0938=2` into fixed multiplier `$0200` before
     * `$85:AB17`; in the host 24.8 bridge this is velocity8.8 * dt. */
    actor->x_fp += (int32_t)actor->velocity_x * 2;
    actor->y_fp += (int32_t)actor->velocity_y * 2;
    if ((dx > 0 && fp_round(actor->x_fp) > actor->target_x) ||
        (dx < 0 && fp_round(actor->x_fp) < actor->target_x))
        actor->x_fp = (int32_t)actor->target_x * 256;
    if ((dy > 0 && fp_round(actor->y_fp) > actor->target_y) ||
        (dy < 0 && fp_round(actor->y_fp) < actor->target_y))
        actor->y_fp = (int32_t)actor->target_y * 256;
    actor->direction = actor->requested_direction;
    actor_set_animation(actor, direction >= 8u ? 0u :
                        slot == tipoff->handler_actor ? 11u : 3u,
                        direction >= 8u ? 0u : 3u);
    actor->action_state = tipoff->cpu_play_state;
}

static void ball_attach_to_actor(NbaTipoff *tipoff, unsigned owner) {
    /* `$87:B649`, `$87:B66A`, `$87:B832`, `$87:B953`: resolve the current independent upper
     * and lower resources, then compose their ROM attachment tables. */
    NbaTipoffActor *actor = &tipoff->actors[owner];
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        tipoff->actors[i].controller_assignment_raw = -1;
    actor->controller_assignment_raw = 0;
    uint8_t direction = actor->direction < 8u ? actor->direction : 0u;
    uint16_t upper_resource = 0u, lower_resource = 0u;
    int16_t offset_x = 0, offset_y = 0, offset_z = 0;
    uint16_t mirror_flags = direction < 3u ? 0x8000u : 0u;
    bool resolved = nba_player_animation_resources(
        tipoff->assets, actor->animation_state, actor->lower_animation_state,
        direction, actor->upper_animation_tick, actor->lower_animation_tick,
        &upper_resource, &lower_resource) &&
        nba_player_ball_attachment_offsets(
            tipoff->assets, upper_resource, lower_resource, mirror_flags,
            &offset_x, &offset_y, &offset_z);
    /* A validated v26 asset pack makes failure unreachable. Keep ownership
     * deterministic if an externally-corrupted pack reaches this boundary. */
    if (!resolved) offset_x = offset_y = offset_z = 0;
    tipoff->ball.x_fp = actor->x_fp + (int32_t)offset_x * 256;
    tipoff->ball.y_fp = actor->y_fp + (int32_t)offset_y * 256;
    tipoff->ball.z_fp = actor->z_fp + (int32_t)offset_z * 256;
    tipoff->ball.owner_actor = (int8_t)owner;
    tipoff->ball.state = NBA_BALL_ATTACHED;
}

static bool ball_attachment_assets_valid(const NbaAssetPack *assets) {
    static const struct {
        uint16_t upper, lower, flags;
        int16_t x, y, z;
    } vectors[] = {
        {328u, 1388u, 0u,      -6, -2, 48},
        {964u, 1474u, 0x8000u,  7,  7, 23},
        {372u, 1737u, 0x8000u,  7, -1, 30},
    };
    for (unsigned i = 0; i < sizeof(vectors) / sizeof(vectors[0]); ++i) {
        int16_t x, y, z;
        if (!nba_player_ball_attachment_offsets(
                assets, vectors[i].upper, vectors[i].lower, vectors[i].flags,
                &x, &y, &z) || x != vectors[i].x || y != vectors[i].y ||
                z != vectors[i].z) return false;
    }
    return true;
}

static void ball_launch(NbaTipoff *tipoff, int target_x, int target_y,
                        unsigned flight_frames, int vertical_velocity,
                        NbaBallMode mode) {
    if (flight_frames == 0u) flight_frames = 1u;
    tipoff->ball.velocity_x = (int16_t)((target_x * 256 - tipoff->ball.x_fp) /
                                        (int)flight_frames);
    tipoff->ball.velocity_y = (int16_t)((target_y * 256 - tipoff->ball.y_fp) /
                                        (int)flight_frames);
    tipoff->ball.velocity_z = (int16_t)vertical_velocity;
    tipoff->ball.owner_actor = -1;
    tipoff->ball.state = (uint8_t)mode;
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        tipoff->actors[i].controller_assignment_raw = -1;
}

static void score_made_basket(NbaTipoff *tipoff) {
    /* `$85:A079-$A345` is the made-basket branch. `$094C` is added to
     * team-record +$26 (`$4711/$4791`), `$0936` becomes `$82`, and
     * `$0952/$0954` seed the dead-ball/inbound path before `$094C` clears. */
    unsigned scoring_side = tipoff->offense_side & 1u;
    unsigned inbound_side = scoring_side ^ 1u;
    tipoff->session->score[scoring_side] = (uint16_t)(
        tipoff->session->score[scoring_side] + tipoff->shot_value_raw);
    tipoff->last_scoring_side = (uint8_t)scoring_side;
    tipoff->live_state_raw = 0x82u;
    tipoff->inbound_state_raw = (uint16_t)(inbound_side * 5u);
    tipoff->inbound_actor_raw = (uint16_t)(tipoff->inbound_state_raw + 2u);
    tipoff->inbound_timer_raw = 300u;
    /* `$85:A219-$A222`: dead-ball scoring requests play `$01`; B128 later
     * preserves it while resetting the stream at the actor-pass boundary. */
    tipoff->play_code = 1u;
    tipoff->play_request_raw = 1u;
    tipoff->offense_side = (uint8_t)inbound_side;
    tipoff->possession_team = (int8_t)inbound_side;
    tipoff->handler_actor = (uint8_t)tipoff->inbound_actor_raw;
    tipoff->receiver_actor = (uint8_t)(inbound_side * 5u + 1u);
    tipoff->shot_result_resolved = true;
    tipoff->ball.owner_actor = -1;
    tipoff->possession_actor = -1;
}

static void begin_inbound_after_score(NbaTipoff *tipoff) {
    unsigned inbound_side = tipoff->last_scoring_side ^ 1u;
    ++tipoff->possession_number;
    tipoff->shot_value_raw = 0u;
    tipoff->live_state_raw = 1u;
    tipoff->inbound_state_raw = 0u;
    tipoff->inbound_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    tipoff->inbound_timer_raw = 0u;
    cpu_begin_possession(tipoff, (uint8_t)inbound_side);
    tipoff->ball.state = NBA_BALL_INBOUND;
    ball_attach_to_actor(tipoff, tipoff->handler_actor);
}

static NbaGameplayRimResult cpu_update_live_ball(NbaTipoff *tipoff) {
    /* The same `$87:9B0D` 30-Hz logical pass drives `$85:9ACB+` ball
     * collision/integration. Flight-table durations count these due passes. */
    if ((tipoff->simulation_tick & 1u) != 0u)
        return NBA_GAMEPLAY_RIM_FLIGHT;
    NbaTipoffBall *ball = &tipoff->ball;
    int32_t old_x = ball->x_fp, old_y = ball->y_fp, old_z = ball->z_fp;
    bool attached = ball->state == NBA_BALL_ATTACHED;
    if (attached) {
        ball_attach_to_actor(tipoff, tipoff->handler_actor);
    } else if (ball->state == NBA_BALL_PASS || ball->state == NBA_BALL_SHOT ||
               ball->state == NBA_BALL_BOUNCE) {
        bool rom_free_flight = ball->state == NBA_BALL_SHOT ||
                               ball->state == NBA_BALL_BOUNCE;
        if (rom_free_flight)
            ball->velocity_z = (int16_t)(ball->velocity_z - 0x18);
        ball->x_fp += ball->velocity_x;
        ball->y_fp += ball->velocity_y;
        ball->z_fp += ball->velocity_z;
        if (ball->state == NBA_BALL_SHOT) {
            /* `$85:9ACB-$A081` runs after gravity/integration. Its X domain
             * places each hoop at +/-336; the host court uses asymmetric
             * world hoop coordinates, so translate locally at unit scale. */
            int hoop_x = basket_x_for_side(tipoff->offense_side);
            int hoop_y = basket_y_for_side(tipoff->offense_side);
            NbaGameplayRimState rim = {
                fp_round(ball->x_fp), fp_round(ball->y_fp),
                fp_round(ball->z_fp),
                ball->velocity_x, ball->velocity_y, ball->velocity_z,
                tipoff->rim_raw_092c, tipoff->rim_raw_0962,
                tipoff->rim_raw_096a, tipoff->rim_raw_097c,
                tipoff->rim_raw_096e, tipoff->rim_raw_13e7
            };
            bool correct_side = tipoff->handler_actor / 5u ==
                                tipoff->offense_side;
            NbaGameplayRimResult result = nba_gameplay_rim_world_step(
                &rim, (int16_t)hoop_x, (int16_t)hoop_y,
                tipoff->offense_side != 0u, tipoff->live_state_raw, false,
                tipoff->shot_inner_veto_raw, correct_side);
            tipoff->rim_raw_092c = rim.raw_092c;
            tipoff->rim_raw_0962 = rim.raw_0962;
            tipoff->rim_raw_096a = rim.raw_096a;
            tipoff->rim_raw_097c = rim.raw_097c;
            tipoff->rim_raw_096e = rim.raw_096e;
            tipoff->rim_raw_13e7 = rim.raw_13e7;
            if (result == NBA_GAMEPLAY_RIM_OUTER_CONTACT) {
                ball->x_fp = (int32_t)rim.x * 256;
                ball->y_fp = (int32_t)rim.y * 256;
                ball->z_fp = (int32_t)rim.z * 256;
                ball->velocity_x = rim.velocity_x;
                ball->velocity_y = rim.velocity_y;
                ball->velocity_z = rim.velocity_z;
            }
            if (result != NBA_GAMEPLAY_RIM_FLIGHT &&
                result != NBA_GAMEPLAY_RIM_OUTER_CONTACT)
                return result;
            if (result == NBA_GAMEPLAY_RIM_OUTER_CONTACT)
                return result;
        }
        if (!rom_free_flight)
            ball->velocity_z = (int16_t)(ball->velocity_z - 48);
        if (ball->z_fp < 0) {
            ball->z_fp = 0;
            if (rom_free_flight) {
                /* `$85:A3B7-$A4DA`: gravity precedes integration; ground
                 * impact applies 7/8 vertical restitution (cap $0400) and
                 * impact-only 15/16 lateral damping. */
                int16_t original_vz = ball->velocity_z;
                int rebound = -(int)original_vz +
                    nba_gameplay_arithmetic_shift_right(original_vz, 3);
                if (rebound > 0x0400) rebound = 0x0400;
                ball->velocity_z = (int16_t)rebound;
                ball->velocity_x = (int16_t)(ball->velocity_x -
                    nba_gameplay_arithmetic_shift_right(ball->velocity_x, 4));
                ball->velocity_y = (int16_t)(ball->velocity_y -
                    nba_gameplay_arithmetic_shift_right(ball->velocity_y, 4));
                ball->state = NBA_BALL_BOUNCE;
            } else {
                ball->velocity_z = (int16_t)(-ball->velocity_z / 2);
                ball->velocity_x = (int16_t)(ball->velocity_x * 3 / 4);
                ball->velocity_y = (int16_t)(ball->velocity_y * 3 / 4);
                ball->state = NBA_BALL_BOUNCE;
            }
        }
    }
    if (attached) {
        ball->velocity_x = (int16_t)(ball->x_fp - old_x);
        ball->velocity_y = (int16_t)(ball->y_fp - old_y);
        ball->velocity_z = (int16_t)(ball->z_fp - old_z);
    }
    return NBA_GAMEPLAY_RIM_FLIGHT;
}

static void cpu_update_tip_ball(NbaTipoff *tipoff) {
    NbaTipoffBall *ball = &tipoff->ball;
    int32_t old_x = ball->x_fp, old_y = ball->y_fp, old_z = ball->z_fp;
    if (tipoff->frame < NBA_TIPOFF_CONTACT_FRAME) {
        int t = tipoff->frame - 170;
        int z = tipoff->frame < NBA_TIPOFF_TOSS_FRAME ? 80 :
            108 - (t * t * (t < 0 ? 28 : 40)) / (t < 0 ? 625 : 784);
        ball->x_fp = ball->y_fp = 0;
        ball->z_fp = (int32_t)z * 256;
        ball->state = tipoff->frame < NBA_TIPOFF_BALL_APPEAR_FRAME ?
                      NBA_BALL_HIDDEN : NBA_BALL_TOSS;
    } else {
        int t = tipoff->frame - NBA_TIPOFF_CONTACT_FRAME;
        if (t > 22) t = 22;
        ball->x_fp = (int32_t)(-92 * t) * 256 / 22;
        ball->y_fp = (int32_t)(36 * t) * 256 / 22;
        ball->z_fp = (int32_t)(67 + (14 - 67) * t / 22) * 256;
        ball->state = NBA_BALL_LOOSE;
    }
    ball->velocity_x = (int16_t)(ball->x_fp - old_x);
    ball->velocity_y = (int16_t)(ball->y_fp - old_y);
    ball->velocity_z = (int16_t)(ball->z_fp - old_z);
}

static bool cpu_load_next_play_control(NbaTipoff *tipoff) {
    NbaGameplayPlayControlRecord record;
    uint8_t count = 0u;
    int next = tipoff->play_step_raw + 1;
    if (!nba_assets_gameplay_play_control(
            tipoff->assets, (uint8_t)tipoff->play_code, (uint8_t)next,
            &record, &count)) {
        if (next < count) return false;
        /* `$85:B309-$B326`: `$09D0` repeats the final record; otherwise the
         * `$23BA` sentinel sets `$09A4`, rewinds `$0998`, and loads record 0. */
        if (tipoff->play_hold_raw != 0u) {
            next = count ? (int)count - 1 : 0;
        } else {
            tipoff->play_cycle_raw = 1u;
            next = 0;
        }
        if (!nba_assets_gameplay_play_control(
                tipoff->assets, (uint8_t)tipoff->play_code, (uint8_t)next,
                &record, &count)) return false;
    }
    tipoff->play_step_raw = (int16_t)next;
    if (record.countdown < 0) {
        tipoff->play_countdown_raw = 0;
        tipoff->play_event_wait_raw = 1u;
    } else {
        tipoff->play_countdown_raw = record.countdown;
        tipoff->play_event_wait_raw = 0u;
    }
    /* DP `$9E` is the active team context; it is independent of the camera's
     * persistent free-ball side proxy `$093A`. */
    int side_base = tipoff->offense_side ? 5 : 0;
    const int16_t source[3] = {
        record.selector_a, record.selector_b, record.selector_c
    };
    for (unsigned i = 0; i < 3u; ++i)
        tipoff->play_selector_raw[i] = source[i] < 0 ? source[i] :
                                       (int16_t)(source[i] + side_base);
    if (tipoff->live_state_raw == 0x82u)
        tipoff->play_cycle_raw = 0u; /* `$85:B31E-$B326` */
    return true;
}

static void cpu_advance_play_control(NbaTipoff *tipoff) {
    unsigned base = tipoff->offense_side ? 5u : 0u;
    /* `$85:B28B-$B2AF`: every record boundary consumes readiness bits 08/40
     * for this side only, then may raise the randomized transition flag. */
    for (unsigned i = 0; i < 5u; ++i)
        tipoff->actors[base + i].behavior_flags_raw &= 0xFFB7u;
    if (tipoff->play_step_raw >= 2 && (tipoff->rng.state & 1u) != 0u &&
        tipoff->play_hold_raw == 0u)
        tipoff->play_cycle_raw = 1u;
    (void)cpu_load_next_play_control(tipoff);
}

static void cpu_reset_play_control(NbaTipoff *tipoff) {
    /* `$85:B377`: initialize the stream registers, then fall through
     * `$85:B28B/$B2DC` to load record zero. */
    tipoff->play_step_raw = -1;
    tipoff->play_countdown_raw = 0;
    tipoff->play_event_wait_raw = 0u;
    tipoff->play_cycle_raw = 0u;
    tipoff->play_mirror_raw = tipoff->play_code >= 0x12u ?
        (tipoff->rng.state & 1u) : 0u;
    for (unsigned i = 0; i < 3u; ++i) tipoff->play_selector_raw[i] = -1;
    cpu_advance_play_control(tipoff);
}

static void cpu_reselect_play_control(NbaTipoff *tipoff) {
    /* `$85:B128-$B24B`: `$0994` is consumed only at the completed logical
     * pass boundary. The first RNG result is intentionally discarded. */
    (void)nba_gameplay_rng_next(&tipoff->rng);
    tipoff->play_request_raw = 0u;
    tipoff->play_event_wait_raw = 0u;
    tipoff->play_cycle_raw = 0u;
    if (tipoff->live_state_raw == 0x82u) {
        /* `$85:B176/$B245`: made-score state preserves `$0996` and `$09D0`
         * while B377 rewinds the selected stream to record zero. */
        cpu_reset_play_control(tipoff);
        return;
    }

    /* The currently represented context has +$2E!=7 and +$56==0. Preserve
     * the exact normal-path RNG rejection order before the team/coin table. */
    uint8_t preliminary;
    do preliminary = (uint8_t)(nba_gameplay_rng_next(&tipoff->rng) & 7u);
    while (preliminary >= 6u);
    (void)preliminary;
    uint8_t coin = (uint8_t)(nba_gameplay_rng_next(&tipoff->rng) & 1u);
    uint8_t team = tipoff->offense_side ? tipoff->session->right_team :
                                          tipoff->session->left_team;
    uint8_t strategy, base, count;
    bool hold;
    if (!nba_assets_gameplay_cpu_strategy(
            tipoff->assets, team, coin, &strategy, &base, &count, &hold))
        return;
    (void)strategy;
    uint8_t offset;
    do offset = (uint8_t)(nba_gameplay_rng_next(&tipoff->rng) & 7u);
    while (offset >= count);
    tipoff->play_code = (uint16_t)(base + offset);
    tipoff->play_hold_raw = hold ? 1u : 0u;
    cpu_reset_play_control(tipoff);
}

static void cpu_update_play_control(NbaTipoff *tipoff) {
    if ((tipoff->simulation_tick & 1u) != 0u) return;
    if (tipoff->play_request_raw != 0u) {
        cpu_reselect_play_control(tipoff);
        return;
    }
    int16_t remaining = (int16_t)(uint16_t)(
        (uint16_t)tipoff->play_countdown_raw - 2u);
    if (remaining >= 0) {
        tipoff->play_countdown_raw = remaining;
        return;
    }
    if (tipoff->play_event_wait_raw != 0u) {
        unsigned base = tipoff->offense_side ? 5u : 0u;
        for (unsigned i = 0; i < 5u; ++i) {
            const NbaTipoffActor *actor = &tipoff->actors[base + i];
            if (actor->controller_assignment_raw < 0 &&
                (actor->behavior_flags_raw & 0x0040u) == 0u) {
                /* `$85:B271-$B279/$B353`: retain the signed underflow and
                 * retry this same five-actor barrier next logical pass. */
                tipoff->play_countdown_raw = remaining;
                return;
            }
        }
        tipoff->play_event_wait_raw = 0u; /* `$85:B288` */
    }
    cpu_advance_play_control(tipoff);
}

static void cpu_begin_possession(NbaTipoff *tipoff, uint8_t offense_side) {
    static const uint8_t play_codes[4] = {0x35u, 0x01u, 0x0Fu, 0x26u};
    static const uint8_t handler_slots[4] = {3u, 3u, 2u, 2u};
    tipoff->offense_side = offense_side;
    tipoff->possession_team = (int8_t)offense_side;
    tipoff->camera_side_group_raw = offense_side ? 5u : 0u;
    tipoff->possession_frame = 0;
    tipoff->play_state_frame = 0;
    tipoff->play_code = play_codes[tipoff->possession_number % 4u];
    tipoff->play_hold_raw = 0u;
    cpu_reset_play_control(tipoff);
    unsigned base = offense_side ? 5u : 0u;
    tipoff->handler_actor = (uint8_t)(base + handler_slots[tipoff->possession_number % 4u]);
    tipoff->receiver_actor = (uint8_t)(base +
        ((handler_slots[tipoff->possession_number % 4u] + 1u) % 5u));
    tipoff->possession_actor = (int8_t)tipoff->handler_actor;
    tipoff->cpu_play_state = NBA_CPU_PLAY_BREAK;
    tipoff->shot_result_resolved = false;
    tipoff->shot_inner_veto_raw = false;
    tipoff->shot_miss_index_raw = 0xFFu;
    tipoff->shot_value_raw = 0u;
    ball_attach_to_actor(tipoff, tipoff->handler_actor);
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor)
        tipoff->actors[actor].control_mode =
            actor / 5u == offense_side ? 1u : 2u;
    tipoff->actors[tipoff->handler_actor].control_mode = 11u;
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaTipoffActor *state = &tipoff->actors[actor];
        state->reaction_threshold = nba_gameplay_reaction_threshold(
            &tipoff->rng, fp_round(state->x_fp), fp_round(state->y_fp),
            fp_round(tipoff->ball.x_fp), fp_round(tipoff->ball.y_fp));
    }
}

static void cpu_commit_rebound(NbaTipoff *tipoff, uint8_t catcher) {
    /* `$86:BAA2/$86:BAEE` commits the collision winner to `$093E`; only
     * that player/ball collision may select the next offense. */
    unsigned side = catcher / 5u;
    ++tipoff->possession_number;
    cpu_begin_possession(tipoff, (uint8_t)side);
    tipoff->handler_actor = catcher;
    tipoff->receiver_actor = (uint8_t)(side * 5u + ((catcher + 1u) % 5u));
    ball_attach_to_actor(tipoff, catcher);
    tipoff->possession_actor = (int8_t)catcher;
}

static void cpu_enter_play_state(NbaTipoff *tipoff, NbaCpuPlayState state) {
    tipoff->cpu_play_state = (uint8_t)state;
    tipoff->play_state_frame = 0u;
}

static void cpu_update_all_actors(NbaTipoff *tipoff) {
    /* `$87:8F01-$8F8D` updates all ten actors as one logical pass with
     * `$C6/$0938=2`. */
    /* `$87:8EFB-$8F92` is one global 30-Hz pass with `$0938/$C6=2`.
     * Possession and inbound changes do not rephase it. */
    if ((tipoff->simulation_tick & 1u) != 0u) return;
    ++tipoff->actor_update_tick;
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaTipoffActor *state = &tipoff->actors[actor];
        if (state->recovery_inhibit_raw != 0u)
            state->recovery_inhibit_raw = state->recovery_inhibit_raw > 2u ?
                (uint16_t)(state->recovery_inhibit_raw - 2u) : 0u;
        cpu_move_actor(tipoff, actor);
        state->behavior_timer = state->behavior_timer >= 2u ?
            (uint16_t)(state->behavior_timer - 2u) :
            (uint16_t)(state->behavior_timer + 45u);
        ++state->upper_animation_tick;
        ++state->lower_animation_tick;
    }
}

static void cpu_update_possession(NbaTipoff *tipoff) {
    if (tipoff->live_state_raw == 0x82u) {
        /* `$85:A262-$A268` seeds `$092E/$0A04=300`. The inbound steering
         * path `$86:F43A+` changes behavior at 240/120/60 and does not stop
         * the made ball from falling through the net. */
        if (tipoff->inbound_actor_raw >= NBA_GAMEPLAY_ACTOR_COUNT)
            return;
        cpu_set_role_targets(tipoff);
        NbaTipoffActor *inbounder = &tipoff->actors[tipoff->inbound_actor_raw];
        bool right_baseline = tipoff->ball.x_fp >= 0;
        inbounder->target_x = (int16_t)(right_baseline ? 394 : -394);
        inbounder->target_y = (int16_t)(right_baseline ? -64 : 64);
        cpu_update_all_actors(tipoff);
        cpu_update_play_control(tipoff);
        (void)cpu_update_live_ball(tipoff);
        if (tipoff->inbound_timer_raw > 0u)
            --tipoff->inbound_timer_raw;
        bool inbound_pass_ready = tipoff->ball.state == NBA_BALL_INBOUND;
        if (tipoff->inbound_timer_raw <= 120u) {
            unsigned inbound = tipoff->inbound_actor_raw;
            int dx = inbounder->target_x - fp_round(inbounder->x_fp);
            int dy = inbounder->target_y - fp_round(inbounder->y_fp);
            if (actor_distance(dx, dy) <= 8u ||
                tipoff->inbound_timer_raw == 0u) {
                ball_attach_to_actor(tipoff, inbound);
                tipoff->ball.state = NBA_BALL_INBOUND;
            }
        }
        if (tipoff->inbound_timer_raw <= 60u && inbound_pass_ready)
            begin_inbound_after_score(tipoff);
        ++tipoff->possession_frame;
        ++tipoff->play_state_frame;
        return;
    }
    cpu_set_role_targets(tipoff);
    cpu_update_all_actors(tipoff);
    NbaGameplayRimResult rim_result = cpu_update_live_ball(tipoff);
    cpu_update_play_control(tipoff);

    NbaTipoffActor *handler = &tipoff->actors[tipoff->handler_actor];
    NbaTipoffActor *receiver = &tipoff->actors[tipoff->receiver_actor];
    int handler_distance = actor_distance(handler->target_x - fp_round(handler->x_fp),
                                          handler->target_y - fp_round(handler->y_fp));
    int receiver_distance = actor_distance(fp_round(receiver->x_fp) -
                                           fp_round(tipoff->ball.x_fp),
                                           fp_round(receiver->y_fp) -
                                           fp_round(tipoff->ball.y_fp));
    switch ((NbaCpuPlayState)tipoff->cpu_play_state) {
        case NBA_CPU_PLAY_BREAK:
            if (handler_distance < 28)
                cpu_enter_play_state(tipoff, NBA_CPU_PLAY_DRIVE);
            break;
        case NBA_CPU_PLAY_DRIVE:
            if (handler_distance < 18) {
                cpu_enter_play_state(tipoff, NBA_CPU_PLAY_PASS);
                handler->control_mode = 15u;
                receiver->control_mode = 10u;
                ball_launch(tipoff, fp_round(receiver->x_fp),
                            fp_round(receiver->y_fp), 38u, 704, NBA_BALL_PASS);
                tipoff->possession_actor = -1;
            }
            break;
        case NBA_CPU_PLAY_PASS:
            if (receiver_distance < 14 || tipoff->ball.z_fp <= 0) {
                tipoff->handler_actor = tipoff->receiver_actor;
                tipoff->receiver_actor = (uint8_t)(
                    (tipoff->handler_actor / 5u) * 5u +
                    ((tipoff->handler_actor + 2u) % 5u));
                ball_attach_to_actor(tipoff, tipoff->handler_actor);
                tipoff->possession_actor = (int8_t)tipoff->handler_actor;
                cpu_enter_play_state(tipoff, NBA_CPU_PLAY_ATTACK);
                tipoff->actors[tipoff->handler_actor].control_mode = 11u;
            }
            break;
        case NBA_CPU_PLAY_ATTACK:
            if (handler_distance < 24) {
                int basket_x = basket_x_for_side(tipoff->offense_side);
                int basket_y = basket_y_for_side(tipoff->offense_side);
                tipoff->shot_origin_x = fp_round(handler->x_fp);
                tipoff->shot_origin_y = fp_round(handler->y_fp);
                /* The host court is centered at X=140 with hoops ±246; map
                 * release X into the ROM's ±336 coordinate domain before
                 * applying `$85:ABFB`. */
                int16_t release_x_rom = (int16_t)(
                    (tipoff->shot_origin_x - 140) * 336 / 246);
                tipoff->shot_value_raw = nba_gameplay_shot_value(
                    false, release_x_rom, tipoff->shot_origin_y,
                    tipoff->offense_side != 0u);
                uint8_t team = tipoff->offense_side ?
                    tipoff->session->right_team : tipoff->session->left_team;
                uint8_t rating_2pt = 0xA8u, rating_3pt = 0xA8u;
                (void)nba_player_gameplay_shot_ratings(
                    tipoff->assets, team,
                    tipoff->actors[tipoff->handler_actor].roster_slot,
                    &rating_2pt, &rating_3pt);
                uint8_t rating = tipoff->shot_value_raw == 3u ?
                                 rating_3pt : rating_2pt;
                uint8_t difficulty = (uint8_t)tipoff->session->config.main_values[2];
                /* Raw actor +$8C/+16 modifiers remain explicit defaults until
                 * their CPU writers are captured; the rating tiers, difficulty
                 * tables, RNG consumption, `$09F8`, and miss table are exact. */
                tipoff->shot_chance_raw = nba_gameplay_shot_chance(
                    rating, rating, difficulty, true);
                uint8_t roll = (uint8_t)nba_gameplay_rng_next(&tipoff->rng);
                tipoff->shot_inner_veto_raw = roll >= tipoff->shot_chance_raw;
                tipoff->shot_miss_index_raw = 0xFFu;
                if (tipoff->shot_inner_veto_raw) {
                    int16_t miss_x, miss_y;
                    tipoff->shot_miss_index_raw =
                        (uint8_t)(nba_gameplay_rng_next(&tipoff->rng) & 0x0Fu);
                    nba_gameplay_miss_offset(tipoff->shot_miss_index_raw,
                        tipoff->offense_side == 0u, &miss_x, &miss_y);
                    basket_x += miss_x;
                    basket_y += miss_y;
                }
                cpu_enter_play_state(tipoff, NBA_CPU_PLAY_SHOT);
                nba_gameplay_shot_launch(tipoff->ball.x_fp,
                    tipoff->ball.y_fp, tipoff->ball.z_fp,
                    (int16_t)basket_x, (int16_t)basket_y,
                    &tipoff->ball.velocity_x, &tipoff->ball.velocity_y,
                    &tipoff->ball.velocity_z);
                tipoff->ball.owner_actor = -1;
                tipoff->ball.state = NBA_BALL_SHOT;
                for (unsigned actor_index = 0;
                     actor_index < NBA_GAMEPLAY_ACTOR_COUNT; ++actor_index)
                    tipoff->actors[actor_index].controller_assignment_raw = -1;
                handler->control_mode = 12u;
                actor_set_animation(handler, 0x31u, 0x03u);
                tipoff->possession_actor = -1;
            }
            break;
        case NBA_CPU_PLAY_SHOT:
            if (!tipoff->shot_result_resolved &&
                rim_result == NBA_GAMEPLAY_RIM_MAKE) {
                score_made_basket(tipoff);
                cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
            } else if (!tipoff->shot_result_resolved &&
                       (rim_result == NBA_GAMEPLAY_RIM_EDGE_CONTACT ||
                        rim_result == NBA_GAMEPLAY_RIM_MISS)) {
                /* The classifier is exact. Later edge/miss impulses still
                 * depend on unresolved WRAM/RNG, so enter the proven loose
                 * ball recovery path without inventing a second contact. */
                tipoff->shot_result_resolved = true;
                tipoff->ball.state = NBA_BALL_BOUNCE;
                cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
            } else if (tipoff->ball.z_fp <= 32 * 256 &&
                tipoff->ball.velocity_z < 0) {
                cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
                tipoff->ball.state = NBA_BALL_BOUNCE;
            }
            break;
        case NBA_CPU_PLAY_REBOUND:
            if (tipoff->ball.z_fp <= 24 * 256) {
                uint8_t catcher = NBA_GAMEPLAY_NO_ACTOR;
                uint16_t nearest = 0xFFFFu;
                for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
                    int dx = fp_round(tipoff->actors[actor].x_fp) -
                             fp_round(tipoff->ball.x_fp);
                    int dy = fp_round(tipoff->actors[actor].y_fp) -
                             fp_round(tipoff->ball.y_fp);
                    uint16_t distance = actor_distance(dx, dy);
                    if (distance < nearest) {
                        nearest = distance;
                        catcher = (uint8_t)actor;
                    }
                }
                if (catcher != NBA_GAMEPLAY_NO_ACTOR && nearest <= 14u)
                    cpu_commit_rebound(tipoff, catcher);
            }
            break;
    }

    ++tipoff->possession_frame;
    ++tipoff->play_state_frame;
}

static void cpu_update_camera(NbaTipoff *tipoff) {
    /* `$87:A9D0-$A9E2/$87:95BB-$95D8`: signed `$093E` selects an actor;
     * FFFF substitutes the ball record before `$85:9192-$93F4` consumes the
     * proxy. `$093A` is persistent and independent from the free ball. */
    if (tipoff->frame < NBA_TIPOFF_POSSESSION_FRAME) return;
    if (tipoff->possession_actor >= 0 &&
        tipoff->possession_actor < NBA_GAMEPLAY_ACTOR_COUNT) {
        unsigned subject = (unsigned)tipoff->possession_actor;
        tipoff->camera.subject_actor = (uint8_t)subject;
        tipoff->camera_side_group_raw = subject >= 5u ? 5u : 0u;
        nba_gameplay_camera_update(&tipoff->camera,
            fp_round(tipoff->actors[subject].x_fp),
            fp_round(tipoff->actors[subject].y_fp),
            tipoff->camera_side_group_raw);
    } else {
        tipoff->camera.subject_actor = NBA_GAMEPLAY_NO_ACTOR;
        nba_gameplay_camera_update(&tipoff->camera,
            fp_round(tipoff->ball.x_fp), fp_round(tipoff->ball.y_fp),
            tipoff->camera_side_group_raw);
    }
    tipoff->camera_x = tipoff->camera.x;
    tipoff->camera_y = tipoff->camera.y;
}

static void draw_ball(const NbaTipoff *tipoff, NbaRenderer *ren, int x, int y) {
    const NbaAssetItem *item = nba_assets_get(tipoff->assets, NBA_ASSET_TIPOFF_BALL);
    if (!item || !item->data || item->size != 56u ||
        memcmp(item->data, "NBBALL1", 8)) return;
    const uint8_t *data = (const uint8_t *)item->data;
    const uint8_t *tile = data + 12;
    for (int py = 0; py < 8; ++py) for (int px = 0; px < 8; ++px) {
        uint8_t index = tile_pixel(tile, px, py);
        if (index < 5u || index > 10u) continue;
        int dx = x + px, dy = y + py;
        if (dx >= 0 && dx < 256 && dy >= 0 && dy < 224)
            ren->pixels[dy * 256 + dx] = bgr555(read_u16(data + 44 + (index - 5u) * 2u));
    }
}

bool nba_tipoff_init(NbaTipoff *tipoff, const NbaAssetPack *assets,
                     NbaSession *session) {
    if (!tipoff || !assets || !session ||
        !nba_gameplay_rng_self_test() || !nba_gameplay_ai_self_test() ||
        !nba_gameplay_ball_self_test() ||
        !ball_attachment_assets_valid(assets) ||
        !nba_assets_gameplay_court_panorama(assets, session->right_team) ||
        !nba_assets_get(assets, NBA_ASSET_TIPOFF_BALL)) return false;
    memset(tipoff, 0, sizeof(*tipoff));
    tipoff->assets = assets;
    tipoff->session = session;
    tipoff->cpu_vs_cpu = true;
    tipoff->camera_x = -128;
    tipoff->camera_y = -124;
    nba_gameplay_camera_init(&tipoff->camera, -128, -124);
    nba_gameplay_rng_seed(&tipoff->rng, 0x9146u);
    tipoff->possession_actor = -1;
    tipoff->possession_team = -1;
    session->score[0] = session->score[1] = 0u;
    session->game_clock_ticks = 0u;
    tipoff->live_state_raw = 1u;
    tipoff->inbound_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    tipoff->ball.x_fp = 0;
    tipoff->ball.y_fp = 0;
    tipoff->ball.z_fp = 80 * 256;
    tipoff->ball.owner_actor = -1;
    static const uint8_t active_lineup[5] = {2u, 0u, 1u, 3u, 4u};
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaTipoffActor *state = &tipoff->actors[actor];
        state->x_fp = (int32_t)formation[actor].world_x * 256;
        state->y_fp = (int32_t)formation[actor].world_y * 256;
        state->direction = formation[actor].direction;
        state->roster_slot = active_lineup[actor % 5u];
        state->requested_direction = formation[actor].direction;
        state->movement_direction = formation[actor].direction;
        state->saved_control_mode = 0u;
        state->controller_assignment_raw = -1;
        state->lower_animation_state = 0u;
        state->assignment_actor = (uint8_t)((actor + 5u) % 10u);
        state->assignment_base_raw = (uint16_t)(state->assignment_actor * 2u);
        state->assignment_current_raw = state->assignment_base_raw;
        state->assignment_alternate_raw = state->assignment_base_raw;
        state->reaction_threshold = nba_gameplay_reaction_threshold(
            &tipoff->rng, formation[actor].world_x, formation[actor].world_y,
            0, 0);
        state->behavior_timer = 0x2Fu;
        state->control_mode = actor == 0u || actor == 5u ? 4u : 2u;
        state->visible = actor != 4u && actor != 9u;
    }
    tipoff->is_initialized = true;
    printf("[TIPOFF] $86:DDA7 formation -> $86:E054 ball -> "
           "$86:ECF4 jump -> $86:D3F9 possession.\n");
    return true;
}

void nba_tipoff_update(NbaTipoff *tipoff, const NbaInput *input) {
    (void)input;
    if (!tipoff || !tipoff->is_initialized) return;
    ++tipoff->frame;
    ++tipoff->simulation_tick;
    ++tipoff->session->game_clock_ticks;
    if (tipoff->frame < NBA_TIPOFF_BREAK_FRAME)
        cpu_update_tip_ball(tipoff);
    if (tipoff->frame == NBA_TIPOFF_POSSESSION_FRAME) {
        /* `$85:B100-$B28B` resolves the tip and writes play code $35.
         * `$87:9244/$9BD0` then dispatches actor +$5E behavior modes. */
        tipoff->possession_actor = 8;
        tipoff->possession_team = 1;
        tipoff->camera_side_group_raw = 5u;
        tipoff->play_code = 0x35u;
        for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor)
            tipoff->actors[actor].control_mode = actor >= 5u ? 1u : 2u;
        tipoff->actors[8].control_mode = 10u;
    }
    if (tipoff->frame == NBA_TIPOFF_BREAK_FRAME) {
        cpu_begin_possession(tipoff, 1u);
    }
    if (tipoff->frame >= NBA_TIPOFF_BREAK_FRAME) {
        cpu_update_possession(tipoff);
        tipoff->possession_actor = tipoff->ball.owner_actor;
    }
    cpu_update_camera(tipoff);
    if (tipoff->frame >= NBA_TIPOFF_BREAK_FRAME) tipoff->phase = NBA_TIPOFF_LIVE;
    else if (tipoff->frame >= NBA_TIPOFF_POSSESSION_FRAME)
        tipoff->phase = NBA_TIPOFF_POSSESSION;
    else if (tipoff->frame >= NBA_TIPOFF_TOSS_FRAME)
        tipoff->phase = NBA_TIPOFF_JUMP_BALL;
}

static int center_jump_height(int frame) {
    if (frame < NBA_TIPOFF_JUMP_FRAME || frame >= NBA_TIPOFF_POSSESSION_FRAME)
        return 0;
    int t = frame - NBA_TIPOFF_JUMP_FRAME;
    /* Mesen actor +$0C: 7,15,18,20,20,18,12,3 across frames 160..195. */
    int d = t - 20;
    int height = 20 - (d * d) / 20;
    return height > 0 ? height : 0;
}

static void ball_position(int frame, int *x, int *y) {
    if (frame < NBA_TIPOFF_TOSS_FRAME) { *x = 125; *y = 40; return; }
    if (frame < NBA_TIPOFF_CONTACT_FRAME) {
        int t = frame - 170;
        int z = 108 - (t * t * (t < 0 ? 28 : 40)) / (t < 0 ? 625 : 784);
        *x = 125; *y = 120 - z; return;
    }
    int t = frame - NBA_TIPOFF_CONTACT_FRAME;
    if (t > 22) t = 22;
    *x = 125 + (38 - 125) * t / 22;
    *y = 51 + (83 - 51) * t / 22;
}

static bool actor_visible(unsigned actor) {
    for (unsigned i = 0; i < sizeof(visible_submission); ++i)
        if (visible_submission[i] == actor) return true;
    return false;
}

static uint8_t actor_animation(const NbaTipoff *tipoff, unsigned actor) {
    if (actor == 5u && tipoff->frame >= NBA_TIPOFF_JUMP_FRAME &&
        tipoff->frame < NBA_TIPOFF_CONTACT_FRAME) return 0x32u;
    if (actor == 5u && tipoff->frame >= NBA_TIPOFF_CONTACT_FRAME &&
        tipoff->frame < 210) return 0x25u;
    if (actor == 0u && tipoff->frame >= NBA_TIPOFF_CONTACT_FRAME &&
        tipoff->frame < 220) return 0x37u;
    return 0;
}

void nba_tipoff_capture_telemetry(const NbaTipoff *tipoff,
                                  const NbaInput *input,
                                  NbaGameplayTelemetry *telemetry) {
    if (!tipoff || !telemetry) return;
    memset(telemetry, 0, sizeof(*telemetry));
    telemetry->scene_frame = (uint32_t)tipoff->frame;
    telemetry->simulation_tick = tipoff->simulation_tick;
    telemetry->phase = (uint8_t)tipoff->phase;
    telemetry->scheduler_due_raw =
        (uint8_t)((tipoff->simulation_tick & 1u) == 0u);
    telemetry->actor_pass_dt_raw = telemetry->scheduler_due_raw ? 2u : 0u;
    telemetry->actor_pass_mask_raw = telemetry->scheduler_due_raw ? 0x03FFu : 0u;
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor)
        telemetry->actor_pass_order_raw[actor] = telemetry->scheduler_due_raw ?
            (uint8_t)actor : 0xFFu;
    telemetry->input_pressed = input ? input->pressed : 0u;
    telemetry->input_held = input ? input->held : 0u;
    telemetry->input_released = input ? input->released : 0u;
    telemetry->pad_held_raw[0] = (uint16_t)(telemetry->input_held & 0x0FFFu);
    for (unsigned pad = 0; pad < NBA_GAMEPLAY_PAD_COUNT; ++pad) {
        telemetry->controller_assignment_raw[pad] = NBA_GAMEPLAY_UNKNOWN_WORD;
        telemetry->controller_repeat_raw[pad] = NBA_GAMEPLAY_UNKNOWN_WORD;
    }
    telemetry->active_controller_raw = -1;
    telemetry->selected_controller_raw = -1;
    telemetry->controlled_side_raw = -1;
    telemetry->initial_controlled_slot_raw = 0;
    telemetry->selected_slot_raw = -1;
    telemetry->controlled_actor = NBA_GAMEPLAY_NO_ACTOR;
    telemetry->controlled_actor_pointer_raw = 0u;
    telemetry->possession_actor = tipoff->possession_actor;
    telemetry->possession_team = tipoff->possession_team;
    telemetry->possession_candidate_raw = tipoff->frame >= NBA_TIPOFF_BREAK_FRAME ?
                                          tipoff->handler_actor : -1;
    telemetry->play_code_raw = tipoff->frame >= NBA_TIPOFF_POSSESSION_FRAME ?
                               tipoff->play_code : NBA_GAMEPLAY_UNKNOWN_WORD;
    telemetry->play_step_raw = tipoff->play_step_raw;
    telemetry->play_countdown_raw = tipoff->play_countdown_raw;
    telemetry->play_mirror_raw = tipoff->play_mirror_raw;
    telemetry->play_event_wait_raw = tipoff->play_event_wait_raw;
    telemetry->play_request_raw = tipoff->play_request_raw;
    telemetry->play_cycle_raw = tipoff->play_cycle_raw;
    telemetry->play_hold_raw = tipoff->play_hold_raw;
    for (unsigned i = 0; i < 3u; ++i)
        telemetry->play_selector_raw[i] = tipoff->play_selector_raw[i];
    telemetry->rng_state_raw = tipoff->rng.state;
    telemetry->score_left_raw = tipoff->session->score[0];
    telemetry->score_right_raw = tipoff->session->score[1];
    telemetry->shot_value_raw = tipoff->shot_value_raw;
    telemetry->shot_chance_raw = tipoff->shot_chance_raw;
    telemetry->shot_miss_index_raw = tipoff->shot_miss_index_raw;
    telemetry->shot_inner_veto_raw = tipoff->shot_inner_veto_raw ? 1u : 0u;
    telemetry->live_state_raw = tipoff->live_state_raw;
    telemetry->inbound_state_raw = tipoff->inbound_state_raw;
    telemetry->inbound_actor_raw = tipoff->inbound_actor_raw;
    telemetry->inbound_timer_raw = tipoff->inbound_timer_raw;
    telemetry->collision_actor_a = tipoff->frame >= NBA_TIPOFF_CONTACT_FRAME &&
                                   tipoff->frame < 211 ? 0 : -1;
    telemetry->collision_actor_b = tipoff->frame >= NBA_TIPOFF_CONTACT_FRAME &&
                                   tipoff->frame < 211 ? 5 : -1;
    telemetry->controller_routine = 0x80CB8Fu;
    telemetry->selection_routine = 0x85C37Du;
    telemetry->collision_routine = tipoff->frame >= NBA_TIPOFF_CONTACT_FRAME ?
                                   SNES_ADDR_TIPOFF_CONTACT : 0u;
    telemetry->possession_routine = tipoff->frame >= NBA_TIPOFF_POSSESSION_FRAME ?
                                    SNES_ADDR_TIPOFF_POSSESSION : 0u;
    telemetry->camera_x = tipoff->camera_x;
    telemetry->camera_y = tipoff->camera_y;
    telemetry->camera_subject_raw = tipoff->camera.subject_actor ==
        NBA_GAMEPLAY_NO_ACTOR ? -1 : (int16_t)tipoff->camera.subject_actor;
    telemetry->camera_side_group_raw = tipoff->camera_side_group_raw;
    telemetry->camera_085c_raw = (uint16_t)tipoff->camera.x;
    telemetry->camera_085e_raw = (uint16_t)tipoff->camera.previous_x;
    telemetry->camera_0860_raw = (uint16_t)tipoff->camera.y;
    telemetry->camera_0862_raw = (uint16_t)tipoff->camera.previous_y;
    telemetry->camera_086c_raw = tipoff->camera.coarse_x;
    telemetry->camera_086e_raw = tipoff->camera.coarse_y;
    telemetry->camera_0874_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    telemetry->camera_0876_raw = tipoff->camera.stream_source;
    telemetry->camera_0878_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    telemetry->camera_087a_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    telemetry->camera_routine = 0x859192u;

    telemetry->ball.world_x = fp_round(tipoff->ball.x_fp);
    telemetry->ball.world_y = fp_round(tipoff->ball.y_fp);
    telemetry->ball.world_z = fp_round(tipoff->ball.z_fp);
    telemetry->ball.screen_x = (int16_t)(telemetry->ball.world_x +
        telemetry->ball.world_y - tipoff->camera_x);
    telemetry->ball.screen_y = (int16_t)((telemetry->ball.world_y -
        telemetry->ball.world_x) / 4 - tipoff->camera_y -
        telemetry->ball.world_z);
    telemetry->ball.velocity_x = fp_round(tipoff->ball.velocity_x);
    telemetry->ball.velocity_y = fp_round(tipoff->ball.velocity_y);
    telemetry->ball.velocity_z = fp_round(tipoff->ball.velocity_z);
    telemetry->ball.owner_actor = tipoff->ball.owner_actor;
    telemetry->ball.state = tipoff->ball.state;
    telemetry->ball.routine = tipoff->frame < NBA_TIPOFF_BREAK_FRAME ?
        SNES_ADDR_TIPOFF_BALL_INIT :
        tipoff->ball.state == NBA_BALL_ATTACHED ? 0x87B649u : 0x85A518u;
    telemetry->ball.flags_raw = NBA_GAMEPLAY_UNKNOWN_WORD;

    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaGameplayActorTelemetry *out = &telemetry->actors[actor];
        const NbaTipoffActor *state = &tipoff->actors[actor];
        out->index = (uint8_t)actor;
        out->team_side = actor >= 5u;
        out->roster_slot = state->roster_slot;
        bool live = tipoff->frame >= NBA_TIPOFF_BREAK_FRAME;
        out->control = NBA_GAMEPLAY_CONTROL_CPU;
        out->world_x = live ? fp_round(state->x_fp) : formation[actor].world_x;
        out->world_y = live ? fp_round(state->y_fp) : formation[actor].world_y;
        out->world_z = live ? fp_round(state->z_fp) :
            actor == 5u ? (int16_t)center_jump_height(tipoff->frame) : 0;
        out->screen_x = live ? (int16_t)(out->world_x + out->world_y -
            tipoff->camera_x) : formation[actor].screen_x;
        out->screen_y = live ? (int16_t)((out->world_y - out->world_x) / 4 -
            tipoff->camera_y - out->world_z) :
            (int16_t)(formation[actor].screen_y - out->world_z);
        out->visible = live ? out->screen_x > -32 && out->screen_x < 288 &&
            out->screen_y > -48 && out->screen_y < 240 : actor_visible(actor);
        /* Actor +$0E/+$10 are already signed 8.8 ROM velocity words. Keep
         * them raw so CLI/JSON can compare directly with the Mesen oracle. */
        out->velocity_x = live ? state->velocity_x : 0;
        out->velocity_y = live ? state->velocity_y : 0;
        out->velocity_z = live ? state->velocity_z : 0;
        out->direction = live ? state->direction : formation[actor].direction;
        out->animation_state = live ? state->animation_state :
            actor_animation(tipoff, actor);
        out->lower_animation_state = live ? state->lower_animation_state :
                                     out->animation_state;
        out->ai_state = live ? (uint8_t)state->action_state : 0u;
        out->ai_target_actor = live ? state->assignment_actor :
                               NBA_GAMEPLAY_NO_ACTOR;
        out->actor_base = (uint16_t)(0x34EBu + actor * 0x100u);
        out->id_raw = (uint16_t)actor;
        out->action_raw = live ? state->behavior_timer : NBA_GAMEPLAY_UNKNOWN_WORD;
        out->flags_raw = 0;
        uint16_t upper_resource = 0u, lower_resource = 0u;
        if (live && nba_player_animation_resources(tipoff->assets,
                state->animation_state, state->lower_animation_state,
                state->direction, state->upper_animation_tick,
                state->lower_animation_tick, &upper_resource,
                &lower_resource)) {
            out->upper_resource_raw = upper_resource;
            out->lower_resource_raw = lower_resource;
        } else {
            out->upper_resource_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
            out->lower_resource_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        }
        out->head_resource_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->motion_38_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->motion_3a_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->motion_3c_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->direction_4e_raw = out->direction;
        out->direction_50_raw = state->requested_direction;
        out->direction_52_raw = out->direction;
        out->target_x_56_raw = state->target_x;
        out->target_y_58_raw = state->target_y;
        out->side_group_raw = actor >= 5u ? 5u : 0u;
        out->control_mode_raw = state->control_mode;
        out->mode_saved_62_raw = state->saved_control_mode;
        out->control_mode_saved_raw = out->control_mode_raw;
        out->assignment_base_raw = state->assignment_base_raw;
        out->assignment_current_raw = state->assignment_current_raw;
        out->assignment_alternate_raw = state->assignment_alternate_raw;
        out->assignment_distance_raw = state->assignment_distance;
        out->assignment_direction_raw = state->assignment_direction;
        out->pair_distance_raw = state->pair_distance;
        out->reaction_threshold_raw = state->reaction_threshold;
        out->movement_boost_raw = state->movement_boost_timer;
        out->controller_assignment_16_raw =
            state->controller_assignment_raw;
        out->movement_magnitude_4c_raw = state->movement_magnitude_raw;
        out->recovery_inhibit_7a_raw = state->recovery_inhibit_raw;
        out->upper_restart_raw = out->lower_restart_raw = 0;
        out->upper_phase_raw = live ? state->upper_animation_tick : 0u;
        out->lower_phase_raw = live ? state->lower_animation_tick : 0u;
        out->behavior_flags_raw = state->behavior_flags_raw;
        out->palette_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->actor_routine = live ? 0x85963Du : 0x80AD92u;
        out->ai_routine = tipoff->frame >= NBA_TIPOFF_BREAK_FRAME ?
            nba_gameplay_behavior_routine(state->control_mode) : 0u;
    }
}

void nba_tipoff_render(const NbaTipoff *tipoff, NbaRenderer *ren) {
    if (!tipoff || !tipoff->is_initialized || !ren) return;
    const uint32_t *court = nba_assets_gameplay_court_panorama(
        tipoff->assets, tipoff->session->right_team);
    int crop_x = tipoff->camera_x + 582;
    int crop_y = tipoff->camera_y + 243;
    if (crop_x < 0) crop_x = 0;
    if (crop_x > 912 - 256) crop_x = 912 - 256;
    if (crop_y < 0) crop_y = 0;
    if (crop_y > 416 - 224) crop_y = 416 - 224;
    for (unsigned y = 0; y < 224u; ++y)
        memcpy(ren->pixels + y * 256u,
               court + (size_t)(crop_y + (int)y) * 912u + crop_x,
               256u * sizeof(uint32_t));

    uint8_t render_order[NBA_GAMEPLAY_ACTOR_COUNT];
    int16_t screen_x[NBA_GAMEPLAY_ACTOR_COUNT];
    int16_t screen_y[NBA_GAMEPLAY_ACTOR_COUNT];
    unsigned render_count = 0;
    if (tipoff->frame < NBA_TIPOFF_BREAK_FRAME) {
        for (int order = 7; order >= 0; --order) {
            unsigned actor = visible_submission[order];
            render_order[render_count] = (uint8_t)actor;
            screen_x[actor] = formation[actor].screen_x;
            screen_y[actor] = formation[actor].screen_y;
            ++render_count;
        }
    } else {
        for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
            int x = fp_round(tipoff->actors[actor].x_fp);
            int y = fp_round(tipoff->actors[actor].y_fp);
            int z = fp_round(tipoff->actors[actor].z_fp);
            screen_x[actor] = (int16_t)(x + y - tipoff->camera_x);
            screen_y[actor] = (int16_t)((y - x) / 4 - tipoff->camera_y - z);
            if (screen_x[actor] >= -32 && screen_x[actor] < 288 &&
                screen_y[actor] >= -48 && screen_y[actor] < 240)
                render_order[render_count++] = (uint8_t)actor;
        }
        for (unsigned i = 1; i < render_count; ++i) {
            uint8_t actor = render_order[i];
            int j = (int)i - 1;
            while (j >= 0 && screen_y[render_order[j]] > screen_y[actor]) {
                render_order[j + 1] = render_order[j];
                --j;
            }
            render_order[j + 1] = actor;
        }
    }

    for (unsigned order = 0; order < render_count; ++order) {
        unsigned actor = render_order[order];
        uint8_t team_side = actor >= 5u;
        uint8_t uniform_side = team_side;
        uint8_t slot = tipoff->actors[actor].roster_slot;
        uint8_t team = team_side ? tipoff->session->right_team :
                                   tipoff->session->left_team;
        uint8_t state = tipoff->frame >= NBA_TIPOFF_BREAK_FRAME ?
                        tipoff->actors[actor].animation_state :
                        actor_animation(tipoff, actor);
        int jump = 0;
        if (tipoff->frame < NBA_TIPOFF_BREAK_FRAME && actor == 5u &&
            tipoff->frame >= NBA_TIPOFF_JUMP_FRAME &&
            tipoff->frame < NBA_TIPOFF_CONTACT_FRAME) {
            jump = center_jump_height(tipoff->frame);
        }
        uint8_t direction = tipoff->frame >= NBA_TIPOFF_BREAK_FRAME ?
                            tipoff->actors[actor].direction :
                            formation[actor].direction;
        uint8_t lower_state = tipoff->frame >= NBA_TIPOFF_BREAK_FRAME ?
                              tipoff->actors[actor].lower_animation_state : state;
        uint32_t upper_tick = tipoff->frame >= NBA_TIPOFF_BREAK_FRAME ?
                              tipoff->actors[actor].upper_animation_tick :
                              (uint32_t)tipoff->frame;
        uint32_t lower_tick = tipoff->frame >= NBA_TIPOFF_BREAK_FRAME ?
                              tipoff->actors[actor].lower_animation_tick :
                              (uint32_t)tipoff->frame;
        nba_player_sprite_render_split(ren, tipoff->assets, team, slot,
                                       uniform_side, state, lower_state,
                                       direction, upper_tick, lower_tick,
                                       screen_x[actor],
                                       screen_y[actor] - jump, 1);
    }
    int ball_x, ball_y;
    if (tipoff->frame < NBA_TIPOFF_BREAK_FRAME) {
        ball_position(tipoff->frame, &ball_x, &ball_y);
    } else {
        int x = fp_round(tipoff->ball.x_fp), y = fp_round(tipoff->ball.y_fp);
        int z = fp_round(tipoff->ball.z_fp);
        ball_x = x + y - tipoff->camera_x;
        ball_y = (y - x) / 4 - tipoff->camera_y - z;
    }
    if (tipoff->frame >= NBA_TIPOFF_BALL_APPEAR_FRAME)
        draw_ball(tipoff, ren, ball_x, ball_y);

    if (tipoff->frame < 15) {
        unsigned brightness = (unsigned)tipoff->frame;
        for (int i = 0; i < 256 * 224; ++i) {
            uint32_t c = ren->pixels[i];
            uint32_t r = ((c >> 16) & 255u) * brightness / 15u;
            uint32_t g = ((c >> 8) & 255u) * brightness / 15u;
            uint32_t b = (c & 255u) * brightness / 15u;
            ren->pixels[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
        }
    }
}
