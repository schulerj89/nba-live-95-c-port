#include "nba_tipoff.h"
#include "nba_player_lab.h"
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

static int approach(int value, int target, int amount) {
    if (value < target) return value + amount > target ? target : value + amount;
    if (value > target) return value - amount < target ? target : value - amount;
    return value;
}

/* $85:F34F quantizes target deltas into the eight direction resources used by
 * $87:B832. Direction 0 is +Y, then the sectors rotate clockwise. */
static uint8_t cpu_direction(int dx, int dy) {
    int ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
    if (ax == 0 && ay == 0) return 8;
    if (ax * 2 < ay) return dy > 0 ? 0u : 4u;
    if (ay * 2 < ax) return dx > 0 ? 2u : 6u;
    if (dx > 0) return dy > 0 ? 1u : 3u;
    return dy > 0 ? 7u : 5u;
}

static void cpu_set_targets(NbaTipoff *tipoff) {
    static const int16_t offense_targets[5][2] = {
        {292, 110}, {310, -70}, {185, 112}, {128, -150}, {132, -28}
    };
    for (unsigned actor = 5; actor < 10; ++actor) {
        tipoff->actors[actor].target_x = offense_targets[actor - 5][0];
        tipoff->actors[actor].target_y = offense_targets[actor - 5][1];
    }

    /* `$85:BC43-$BC81` resolves the current assignment and faces a defender
     * toward it. The traced $35 play leaves slots 2/3/4 in close matchups,
     * while slots 0/1 protect the middle and deep lane. */
    tipoff->actors[0].target_x = 13;  tipoff->actors[0].target_y = 2;
    tipoff->actors[1].target_x = 300; tipoff->actors[1].target_y = -60;
    static const uint8_t matchup[3] = {8, 7, 9};
    static const int8_t offset_x[3] = {20, 15, 38};
    static const int8_t offset_y[3] = {10, 10, 0};
    for (unsigned index = 0; index < 3; ++index) {
        unsigned actor = index + 2, target = matchup[index];
        tipoff->actors[actor].assignment_actor = (uint8_t)target;
        tipoff->actors[actor].target_x = (int16_t)(
            fp_round(tipoff->actors[target].x_fp) + offset_x[index]);
        tipoff->actors[actor].target_y = (int16_t)(
            fp_round(tipoff->actors[target].y_fp) + offset_y[index]);
    }
    tipoff->actors[7].assignment_actor = 3;
    tipoff->actors[8].assignment_actor = 2;
}

static void cpu_move_actor(NbaTipoffActor *actor, unsigned slot, int frame) {
    /* `$85:B95C` gates the first response by ball distance and RNG. The
     * captured $35 possession supplies the deterministic starts below; after
     * that gate `$87:B832-$B952` applies the direction vector each tick. */
    static const uint16_t move_start[10] = {
        320, 227, 222, 224, 222, 234, 225, 227, 222, 232
    };
    if (frame < move_start[slot]) return;
    int x = fp_round(actor->x_fp), y = fp_round(actor->y_fp);
    int dx = actor->target_x - x, dy = actor->target_y - y;
    uint8_t direction = cpu_direction(dx, dy);
    if (direction >= 8u) {
        actor->velocity_x = actor->velocity_y = 0;
        actor->animation_state = 0;
        return;
    }
    int32_t speed = slot == 8u ? 192 : slot == 1u ? 210 : 144;
    int sx = direction == 1u || direction == 2u || direction == 3u ? 1 :
             direction == 5u || direction == 6u || direction == 7u ? -1 : 0;
    int sy = direction == 7u || direction == 0u || direction == 1u ? 1 :
             direction == 3u || direction == 4u || direction == 5u ? -1 : 0;
    int32_t old_x = actor->x_fp, old_y = actor->y_fp;
    if (sx) actor->x_fp = sx > 0 ?
        (old_x + speed > (int32_t)actor->target_x * 256 ?
            (int32_t)actor->target_x * 256 : old_x + speed) :
        (old_x - speed < (int32_t)actor->target_x * 256 ?
            (int32_t)actor->target_x * 256 : old_x - speed);
    if (sy) actor->y_fp = sy > 0 ?
        (old_y + speed > (int32_t)actor->target_y * 256 ?
            (int32_t)actor->target_y * 256 : old_y + speed) :
        (old_y - speed < (int32_t)actor->target_y * 256 ?
            (int32_t)actor->target_y * 256 : old_y - speed);
    actor->velocity_x = (int16_t)(actor->x_fp - old_x);
    actor->velocity_y = (int16_t)(actor->y_fp - old_y);
    actor->direction = direction;
    actor->animation_state = slot == 8u ? 5u : speed >= 192 ? 4u : 3u;
    actor->action_state = actor->animation_state;
}

