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
    tipoff->is_initialized = true;
    printf("[TIPOFF] $86:DDA7 formation -> $86:E054 ball -> "
           "$86:ECF4 jump -> $86:D3F9 possession.\n");
    return true;
}

void nba_tipoff_update(NbaTipoff *tipoff, const NbaInput *input) {
    (void)input;
    if (!tipoff || !tipoff->is_initialized) return;
    ++tipoff->frame;
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
    telemetry->simulation_tick = (uint32_t)tipoff->frame;
    telemetry->phase = (uint8_t)tipoff->phase;
    telemetry->input_pressed = input ? input->pressed : 0u;
    telemetry->input_held = input ? input->held : 0u;
    telemetry->input_released = input ? input->released : 0u;
    telemetry->pad_held_raw[0] = (uint16_t)(telemetry->input_held & 0x0FFFu);
    for (unsigned pad = 0; pad < NBA_GAMEPLAY_PAD_COUNT; ++pad) {
        telemetry->controller_assignment_raw[pad] = NBA_GAMEPLAY_UNKNOWN_WORD;
        telemetry->controller_repeat_raw[pad] = NBA_GAMEPLAY_UNKNOWN_WORD;
    }
    telemetry->active_controller_raw = 0;
    telemetry->selected_controller_raw = 0;
    telemetry->controlled_side_raw = tipoff->session->player_one_side ? 5 : 0;
    telemetry->initial_controlled_slot_raw = tipoff->session->player_one_side ? 7 : 2;
    telemetry->selected_slot_raw = telemetry->initial_controlled_slot_raw;
    telemetry->controlled_actor = (uint8_t)telemetry->selected_slot_raw;
    telemetry->controlled_actor_pointer_raw = (uint16_t)(
        0x34EBu + (unsigned)telemetry->controlled_actor * 0x100u);
    telemetry->controller_assignment_raw[0] = telemetry->controlled_actor;
    telemetry->possession_actor = -1;
    telemetry->possession_team = -1;
    telemetry->possession_candidate_raw = -1;
    telemetry->play_code_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    telemetry->rng_state_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    telemetry->collision_actor_a = tipoff->frame >= NBA_TIPOFF_CONTACT_FRAME ? 0 : -1;
    telemetry->collision_actor_b = tipoff->frame >= NBA_TIPOFF_CONTACT_FRAME ? 5 : -1;
    telemetry->controller_routine = 0x80CB8Fu;
    telemetry->selection_routine = 0x85C37Du;
    telemetry->collision_routine = tipoff->frame >= NBA_TIPOFF_CONTACT_FRAME ?
                                   SNES_ADDR_TIPOFF_CONTACT : 0u;
    telemetry->possession_routine = tipoff->frame >= NBA_TIPOFF_POSSESSION_FRAME ?
                                    SNES_ADDR_TIPOFF_POSSESSION : 0u;
    telemetry->camera_085c_raw = telemetry->camera_085e_raw =
        telemetry->camera_0860_raw = telemetry->camera_0862_raw =
        telemetry->camera_086c_raw = telemetry->camera_086e_raw =
        telemetry->camera_0874_raw = telemetry->camera_0876_raw =
        telemetry->camera_0878_raw = telemetry->camera_087a_raw =
            NBA_GAMEPLAY_UNKNOWN_WORD;

    int ball_x, ball_y, previous_x, previous_y;
    ball_position(tipoff->frame, &ball_x, &ball_y);
    ball_position(tipoff->frame > 0 ? tipoff->frame - 1 : 0,
                  &previous_x, &previous_y);
    telemetry->ball.world_x = (int16_t)ball_x;
    telemetry->ball.world_y = (int16_t)ball_y;
    telemetry->ball.world_z = 0;
    telemetry->ball.screen_x = (int16_t)ball_x;
    telemetry->ball.screen_y = (int16_t)ball_y;
    telemetry->ball.velocity_x = (int16_t)(ball_x - previous_x);
    telemetry->ball.velocity_y = (int16_t)(ball_y - previous_y);
    telemetry->ball.owner_actor = -1;
    telemetry->ball.state = tipoff->frame < NBA_TIPOFF_BALL_APPEAR_FRAME ? 0u :
                            tipoff->frame < NBA_TIPOFF_CONTACT_FRAME ? 1u : 2u;
    telemetry->ball.routine = SNES_ADDR_TIPOFF_BALL_INIT;
    telemetry->ball.flags_raw = NBA_GAMEPLAY_UNKNOWN_WORD;

    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaGameplayActorTelemetry *out = &telemetry->actors[actor];
        out->index = (uint8_t)actor;
        out->team_side = actor >= 5u;
        out->roster_slot = (uint8_t)(actor % 5u);
        out->control = actor == telemetry->controlled_actor ?
                       NBA_GAMEPLAY_CONTROL_PLAYER_1 : NBA_GAMEPLAY_CONTROL_CPU;
        out->visible = actor_visible(actor);
        out->world_x = formation[actor].world_x;
        out->world_y = formation[actor].world_y;
        out->world_z = actor == 5u ? (int16_t)center_jump_height(tipoff->frame) : 0;
        out->screen_x = formation[actor].screen_x;
        out->screen_y = (int16_t)(formation[actor].screen_y - out->world_z);
        out->direction = formation[actor].direction;
        out->animation_state = actor_animation(tipoff, actor);
        out->lower_animation_state = out->animation_state;
        out->ai_target_actor = NBA_GAMEPLAY_NO_ACTOR;
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
        out->control_mode_raw = actor == telemetry->controlled_actor ? 0x0Bu :
                                out->side_group_raw ==
                                    (uint16_t)telemetry->controlled_side_raw ? 1u : 2u;
        out->control_mode_saved_raw = out->control_mode_raw;
        out->assignment_base_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->assignment_current_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->assignment_alternate_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->assignment_distance_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->assignment_direction_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->pair_distance_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->reaction_threshold_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->upper_restart_raw = out->lower_restart_raw = 0;
        out->upper_phase_raw = out->lower_phase_raw = 0;
        out->behavior_flags_raw = 0;
        out->palette_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->actor_routine = 0x80AD92u;
        out->ai_routine = tipoff->frame >= NBA_TIPOFF_POSSESSION_FRAME ?
                          0x87A160u : 0u;
    }
}