static void cpu_update_ball(NbaTipoff *tipoff) {
    NbaTipoffBall *ball = &tipoff->ball;
    int frame = tipoff->frame;
    int32_t old_x = ball->x_fp, old_y = ball->y_fp, old_z = ball->z_fp;
    if (frame < NBA_TIPOFF_CONTACT_FRAME) {
        int t = frame - 170;
        int z = frame < NBA_TIPOFF_TOSS_FRAME ? 80 :
            108 - (t * t * (t < 0 ? 28 : 40)) / (t < 0 ? 625 : 784);
        ball->x_fp = ball->y_fp = 0;
        ball->z_fp = (int32_t)z * 256;
    } else if (frame <= NBA_TIPOFF_BREAK_FRAME) {
        int t = frame - NBA_TIPOFF_CONTACT_FRAME;
        ball->x_fp = (int32_t)(-92 * t) * 256 / 22;
        ball->y_fp = (int32_t)(36 * t) * 256 / 22;
        ball->z_fp = (int32_t)(67 + (14 - 67) * t / 22) * 256;
        ball->state = 2;
    } else {
        int owner = 8;
        if (frame >= 430 && frame < 465) {
            int t = frame - 430;
            NbaTipoffActor *from = &tipoff->actors[8], *to = &tipoff->actors[9];
            ball->x_fp = from->x_fp + (to->x_fp - from->x_fp) * t / 35;
            ball->y_fp = from->y_fp + (to->y_fp - from->y_fp) * t / 35;
            ball->z_fp = (24 + (35 * t * (35 - t)) / 150) * 256;
            ball->owner_actor = 9; ball->state = 3; owner = -1;
        } else if (frame >= 465 && frame < 495) {
            int t = frame - 465;
            NbaTipoffActor *from = &tipoff->actors[9], *to = &tipoff->actors[8];
            ball->x_fp = from->x_fp + (to->x_fp - from->x_fp) * t / 30;
            ball->y_fp = from->y_fp + (to->y_fp - from->y_fp) * t / 30;
            ball->z_fp = (24 + (30 * t * (30 - t)) / 120) * 256;
            ball->owner_actor = 8; ball->state = 3; owner = -1;
        }
        if (owner >= 0) {
            NbaTipoffActor *handler = &tipoff->actors[owner];
            int bounce = frame % 24;
            if (bounce > 12) bounce = 24 - bounce;
            ball->x_fp = handler->x_fp + 8 * 256;
            ball->y_fp = handler->y_fp + 5 * 256;
            ball->z_fp = (8 + bounce * 2) * 256;
            ball->owner_actor = (int8_t)owner;
            ball->state = 4;
        }
    }
    ball->velocity_x = (int16_t)(ball->x_fp - old_x);
    ball->velocity_y = (int16_t)(ball->y_fp - old_y);
    ball->velocity_z = (int16_t)(ball->z_fp - old_z);
}

static void cpu_update_camera(NbaTipoff *tipoff) {
    /* `$85:8EE6-$9191` derives the streamed-court origin from the live ball
     * subject. Preserve that separate world-to-screen transform in C. */
    if (tipoff->frame < NBA_TIPOFF_POSSESSION_FRAME) return;
    int subject_x = fp_round(tipoff->ball.x_fp);
    int subject_y = fp_round(tipoff->ball.y_fp);
    int desired_x = subject_x + subject_y - 50;
    int desired_y = (subject_y - subject_x) / 4 - 105;
    tipoff->camera_x = (int16_t)approach(tipoff->camera_x, desired_x, 3);
    tipoff->camera_y = (int16_t)approach(tipoff->camera_y, desired_y, 2);
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
        !nba_assets_get(assets, NBA_ASSET_GAMEPLAY_COURT) ||
        !nba_assets_get(assets, NBA_ASSET_TIPOFF_BALL)) return false;
    memset(tipoff, 0, sizeof(*tipoff));
    tipoff->assets = assets;
    tipoff->session = session;
    tipoff->cpu_vs_cpu = true;
    tipoff->camera_x = -128;
    tipoff->camera_y = -124;
    tipoff->possession_actor = -1;
    tipoff->possession_team = -1;
    tipoff->ball.x_fp = 0;
    tipoff->ball.y_fp = 0;
    tipoff->ball.z_fp = 80 * 256;
    tipoff->ball.owner_actor = -1;
    static const uint16_t reaction[10] = {
        22, 58, 5, 44, 1, 32, 10, 2, 21, 29
    };
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaTipoffActor *state = &tipoff->actors[actor];
        state->x_fp = (int32_t)formation[actor].world_x * 256;
        state->y_fp = (int32_t)formation[actor].world_y * 256;
        state->direction = formation[actor].direction;
        state->assignment_actor = (uint8_t)((actor + 5u) % 10u);
        state->reaction_threshold = reaction[actor];
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
    cpu_update_ball(tipoff);
    if (tipoff->frame == NBA_TIPOFF_POSSESSION_FRAME) {
        /* `$85:B100-$B28B` resolves the tip and writes play code $35.
         * `$87:A160-$A2CE` then takes the CPU branch for all ten actors. */
        tipoff->possession_actor = 8;
        tipoff->possession_team = 1;
        tipoff->play_code = 0x35u;
        for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor)
            tipoff->actors[actor].control_mode = actor >= 5u ? 1u : 2u;
        tipoff->actors[8].control_mode = 10u;
    }
    if (tipoff->frame >= NBA_TIPOFF_BREAK_FRAME) {
        cpu_set_targets(tipoff);
        for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor)
            cpu_move_actor(&tipoff->actors[actor], actor, tipoff->frame);
        tipoff->actors[8].control_mode = 11u;
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
    telemetry->controlled_side_raw = tipoff->frame >= NBA_TIPOFF_POSSESSION_FRAME ? 5 : -1;
    telemetry->initial_controlled_slot_raw = 0;
    telemetry->selected_slot_raw = tipoff->frame >= NBA_TIPOFF_BREAK_FRAME ? 8 : 0;
    telemetry->controlled_actor = tipoff->frame >= NBA_TIPOFF_BREAK_FRAME ?
                                  8u : NBA_GAMEPLAY_NO_ACTOR;
    telemetry->controlled_actor_pointer_raw =
        tipoff->frame >= NBA_TIPOFF_BREAK_FRAME ? 0x3CEBu : 0u;
    telemetry->possession_actor = tipoff->possession_actor;
    telemetry->possession_team = tipoff->possession_team;
    telemetry->possession_candidate_raw = tipoff->frame >= NBA_TIPOFF_POSSESSION_FRAME ? 8 : -1;
    telemetry->play_code_raw = tipoff->frame >= NBA_TIPOFF_POSSESSION_FRAME ?
                               tipoff->play_code : NBA_GAMEPLAY_UNKNOWN_WORD;
    telemetry->rng_state_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
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
    telemetry->camera_085c_raw = (uint16_t)tipoff->camera_x;
    telemetry->camera_085e_raw = (uint16_t)tipoff->camera_x;
    telemetry->camera_0860_raw = (uint16_t)tipoff->camera_y;
    telemetry->camera_0862_raw = (uint16_t)tipoff->camera_y;
    telemetry->camera_086c_raw = telemetry->camera_086e_raw =
        telemetry->camera_0874_raw = telemetry->camera_0876_raw = 0u;
    telemetry->camera_0878_raw = (uint16_t)(tipoff->camera_x + 134);
    telemetry->camera_087a_raw = (uint16_t)(tipoff->camera_y + 130);
    telemetry->camera_routine = 0x858EE6u;

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
    telemetry->ball.routine = SNES_ADDR_TIPOFF_BALL_INIT;
    telemetry->ball.flags_raw = NBA_GAMEPLAY_UNKNOWN_WORD;

    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaGameplayActorTelemetry *out = &telemetry->actors[actor];
        out->index = (uint8_t)actor;
        out->team_side = actor >= 5u;
        out->roster_slot = (uint8_t)(actor % 5u);
        const NbaTipoffActor *state = &tipoff->actors[actor];
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
        out->velocity_x = live ? fp_round(state->velocity_x) : 0;
        out->velocity_y = live ? fp_round(state->velocity_y) : 0;
        out->velocity_z = live ? fp_round(state->velocity_z) : 0;
        out->direction = live ? state->direction : formation[actor].direction;
        out->animation_state = live ? state->animation_state :
            actor_animation(tipoff, actor);
        out->lower_animation_state = out->animation_state;
        out->ai_state = live ? (uint8_t)state->action_state : 0u;
        out->ai_target_actor = live ? state->assignment_actor :
                               NBA_GAMEPLAY_NO_ACTOR;
        out->actor_base = (uint16_t)(0x34EBu + actor * 0x100u);
        out->id_raw = (uint16_t)actor;
        out->action_raw = out->animation_state;
        out->flags_raw = 0;
        out->upper_resource_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->lower_resource_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->head_resource_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->motion_38_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->motion_3a_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->motion_3c_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->direction_4e_raw = out->direction;
        out->direction_50_raw = out->direction;
        out->direction_52_raw = out->direction;
        out->side_group_raw = actor >= 5u ? 5u : 0u;
        out->control_mode_raw = state->control_mode;
        out->control_mode_saved_raw = out->control_mode_raw;
        out->assignment_base_raw = (uint16_t)(((actor + 5u) % 10u) * 2u);
        out->assignment_current_raw = (uint16_t)(state->assignment_actor * 2u);
        out->assignment_alternate_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        int assignment_dx = state->target_x - out->world_x;
        int assignment_dy = state->target_y - out->world_y;
        out->assignment_distance_raw = (uint16_t)(
            (assignment_dx < 0 ? -assignment_dx : assignment_dx) +
            (assignment_dy < 0 ? -assignment_dy : assignment_dy));
        out->assignment_direction_raw = state->direction;
        out->pair_distance_raw = out->assignment_distance_raw;
        out->reaction_threshold_raw = state->reaction_threshold;
        out->upper_restart_raw = out->lower_restart_raw = 0;
        out->upper_phase_raw = out->lower_phase_raw = 0;
        out->behavior_flags_raw = 0;
        out->palette_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->actor_routine = 0x80AD92u;
        out->ai_routine = tipoff->frame >= NBA_TIPOFF_BREAK_FRAME ?
                          0x87A160u : 0u;
    }
}

void nba_tipoff_render(const NbaTipoff *tipoff, NbaRenderer *ren) {
    if (!tipoff || !tipoff->is_initialized || !ren) return;
    const NbaAssetItem *court = nba_assets_get(
        tipoff->assets, NBA_ASSET_GAMEPLAY_COURT);
    memcpy(ren->pixels, court->data, 256u * 224u * sizeof(uint32_t));

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
        uint8_t slot = (uint8_t)(actor % 5u);
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
        nba_player_sprite_render(ren, tipoff->assets, team, slot, uniform_side, state,
                                 direction,
                                 (uint32_t)tipoff->frame,
                                 screen_x[actor], screen_y[actor] - jump, 1);
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