void nba_tipoff_render(const NbaTipoff *tipoff, NbaRenderer *ren) {
    if (!tipoff || !tipoff->is_initialized || !ren) return;
    const NbaAssetItem *court = nba_assets_get(
        tipoff->assets, NBA_ASSET_GAMEPLAY_COURT);
    memcpy(ren->pixels, court->data, 256u * 224u * sizeof(uint32_t));

    for (int order = 7; order >= 0; --order) {
        unsigned actor = visible_submission[order];
        uint8_t team_side = actor >= 5u;
        uint8_t uniform_side = team_side;
        uint8_t slot = (uint8_t)(actor % 5u);
        uint8_t team = team_side ? tipoff->session->right_team :
                                   tipoff->session->left_team;
        uint8_t state = actor_animation(tipoff, actor);
        int jump = 0;
        if (actor == 5u && tipoff->frame >= NBA_TIPOFF_JUMP_FRAME &&
            tipoff->frame < NBA_TIPOFF_CONTACT_FRAME) {
            jump = center_jump_height(tipoff->frame);
        }
        nba_player_sprite_render(ren, tipoff->assets, team, slot, uniform_side, state,
                                 formation[actor].direction,
                                 (uint32_t)tipoff->frame,
                                 formation[actor].screen_x,
                                 formation[actor].screen_y - jump, 1);
    }
    int ball_x, ball_y;
    ball_position(tipoff->frame, &ball_x, &ball_y);
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
