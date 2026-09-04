#include "nba_tipoff.h"
#include "nba_player_lab.h"
#include "nba_shot_action.h"
#include "nba_owner_flow.h"
#include "nba_snes_ppu.h"
#include "nba_font.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Production host equivalent of the captured gameplay ownership families:
 * `$85:8000-$FFFF` actor/camera/ball/play/event helpers,
 * `$86:8000-$FFFF` input/shot/contact/AI/inbound helpers, and
 * `$87:8000-$BFFF` scheduler/event/draw/animation helpers. Exact callable
 * children stay separated in nba_gameplay_*.c, nba_shot_*.c, owner-flow and
 * animation modules. This parent owns their native ordering and shared state.
 * Focused vector replays remain the function oracles; the deterministic
 * 48,000-frame gameplay85 gate protects production composition across three
 * asymmetric team pairs, including rendered asset-pack output. */

/* Captured-bank closure: `$83:8000-$83:FFFF`; only retained executed
 * positions inside this window are claimed. The small captured
 * `$83:B99D-$B9B2/$83:BC29` prefix is shared RNG and
 * indirect event dispatch. nba_gameplay_rng_next and the typed event queues
 * replace the native pointer jump while preserving its mutated state. The
 * gameplay100 journey seeds that RNG, reaches physical tip possession, then
 * hashes 6,000 ordered CPU frames; focused RNG/event witnesses remain the
 * branch-level oracle. */

typedef struct {
    int16_t world_x, world_y;
    int16_t screen_x, screen_y;
    uint8_t direction;
} TipoffFormation;

typedef enum {
    CPU_MODE11_NORMAL_RETURN = 0,
    CPU_MODE11_CONSUMED_ACTION,
    CPU_MODE11_SHOT_STARTED
} CpuMode11Outcome;

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

/* `$86:DA8D-$DAAB`, normal Exhibition (`$15C3=0`): gameplay context0
 * `$46EB` is home/right; context1 `$476B` is visitor/left. Only this
 * publication boundary reads the frontend's left/right team choices.
 * Native first-court fixtures cover Rockets/Knicks and Pacers/Magic and are
 * checked against the owning Ghidra/recomp stores. Other game-mode swap paths remain
 * outside this Exhibition initializer; never infer them from UI truthiness. */
static void publish_exhibition_team_ids(NbaTipoff *tipoff) {
    tipoff->team_context[0].strategy_team_raw_00 = tipoff->session->right_team;
    tipoff->team_context[1].strategy_team_raw_00 = tipoff->session->left_team;
}

static uint8_t team_id_for_context(const NbaTipoff *tipoff, unsigned context) {
    return (uint8_t)tipoff->team_context[context].strategy_team_raw_00;
}

static NbaGameplayHudInput hud_input(const NbaTipoff *t) {
    NbaGameplayHudInput in={0};
    for(unsigned i=0;i<2u;++i) {
        in.teams[i]=t->team_context[i].strategy_team_raw_00;
        in.scores[i]=t->session->score[i];
    }
    in.period_raw_0926=t->period_raw_0926;
    in.phase_raw_08e4=t->hud.phase_raw_08e4;
    in.clock_raw_0928=t->match_clock_raw_0928;
    in.clock_snapshot_raw_092a=t->hud_clock_snapshot_raw_092a;
    in.clock_gate_raw_492b=t->hud_clock_gate_raw_492b;
    in.presentation_timer_raw_08de=(uint16_t)t->fouls.whistle_timer_raw_08de;
    in.presentation_kind_raw_08e8=t->fouls.whistle_state_mirror_raw_08e8;
    in.presentation_sequence_raw_08e6=t->fouls.whistle_state_raw_08e6;
    in.dead_ball_busy_raw_09b4=t->dead_ball_dispatch_busy_raw_09b4;
    in.event_bits_raw_13e7=t->rim_raw_13e7;
    in.dispatch_mode_raw_0960=t->hud_dispatch_mode_raw_0960;
    in.requester_raw_095e=t->hud_requester_raw_095e;
    in.shot_clock_raw_092c=t->rim_raw_092c;
    in.style_raw_17ab=t->session->config.main_values[0];
    in.presentation_gate_raw_08e2=t->fouls.presentation_gate_raw_08e2;
    in.rng_raw_07f6=t->rng.state;
    in.latched_event_raw_08f0=t->fouls.latched_event_raw_08f0;
    in.event_actor_raw_492d=t->hud_event_actor_raw_492d;
    in.contact_context_raw_497f=t->fouls.contact_context_raw_497f;
    in.foul_out_state_raw_09ca=t->fouls.foul_out_state_raw_09ca;
    return in;
}

static void hud_store(NbaTipoff *t,const NbaGameplayHudInput *in) {
    /* Shared timer/event fields stay canonical in the preexisting owners.
     * BBE9 can signal expiry; never discard or HUD-locally shadow that write. */
    t->fouls.whistle_timer_raw_08de=(int16_t)in->presentation_timer_raw_08de;
    t->fouls.whistle_state_mirror_raw_08e8=in->presentation_kind_raw_08e8;
    t->fouls.whistle_state_raw_08e6=in->presentation_sequence_raw_08e6;
    t->fouls.presentation_gate_raw_08e2=in->presentation_gate_raw_08e2;
    t->dead_ball_dispatch_busy_raw_09b4=in->dead_ball_busy_raw_09b4;
    t->rim_raw_13e7=in->event_bits_raw_13e7;
    t->rng.state=in->rng_raw_07f6;
}

static void hud_report_incomplete(NbaTipoff *t,bool complete) {
    if(complete || !t->hud.pending_routine ||
       t->hud.reported_pending_routine==t->hud.pending_routine)return;
    t->hud.reported_pending_routine=t->hud.pending_routine;
    fprintf(stderr,"[HUD] Untranslated original overlay child $%06X (kind=%u); gameplay continues, no substituted panel\n",
        t->hud.pending_routine,t->fouls.whistle_state_mirror_raw_08e8);
}

static void hud_request_score(NbaTipoff *t) {
    if(!t->hud.initialized)return; /* standalone gameplay leaf self-tests */
    NbaGameplayHudInput in=hud_input(t);
    bool complete=nba_gameplay_hud_request_score(&t->hud,t->assets,&in);
    hud_store(t,&in);
    hud_report_incomplete(t,complete);
}

static void hud_dispatch(NbaTipoff *t) {
    if(!t->hud.initialized)return;
    NbaGameplayHudInput in=hud_input(t);
    bool complete=nba_gameplay_hud_dispatch(&t->hud,t->assets,&in);
    hud_store(t,&in);
    hud_report_incomplete(t,complete);
}

static void hud_publish(NbaTipoff *t,uint32_t pc) {
    if(!t->hud.initialized)return;
    NbaGameplayHudInput in=hud_input(t);
    bool complete=nba_gameplay_hud_publish(&t->hud,t->assets,pc,&in);
    hud_store(t,&in);
    hud_report_incomplete(t,complete);
}

/* Actor +16 remains canonical for gameplay consumers. The controller module
 * receives/returns a projection only at an ownership mutation boundary. */
static void controller_read_actors(NbaTipoff *t) {
    for (unsigned actor=0;actor<10;++actor)
        t->controllers.actor_assignment[actor]=t->actors[actor].controller_assignment_raw;
}

static void controller_write_actors(NbaTipoff *t) {
    for (unsigned actor=0;actor<10;++actor)
        t->actors[actor].controller_assignment_raw=(int8_t)t->controllers.actor_assignment[actor];
}

bool nba_tipoff_initialize_controllers(NbaTipoff *t,const uint16_t selections[5],
    const uint16_t flags[5],uint16_t override_07f8) {
    if (!t) return false;
    controller_read_actors(t);
    if (!nba_controller_initialize(&t->controllers,selections,flags,override_07f8)) {
        t->controller_contract_fault=true;return false;
    }
    t->controller_override_raw_07f8=override_07f8;
    controller_write_actors(t);
    return true;
}

bool nba_tipoff_transfer_controller(NbaTipoff *t,unsigned target) {
    if (!t || target>=10) return false;
    controller_read_actors(t);
    if (!nba_controller_transfer(&t->controllers,target,t->actors[target].team_group_raw_6e)) {
        t->controller_contract_fault=true;return false;
    }
    controller_write_actors(t);
    return true;
}

void nba_tipoff_publish_controller_input(NbaTipoff *t,unsigned actor,uint16_t held) {
    if (!t || actor>=10) return;
    NbaTipoffActor *a=&t->actors[actor];
    int pad=a->controller_assignment_raw;
    if (pad<0 || pad>=5) return;
    unsigned roster=t->fatigue.active_roster[actor];
    if (roster>=24) {t->controller_contract_fault=true;return;}
    NbaControllerInputContext c={(uint16_t)actor,(uint16_t)((int32_t)a->z_fp>>8),
        t->fatigue.stamina[roster],a->movement_boost_timer,
        t->fouls.free_throw_state_raw_0978,t->live_state_raw,t->attached_ball_state_raw_09f6,
        t->session->config.rules[4],(uint16_t)t->possession_actor,
        t->fouls.shooting_foul_raw_09bc,t->fouls.foul_event_raw_0964,
        t->fouls.whistle_active_raw_09b6,(uint16_t)(int16_t)t->collision_actor_a_raw};
    nba_controller_publish_input(&t->controllers.record[pad],held,&c);
    a->movement_boost_timer=c.boost;
    t->fouls.foul_event_raw_0964=c.event_0964;
    t->collision_actor_a_raw=(int8_t)c.event_actor_492d;
}

/* Legacy pause compatibility still consumes player_one_side left0/right1.
 * The allocator uses selection0 -> group5/context1 and selection2 ->
 * group0/context0. Native requesting-pad pause routing, including neutral,
 * must replace this compatibility path before human gameplay is enabled. */
static uint8_t context_for_ui_side(uint8_t ui_side) {
    return ui_side ? 0u : 1u;
}

/* `$86:D789-$D7B5` follows the appearance sort; `$D7A8` stores rank4..0
 * through each sorted actor offset. `$D97A/$DA07` apply it to both teams.
 * This is gameplay actor+$92, not the roster-position category. The helper
 * consumes the translated sort's results without recomputing its keys. */
static void publish_appearance_assignment_roles(
    NbaTipoffActor actors[NBA_GAMEPLAY_ACTOR_COUNT],
    const NbaPlayerActiveAppearance *appearance) {
    for (unsigned context = 0; context < 2u; ++context)
        for (unsigned rank = 0; rank < 5u; ++rank) {
            unsigned actor = appearance->sorted_actor_offset[context][rank] / 2u;
            actors[actor].assignment_role_raw_92 = (uint8_t)(4u - rank);
        }
}

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

static void draw_gameplay_court_bg2(const NbaTipoff *tipoff,
                                    NbaRenderer *ren,
                                    const uint8_t *vram,
                                    const uint8_t *cgram,
                                    int crop_x, int crop_y) {
    const NbaAssetItem *map = nba_assets_gameplay_court_map(
        tipoff->assets, team_id_for_context(tipoff, 0u));
    if (!map || map->size != 15398u) return;
    const uint8_t *data = (const uint8_t *)map->data;
    for (int sy = 0; sy < NBA_SNES_HEIGHT; ++sy) {
        int py = crop_y + sy;
        for (int sx = 0; sx < NBA_SNES_WIDTH; ++sx) {
            int px = crop_x + sx;
            size_t offset = 6u + ((size_t)(px >> 3) * 52u +
                                  (size_t)(py >> 3)) * 2u;
            uint16_t entry = read_u16(data + offset);
            int tx = px & 7, ty = py & 7;
            if (entry & 0x4000u) tx = 7 - tx;
            if (entry & 0x8000u) ty = 7 - ty;
            uint8_t color = tile_pixel(
                vram + 0x4000u + (entry & 0x03ffu) * 32u, tx, ty);
            if (!color) continue; /* Hardware BG transparency. */
            uint8_t palette = (uint8_t)(((entry >> 10) & 7u) * 16u + color);
            nba_snes_mode1_submit_indexed(
                ren, cgram, 15, sx, sy, NBA_SNES_LAYER_BG2,
                (uint8_t)((entry >> 13) & 1u), palette, color, 127u);
        }
    }
}

static void draw_gameplay_hud_bg3(NbaRenderer *ren, const uint8_t *vram,
                                  const uint8_t *cgram) {
    /* Settled gameplay PPU state: BG3 map word $0400, shared CHR word
     * $1000, HOFS=0, VOFS=$03FF. The asset retains indexed tiles so color
     * zero exposes the court and Mode-1 BG3-high can cover players exactly. */
    for (int y = 0; y < NBA_SNES_HEIGHT; ++y) {
        for (int x = 0; x < NBA_SNES_WIDTH; ++x) {
            NbaSnesBgPixel pixel;
            if (!nba_snes_sample_bg(vram, 0x0800, 0x2000, 2, false, false,
                                    0, 0x03ff, x, y, &pixel)) continue;
            uint8_t palette = (uint8_t)(pixel.palette * 4 + pixel.color_index);
            nba_snes_mode1_submit_indexed(
                ren, cgram, 15, x, y, NBA_SNES_LAYER_BG3,
                (uint8_t)pixel.priority, palette,
                (uint8_t)pixel.color_index, 127u);
        }
    }
}

static void draw_animated_crowd(const NbaTipoff *tipoff, NbaRenderer *ren,
                                int crop_x, int crop_y) {
    const NbaAssetItem *anim = nba_assets_get(
        tipoff->assets, NBA_ASSET_GAMEPLAY_CROWD_TILES);
    const NbaAssetItem *map = nba_assets_gameplay_court_map(
        tipoff->assets, team_id_for_context(tipoff, 0u));
    if (!anim || !map || anim->size != 3548u || map->size != 15398u ||
        memcmp(anim->data, "NBCROWD1", 8)) return;
    const uint8_t *data = (const uint8_t *)anim->data;
    const uint8_t *map_data = (const uint8_t *)map->data;
    /* $85:8BBF selects descriptor $89:FF81 or $AF:E4F8. Standard
     * destinations are $40 VRAM words (four 4bpp tiles) below parquet.
     * Native frames 20..180 match at all 28 relocated tile destinations. */
    const unsigned tile_bias = map->id == NBA_ASSET_GAMEPLAY_STANDARD_COURT_MAP ? 4u : 0u;
    const unsigned frame_count = (unsigned)read_u16(data + 12);
    const unsigned tile_count = (unsigned)read_u16(data + 16);
    if (frame_count != 3u || tile_count != 28u) return;
    /* `$80:82A3` presents one graphics queue every other simulation pass.
     * Hold each sampled ROM fan state for four such passes. */
    const unsigned frame = (unsigned)(tipoff->frame / 8u) % frame_count;
    const size_t frame_stride = 4u + 0x100u + tile_count * 32u;
    const uint8_t *frame_data = data + 24u + tile_count * 2u +
                                frame * frame_stride;
    const uint8_t *palette = frame_data + 4u;
    const uint8_t *tiles = palette + 0x100u;
    for (int sy = 0; sy < NBA_SNES_HEIGHT; ++sy) {
        int py = crop_y + sy, tile_y = py >> 3;
        if (tile_y < 0 || tile_y >= 52) continue;
        for (int sx = 0; sx < NBA_SNES_WIDTH; ++sx) {
            int px = crop_x + sx, tile_x = px >> 3;
            if (tile_x < 0 || tile_x >= 148) continue;
            size_t map_offset = 6u + ((size_t)tile_x * 52u +
                                      (size_t)tile_y) * 2u;
            uint16_t entry = read_u16(map_data + map_offset);
            uint16_t tile_id = entry & 0x03ffu;
            unsigned slot;
            for (slot = 0; slot < tile_count; ++slot)
                if (read_u16(data + 24u + slot * 2u) == tile_id + tile_bias) break;
            if (slot == tile_count) continue;
            int tx = px & 7, ty = py & 7;
            if (entry & 0x4000u) tx = 7 - tx;
            if (entry & 0x8000u) ty = 7 - ty;
            uint8_t color = tile_pixel(tiles + slot * 32u, tx, ty);
            unsigned palette_index = ((entry >> 10) & 7u) * 16u + color;
            nba_snes_mode1_submit_indexed(
                ren, palette, 15, sx, sy, NBA_SNES_LAYER_BG2,
                (uint8_t)((entry >> 13) & 1u), (uint8_t)palette_index,
                color, 127u);
        }
    }
}

static void draw_gameplay_goal_bg(const NbaTipoff *tipoff, NbaRenderer *ren,
                                  const uint8_t *vram,
                                  const uint8_t *cgram) {
    const uint8_t *map = vram;
    const uint8_t *chr = vram + 0x2000u;
    int window_low = (int)tipoff->court_presentation.window_right_0882;
    int window_high = (int)tipoff->court_presentation.window_left_0880;
    if (window_low <= window_high) {
        unsigned hscroll = tipoff->court_presentation.window_x_087c & 0xffu;
        unsigned vscroll = tipoff->court_presentation.window_y_087e & 0xffu;
        /* Gameplay PPU state proves BG1 map word $0000 / CHR word $1000,
         * with `$0882` and `$0880` copied to the active clip window by
         * `$87:A81D-$A845`. Color zero exposes BG2 beneath the structure. */
        /* The raster IRQ follows $087E. A fixed scanline 123 only matched
         * camera Y=-220 and exposed the wrapped second board on the floor
         * as the camera moved. Apply both baskets' native TM/WH3 schedule. */
        for (int sy = 0; sy < NBA_SNES_HEIGHT; ++sy) {
            uint8_t line_window_right;
            if (!nba_court_goal_scanline(&tipoff->court_presentation,
                    tipoff->camera_x, (unsigned)sy, &line_window_right)) continue;
            unsigned py = ((unsigned)sy + vscroll + 1u) & 0xffu;
            for (int sx = 0; sx < NBA_SNES_WIDTH; ++sx) {
                if (!nba_snes_window_visible(sx, (uint8_t)window_low,
                                              line_window_right, true))
                    continue;
                unsigned px = ((unsigned)sx + hscroll) & 0xffu;
                size_t map_offset = (((py >> 3) * 32u + (px >> 3)) * 2u);
                uint16_t entry = read_u16(map + map_offset);
                int tx = (int)(px & 7u), ty = (int)(py & 7u);
                if (entry & 0x4000u) tx = 7 - tx;
                if (entry & 0x8000u) ty = 7 - ty;
                uint8_t color = tile_pixel(chr + (entry & 0x03ffu) * 32u,
                                           tx, ty);
                if (!color) continue;
                unsigned palette_index = ((entry >> 10) & 7u) * 16u + color;
                nba_snes_mode1_submit_indexed(
                    ren, cgram, 15, sx, sy, NBA_SNES_LAYER_BG1,
                    (uint8_t)((entry >> 13) & 1u),
                    (uint8_t)palette_index, color, 127u);
            }
        }
    }
}

static void draw_gameplay_goal_obj(const NbaTipoff *tipoff,
                                   NbaRenderer *ren,
                                   const uint8_t *cgram) {
    int16_t basket_x = (int16_t)tipoff->court_presentation.basket_x_3fef;
    int16_t screen_x, screen_y;
    nba_court_project_actor(basket_x, 0, 80, tipoff->camera_x,
                            tipoff->camera_y, &screen_x, &screen_y);
    uint16_t resource = tipoff->rim_effect.resource_raw_4015;
    if (resource != 0x0822u && (resource < 0x082cu || resource > 0x082fu))
        resource = 0x0822u;
    nba_rom_sprite_resource_render(ren, tipoff->assets, resource,
        cgram + (128u + 6u * 16u) * 2u, screen_x, screen_y,
        basket_x < 0, 1);
}

static void submit_argb_object(NbaRenderer *ren, const NbaRenderer *object,
                               uint8_t priority, uint8_t oam_index) {
    for (int y = 0; y < NBA_SNES_HEIGHT; ++y) {
        for (int x = 0; x < NBA_SNES_WIDTH; ++x) {
            uint32_t argb = object->pixels[y * NBA_SNES_WIDTH + x];
            if ((argb >> 24) == 0u) continue;
            nba_snes_mode1_submit_color(ren, x, y, NBA_SNES_LAYER_OBJ,
                                        priority, oam_index, argb);
        }
    }
}

static uint8_t actor_oam_index(const NbaTipoff *tipoff, unsigned actor) {
    if (tipoff->draw_order_initialized && tipoff->tip_contact_actor >= 0) {
        uint16_t pointer = (uint16_t)(0x34ebu + actor * 256u);
        for (unsigned i = 0; i < 12u; ++i)
            if (tipoff->draw_order.order[i] == pointer)
                return (uint8_t)((11u - i) * 4u);
    }
    for (unsigned i = 0; i < sizeof(visible_submission); ++i)
        if (visible_submission[i] == actor) return (uint8_t)(i * 4u);
    return (uint8_t)(64u + actor);
}

static int16_t fp_round(int32_t value) {
    return (int16_t)(value >= 0 ? (value + 128) >> 8 : -((-value + 128) >> 8));
}

/* Gameplay records keep the integer coordinate and its subpixel byte
 * separately. Reading the ROM integer word is a signed floor, not the
 * nearest-pixel conversion used by screen placement. */
static int16_t fp_integer_word(int32_t value) {
    return (int16_t)(value >= 0 ? value / 256 :
                     -(((-value) + 255) / 256));
}

static int32_t fp_replace_integer_word(int32_t value, int16_t integer) {
    return (int32_t)integer * 256 + (int32_t)((uint32_t)value & 0xFFu);
}

static int basket_x_for_side(unsigned side) {
    return side ? 336 : -336;
}

static int basket_y_for_side(unsigned side) {
    (void)side;
    return 0;
}

static void cpu_apply_ball_acquisition_core(NbaTipoff *tipoff,uint8_t catcher);
static void ball_attach_to_actor(NbaTipoff *tipoff, unsigned owner);
static void ball_launch(NbaTipoff *tipoff, int target_x, int target_y,
                        unsigned flight_frames, int vertical_velocity,
                        NbaBallMode mode);
bool nba_tipoff_update_rom_passer(NbaTipoff *tipoff, unsigned slot);
static void cpu_enter_play_state(NbaTipoff *tipoff, NbaCpuPlayState state);
static CpuMode11Outcome cpu_dispatch_rom_mode11(NbaTipoff *tipoff,
                                                 unsigned slot,
                                                 uint8_t *direction);
static bool cpu_update_rom_shooter(NbaTipoff *tipoff, unsigned slot);
static bool cpu_update_rom_special_shooter(NbaTipoff *tipoff, unsigned slot);
static bool cpu_update_rom_layup(NbaTipoff *tipoff, unsigned slot);
static bool cpu_update_rom_special_receiver(NbaTipoff *tipoff, unsigned slot);
static bool actor_animation_resources(const NbaTipoff *tipoff,
    const NbaTipoffActor *actor, uint8_t direction,
    uint16_t *upper_resource, uint16_t *lower_resource);
static void cpu_commit_ball_acquisition(NbaTipoff *tipoff, uint8_t catcher);
static bool cpu_try_owned_ball_contact(NbaTipoff *tipoff);
static bool cpu_try_detached_shot_contact(NbaTipoff *tipoff);
static int cpu_first_pass_contact(NbaTipoff *tipoff);
static int cpu_first_loose_ball_contact(NbaTipoff *tipoff);
static bool cpu_dead_ball_contact_gate(NbaTipoff *tipoff, unsigned actor);
static int cpu_first_inbound_ball_contact(NbaTipoff *tipoff);
static void cpu_dispatch_pending_event(NbaTipoff *tipoff);
static bool cpu_update_free_throw_scene(NbaTipoff *tipoff);

/* `$86:A613-$A628`, reached by the shared `$85:A656-$A726` rectangular
 * actor/ball clamp. The port represents the proven globals it mutates; the
 * unrepresented `$0944/$094A/$09B8` words remain documented in the dump. */
static void cpu_cancel_boundary_activity(NbaTipoff *tipoff) {
    tipoff->pass_actor_raw = -1;
    tipoff->pass_receiver_raw = -1;
    tipoff->ball_activity_raw = 0u;
}

/* Complete `$86:A613-$A628` mutation. The helper deliberately leaves
 * `$093E`, `$09C4`, the ball record and its velocities intact: mode 15 and
 * `$86:CCFC-$D3C5` decide whether a canceled pass aborts before release or
 * becomes a generic loose-ball acquisition after release. */
static void cpu_cancel_rom_pass_activity(NbaTipoff *tipoff) {
    cpu_cancel_boundary_activity(tipoff);
    tipoff->pass_aux_raw = -1;
    tipoff->rim_raw_094a = 0u;
    tipoff->inbound_transfer_raw = 0u;
}

/* `$86:9846-$986C`: restore an actor from a transient receiver/action mode.
 * Native +$60 is represented by reaction_threshold for modes 10/14. */
static void cpu_restore_normal_mode(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    NbaShotAction action={0};
    nba_shot_action_restore(&action,actor->team_group_raw_6e,
                            tipoff->camera_side_group_raw);
    actor->control_mode=(uint8_t)action.mode;
    actor->behavior_timer=action.behavior_timer;
    actor->reaction_threshold=action.timer;
    actor->behavior_flags_raw=action.flags;
    actor->actor_status_raw_28=action.status;
}

/* Player movement has its own court restriction contract. Reusing the ball
 * integrator's `$86:A613` side effect here made a baseline player cancel an
 * unrelated pass every 30-Hz sweep. */
static void cpu_clamp_actor_to_court(int32_t *x_fp, int32_t *y_fp,
                                     int16_t *velocity_x,
                                     int16_t *velocity_y) {
    (void)nba_gameplay_court_clamp(
        x_fp, y_fp, velocity_x, velocity_y);
}

/* `$85:A3B7-$A813` is the ownerless-ball physics chain. Its four rectangular
 * edge branches call `$86:A613` before clamping the ball axis; the later
 * diagonal/isometric correction does not. */
static void cpu_clamp_ball_to_court(NbaTipoff *tipoff,
                                    int32_t *x_fp, int32_t *y_fp,
                                    int16_t *velocity_x,
                                    int16_t *velocity_y) {
    if (nba_gameplay_court_clamp(
            x_fp, y_fp, velocity_x, velocity_y))
        cpu_cancel_rom_pass_activity(tipoff);
}

static int cpu_select_rom_receiver(const NbaTipoff *tipoff,
                                   uint8_t passer_slot) {
    NbaGameplayReceiverState actors[NBA_GAMEPLAY_ACTOR_COUNT];
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        /* `$85:F5E4` consumes actor +$04/+$08 integer words. Subpixel
         * screen rounding can move a defender across a half-open corridor. */
        actors[i].x = fp_integer_word(tipoff->actors[i].x_fp);
        actors[i].y = fp_integer_word(tipoff->actors[i].y_fp);
        actors[i].control_mode = tipoff->actors[i].control_mode;
        actors[i].travel_direction = tipoff->actors[i].assignment_direction;
        actors[i].travel_distance = tipoff->actors[i].pair_distance;
    }
    int16_t special = tipoff->special_actor_raw == NBA_GAMEPLAY_UNKNOWN_WORD ?
                      -1 : (int16_t)tipoff->special_actor_raw;
    bool attack_right =
        tipoff->team_context[tipoff->offense_side].anchor_x_raw_0a >= 0;
    return nba_gameplay_select_pass_receiver(
        passer_slot, special, tipoff->play_selector_raw, actors,
        NBA_GAMEPLAY_ACTOR_COUNT, attack_right);
}

static uint16_t actor_distance(int dx, int dy) {
    unsigned ax = (unsigned)(dx < 0 ? -dx : dx);
    unsigned ay = (unsigned)(dy < 0 ? -dy : dy);
    unsigned high = ax > ay ? ax : ay, low = ax > ay ? ay : ax;
    return (uint16_t)(high + (low >> 2));
}

/* `$85:F5E4-$F727`: an opponent blocks the cutter when its center is in the
 * ROM's lower-inclusive/upper-exclusive actor-to-basket rectangle. It
 * traverses linked actor neighbors; the fixed ten-record C array yields the
 * same predicate. */
static bool cpu_lane_to_basket_is_clear(const NbaTipoff *tipoff,
                                        unsigned slot) {
    NbaGameplayLaneActor actors[NBA_GAMEPLAY_ACTOR_COUNT];
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        actors[i].x = fp_round(tipoff->actors[i].x_fp);
        actors[i].y = fp_round(tipoff->actors[i].y_fp);
        actors[i].team_group = (uint8_t)((i / 5u) * 5u);
    }
    return nba_gameplay_lane_to_basket_clear(
        (uint8_t)slot,
        tipoff->team_context[slot / 5u].anchor_x_raw_0a,
        actors, NBA_GAMEPLAY_ACTOR_COUNT);
}

/* `$85:B4B9-$B50D`: actor +$64 cadence selects a clear-lane cutter while
 * `$09A4` is active. This runs before the mode's formation/arrival work. */
static void cpu_update_special_actor(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    bool has_owner = tipoff->possession_actor >= 0;
    uint16_t owner_distance = UINT16_MAX;
    if (has_owner) {
        const NbaTipoffActor *owner =
            &tipoff->actors[tipoff->possession_actor];
        int dx = fp_round(owner->x_fp) - fp_round(actor->x_fp);
        int dy = fp_round(owner->y_fp) - fp_round(actor->y_fp);
        owner_distance = actor_distance(dx, dy);
    }
    nba_gameplay_special_actor_step(
        &actor->behavior_timer, actor->control_mode,
        tipoff->play_cycle_raw, has_owner,
        cpu_lane_to_basket_is_clear(tipoff, slot), owner_distance,
        (uint8_t)slot, &tipoff->special_actor_raw);
}

static NbaPlayerAnimationChannels actor_animation_channels(const NbaTipoffActor *a) {
    NbaPlayerAnimationChannels c = {
        a->animation_upper_queue_cursor_raw_18, a->animation_lower_queue_cursor_raw_1a,
        a->animation_state, a->lower_animation_state, a->base_animation_state_raw_38,
        a->rom_upper_animation_phase_raw_3a, a->rom_lower_animation_phase_raw_3c,
        a->upper_animation_accumulator_raw_42, a->lower_animation_accumulator_raw_44,
        a->upper_animation_lock_raw_46, a->lower_animation_lock_raw_48, {0}, {0}
    };
    memcpy(c.upper_queue, a->animation_upper_queue_raw_1c, sizeof(c.upper_queue));
    memcpy(c.lower_queue, a->animation_lower_queue_raw_22, sizeof(c.lower_queue));
    c.upper_phase_target = a->upper_phase_target_raw_b0;
    return c;
}

static void actor_store_animation_channels(NbaTipoffActor *a,
                                           const NbaPlayerAnimationChannels *c) {
    if (a->animation_state != c->upper_state) {
        a->upper_animation_tick = 0;
        a->upper_animation_phase_raw = 0;
    }
    if (a->lower_animation_state != c->lower_state) {
        a->lower_animation_tick = 0;
        a->lower_animation_phase_raw = 0;
    }
    a->animation_state = (uint8_t)c->upper_state;
    a->lower_animation_state = (uint8_t)c->lower_state;
    a->base_animation_state_raw_38 = (uint8_t)c->base_state;
    a->rom_upper_animation_phase_raw_3a = c->upper_phase;
    a->rom_lower_animation_phase_raw_3c = c->lower_phase;
    a->upper_animation_accumulator_raw_42 = c->upper_accumulator;
    a->lower_animation_accumulator_raw_44 = c->lower_accumulator;
    a->upper_animation_lock_raw_46 = c->upper_lock;
    a->lower_animation_lock_raw_48 = c->lower_lock;
    a->animation_upper_queue_cursor_raw_18 = c->upper_queue_cursor;
    a->animation_lower_queue_cursor_raw_1a = c->lower_queue_cursor;
    a->upper_phase_target_raw_b0 = c->upper_phase_target;
}

/* `$87:AB48-$AC22`: every successful animation publication first clears
 * actor+$28 bit15, then sets it iff resolved actor+$52 is below three. This
 * is the body mirror producer; head selection at A5FB-A609 owns only bit2. */
static void actor_publish_body_mirror(NbaTipoffActor *actor) {
    actor->actor_status_raw_28 &= 0x7fffu;
    if (actor->direction < 3u) actor->actor_status_raw_28 |= 0x8000u;
}

static void actor_animation_command(NbaTipoff *tipoff, NbaTipoffActor *actor,
    NbaPlayerAnimationCommand command, uint16_t state) {
    NbaPlayerAnimationChannels channels = actor_animation_channels(actor);
    if (!nba_player_animation_command_scratch(tipoff->assets, &channels, command,
            &state, actor->movement_boost_timer != 0,
            actor->free_throw_launch_half_raw_a8 != 0,&tipoff->scratch_0047)) return;
    tipoff->scratch_0046=(uint16_t)((tipoff->scratch_0046&255u)|(tipoff->scratch_0047<<8));
    actor_store_animation_channels(actor, &channels);
    actor->animation_resources_valid = false;
    /* `$87:AEC3` refreshes the new pose without consuming a cadence tick.
     * Adopt this only for the already-integrated live-pass action path. */
    if (actor->exact_pass_animation || actor->exact_jump_animation) {
        NbaPlayerResolvedPose pose = {0};
        pose.direction = actor->direction;
        actor->animation_resources_valid = nba_player_resolve_pose(
            tipoff->assets, &channels, actor->direction,
            actor->free_throw_launch_half_raw_a8 != 0u,
            actor->animation_variant_raw_6c, &pose);
        if (actor->animation_resources_valid) {
            actor->upper_animation_resource_raw_2a = pose.upper_resource;
            actor->lower_animation_resource_raw_2c = pose.lower_resource;
            actor_publish_body_mirror(actor);
        }
    }
}

/* EC32 caller binding. The focal record is 0910 (not always the ball),
 * paired facing uses +74, and ratings are literal pack roster +3C/+3D. */
bool nba_tipoff_jump_reach(NbaTipoff *t,unsigned slot) {
    if(!t || !t->session || slot>=NBA_GAMEPLAY_ACTOR_COUNT)return false;
    NbaTipoffActor *a=&t->actors[slot];
    unsigned pair=a->assignment_current_raw>>1;
    if(pair>=NBA_GAMEPLAY_ACTOR_COUNT)return false;
    uint8_t r3c,r3d,team=team_id_for_context(t, slot / 5u);
    if(!nba_player_gameplay_jump_ratings(t->assets,team,a->roster_slot,&r3c,&r3d))return false;
    NbaJumpReachInput in={0};
    in.actor_x=fp_integer_word(a->x_fp);in.actor_y=fp_integer_word(a->y_fp);
    in.actor_z=fp_integer_word(a->z_fp);in.lower_state=a->lower_animation_state;
    in.distance=a->focal_distance_raw_8e;in.direction=a->assignment_direction;
    in.movement=a->movement_magnitude_raw;
    const NbaTipoffActor *subject=NULL;
    if(t->catch_actor_record_raw_0910==0x3eeb) {
        in.subject_x=fp_integer_word(t->ball.x_fp);in.subject_y=fp_integer_word(t->ball.y_fp);
        in.subject_z=fp_integer_word(t->ball.z_fp);in.subject_vz=t->ball.velocity_z;
    } else {
        uint16_t offset=(uint16_t)(t->catch_actor_record_raw_0910-0x34eb);
        if((offset&255u) || (offset>>8)>=10)return false;
        subject=&t->actors[offset>>8];
        in.subject_x=fp_integer_word(subject->x_fp);in.subject_y=fp_integer_word(subject->y_fp);
        in.subject_z=fp_integer_word(subject->z_fp);in.subject_vz=subject->velocity_z;
        in.subject_direction=subject->anchor_direction_raw;
    }
    in.paired_direction=t->actors[pair].assignment_direction;
    in.ball_x=fp_integer_word(t->ball.x_fp);in.ball_z=fp_integer_word(t->ball.z_fp);in.ball_vz=t->ball.velocity_z;
    in.activity=t->ball_activity_raw;in.owner=t->possession_actor;in.receiver=t->pass_receiver_raw;
    in.live_state=t->live_state_raw;in.block_mode=t->rim_raw_0962;in.raw_0046=t->scratch_0046;
    in.velocity_x=a->velocity_x;in.velocity_y=a->velocity_y;in.velocity_z=a->velocity_z;
    in.rng=t->rng.state;in.rating_3c=r3c;in.rating_3d=r3d;
    NbaJumpReachResult out;
    ++t->jump_decision_calls;t->last_jump_input=in;
    if(!nba_jump_reach_decide(t->assets,&in,&out)) {
        ++t->jump_rejected_contexts;return false;
    }
    t->last_jump_result=out;
    a->velocity_x=(int16_t)out.velocity_x;a->velocity_y=(int16_t)out.velocity_y;
    a->velocity_z=(int16_t)out.velocity_z;t->rng.state=out.rng;
    for(unsigned i=0;i<out.request_count;++i) {
        NbaJumpReachRequest request=out.requests[i];
        if(request.routine==0x86EAA8) {
            if(!subject){++t->jump_rejected_contexts;return false;}
            NbaReachLaunch s={in.actor_x,in.actor_y,in.subject_x,in.subject_y,
                (uint16_t)subject->velocity_x,(uint16_t)subject->velocity_y,
                subject->anchor_distance_raw,(uint16_t)t->team_context[slot/5].anchor_x_raw_0a,
                a->movement_direction,a->requested_direction,0,0,0,t->shot_bounce_timer_raw_091c};
            nba_reach_launch(&s);
            a->velocity_x=(int16_t)s.velocity_x;a->velocity_y=(int16_t)s.velocity_y;a->velocity_z=(int16_t)s.velocity_z;
            a->movement_direction=(uint8_t)s.direction_4e;a->requested_direction=(uint8_t)s.direction_50;
            t->shot_bounce_timer_raw_091c=s.timer_091c;
            request=(NbaJumpReachRequest){0x87B3BD,0x32};
        }
        if(request.routine==0x86BD1F) {
            /* BD25 uses N after CMP80, not a host unsigned comparison. */
            if(((uint16_t)(t->live_state_raw-0x80u)&0x8000u)==0)continue;
            a->exact_jump_animation=true;
            actor_animation_command(t,a,NBA_ANIMATION_INSTALL_UPPER,0x13);
            if(a->animation_state==0x13)a->velocity_x=a->velocity_y=0;
        } else {
            a->exact_jump_animation=true;
            actor_animation_command(t,a,request.routine==0x87B3BD?NBA_ANIMATION_INSTALL_BOTH:
                request.routine==0x87B47A?NBA_ANIMATION_INSTALL_UPPER:NBA_ANIMATION_INSTALL_LOWER,request.value);
        }
    }
    if(out.request_count && a->velocity_z!= (int16_t)in.velocity_z)++t->jump_launches;
    return true;
}

static void actor_set_upper_animation(NbaTipoffActor *actor, uint8_t upper) {
    if (actor->animation_state == upper) return;
    actor->animation_state = upper;
    actor->upper_animation_tick = 0u;
    actor->upper_animation_phase_raw = 0u;
    actor->rom_upper_animation_phase_raw_3a = 0u;
    actor->upper_animation_accumulator_raw_42 = 0u;
    actor->animation_resources_valid = false;
}

static void actor_set_animation(NbaTipoffActor *actor, uint8_t upper,
                                uint8_t lower) {
    /* Non-pass callers still have provisional action scheduling. Migrate
     * those with their own release/cancel boundaries, not all at once. */
    actor_set_upper_animation(actor, upper);
    if (actor->lower_animation_state != lower) {
        actor->lower_animation_state = lower;
        actor->lower_animation_tick = 0u;
        actor->lower_animation_phase_raw = 0u;
        actor->rom_lower_animation_phase_raw_3c = 0u;
        actor->lower_animation_accumulator_raw_44 = 0u;
        actor->animation_resources_valid = false;
    }
}

void nba_tipoff_ease_display_direction(uint8_t desired,
                                       uint16_t upper_animation_lock,
                                       uint8_t *shown_raw, uint8_t *timer) {
    /* `$87:8F13-$8F5E`: +$52 eases toward desired +$4E. The final adjacent
     * octant is held for three logical actor passes; locked upper actions
     * snap immediately so their pose-point geometry remains coherent. */
    if (!shown_raw || !timer || desired >= 8u) return;
    if (upper_animation_lock != 0u) {
        *shown_raw = desired;
        *timer = 0u;
        return;
    }
    uint8_t shown = *shown_raw & 7u;
    uint8_t delta = (uint8_t)((desired - shown) & 7u);
    if (delta == 0u) return;
    uint8_t candidate = delta < 4u ? (uint8_t)((shown + 1u) & 7u) :
                                     (uint8_t)((shown - 1u) & 7u);
    if (candidate != desired) {
        *shown_raw = candidate;
        *timer = 0u;
        return;
    }
    if (++*timer >= 3u) {
        *shown_raw = desired;
        *timer = 0u;
    }
}

static void cpu_ease_display_direction(NbaTipoffActor *actor) {
    nba_tipoff_ease_display_direction(
        actor->movement_direction, actor->upper_animation_lock_raw_46,
        &actor->direction, &actor->facing_ease_timer_raw_be);
}

static void cpu_resolve_locomotion_animation(NbaTipoff *tipoff,
                                             unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    uint8_t state = actor->base_animation_state_raw_38;
    if (state >= 19u) return;
    uint16_t velocity_or = (uint16_t)actor->velocity_x |
                           (uint16_t)actor->velocity_y;
    bool is_stationary = velocity_or < 5u;
    if (!is_stationary) actor->catcher_latch_raw_ae = 0u;
    state = nba_player_locomotion_state(
        state, is_stationary, actor->movement_boost_timer != 0u,
        tipoff->possession_actor == (int8_t)slot,
        fp_integer_word(actor->z_fp) != 0);
    bool alternate_lower = actor->free_throw_launch_half_raw_a8 != 0u;
    /* `$87:B5FC/$B62F`: changing locomotion state clears only the channel
     * accumulator. The existing phase survives when the new descriptor has
     * that frame; resetting it unconditionally caused the visible wiggle. */
    if (actor->upper_animation_lock_raw_46 == 0u &&
        actor->animation_state != state) {
        uint16_t count = 0u;
        actor->animation_state = state;
        actor->upper_animation_tick = 0u; /* legacy collision phase */
        actor->upper_animation_accumulator_raw_42 = 0u;
        if (!nba_player_animation_frame_count(tipoff->assets, true, state,
                                               alternate_lower, &count) ||
            actor->rom_upper_animation_phase_raw_3a >= count)
            actor->rom_upper_animation_phase_raw_3a = 0u;
        actor->animation_resources_valid = false;
    }
    if (actor->lower_animation_lock_raw_48 == 0u &&
        actor->lower_animation_state != state) {
        uint16_t count = 0u;
        actor->lower_animation_state = state;
        actor->lower_animation_tick = 0u; /* legacy collision phase */
        actor->lower_animation_accumulator_raw_44 = 0u;
        if (!nba_player_animation_frame_count(tipoff->assets, false, state,
                                               alternate_lower, &count) ||
            actor->rom_lower_animation_phase_raw_3c >= count)
            actor->rom_lower_animation_phase_raw_3c = 0u;
        actor->animation_resources_valid = false;
    }
}

static void cpu_advance_actor_animation(NbaTipoff *tipoff,
                                        NbaTipoffActor *actor) {
    ++actor->upper_animation_tick;
    ++actor->lower_animation_tick;
    if (actor->exact_jump_animation || actor->upper_animation_lock_raw_46 != 0u ||
        actor->animation_state == 7u || actor->animation_state == 13u || actor->animation_state == 18u ||
        actor->lower_animation_lock_raw_48 != 0u ||
        (actor->control_mode == 15u && actor->exact_pass_animation) ||
        ((actor->control_mode == 12u || actor->control_mode == 17u) && actor->exact_shot_animation)) {
        NbaPlayerAnimationChannels channels = actor_animation_channels(actor);
        actor->animation_resources_valid = nba_player_animation_step_channels(
            tipoff->assets, &channels, actor->direction,
            (uint16_t)(actor->movement_magnitude_raw << 1), 0x200u,
            actor->free_throw_launch_half_raw_a8 != 0u, actor->animation_variant_raw_6c,
            &tipoff->rng.state,
            &actor->upper_animation_resource_raw_2a,
            &actor->lower_animation_resource_raw_2c);
        if (actor->animation_resources_valid)
            actor_store_animation_channels(actor, &channels);
    } else {
        /* Ordinary locomotion uses the common descriptor cadence. Mode-2
         * idle/held states above consume their exact shared RNG/duration. */
        actor->animation_resources_valid = nba_player_animation_rom_step(
            tipoff->assets, actor->animation_state,
            actor->lower_animation_state, actor->direction,
            (uint16_t)(actor->movement_magnitude_raw << 1),
            actor->free_throw_launch_half_raw_a8 != 0u,
            actor->animation_variant_raw_6c,
            &actor->upper_animation_accumulator_raw_42,
            &actor->lower_animation_accumulator_raw_44,
            &actor->rom_upper_animation_phase_raw_3a,
            &actor->rom_lower_animation_phase_raw_3c,
            &actor->upper_animation_resource_raw_2a,
            &actor->lower_animation_resource_raw_2c);
    }
    /* Other action branches ported before the descriptor accumulator consume
     * their verified logical-tick phase. Keep that integration boundary
     * stable while the exact ROM phases above drive the visible resources. */
    uint16_t lower_phase = 0u;
    if (nba_player_animation_phases(
            tipoff->assets, actor->animation_state,
            actor->lower_animation_state, actor->upper_animation_tick,
            actor->lower_animation_tick,
            &actor->upper_animation_phase_raw, &lower_phase))
        actor->lower_animation_phase_raw = lower_phase;
    /* Action release gates consume the same phase that the locked resource
     * advances. Keeping the old half-rate tick here can finish/unlock a pass
     * before its release threshold is ever reached. Ordinary locomotion's
     * legacy contact phase remains a separate integration boundary; owned
     * ball physics reads the literal ROM phase directly. */
    if (actor->animation_resources_valid &&
        (actor->upper_animation_lock_raw_46 != 0u ||
         (actor->control_mode == 15u && actor->exact_pass_animation) ||
         ((actor->control_mode == 12u || actor->control_mode == 17u) && actor->exact_shot_animation)))
        actor->upper_animation_phase_raw = actor->rom_upper_animation_phase_raw_3a;
    if (actor->animation_resources_valid && actor->lower_animation_lock_raw_48 != 0u)
        actor->lower_animation_phase_raw = actor->rom_lower_animation_phase_raw_3c;
    if (actor->animation_resources_valid) actor_publish_body_mirror(actor);
}

static int16_t pass_predict_component(int16_t value, unsigned shift) {
    return nba_gameplay_arithmetic_shift_right(value, shift);
}

static int32_t pass_lead_component(int16_t velocity, uint16_t duration) {
    int32_t product = (int32_t)velocity * duration;
    if (product >= 0) return product >> 8;
    return -(((-product) + 255) >> 8);
}

static uint8_t pass_band_from_distance(uint16_t distance) {
    return distance < 0x41u ? 0u : distance < 0x79u ? 1u :
           distance < 0xC9u ? 2u : distance < 0x119u ? 3u :
           distance < 0x191u ? 4u : 5u;
}

/* `$86:AB2D-$AF65`: grounded mode-15 setup. This includes the aligned
 * `$2A-$2C` selector at `$86:AE52-$AED7`; airborne `$AFC4` and catch-preinit
 * `$AF66` still depend on raw writers not represented by the port. */
bool nba_tipoff_begin_rom_pass(NbaTipoff *tipoff, unsigned passer_slot,
                               unsigned receiver_slot) {
    NbaTipoffActor *passer = &tipoff->actors[passer_slot];
    NbaTipoffActor *receiver = &tipoff->actors[receiver_slot];
    /* `$86:AB83-$ABE5` reads the signed integer position words directly;
     * subpixel bytes do not round either endpoint before the velocity lead. */
    int16_t passer_x = (int16_t)(fp_integer_word(passer->x_fp) +
        pass_predict_component(passer->velocity_x, 4u));
    int16_t passer_y = (int16_t)(fp_integer_word(passer->y_fp) +
        pass_predict_component(passer->velocity_y, 4u));
    int16_t receiver_x = (int16_t)(fp_integer_word(receiver->x_fp) +
        pass_predict_component(receiver->velocity_x, 3u));
    int16_t receiver_y = (int16_t)(fp_integer_word(receiver->y_fp) +
        pass_predict_component(receiver->velocity_y, 3u));
    uint16_t distance = 0u;
    uint8_t fine = nba_gameplay_pass_direction(
        (int16_t)(receiver_x - passer_x),
        (int16_t)(receiver_y - passer_y), &distance);
    uint8_t pass_direction = fine >> 1;
    if (pass_direction >= 8u) return false;

    uint8_t band = pass_band_from_distance(distance);
    uint8_t relative = (uint8_t)(
        (pass_direction - passer->movement_direction) & 7u);
    if (fp_integer_word(passer->z_fp) != 0) return false;
    /* `$86:AB3D` cancels the prior upper action before installing a pass. */
    actor_animation_command(tipoff, passer, NBA_ANIMATION_CANCEL_UPPER, 0u);
    passer->exact_pass_animation = tipoff->live_state_raw < 0x80u;
    uint8_t upper = 0u;
    int16_t family = -1;
    bool airborne_family = false;
    /* `$86:AB7D-$AB80` seeds every receiver with mode 10/timer $28. The
     * side/back path alone overwrites it with $3C at `$86:ACA9-$ACAE`. */
    uint16_t receiver_timer = 0x28u;
    if (relative >= 3u && relative < 6u) {
        receiver_timer = 0x3Cu;
        uint8_t team = team_id_for_context(tipoff, passer_slot / 5u);
        uint8_t profile_39 = 0u, profile_3e = 0xFFu;
        (void)nba_player_gameplay_pass_profiles(
            tipoff->assets, team, passer->roster_slot,
            &profile_39, &profile_3e);
        (void)profile_39; /* catch-preinit input, deliberately gated above. */
        bool stationary = (passer->velocity_x | passer->velocity_y) == 0;
        bool forced_special = profile_3e < 0x55u ||
            passer->lower_animation_state == 0x09u ||
            passer->lower_animation_state == 0x0Bu;
        /* `$86:AC87-$AC96` selects the still-unported `$AFC4` family for
         * a long, boosted, otherwise-normal grounded pass. */
        if (!forced_special && distance >= 0x119u &&
            passer->movement_boost_timer != 0u) {
            /* `$86:AC87-$AC96 -> $86:AFC4-$B00A`: long boosted pass.
             * Receiver +$60 becomes $46; passer +$C0 selects table 2,
             * planar velocity is halved, and the independent upper/lower
             * resources are `$18/$1F`. `$86:A629` supplies the jump launch
             * which later changes family 2 to release family 4 at the apex. */
            receiver_timer = 0x46u;
            family = 2;
            upper = 0x18u;
            airborne_family = true;
            passer->velocity_x = nba_gameplay_arithmetic_shift_right(
                passer->velocity_x, 1u);
            passer->velocity_y = nba_gameplay_arithmetic_shift_right(
                passer->velocity_y, 1u);
            passer->velocity_z = 0x0258;
        }
        bool grounded_special = forced_special || stationary;
        if (airborne_family) {
            /* Selection is complete; do not fall through into `$ACB1`. */
        } else if (grounded_special) {
            /* `$86:B00B-$B04A`: upper-only state `$2F`, family 1. */
            receiver_timer = 0x50u;
            passer->velocity_x = passer->velocity_y = 0;
            passer->movement_magnitude_raw = 0u;
            passer->behavior_flags_raw |= 0x0006u;
            family = 5;
            upper = 0x2Fu;
        } else {
            /* `$86:ACB1-$AD0B`: normal side/back pass. */
            int selector = relative;
            if (relative == 4u) {
                uint8_t fine_relative = (uint8_t)(
                    (fine - passer->movement_direction -
                     passer->movement_direction) & 15u);
                if (fine_relative == 8u) {
                    passer->movement_direction = (uint8_t)(
                        (passer->movement_direction + 1u) & 7u);
                    relative = (uint8_t)(
                        (pass_direction - passer->movement_direction) & 7u);
                    selector = relative;
                /* `$86:ACF4-$ACFB` maps fine-relative 9 to selector 3 and
                 * the opposite oblique to 5. Preserve that asymmetric table
                 * order: reversing it swaps the native `$2D/$2E` pose. */
                } else selector = fine_relative == 9u ? 3 : 5;
            }
            int16_t sign_test = (int16_t)(selector * 2 - 7);
            if (passer->movement_direction < 3u)
                sign_test = (int16_t)~sign_test;
            upper = sign_test < 0 ? 0x2Eu : 0x2Du;
        }
    } else {
        /* `$86:AE10-$AE4F`: off-axis route. */
        int selector = relative;
        if (relative == 0u) {
            selector = (fine - passer->movement_direction -
                        passer->movement_direction) & 15u;
            if (selector == 0) {
                /* `$86:AE52-$AED7`: captured inbound layout 5 forces the
                 * straight-ahead `$2B` family. This is the route reached by
                 * `$86:F43A-$F653` after a made basket. It deliberately
                 * retains the common `$28` receiver timer seeded above. */
                if (tipoff->live_state_raw == 0x82u &&
                    tipoff->inbound_layout_raw == 5) {
                    upper = 0x2Bu;
                    family = -1;
                } else {
                    /* `$86:AEC3-$AECD` is the native conservative aligned
                     * fallback. For distances below `$F1`, `$86:AEF2-AEFE`
                     * promotes `$2C` to the shared `$2F` visible pose while
                     * preserving family 1. */
                    upper = distance < 0xF1u ? 0x2Fu : 0x2Cu;
                    family = 1;
                }
            }
        }
        if (upper == 0u) {
            bool choose_30 = selector < 3;
            if (passer->movement_direction < 3u) choose_30 = !choose_30;
            upper = choose_30 ? 0x30u : 0x31u;
        }
    }

    uint8_t threshold = 0u;
    if (!airborne_family && !nba_assets_gameplay_pass_release_threshold(
            tipoff->assets, upper, &threshold)) return false;
    passer->pass_band_raw = (uint16_t)(band * 6u);
    /* +$66 is animation-resource state, not the coarse pass direction.
     * `$87:B47A/$B649` may rewrite it while resolving the selected pose. */
    passer->pass_family_raw = family;
    passer->pass_release_threshold_raw = threshold;
    passer->pass_released_raw = false;
    passer->control_mode = 15u;
    passer->behavior_flags_raw |= 0x0006u;
    receiver->control_mode = 10u;
    receiver->reaction_threshold = receiver_timer;
    if (passer->exact_pass_animation) {
        actor_animation_command(tipoff, passer, NBA_ANIMATION_INSTALL_UPPER, upper);
        if (airborne_family)
            actor_animation_command(tipoff, passer, NBA_ANIMATION_INSTALL_LOWER, 0x1Fu);
    } else if (airborne_family) {
        actor_set_animation(passer, upper, 0x1Fu);
    } else {
        actor_set_upper_animation(passer, upper);
    }
    passer->upper_animation_phase_raw = 0u;
    tipoff->pass_actor_raw = (int16_t)passer_slot;
    tipoff->pass_receiver_raw = (int16_t)receiver_slot;
    tipoff->pass_active_raw = 1u;
    tipoff->pass_distance_raw = distance;
    if (tipoff->live_state_raw == 0x82u)
        tipoff->inbound_transfer_raw = 1u;
    else if (tipoff->live_state_raw < 0x80u)
        tipoff->live_state_raw = 2u;
    /* `$86:AF1D-$AF21` resolves the newly selected resource before the
     * initializer returns, so the first visible pass frame already owns the
     * correct hand attachment. */
    ball_attach_to_actor(tipoff, passer_slot);
    return true;
}

static void cpu_predicted_ball_xy(const NbaTipoff *tipoff,
                                  int16_t *x, int16_t *y) {
    /* `$85:A76D-$A79E` writes `$0918/$091A` after ball integration. Four
     * signed RORs on 8.8 velocity are a 16-simulation-tick lookahead. */
    int16_t predicted_x = (int16_t)(
        (uint16_t)fp_integer_word(tipoff->ball.x_fp) +
        (uint16_t)nba_gameplay_arithmetic_shift_right(
            tipoff->ball.velocity_x, 4u));
    int16_t predicted_y = (int16_t)(
        (uint16_t)fp_integer_word(tipoff->ball.y_fp) +
        (uint16_t)nba_gameplay_arithmetic_shift_right(
            tipoff->ball.velocity_y, 4u));
    if (x) *x = predicted_x;
    if (y) *y = predicted_y;
}

static void cpu_cache_predicted_ball_xy(NbaTipoff *tipoff) {
    cpu_predicted_ball_xy(tipoff, &tipoff->role_focal_x_raw_0918,
                          &tipoff->role_focal_y_raw_091a);
}

static bool cpu_refresh_defense_target(NbaTipoff *tipoff, unsigned slot,
                                       bool *stop_velocity) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    unsigned paired_slot = actor->assignment_current_raw >> 1;
    if (paired_slot >= NBA_GAMEPLAY_ACTOR_COUNT ||
        paired_slot / 5u == slot / 5u) {
        paired_slot = actor->assignment_base_raw >> 1;
        if (paired_slot >= NBA_GAMEPLAY_ACTOR_COUNT ||
            paired_slot / 5u == slot / 5u) return false;
        actor->assignment_current_raw = actor->assignment_base_raw;
    }
    actor->assignment_actor = (uint8_t)paired_slot;
    NbaTipoffActor *paired = &tipoff->actors[paired_slot];
    /* E6B7/E9B3 read actor +$04/+$08 integer words. Subpixel rounding can
     * cross a target-direction boundary one scheduled pass too early. */
    int16_t actor_x = fp_integer_word(actor->x_fp);
    int16_t actor_y = fp_integer_word(actor->y_fp);
    int16_t paired_x = fp_integer_word(paired->x_fp);
    int16_t paired_y = fp_integer_word(paired->y_fp);

    /* `$85:BC52-$BC81`: refresh coarse +$86 and shared +$8A only at the
     * bound-pair decision boundary, not by rebuilding assignments per frame. */
    uint16_t pair_distance = 0u;
    uint8_t pair_direction = nba_gameplay_target_direction(
        (int16_t)(paired_x - actor_x), (int16_t)(paired_y - actor_y),
        &pair_distance);
    actor->assignment_direction = pair_direction;
    actor->assignment_distance = pair_distance;
    actor->pair_distance = pair_distance;
    paired->assignment_direction = pair_direction < 8u ?
        (uint8_t)(pair_direction ^ 4u) : pair_direction;
    paired->assignment_distance = pair_distance;
    paired->pair_distance = pair_distance;

    /* `$85:AFC2-$AFE5`: actor +$88/+8C are fine direction and distance to
     * that actor's own team-context basket anchor. */
    int16_t paired_anchor = paired_slot < 5u ? -336 : 336;
    paired->anchor_direction_raw = nba_gameplay_pass_direction(
        (int16_t)(paired_anchor - paired_x), (int16_t)(-paired_y),
        &paired->anchor_distance_raw);

    uint8_t paired_team = team_id_for_context(tipoff, paired_slot / 5u);
    uint8_t rating_2pt = 0u, rating_3pt = 0u;
    (void)nba_player_gameplay_shot_ratings(
        tipoff->assets, paired_team, paired->roster_slot,
        &rating_2pt, &rating_3pt);
    (void)rating_2pt;
    unsigned side = slot / 5u;
    int16_t context_anchor = side == 0u ? -336 : 336;
    NbaGameplayDefenseTargetInput input = {
        .actor_x = actor_x, .actor_y = actor_y,
        .actor_pair_direction_raw_86 = actor->assignment_direction,
        .actor_pair_distance_raw_8a = actor->assignment_distance,
        .paired_x = paired_x, .paired_y = paired_y,
        .paired_velocity_x = paired->velocity_x,
        .paired_velocity_y = paired->velocity_y,
        .paired_anchor_direction_raw_88 = paired->anchor_direction_raw,
        .paired_anchor_distance_raw_8c = paired->anchor_distance_raw,
        .paired_position_raw_92 = paired->assignment_role_raw_92,
        .context_anchor_x = context_anchor,
        .context_mode_raw_30 = tipoff->team_context[side].mode_raw_30,
        .context_flags_raw_32 = tipoff->team_context[side].flags_raw_32,
        .paired_on_three_point_arc = nba_gameplay_shot_value(
            false, paired_x, paired_y, paired_slot >= 5u) == 3u,
        .paired_three_point_rating = rating_3pt
    };
    NbaGameplayDefenseTargetOutput output = {0};
    if (!nba_gameplay_defense_mode_target(
            actor->control_mode, &input, &output)) return false;
    if (output.target_written) {
        actor->target_x = output.target_x;
        actor->target_y = output.target_y;
    }
    if (stop_velocity) *stop_velocity = output.stop_velocity;

    /* `$86:E3E1-$E4A6` follows the defensive target calculation. It owns
     * the stationary state-7 selector and lateral state 8/10 presentation;
     * the former planner skipped this caller and left every defender on the
     * generic locomotion base. */
    NbaGameplayDefensivePoseInput pose_input = {
        .actor_z = fp_round(actor->z_fp),
        .free_throw_state_raw_0978 = tipoff->fouls.free_throw_state_raw_0978,
        .live_state_raw_0936 = tipoff->live_state_raw,
        .owner_actor_raw_093e = tipoff->possession_actor,
        .receiver_actor_raw_0946 = tipoff->pass_receiver_raw,
        .context_anchor_x_raw_0a = context_anchor,
        .actor_x = actor_x,
        .control_mode = actor->control_mode,
        .actor_movement_raw_4c = actor->movement_magnitude_raw,
        .paired_movement_raw_4c = paired->movement_magnitude_raw,
        .actor_pair_distance_raw_8a = actor->assignment_distance,
        .actor_pair_direction_raw_86 = actor->assignment_direction,
        .actor_anchor_distance_raw_8c = actor->anchor_distance_raw,
        .paired_anchor_distance_raw_8c = paired->anchor_distance_raw,
        .velocity_x = actor->velocity_x,
        .velocity_y = actor->velocity_y,
        .upper_state_raw_30 = actor->animation_state,
        .base_state_raw_38 = actor->base_animation_state_raw_38,
        .facing_raw_4e = actor->movement_direction,
        .requested_direction_raw_50 = actor->requested_direction,
        .selected_count_raw_1868 = tipoff->defensive_pose_count_raw_1868
    };
    NbaGameplayDefensivePoseOutput pose_output;
    if (!nba_gameplay_defensive_pose(&pose_input, &pose_output)) return false;
    actor->movement_direction = pose_output.facing_raw_4e;
    actor->requested_direction = pose_output.requested_direction_raw_50;
    tipoff->defensive_pose_count_raw_1868 =
        pose_output.selected_count_raw_1868;
    if (pose_output.install_both)
        actor_animation_command(tipoff, actor, NBA_ANIMATION_INSTALL_BOTH,
                                pose_output.install_state);
    else
        actor->base_animation_state_raw_38 = pose_output.base_state_raw_38;
    return true;
}

bool nba_tipoff_replay_defensive_pose(NbaTipoff *tipoff, uint8_t actor) {
    if (!tipoff || actor >= NBA_GAMEPLAY_ACTOR_COUNT) return false;
    bool stop_velocity = false;
    return cpu_refresh_defense_target(tipoff, actor, &stop_velocity);
}

/* Proven passive behavior executors from `$87:9244/$87:9BD3`. Returning
 * true means the mode consumed this actor's scheduled pass. Modes 1-6 and
 * the active ball handlers remain in the explicitly provisional planner. */
static bool cpu_apply_passive_mode(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    if (actor->control_mode == 0u) { /* `$87:9C1B`: no-op wrapper. */
        actor->velocity_x = actor->velocity_y = 0;
        actor_set_animation(actor, 0u, 0u);
        return true;
    }
    if (actor->control_mode == 9u) { /* `$86:F0B7`: timed target override. */
        /* `$86:F0B7-$F0FC`: +$7A pauses the override clock. Once active,
         * values >=10 keep steering; the final 0..9 window holds position.
         * Expiry restores the mode saved in overloaded +$62, not the
         * per-frame +$84 snapshot. */
        if (actor->recovery_inhibit_raw != 0u) {
            actor->velocity_x = actor->velocity_y = 0;
            return true;
        }
        uint16_t remaining = (uint16_t)(actor->reaction_threshold - 2u);
        actor->reaction_threshold = remaining;
        if ((remaining & 0x8000u) != 0u) {
            actor->reaction_threshold = 0u;
            actor->movement_boost_timer = 0u;
            actor->control_mode = (uint8_t)actor->pass_band_raw;
            actor->action_state = 3u;
            actor_set_animation(actor, actor->animation_state, 3u);
            return true;
        }
        if (remaining < 10u) {
            actor->velocity_x = actor->velocity_y = 0;
            return true;
        }
        return false;
    }
    if (actor->control_mode == 16u) { /* `$86:B0F7-$B153`: post-shot hold. */
        /* `$86:B0F9-$B0FC` marks this actor as held before the timer pass.
         * This is not an animation install; the current pose/resources must
         * survive because `$87:B832` samples attachment point one from it. */
        actor->behavior_flags_raw = 1u;
        uint16_t remaining = (uint16_t)(actor->reaction_threshold - 2u);
        actor->reaction_threshold = remaining;
        if ((remaining & 0x8000u) != 0u) {
            /* `$86:B10A-$B122`: clear the hold marker, run the common
             * `$86:9846` restore, then override it with mode 7/+60=$B4. */
            cpu_restore_normal_mode(tipoff, slot);
            actor->control_mode = 7u;
            actor->reaction_threshold = 0xB4u;
        } else {
            actor->velocity_x = actor->velocity_y = actor->velocity_z = 0;
            /* `$86:B126-$B153`: `$87:B832` receives DP $00=1 and samples
             * attachment point one from the already-resolved +$2A/+$2C
             * resources. Actor +$28 supplies the ROM mirror masks. */
            int16_t offset_x = 0, offset_y = 0, offset_z = 0;
            if (actor->animation_resources_valid &&
                nba_player_ball_attachment_point_offsets(
                    tipoff->assets,
                    actor->upper_animation_resource_raw_2a,
                    actor->lower_animation_resource_raw_2c,
                    actor->actor_status_raw_28, 1u,
                    &offset_x, &offset_y, &offset_z)) {
                int16_t basket_x =
                    (int16_t)tipoff->court_presentation.basket_x_3fef;
                if (basket_x == 0)
                    basket_x = (int16_t)basket_x_for_side(tipoff->offense_side);
                actor->x_fp = (int32_t)(int16_t)(
                    basket_x - offset_x) * 256 +
                    (actor->x_fp & 255);
                actor->y_fp = (int32_t)(int16_t)(
                    -offset_y) * 256 +
                    (actor->y_fp & 255);
                actor->z_fp = (int32_t)(int16_t)(80 - offset_z) * 256 +
                    (actor->z_fp & 255);
            }
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

bool nba_tipoff_replay_passive_mode(NbaTipoff *tipoff, uint8_t actor) {
    return tipoff && actor < NBA_GAMEPLAY_ACTOR_COUNT &&
           cpu_apply_passive_mode(tipoff, actor);
}

/* `$87:9BD3` maps behavior mode 10 to wrapper `$87:9C3A`, which calls
 * `$86:A5B0-$A628`. A valid `$0946` receiver counts actor +$60 down by the
 * current actor delta. Once it expires, or whenever `$0946` is negative,
 * `$86:9846-$986C` restores the actor to the ordinary offense/defense mode
 * selected by actor +$6E versus `$093A`. The previous host implementation
 * skipped this executor and therefore allowed stale receivers to coast to a
 * court edge forever after A613 cleared the pass globals. */
static bool cpu_update_rom_receiver(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    if (actor->control_mode != 10u) return false;
    bool receiver_valid = tipoff->pass_receiver_raw >= 0;
    if (receiver_valid) {
        uint16_t remaining = (uint16_t)(actor->reaction_threshold - 2u);
        actor->reaction_threshold = remaining;
        if ((remaining & 0x8000u) == 0u) return true;
    }
    /* `$86:9846`: offense group `$093A` receives mode 1; the other group
     * receives mode 2. It also clears +$60, +$7E and +$28. */
    cpu_restore_normal_mode(tipoff, slot);
    return false;
}

static bool cpu_active_decision_due(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    uint8_t mode = actor->control_mode;
    if ((mode < 1u || mode > 6u) && mode != 11u) return true;
    uint8_t team = team_id_for_context(tipoff, slot / 5u);
    uint8_t profile_3f = 0u, profile_40 = 0u;
    (void)nba_player_gameplay_decision_profiles(
        tipoff->assets, team, actor->roster_slot,
        &profile_3f, &profile_40);
    int16_t actor_x = fp_round(actor->x_fp);
    /* Modes 2/4/6 compare signed-word actor +$04 against side context +$0A.
     * `$87:8EFE/$8F11` keeps DP $9E at $46EB for slots 0..4 and $476B for
     * slots 5..9; full words `$FEB0/$0150` are -336/+336. This is
     * deliberately not a ball-position or matchup comparison. */
    int16_t side_anchor = tipoff->team_context[slot / 5u].anchor_x_raw_0a;
    bool same_half = nba_gameplay_same_x_half(actor_x, side_anchor);
    if (mode == 1u || mode == 3u || mode == 5u || mode == 11u)
        return nba_gameplay_decision_timer_step(
            &actor->reaction_threshold, profile_3f, 0x40u, false);
    /* Mode 2 reloads `$30 + profile[$40]`; modes 4/6 use `$20`.
     * All three add another `$20` when actor/related X signs match. */
    return nba_gameplay_decision_timer_step(
        &actor->reaction_threshold, profile_40,
        mode == 2u ? 0x30u : 0x20u, same_half);
}

/* `$86:E5AB-$E612`: finalize +$50 after a normal CPU behavior decision.
 * CPU actors keep their current +$4E unless they are already inside the
 * target's [-20,+19] box, where they may face the loose ball. Human actors
 * use the same ball-facing result only while planar velocity is inside
 * [-128,+127]. */
static void cpu_finalize_requested_direction(NbaTipoff *tipoff,
                                             unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    bool use_ball = actor->lower_animation_state != 0x25u;
    if (use_ball && actor->controller_assignment_raw < 0) {
        int16_t dx = (int16_t)(actor->target_x -
                               fp_integer_word(actor->x_fp));
        int16_t dy = (int16_t)(actor->target_y -
                               fp_integer_word(actor->y_fp));
        use_ball = dx >= -20 && dx < 20 && dy >= -20 && dy < 20;
    } else if (use_ball) {
        use_ball = actor->velocity_x >= -128 && actor->velocity_x < 128 &&
                   actor->velocity_y >= -128 && actor->velocity_y < 128;
    }
    uint8_t requested = actor->movement_direction;
    if (use_ball) {
        uint8_t toward_ball = nba_gameplay_target_direction(
            (int16_t)(fp_integer_word(tipoff->ball.x_fp) -
                      fp_integer_word(actor->x_fp)),
            (int16_t)(fp_integer_word(tipoff->ball.y_fp) -
                      fp_integer_word(actor->y_fp)), NULL);
        if (toward_ball != 8u) requested = toward_ball;
    }
    actor->requested_direction = requested;
}

bool nba_tipoff_replay_requested_direction(NbaTipoff *tipoff,
                                           uint8_t actor) {
    if (!tipoff || actor >= NBA_GAMEPLAY_ACTOR_COUNT) return false;
    cpu_finalize_requested_direction(tipoff, actor);
    return true;
}

/* `$85:AE35-$AE95`: during the left-baseline state-$82 inbound plays 6..9,
 * scan the current actor's side from roster role 4 down to role 0. The first
 * CPU actor that is not provisional `$0954` receives the ROM's exact
 * `(-40,160)` formation target. This is a side effect on a teammate, not a
 * receiver substitution in `$86:F5C7`; preserving that distinction keeps the
 * later native selector and side gates authoritative. */
static int cpu_apply_inbound_formation_override(NbaTipoff *tipoff,
                                                unsigned current_slot) {
    if (tipoff->live_state_raw != 0x82u || tipoff->play_code < 6u ||
        tipoff->play_code >= 10u || tipoff->inbound_target_y_raw < 0 ||
        tipoff->inbound_target_x_raw >= 0) return -1;
    unsigned base = current_slot < 5u ? 0u : 5u;
    for (int role = 4; role >= 0; --role) {
        unsigned slot = base + (unsigned)role;
        if (slot == tipoff->inbound_actor_raw) continue;
        if (tipoff->actors[slot].controller_assignment_raw < 0) {
            tipoff->actors[slot].target_x = -40;
            tipoff->actors[slot].target_y = 160;
            return (int)slot;
        }
    }
    return -1;
}

/* `$85:AD6B-$AF5B`: install one formation coordinate per play-step and run
 * the normal `$85:B402` completion route, including the short-timer
 * DP-$5C/actor-+$5C override at `$AE97-$AEBB`. */
static bool cpu_formation_route(NbaTipoff *tipoff, unsigned slot,
                                uint8_t *direction) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    uint8_t role = (uint8_t)(slot % 5u);
    /* `$85:AD6B-$AD77`: a nonzero dead-ball height makes the current owner
     * return without touching target or steering scratch. */
    if (tipoff->possession_actor == (int8_t)slot &&
        tipoff->dead_ball_raw_0968 != 0u) return false;
    if (tipoff->special_actor_raw == slot) {
        /* `$85:AE1F-$AE32`: the selected cutter never owns the formation
         * install latch while it targets the team basket anchor. */
        actor->behavior_flags_raw &= 0xFFF7u;
        actor->target_x = tipoff->team_context[slot / 5u].anchor_x_raw_0a;
        actor->target_y = 0;
    } else if (!(tipoff->live_state_raw == 0x82u &&
                 tipoff->inbound_actor_raw == slot) &&
               (actor->behavior_flags_raw & 0x0008u) == 0u) {
        int16_t target_x = actor->target_x, target_y = actor->target_y;
        /* `$85:ADF5-$AE1A` reads the live context +$0A. Halftime reverses
         * this sign without swapping actor groups or roster slots. */
        int16_t side_anchor_x =
            tipoff->team_context[slot / 5u].anchor_x_raw_0a;
        if (nba_assets_gameplay_formation_offset(
                tipoff->assets, (uint8_t)tipoff->play_code, role,
                (uint8_t)tipoff->play_step_raw,
                tipoff->play_mirror_raw != 0u, side_anchor_x,
                &target_x, &target_y)) {
            actor->target_x = target_x;
            actor->target_y = target_y;
            actor->behavior_flags_raw |= 0x0008u;
        }
    }

    (void)cpu_apply_inbound_formation_override(tipoff, slot);

    /* `$85:B408/$B44B` consume actor +$04/+$08 integer words directly.
     * Screen-space nearest-pixel rounding changes octants at subpixel edges. */
    int16_t actor_x = fp_integer_word(actor->x_fp);
    int16_t actor_y = fp_integer_word(actor->y_fp);
    if (tipoff->formation_override_raw_005c != 0u &&
        actor->formation_timer_raw_5c < 0x78u) {
        actor->target_x = actor_x;
        actor->target_y = actor_y < 0 ? -80 : 80;
    }
    bool opposite_x_sign = (int16_t)(actor->target_x ^ actor_x) < 0;
    bool special_edge = (tipoff->ball_activity_raw | tipoff->rim_raw_097c) != 0u &&
                        role >= 3u;
    if (special_edge) {
        /* `$85:AED4-$AEDC/$85:AF2A-$AF3D`: activity routes back roles to
         * the court edge on either side of midcourt. */
        int16_t edge_x = actor_x < 0 ? -0x152 : 0x152;
        int16_t edge_y = actor_y < 0 ? -16 : 16;
        uint8_t steering = 8u;
        (void)nba_gameplay_predictive_arrival(
            actor_x, actor_y, actor->velocity_x, actor->velocity_y,
            edge_x, edge_y, 16u, &steering, NULL);
        if (direction) *direction = steering;
        return true;
    }
    if (opposite_x_sign) {
        /* `$85:AEC7-$AEF3`: without the special activity edge case, every
         * role crosses midcourt through local X=+/-16 using `$85:B3AA`.
         * The former role<3 shortcut made roles 3/4 slide on the wrong axis. */
        int16_t gate_x = actor->target_x < 0 ? -16 : 16;
        if (direction) *direction = nba_gameplay_target_direction(
            (int16_t)(gate_x - actor_x),
            (int16_t)(actor->target_y - actor_y), NULL);
        return true;
    }

    uint8_t steering = 8u;
    if (nba_gameplay_predictive_arrival(
            actor_x, actor_y, actor->velocity_x, actor->velocity_y,
            actor->target_x, actor->target_y, 16u, &steering, NULL))
        actor->behavior_flags_raw |= 0x0040u;
    if (direction) *direction = steering;
    return true;
}

static void cpu_owner_accelerate(NbaTipoff *t,unsigned slot,uint8_t direction);

bool nba_tipoff_replay_formation_route(NbaTipoff *tipoff, uint8_t actor,
                                       uint8_t *direction) {
    if (!tipoff || actor >= NBA_GAMEPLAY_ACTOR_COUNT) return false;
    bool ran = cpu_formation_route(tipoff, actor, direction);
    if (ran) cpu_owner_accelerate(tipoff, actor, direction ? *direction : 8u);
    return ran;
}

/* Common `$85:96B5-$973A` actor Z integration. `$C6=2` makes gravity
 * `$18*$C6=$30` and the fixed-point displacement uses the same dt. */
static void cpu_integrate_actor_vertical(NbaTipoffActor *actor) {
    if (actor->z_fp == 0 && actor->velocity_z == 0) return;
    actor->velocity_z = (int16_t)(actor->velocity_z - 0x0030);
    actor->z_fp += (int32_t)actor->velocity_z * 2;
    if (actor->z_fp < 0) {
        actor->z_fp = 0;
        actor->velocity_z = 0;
    }
}

/* `$86:F43A-$F4E2`: mode 11 has a dedicated dead-ball steering branch.
 * Its target compensation uses the ROM's negative-quotient +1 bias. */
static bool cpu_move_inbound_actor(NbaTipoff *tipoff, unsigned slot) {
    if (tipoff->live_state_raw != 0x82u ||
        tipoff->possession_actor != (int8_t)slot ||
        tipoff->actors[slot].control_mode != 11u ||
        (tipoff->inbound_transfer_raw != 0u &&
         tipoff->actors[slot].control_mode != 11u)) return false;
    NbaTipoffActor *actor = &tipoff->actors[slot];
    /* `$86:F43A-$F4F1` temporarily compensates DP $AA/$AE only for
     * `$85:B3C9` steering. `$86:F4E6-$F4F0` restores the raw target before
     * `$86:F4F2` applies its [-9,+8] arrival box on every dispatch. `$09BA`
     * is written after arrival and cannot replace this geometry test. */
    if (nba_gameplay_inbound_arrived(
            fp_integer_word(actor->x_fp), fp_integer_word(actor->y_fp),
            tipoff->inbound_target_x_raw, tipoff->inbound_target_y_raw)) {
        actor->velocity_x = actor->velocity_y = 0;
        actor->movement_magnitude_raw = 0u;
        actor->direction = (uint8_t)tipoff->inbound_direction_raw;
        actor->requested_direction = actor->direction;
        actor_set_animation(actor, 11u, 3u);
        actor->action_state = tipoff->cpu_play_state;
        return true;
    }
    uint8_t team = team_id_for_context(tipoff, slot / 5u);
    uint8_t profile_42 = 0x58u;
    (void)nba_player_gameplay_movement_profile(
        tipoff->assets, team, actor->roster_slot, &profile_42);
    NbaGameplayInboundMotion motion = {
        .actor_x = fp_integer_word(actor->x_fp),
        .actor_y = fp_integer_word(actor->y_fp),
        .target_x = tipoff->inbound_target_x_raw,
        .target_y = tipoff->inbound_target_y_raw,
        .velocity_x = actor->velocity_x,
        .velocity_y = actor->velocity_y,
        .boost_timer = actor->movement_boost_timer,
        .profile_42 = profile_42,
        .dispatch_dt = 2u,
        .movement_blocked = false,
        .owner_actor_raw_093e = (int16_t)tipoff->possession_actor
    };
    nba_gameplay_inbound_motion_step(&motion);
    actor->velocity_x = motion.velocity_x;
    actor->velocity_y = motion.velocity_y;
    actor->movement_boost_timer = motion.boost_timer;
    uint8_t direction = motion.direction;
    actor->movement_direction = direction;
    if (direction < 8u) actor->requested_direction = direction;
    actor->movement_magnitude_raw = actor_distance(
        actor->velocity_x, actor->velocity_y);
    /* `$85:963D` committed the prior velocity before `$86:F34F` dispatched
     * this continuation. Native F43A writes only the next velocity; applying
     * it here as position too creates a duplicate physics step and can skip
     * across the signed arrival box. */
    actor->direction = actor->requested_direction;
    actor_set_animation(actor, direction >= 8u ? 0u : 11u,
                        direction >= 8u ? 0u : 3u);
    actor->action_state = tipoff->cpu_play_state;
    return true;
}

/* `$87:9C67 -> $86:C6AD-$C74D`: mode 8 is timer-authoritative knockdown
 * recovery. The global scheduler saturates `+$5A`; this executor wraps
 * `+$60` through negative. Action $36's nonnegative `+$56` enables its
 * landing hop/settle. */
static bool cpu_update_knockdown_actor(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    if (actor->control_mode != 8u) return false;
    /* Native `$85:963D` commits the velocity from the preceding behavior
     * pass before `$87:9244` reaches C6AD. Landing writes below therefore
     * become motion input on the next 30-Hz pass. */
    actor->x_fp += (int32_t)actor->velocity_x * 2;
    actor->y_fp += (int32_t)actor->velocity_y * 2;
    cpu_integrate_actor_vertical(actor);
    actor->behavior_flags_raw |= 6u;
    if (actor->special_contact_raw_56 >= 0 &&
        actor->velocity_z == 0 && actor->z_fp == 0) {
        if ((int16_t)actor->pass_direction_raw >= 0) {
            tipoff->rim_raw_13e7 |= 0x0100u;
            actor->velocity_z = 0x00F0;
            actor->velocity_x = nba_gameplay_arithmetic_shift_right(
                actor->velocity_x, 1u);
            actor->velocity_y = nba_gameplay_arithmetic_shift_right(
                actor->velocity_y, 1u);
        } else {
            actor->velocity_x = 0;
            actor->velocity_y = 0;
        }
        actor->pass_direction_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    }
    actor->contact_action_timer_raw_60 = (uint16_t)(
        actor->contact_action_timer_raw_60 - 2u);
    int16_t presentation_phase = (int16_t)(uint16_t)(
        actor->contact_action_timer_raw_60 - 0x36u);
    actor->actor_status_raw_28 &= 0xFFE7u;
    if (presentation_phase >= 10 && presentation_phase < 20)
        actor->actor_status_raw_28 |= 0x0010u;
    else if (presentation_phase >= 20 && presentation_phase < 30)
        actor->actor_status_raw_28 |= 0x0008u;
    else if (presentation_phase >= 30 && presentation_phase < 40)
        actor->actor_status_raw_28 |= 0x0010u;

    if ((int16_t)actor->contact_action_timer_raw_60 >= 0) return true;

    uint16_t actor_group = slot >= 5u ? 5u : 0u;
    actor->control_mode = actor_group == tipoff->camera_side_group_raw ? 1u : 2u;
    actor->behavior_timer = 0x2Fu;
    actor->contact_action_timer_raw_60 = 0u;
    actor->behavior_flags_raw = 0u;
    actor->actor_status_raw_28 = 0u;
    actor->contact_inhibit_raw_5a = 0u;
    if (tipoff->possession_actor == (int8_t)slot)
        actor->control_mode = 11u;
    return true;
}

/* `$87:9244/$9BD0` is a second ten-actor scheduler pass. It runs after the
 * `$87:8EFB-$8F92 -> $85:963D` locomotion/physics loop and the intervening
 * ball/contact work. Keeping the dispatcher separate means a new owner can
 * receive its mode-11 target and velocity without retroactively moving in
 * the already-completed physics pass. */
typedef struct { NbaTipoff *game; unsigned slot; } CpuOwnerContext;

static NbaOwnerFlow cpu_owner_flow_read(const CpuOwnerContext *c) {
    NbaTipoff *t=c->game;NbaTipoffActor *a=&t->actors[c->slot];
    uint8_t rating=0,unused=0;
    uint8_t team=team_id_for_context(t, c->slot / 5u);
    (void)nba_player_gameplay_decision_profiles(t->assets,team,a->roster_slot,&rating,&unused);
    NbaOwnerFlow s={
        (uint16_t)c->slot,(uint16_t)(int16_t)t->possession_actor,t->shot_inner_veto_raw,
        t->fouls.shooting_foul_raw_09bc,t->deferred_shot_foul_phase_raw_0a02,
        a->team_group_raw_6e,t->owner_team_group_raw_09f4,t->play_code,t->inbound_actor_raw,
        t->inbound_transfer_raw,t->play_request_raw,t->dead_ball_raw_0968,t->attached_ball_state_raw_09f6,
        (uint16_t)a->velocity_x,(uint16_t)a->velocity_y,a->movement_direction,a->assignment_base_raw,
        /* $87:8E7F-8E9C: C6=2 physical ticks, C8=C6<<4=$20 decision units. */
        t->live_state_raw,t->camera_side_group_raw,a->reaction_threshold,0x20,a->control_mode,
        a->behavior_timer,a->behavior_flags_raw,(uint16_t)(int16_t)a->controller_assignment_raw,
        a->recovery_inhibit_raw,rating
    };
    return s;
}

static void cpu_owner_flow_commit(const CpuOwnerContext *c,const NbaOwnerFlow *s) {
    NbaTipoff *t=c->game;NbaTipoffActor *a=&t->actors[c->slot];
    /* Only words owned by the caller. Child-specific writes stay in their
     * normal C subsystem and are re-read after each callback. */
    t->shot_inner_veto_raw=s->veto_09f8!=0;
    t->deferred_shot_foul_phase_raw_0a02=s->deferred_0a02;
    t->owner_team_group_raw_09f4=s->owner_team_09f4;
    t->play_request_raw=s->request_0994;
    t->camera_side_group_raw=(uint8_t)s->offense_093a;
    a->velocity_x=(int16_t)s->vx_0e;a->velocity_y=(int16_t)s->vy_10;
    a->movement_direction=(uint8_t)s->facing_4e;
    a->reaction_threshold=s->timer_60;a->control_mode=(uint8_t)s->mode_5e;
    a->behavior_timer=s->behavior_64;a->behavior_flags_raw=s->flags_7e;
}

static bool cpu_owner_pose_call(NbaTipoff *t,unsigned slot,unsigned paired) {
    NbaTipoffActor *a=&t->actors[slot],*other=&t->actors[paired];
    NbaGameplayOwnerDribbleGate gate=nba_gameplay_owner_dribble_gate(
        fp_integer_word(a->z_fp),t->fouls.free_throw_state_raw_0978,
        t->live_state_raw,a->movement_magnitude_raw);
    if(gate==NBA_GAMEPLAY_OWNER_DRIBBLE_SKIP)return true;
    NbaGameplayOwnerProximityResult proximity=NBA_GAMEPLAY_OWNER_PROXIMITY_FALLBACK;
    if(gate==NBA_GAMEPLAY_OWNER_DRIBBLE_CONTINUE)
        proximity=nba_gameplay_owner_dribble_proximity(t->team_context[slot/5].anchor_x_raw_0a,
            fp_integer_word(a->x_fp),other->movement_magnitude_raw,a->assignment_distance,
            other->assignment_direction,t->dead_ball_raw_0968,a->catcher_latch_raw_ae,
            &a->requested_direction);
    if(proximity==NBA_GAMEPLAY_OWNER_PROXIMITY_UNLATCHED) {
        NbaPlayerAnimationChannels channels=actor_animation_channels(a);
        uint16_t facing=a->movement_direction;
        if(!nba_player_owner_unlatched_pose(t->assets,&channels,a->velocity_x,a->velocity_y,
                a->requested_direction,&facing,a->movement_boost_timer!=0,
                a->free_throw_launch_half_raw_a8!=0))return false;
        actor_store_animation_channels(a,&channels);
        a->movement_direction=(uint8_t)facing;
    } else if(proximity==NBA_GAMEPLAY_OWNER_PROXIMITY_LATCHED) {
        a->base_animation_state_raw_38=nba_gameplay_owner_latched_pose(a->controller_assignment_raw,
            t->attached_ball_state_raw_09f6,t->dead_ball_raw_0968,a->assignment_distance,
            a->requested_direction,&a->movement_direction);
    } else a->base_animation_state_raw_38=nba_gameplay_owner_dribble_fallback_pose(
        t->dead_ball_raw_0968,a->catcher_latch_raw_ae);
    return true;
}

/* Native formation completion includes the A82C accelerator. Commit it
 * before B50E selects/leads a receiver, not after the pass has been built. */
static void cpu_owner_accelerate(NbaTipoff *t,unsigned slot,uint8_t direction) {
    NbaTipoffActor *a=&t->actors[slot];uint8_t profile=0x58;
    uint8_t team=team_id_for_context(t, slot / 5u);
    (void)nba_player_gameplay_movement_profile(t->assets,team,a->roster_slot,&profile);
    nba_gameplay_velocity_step(&a->velocity_x,&a->velocity_y,&a->movement_boost_timer,
        direction,profile,2,t->live_state_raw==0x81 || fp_integer_word(a->z_fp)!=0,
        (int16_t)t->possession_actor);
    a->movement_magnitude_raw=actor_distance(a->velocity_x,a->velocity_y);
    a->velocity_direction_raw_a2=nba_gameplay_target_direction(a->velocity_x,a->velocity_y,NULL);
    if(direction<8)a->requested_direction=direction;
}

static bool cpu_owner_flow_call(void *context,NbaOwnerFlow *s,NbaOwnerCall call,unsigned paired) {
    CpuOwnerContext *c=context;NbaTipoff *t=c->game;
    NbaTipoffActor *a=&t->actors[c->slot];bool returns=true;
    cpu_owner_flow_commit(c,s);
    if(call==NBA_OWNER_CALL_POSE)returns=cpu_owner_pose_call(t,c->slot,paired);
    else if(call==NBA_OWNER_CALL_CPU) {
        uint8_t direction=a->movement_direction;
        CpuMode11Outcome outcome=cpu_dispatch_rom_mode11(t,c->slot,&direction);
        returns=outcome==CPU_MODE11_NORMAL_RETURN;
        if(!returns && a->control_mode==13)ball_attach_to_actor(t,c->slot);
        else if(outcome==CPU_MODE11_CONSUMED_ACTION)
            cpu_owner_accelerate(t,c->slot,direction);
    } else if(call==NBA_OWNER_CALL_FORMATION) {
        uint8_t direction=a->movement_direction;
        if(cpu_formation_route(t,c->slot,&direction))
            cpu_owner_accelerate(t,c->slot,direction);
    } else {
        int selected=cpu_select_rom_receiver(t,(uint8_t)c->slot);
        bool special=selected>=0 && (uint16_t)selected==t->special_actor_raw;
        if(selected>=0) {
            if(!special)a->reaction_threshold=1;
            if(nba_tipoff_begin_rom_pass(t,c->slot,(unsigned)selected)) {
                t->receiver_actor=(uint8_t)selected;
                NbaTipoffActor *receiver=&t->actors[selected];
                receiver->control_mode=special?14:10;
                if(special) {
                    receiver->mode13_baseline_velocity_x=receiver->velocity_x;
                    receiver->mode13_baseline_velocity_y=receiver->velocity_y;
                }
                cpu_enter_play_state(t,NBA_CPU_PLAY_PASS);
            }
        }
    }
    *s=cpu_owner_flow_read(c);
    return returns;
}

static void cpu_dispatch_normal_actor_behavior(NbaTipoff *tipoff,
                                               unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    if (!((actor->control_mode >= 1u && actor->control_mode <= 6u) ||
          actor->control_mode == 11u)) return;

    /* Natural F1B0/F23F parent captures include the state-$81 lineup: the
     * normal reaction timer and behavior continuation still run there.
     * F79F's jump/reach branch is a later action decision, not a replacement
     * for this parent dispatcher. */

    bool mode_five = actor->control_mode == 5u;
    if (mode_five) {
        /* `$86:F2CA-$F2E3`: mode five has its own normal-actor parent. It
         * repairs the locomotion base first, then bypasses the cutter/timer
         * work unless state `$82` is active or the actor belongs to `$093A`.
         * The bypass copies requested +$50 to facing +$4E and preserves mode
         * five, its timers, and the complete behavior-flags word. */
        if (actor->animation_state == 8u || actor->animation_state == 10u)
            actor->base_animation_state_raw_38 = 3u;
        if (tipoff->live_state_raw != 0x82u &&
            actor->team_group_raw_6e != tipoff->camera_side_group_raw) {
            actor->movement_direction = actor->requested_direction;
            actor->action_state = tipoff->cpu_play_state;
            return;
        }
    }

    cpu_update_special_actor(tipoff, slot);
    if (actor->control_mode == 11u) {
        CpuOwnerContext context={tipoff,slot};
        NbaOwnerFlow flow=cpu_owner_flow_read(&context);
        NbaOwnerFlowResult result=nba_owner_flow_run(&flow,cpu_owner_flow_call,&context);
        cpu_owner_flow_commit(&context,&flow);
        /* F43A consumes the prefix's flags/pose writes, but never the
         * ordinary countdown. Keep its existing continuation separate. */
        if (result==NBA_OWNER_FLOW_INBOUND)
            (void)cpu_move_inbound_actor(tipoff,slot);
        if (result==NBA_OWNER_FLOW_INVALID)
            fprintf(stderr,"[OWNER FLOW] invalid asset/pair slot=%u base=%04x\n",
                    slot,actor->assignment_base_raw);
        actor->action_state=tipoff->cpu_play_state;
        return;
    }
    int x = fp_round(actor->x_fp), y = fp_round(actor->y_fp);
    /* $86:E3CB-E3DD: modes 1-6 repair special locomotion bases. */
    if (!mode_five && actor->control_mode >= 1u && actor->control_mode <= 6u &&
        (actor->animation_state == 8u || actor->animation_state == 10u))
        actor->base_animation_state_raw_38 = 3u;

    bool decision_due = cpu_active_decision_due(tipoff, slot);
    uint8_t direction = actor->movement_direction;
    bool stop_velocity = false;
    bool apply_velocity_step = false;
    NbaGameplayLoosePursuitGateInput pursuit = {
        .live_state_raw_0936 = tipoff->live_state_raw,
        .ball_activity_raw_0948 = tipoff->ball_activity_raw,
        .bounce_age_raw_094a = tipoff->rim_raw_094a,
        .free_throw_state_raw_0978 =
            tipoff->fouls.free_throw_state_raw_0978,
        .play_code_raw_0996 = tipoff->play_code,
        .foul_actor_raw_7e492f = tipoff->collision_actor_b_raw,
        .actor_id = (uint8_t)slot,
        .actor_control_mode = actor->control_mode,
        .actor_team_group_raw_6e = slot < 5u ? 0u : 5u,
        .offense_group_raw_093a = tipoff->camera_side_group_raw,
        .inbound_group_raw_0952 = (uint8_t)tipoff->inbound_state_raw
    };
    /* `$86:F312-$F335`: mode five enters the loose-ball child only for a
     * due, uninhibited CPU actor when the role pass set `$09D8`. The current
     * ball-owner field is not a substitute: `$09D8` can request this scan
     * while the possession record still names an actor. */
    bool pursuit_context = mode_five ?
        decision_due && actor->recovery_inhibit_raw == 0u &&
            actor->controller_assignment_raw < 0 &&
            tipoff->role_ownerless_raw_09d8 != 0u :
        tipoff->ball.owner_actor < 0;
    bool loose_pursuit = pursuit_context &&
        nba_gameplay_loose_ball_pursuit_allowed(&pursuit);
    if (loose_pursuit && decision_due &&
        actor->recovery_inhibit_raw == 0u) {
        int16_t pursuit_x = 0, pursuit_y = 0;
        /* F176 loads the predicted ball words into DP scratch for B3AA; it
         * does not overwrite the actor's persistent +$56/+$58 formation or
         * defensive target. */
        cpu_predicted_ball_xy(tipoff, &pursuit_x, &pursuit_y);
        direction = nba_gameplay_target_direction(
            (int16_t)(pursuit_x - x),
            (int16_t)(pursuit_y - y), NULL);
        apply_velocity_step = true;
    } else if (decision_due) {
        uint8_t mode = actor->control_mode;
        if ((mode == 1u || mode == 3u || mode == 5u) &&
                actor->controller_assignment_raw < 0 &&
                actor->recovery_inhibit_raw == 0u) {
            apply_velocity_step = cpu_formation_route(tipoff, slot, &direction);
        }
        else if (tipoff->cpu_play_state != NBA_CPU_PLAY_REBOUND &&
                 (mode == 2u || mode == 4u || mode == 6u) &&
                 actor->recovery_inhibit_raw == 0u) {
            (void)cpu_refresh_defense_target(tipoff, slot, &stop_velocity);
            direction = stop_velocity ? 8u : nba_gameplay_target_direction(
                (int16_t)(actor->target_x - x),
                (int16_t)(actor->target_y - y), NULL);
            apply_velocity_step = !stop_velocity;
        } else
            direction = actor->movement_direction;
    }
    if (stop_velocity) actor->velocity_x = actor->velocity_y = 0;
    uint8_t team = team_id_for_context(tipoff, slot / 5u);
    uint8_t profile_42 = 0x58u;
    (void)nba_player_gameplay_movement_profile(
        tipoff->assets, team, actor->roster_slot, &profile_42);
    if (apply_velocity_step)
        nba_gameplay_velocity_step(
            &actor->velocity_x, &actor->velocity_y,
            &actor->movement_boost_timer, direction, profile_42, 2u,
            tipoff->live_state_raw == 0x81u || fp_round(actor->z_fp) != 0,
            (int16_t)tipoff->possession_actor);
    /* `$86:F236-$F23E/$86:F2C1-$F2C9`: a timer-hold pass restores the
     * current movement direction (+$4E) from the requested direction (+$50).
     * This happens even though no velocity decision ran. */
    if (!decision_due)
        actor->movement_direction = actor->requested_direction;
    else if (loose_pursuit)
        /* `$86:F22D-$F235`: accepted pursuit retains +$4E in +$50. */
        actor->requested_direction = actor->movement_direction;
    else
        cpu_finalize_requested_direction(tipoff, slot);
    /* F1B0/F23F install next-pass velocity but do not publish +$4C, +$4E
     * or +$A2 from that velocity. The following `$85:963D` actor commit owns
     * those derived fields. Doing it here made the visible facing and
     * movement magnitude lead the ROM by one scheduled actor pass. */
    actor->action_state = tipoff->cpu_play_state;
    /* F780/F886: the normal defensive decision continuation. */
    if(decision_due && actor->controller_assignment_raw<0 &&
       (actor->control_mode==2 || actor->control_mode==4 || actor->control_mode==6) &&
       !actor->movement_boost_timer)
        (void)nba_tipoff_jump_reach(tipoff,slot);
}

bool nba_tipoff_replay_normal_actor(NbaTipoff *tipoff, uint8_t actor) {
    if (!tipoff || actor >= NBA_GAMEPLAY_ACTOR_COUNT) return false;
    cpu_dispatch_normal_actor_behavior(tipoff, actor);
    return true;
}

/* Binding checks, not additional ROM witnesses: the independently replayed
 * helpers must receive +4E (not +52), retain +B0, and publish pack resources. */
static bool cpu_owner_pose_animation_self_test(const NbaAssetPack *assets,
                                               NbaSession *session) {
    /* Runtime bindings: stop-before-pose, base (+74) not current (+76),
     * lost-owner immediate return, and no direction clobber on timer hold. */
    {
        NbaTipoff game={0};game.assets=assets;game.session=session;publish_exhibition_team_ids(&game);
        game.possession_actor=0;game.live_state_raw=2;
        game.special_actor_raw=NBA_GAMEPLAY_UNKNOWN_WORD;
        NbaTipoffActor *a=&game.actors[0];
        a->control_mode=11;a->controller_assignment_raw=-1;
        a->assignment_base_raw=0xffff;a->assignment_actor=1;
        a->reaction_threshold=100;a->direction=6;a->movement_direction=7;
        a->velocity_x=0x100;game.dead_ball_raw_0968=1;game.attached_ball_state_raw_09f6=2;
        cpu_dispatch_normal_actor_behavior(&game,0);
        if(a->velocity_x || a->velocity_y || a->movement_direction!=2 ||
           a->direction!=6 || a->reaction_threshold!=68)return false;
        game.possession_actor=1;a->control_mode=11;a->reaction_threshold=0;
        a->behavior_flags_raw=0xffff;a->velocity_x=0x80;
        game.dead_ball_raw_0968=0;
        cpu_dispatch_normal_actor_behavior(&game,0);
        if(a->control_mode!=1 || a->reaction_threshold!=0 || a->behavior_timer!=47 ||
           a->behavior_flags_raw || a->velocity_x!=0x80)return false;
        game.possession_actor=0;game.live_state_raw=0x82;game.shot_inner_veto_raw=true;
        a->control_mode=11;a->reaction_threshold=100;a->team_group_raw_6e=5;
        cpu_dispatch_normal_actor_behavior(&game,0);
        if(game.shot_inner_veto_raw || game.owner_team_group_raw_09f4!=5 ||
           a->reaction_threshold!=100 || a->control_mode!=11)return false;
        game.live_state_raw=2;a->team_group_raw_6e=0;
        /* The real animation dispatcher, including RNG and resolved assets. */
        a->animation_state=a->lower_animation_state=a->base_animation_state_raw_38=7;
        a->animation_upper_queue_cursor_raw_18=a->animation_lower_queue_cursor_raw_1a=0xffff;
        a->upper_animation_accumulator_raw_42=0x600;a->lower_animation_accumulator_raw_44=0x600;
        a->movement_magnitude_raw=0;game.rng.state=0x9146;
        for(unsigned tick=0;tick<100;++tick) {
            NbaPlayerAnimationChannels expected=actor_animation_channels(a);
            uint16_t rng=game.rng.state,ur=0,lr=0;
            if(!nba_player_animation_step_channels(assets,&expected,a->direction,0,
                    0x200,false,0,&rng,&ur,&lr))return false;
            cpu_advance_actor_animation(&game,a);
            NbaPlayerAnimationChannels actual=actor_animation_channels(a);
            if(memcmp(&expected,&actual,sizeof(actual)) || rng!=game.rng.state ||
               !a->animation_resources_valid || ur!=a->upper_animation_resource_raw_2a ||
               lr!=a->lower_animation_resource_raw_2c)return false;
        }
        if(game.rng.state==0x9146)return false;
        /* Opposite pose reverses its lower phase without replacing the
         * display-facing word or resolving current resources prematurely. */
        for(unsigned state=9;state<=11;state+=2) {
            a->animation_state=a->lower_animation_state=(uint8_t)state;
            a->rom_upper_animation_phase_raw_3a=1;a->rom_lower_animation_phase_raw_3c=1;
            a->control_mode=11;a->reaction_threshold=100;game.possession_actor=0;
            a->assignment_base_raw=2;a->assignment_actor=2;a->assignment_current_raw=4;
            a->assignment_distance=0;a->catcher_latch_raw_ae=0;
            a->velocity_x=0x100;a->velocity_y=0;a->direction=5;
            game.actors[1].assignment_direction=(uint8_t)(state==9?2:6);
            uint16_t ur=a->upper_animation_resource_raw_2a,lr=a->lower_animation_resource_raw_2c;
            NbaPlayerAnimationChannels expected=actor_animation_channels(a);
            uint16_t facing=0;
            if(!nba_player_owner_unlatched_pose(assets,&expected,0x100,0,
                    game.actors[1].assignment_direction,&facing,false,false))return false;
            cpu_dispatch_normal_actor_behavior(&game,0);
            NbaPlayerAnimationChannels actual=actor_animation_channels(a);
            if(memcmp(&expected,&actual,sizeof(actual)) || a->movement_direction!=facing ||
               a->direction!=5 || ur!=a->upper_animation_resource_raw_2a ||
               lr!=a->lower_animation_resource_raw_2c)return false;
        }
    }
    for (unsigned distance=16;distance<=17;++distance) {
        NbaTipoff game={0};
        game.assets=assets;game.session=session;publish_exhibition_team_ids(&game);game.possession_actor=0;
        game.ball.owner_actor=0;game.live_state_raw=2;game.rng.state=0x9146;
        game.special_actor_raw=NBA_GAMEPLAY_UNKNOWN_WORD;
        NbaTipoffActor *a=&game.actors[0];
        a->control_mode=11;a->controller_assignment_raw=-1;a->behavior_timer=100;
        a->reaction_threshold=100; /* +60: no new CPU decision during this held-pose case */
        a->assignment_actor=2;a->assignment_base_raw=2;a->assignment_current_raw=4;
        a->assignment_distance=(uint16_t)distance;
        a->catcher_latch_raw_ae=1;a->direction=4;a->movement_direction=4;
        a->upper_phase_target_raw_b0=0x8007;
        a->animation_upper_queue_cursor_raw_18=a->animation_lower_queue_cursor_raw_1a=0xFFFF;
        game.actors[1].assignment_direction=6;
        cpu_dispatch_normal_actor_behavior(&game,0);
        if(a->base_animation_state_raw_38!=(distance==16?13:18) ||
           a->movement_direction!=(distance==16?6:2) || a->direction!=4) {
            fprintf(stderr,"[OWNER POSE BINDING] distance=%u base=%u facing=%u display=%u latch=%u mode=%u\n",
                distance,a->base_animation_state_raw_38,a->movement_direction,a->direction,a->catcher_latch_raw_ae,a->control_mode);
            return false;
        }
        cpu_resolve_locomotion_animation(&game,0);
        if(a->animation_state!=a->base_animation_state_raw_38 ||
           a->lower_animation_state!=a->base_animation_state_raw_38) {
            fprintf(stderr,"[OWNER POSE BINDING] locomotion distance=%u upper=%u lower=%u base=%u\n",distance,a->animation_state,a->lower_animation_state,a->base_animation_state_raw_38);
            return false;
        }
        for(unsigned tick=0;tick<40;++tick) {
            NbaPlayerAnimationChannels expected=actor_animation_channels(a);
            uint16_t rng=game.rng.state,ur=0,lr=0;
            if(!nba_player_animation_step_channels(assets,&expected,a->direction,0,
                    0x200,false,0,&rng,&ur,&lr)) {
                fprintf(stderr,"[OWNER POSE BINDING] expected cadence rejected distance=%u tick=%u\n",distance,tick);return false;
            }
            cpu_advance_actor_animation(&game,a);
            NbaPlayerAnimationChannels actual=actor_animation_channels(a);
            if(memcmp(&actual,&expected,sizeof(expected)) || game.rng.state!=rng ||
               !a->animation_resources_valid || a->upper_animation_resource_raw_2a!=ur ||
               a->lower_animation_resource_raw_2c!=lr) {
                fprintf(stderr,"[OWNER POSE BINDING] cadence distance=%u tick=%u valid=%u rng=%04x/%04x\n",distance,tick,a->animation_resources_valid,game.rng.state,rng);return false;
            }
        }
    }
    return true;
}

static void cpu_commit_actor_common(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    /* `$87:B572` resolves the pose from the velocity installed by the prior
     * behavior pass. `$85:963D-$985F` then commits that velocity before the
     * mode dispatcher can replace it for the next 30-Hz pass. */
    cpu_resolve_locomotion_animation(tipoff, slot);
    NbaGameplayActorCommit commit = {
        .x_fraction = (uint16_t)((uint16_t)actor->x_fp << 8),
        .y_fraction = (uint16_t)((uint16_t)actor->y_fp << 8),
        .z_fraction = (uint16_t)((uint16_t)actor->z_fp << 8),
        .x = (int16_t)(actor->x_fp >> 8),
        .y = (int16_t)(actor->y_fp >> 8),
        .z = (int16_t)(actor->z_fp >> 8),
        .velocity_x = actor->velocity_x,
        .velocity_y = actor->velocity_y,
        .velocity_z = actor->velocity_z,
        .control_mode_raw_5e = actor->control_mode,
        .reaction_timer_raw_60 = actor->reaction_threshold,
        .behavior_flags_raw_7e = actor->behavior_flags_raw,
        .speed_raw_4a = (uint16_t)(actor->movement_magnitude_raw << 1),
        .movement_distance_raw_4c = actor->movement_magnitude_raw,
        .facing_raw_4e = actor->movement_direction,
        .velocity_direction_raw_a2 = actor->velocity_direction_raw_a2
    };
    nba_gameplay_actor_commit(&commit, 2u, true);
    actor->x_fp = (int32_t)commit.x * 256 + (commit.x_fraction >> 8);
    actor->y_fp = (int32_t)commit.y * 256 + (commit.y_fraction >> 8);
    actor->z_fp = (int32_t)commit.z * 256 + (commit.z_fraction >> 8);
    actor->velocity_x = commit.velocity_x;
    actor->velocity_y = commit.velocity_y;
    actor->velocity_z = commit.velocity_z;
    actor->reaction_threshold = commit.reaction_timer_raw_60;
    actor->planar_edge_raw_a0 = commit.planar_scratch_raw_a0;
    actor->movement_speed_raw_4a = commit.speed_raw_4a;
    actor->movement_magnitude_raw = commit.movement_distance_raw_4c;
    actor->velocity_direction_raw_a2 = commit.velocity_direction_raw_a2;
    actor->movement_direction = commit.facing_raw_4e;
}

static bool cpu_move_actor(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    if (actor->control_mode == 15u &&
        nba_tipoff_update_rom_passer(tipoff, slot))
        return false;
    if (actor->control_mode == 12u && cpu_update_rom_shooter(tipoff, slot))
        return false;
    if (actor->control_mode == 17u && cpu_update_rom_special_shooter(tipoff, slot))
        return false;
    if (actor->control_mode == 13u && cpu_update_rom_layup(tipoff, slot))
        return false;
    if (actor->control_mode == 10u && cpu_update_rom_receiver(tipoff, slot)) {
        /* `$86:A5B0` (normal receiver) and `$86:B154` (special receiver)
         * do not dispatch the generic formation accelerator. `$86:99C4`
         * already led the pass by the receiver's existing velocity, so keep
         * that motion through the common `$85:963D` coordinate commit. */
        actor->x_fp += (int32_t)actor->velocity_x * 2;
        actor->y_fp += (int32_t)actor->velocity_y * 2;
        actor->movement_magnitude_raw = actor_distance(
            actor->velocity_x, actor->velocity_y);
        /* A5B8-A5BD already consumed the current actor delta inside
         * cpu_update_rom_receiver.  ROM-COMPATIBILITY: do not apply a
         * second host-side decrement here; the old duplicate countdown
         * expired receivers twice as fast as the native 30-Hz pass. */
        cpu_integrate_actor_vertical(actor);
        return false;
    }
    if (actor->control_mode == 14u) {
        cpu_commit_actor_common(tipoff, slot);
        (void)cpu_update_rom_special_receiver(tipoff, slot);
        return true;
    }
    if (cpu_update_knockdown_actor(tipoff, slot)) return false;
    if (cpu_apply_passive_mode(tipoff, slot)) {
        cpu_integrate_actor_vertical(actor);
        return false;
    }
    /* Keeping the common commit ahead of the next decision is essential:
     * calling A82C first makes CPU actors skate and prevents idle dribble. */
    cpu_commit_actor_common(tipoff, slot);
    return true;
}

static bool actor_animation_resources(const NbaTipoff *tipoff,
                                      const NbaTipoffActor *actor,
                                      uint8_t direction,
                                      uint16_t *upper_resource,
                                      uint16_t *lower_resource) {
    /* Adopted live-play passes share the rendered ROM phase with their hand
     * point and release gate. Inbound remains a separate integration boundary.
     * `$87:AEC3` also resolves a just-installed action before its first tick. */
    if ((actor->exact_jump_animation || (actor->control_mode == 15u && actor->exact_pass_animation) ||
         ((actor->control_mode == 12u || actor->control_mode == 17u) && actor->exact_shot_animation)) &&
        actor->animation_resources_valid &&
        direction == actor->direction) {
        *upper_resource = actor->upper_animation_resource_raw_2a;
        *lower_resource = actor->lower_animation_resource_raw_2c;
        return true;
    }
    /* Other collision/contact routines still use their separately verified logical
     * tick representation. The exact +$2A/+$2C resources drive rendering;
     * switching physics here belongs with the remaining `$87:B649-$B952`
     * integration branch, not this animation-only increment. */
    return nba_player_animation_resources_for_appearance(
        tipoff->assets, actor->animation_state, actor->lower_animation_state,
        direction, actor->upper_animation_tick, actor->lower_animation_tick,
        actor->free_throw_launch_half_raw_a8 != 0u,
        actor->animation_variant_raw_6c,
        upper_resource, lower_resource);
}

static NbaGameplayDrawDirection actor_draw_direction_input(
                                    const NbaTipoff *tipoff,
                                    unsigned actor_index) {
    const NbaTipoffActor *actor = &tipoff->actors[actor_index];
    NbaGameplayDrawDirection input = {
        .current_direction = actor->direction,
        .control_mode = actor->control_mode,
        .actor_status = actor->actor_status_raw_28,
        .upper_state = actor->animation_state,
        .anchor_direction = actor->anchor_direction_raw
    };
    if (actor->control_mode == 15u && tipoff->pass_receiver_raw >= 0 &&
        tipoff->pass_receiver_raw < NBA_GAMEPLAY_ACTOR_COUNT) {
        const NbaTipoffActor *target =
            &tipoff->actors[(unsigned)tipoff->pass_receiver_raw];
        input.candidate_valid = true;
        input.candidate_dx = (int16_t)(
            fp_integer_word(target->x_fp) - fp_integer_word(actor->x_fp));
        input.candidate_dy = (int16_t)(
            fp_integer_word(target->y_fp) - fp_integer_word(actor->y_fp));
    } else if (actor->control_mode == 10u ||
               (actor->control_mode == 14u &&
                tipoff->camera.subject_pointer_0940 !=
                    (uint16_t)(0x34ebu + actor_index * 0x100u))) {
        /* `$87:A58A/$A58D` takes the ball branch when this actor does NOT
         * match the camera subject at0940. Possession is a different owner.
         * `$3EEB+$04/+$08` are the live ball integer words. */
        input.candidate_valid = true;
        input.candidate_dx = (int16_t)(
            fp_integer_word(tipoff->ball.x_fp) - fp_integer_word(actor->x_fp));
        input.candidate_dy = (int16_t)(
            fp_integer_word(tipoff->ball.y_fp) - fp_integer_word(actor->y_fp));
    }
    return input;
}

static uint8_t actor_draw_direction(const NbaTipoff *tipoff,
                                    unsigned actor_index) {
    const NbaTipoffActor *actor = &tipoff->actors[actor_index];
    /* `$87:A59C-$A5A2` uses actor+88 >>1 for upper states20/21 when the
     * preceding mode/target branches do not apply. It skips F02D entirely;
     * the old stale-AE comment described a port omission, not a native bug. */
    NbaGameplayDrawPreparationInput preparation = {
        .direction = actor_draw_direction_input(tipoff, actor_index),
        .status = actor->actor_status_raw_28,
        .upper_resource = actor->upper_animation_resource_raw_2a,
        .lower_resource = actor->lower_animation_resource_raw_2c,
        .world_x = fp_integer_word(actor->x_fp),
        .world_y = fp_integer_word(actor->y_fp),
        .world_z = fp_integer_word(actor->z_fp),
        .screen_x = tipoff->player_screen_x[actor_index],
        .screen_y = tipoff->player_screen_y[actor_index]
    };
    NbaGameplayDrawPreparation output;
    nba_gameplay_prepare_player_draw(&preparation, &output);
    return output.direction;
}

static bool actor_draw_body_resources(const NbaTipoff *tipoff,
                                      const NbaTipoffActor *actor,
                                      uint8_t head_direction,
                                      uint16_t *upper, uint16_t *lower) {
    /* 87:A4E1/A4E4 and A517/A51A latch actor +2A/+2C into D6/D4 BEFORE
     * the A52C-A5FA head-facing selection. Both 80:AD92 and camera-subject
     * 80:AF1E consume those unchanged body resources. Re-resolving the body
     * using head facing detached the visible hands from the correctly
     * attached ball (first CPU pass, frame306: 332/1168 became324/1154).
     * The uninitialized host preview remains outside this live contract. */
    if (actor->animation_resources_valid) {
        *upper = actor->upper_animation_resource_raw_2a;
        *lower = actor->lower_animation_resource_raw_2c;
        return true;
    }
    return actor_animation_resources(tipoff, actor, head_direction,
                                     upper, lower);
}

static bool actor_attachment_resources(const NbaTipoff *tipoff,
                                       const NbaTipoffActor *actor,
                                       uint8_t direction,
                                       uint16_t *upper_resource,
                                       uint16_t *lower_resource) {
    /* `$87:B649/$87:B66A -> $87:B832/$87:B953` consume the resources already
     * published in actor +$2A/+$2C by the preceding `$87:8EFB-$8F92`
     * animation pass. They do not reconstruct an ordinary dribble resource
     * from the host's logical `upper_animation_tick`. This distinction is
     * observable when `$86:E545-$E592` reverses bases 9/11 after the ball
     * pass: the preserved resource remains the attachment oracle until the
     * next native animation cadence step. */
    if (actor->animation_resources_valid && direction == actor->direction) {
        *upper_resource = actor->upper_animation_resource_raw_2a;
        *lower_resource = actor->lower_animation_resource_raw_2c;
        return true;
    }
    return actor_animation_resources(tipoff, actor, direction,
                                     upper_resource, lower_resource);
}

static bool actor_ball_attachment_offsets(const NbaTipoff *tipoff,
        const NbaTipoffActor *actor, int16_t *offset_x, int16_t *offset_y,
        int16_t *offset_z) {
    uint8_t direction = actor->direction < 8u ? actor->direction : 0u;
    uint16_t upper_resource = 0u, lower_resource = 0u;
    /* `$87:B649/$87:B66A` pass actor `+$28` through to `$87:B832`.
     * Its sign toggles masks 1/2, and the low masks may independently mirror
     * the upper and lower attachment inputs. Rebuilding only bit 15 from
     * direction loses those live pose masks and can detach the ball from the
     * submitted hand even while the sprite compositor uses the exact word. */
    uint16_t mirror_flags = actor->actor_status_raw_28;
    return actor_attachment_resources(
        tipoff, actor, direction, &upper_resource, &lower_resource) &&
        nba_player_ball_attachment_offsets(
            tipoff->assets, upper_resource, lower_resource, mirror_flags,
            offset_x, offset_y, offset_z);
}

static void ball_position_at_actor(NbaTipoff *tipoff, unsigned owner) {
    /* The position writes in `$87:B649/$87:B66A` and `$86:B7AF-$B7CA`
     * replace only integer XYZ words; caller-owned history is separate.
     * The ball keeps its own fractions; copying the actor fractions here
     * invents subpixel motion that is absent from the native composers. */
    NbaTipoffActor *actor = &tipoff->actors[owner];
    /* Mode 17's phase gate owns attachment; phase >=3 must hold the last
     * ball point until its release call, not resample it here. */
    if (actor->control_mode == 17u && actor->exact_shot_animation) return;
    int16_t offset_x = 0, offset_y = 0, offset_z = 0;
    bool resolved = actor_ball_attachment_offsets(
        tipoff, actor, &offset_x, &offset_y, &offset_z);
    /* A validated v26 asset pack makes failure unreachable. Keep ownership
     * deterministic if an externally-corrupted pack reaches this boundary. */
    if (!resolved) offset_x = offset_y = offset_z = 0;
    tipoff->ball.x_fp = fp_replace_integer_word(tipoff->ball.x_fp,
        (int16_t)(fp_integer_word(actor->x_fp) + offset_x));
    tipoff->ball.y_fp = fp_replace_integer_word(tipoff->ball.y_fp,
        (int16_t)(fp_integer_word(actor->y_fp) + offset_y));
    tipoff->ball.z_fp = fp_replace_integer_word(tipoff->ball.z_fp,
        (int16_t)(fp_integer_word(actor->z_fp) + offset_z));
}

static void ball_attach_to_actor(NbaTipoff *tipoff, unsigned owner) {
    ball_position_at_actor(tipoff, owner);
    NbaTipoffActor *actor = &tipoff->actors[owner];
    /* Attachment is a render/ball-state projection. Controller transfer is
     * owned by D25A acquisition and explicit BC9B callers, never this helper. */
    tipoff->ball.owner_actor = (int8_t)owner;
    tipoff->ball.state = NBA_BALL_ATTACHED;
    /* Mode 12 owns its wind-up counter: attachment must not replace a
     * stationary shot's positive `$0948` with the airborne $FFFF marker. */
    if (actor->control_mode != 12u && actor->control_mode != 17u) tipoff->ball_activity_raw = 0u;
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

static bool cpu_actor_pose_points(const NbaTipoff *tipoff, unsigned actor_index,
                                  NbaGameplayPosePoint points[2]) {
    if (!tipoff || !points || actor_index >= NBA_GAMEPLAY_ACTOR_COUNT)
        return false;
    const NbaTipoffActor *actor = &tipoff->actors[actor_index];
    uint8_t direction = actor->direction < 8u ? actor->direction : 0u;
    uint16_t upper_resource = 0u, lower_resource = 0u;
    uint16_t mirror_flags = actor->actor_status_raw_28;
    if (!actor_animation_resources(tipoff, actor, direction,
                                   &upper_resource, &lower_resource))
        return false;
    for (unsigned point = 0; point < 2u; ++point) {
        int16_t x, y, z;
        if (!nba_player_ball_attachment_point_offsets(
                tipoff->assets, upper_resource, lower_resource, mirror_flags,
                (uint8_t)point, &x, &y, &z)) return false;
        points[point].x = (int16_t)(fp_round(actor->x_fp) + x);
        points[point].y = (int16_t)(fp_round(actor->y_fp) + y);
        points[point].z = (int16_t)(fp_round(actor->z_fp) + z);
    }
    return true;
}

static bool cpu_actor_contact_height(const NbaTipoff *tipoff,
                                     unsigned actor_index,
                                     uint16_t *height) {
    const NbaTipoffActor *actor = &tipoff->actors[actor_index];
    uint8_t direction = actor->direction < 8u ? actor->direction : 0u;
    uint16_t upper_resource = 0u, lower_resource = 0u;
    uint8_t team = team_id_for_context(tipoff, actor_index / 5u);
    return actor_animation_resources(tipoff, actor, direction,
               &upper_resource, &lower_resource) &&
           nba_player_animation_contact_height(
               tipoff->assets, team, actor->roster_slot, upper_resource,
               lower_resource, direction, height);
}

static bool cpu_actor_body_contacts_ball(const NbaTipoff *tipoff,
                                         unsigned actor_index,
                                         bool intended_receiver,
                                         uint8_t radius) {
    const NbaTipoffActor *actor = &tipoff->actors[actor_index];
    uint16_t height = 0u;
    if (!cpu_actor_contact_height(tipoff, actor_index, &height)) return false;
    int dz = fp_round(tipoff->ball.z_fp) - fp_round(actor->z_fp);
    if (dz < 0) return false;
    if (intended_receiver) dz -= 8; /* `$86:CF59-$CF68` */
    int dx = fp_round(actor->x_fp) - fp_round(tipoff->ball.x_fp);
    int dy = fp_round(actor->y_fp) - fp_round(tipoff->ball.y_fp);
    return dz < (int)height && dx >= -(int)radius && dx < (int)radius &&
           dy >= -(int)radius && dy < (int)radius;
}

static bool cpu_actor_ball_contact_allowed(const NbaTipoffActor *actor) {
    /* `$86:CCCD-$CCF3` rejects behavior mode `+$5E` 7/8; `$86:CCFC-$CD08`
     * separately rejects nonzero contact inhibit `+$5A`. Neither gate tests
     * the selected upper-animation resource at `+$30`. */
    return actor && actor->contact_inhibit_raw_5a == 0u &&
           actor->control_mode != 7u && actor->control_mode != 8u;
}

static bool cpu_actor_contacts_ball(const NbaTipoff *tipoff,
                                    unsigned actor_index,
                                    bool intended_receiver,
                                    uint8_t threshold) {
    NbaGameplayPosePoint points[2];
    const NbaTipoffActor *actor = &tipoff->actors[actor_index];
    int16_t ball_x = fp_round(tipoff->ball.x_fp);
    int16_t ball_y = fp_round(tipoff->ball.y_fp);
    int16_t ball_z = fp_round(tipoff->ball.z_fp);
    if (!cpu_actor_ball_contact_allowed(actor) ||
        !nba_gameplay_ball_coarse_contact(
            fp_round(actor->x_fp), fp_round(actor->y_fp),
            fp_round(actor->z_fp), ball_x, ball_y, ball_z,
            intended_receiver)) return false;
    if (cpu_actor_pose_points(tipoff, actor_index, points) &&
        nba_gameplay_ball_pose_contact(
            points, ball_x, ball_y, ball_z, threshold)) return true;
    return cpu_actor_body_contacts_ball(
        tipoff, actor_index, intended_receiver, threshold);
}

static int cpu_actor_ball_contact_index(const NbaTipoff *tipoff,
                                        unsigned actor_index,
                                        bool intended_receiver,
                                        uint8_t threshold) {
    const NbaTipoffActor *actor = &tipoff->actors[actor_index];
    int16_t ball_x = fp_round(tipoff->ball.x_fp);
    int16_t ball_y = fp_round(tipoff->ball.y_fp);
    int16_t ball_z = fp_round(tipoff->ball.z_fp);
    if (!cpu_actor_ball_contact_allowed(actor) ||
        !nba_gameplay_ball_coarse_contact(
            fp_round(actor->x_fp), fp_round(actor->y_fp),
            fp_round(actor->z_fp), ball_x, ball_y, ball_z,
            intended_receiver)) return -1;
    NbaGameplayPosePoint points[2];
    if (!cpu_actor_pose_points(tipoff, actor_index, points)) return -1;
    return nba_gameplay_ball_pose_contact_index(
        points, ball_x, ball_y, ball_z, threshold);
}

/* `$86:CCCD-$D5DA` supplies the broadphase, classifier and pose hitbox;
 * `$86:D5DB` insertion-sorts the zero-terminated `$34D3` actor list by
 * signed world X, retaining source order for ties. `$86:D652-$D728` then
 * presents actor/ball pairs in that stable order. */
static void cpu_actor_contact_order(const NbaTipoff *tipoff,
                                    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT]) {
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        order[i] = (uint8_t)i;
        unsigned at = i;
        while (at > 0u) {
            int16_t left = fp_round(tipoff->actors[order[at - 1u]].x_fp);
            int16_t right = fp_round(tipoff->actors[order[at]].x_fp);
            if (left <= right) break;
            uint8_t swap = order[at - 1u];
            order[at - 1u] = order[at];
            order[at] = swap;
            --at;
        }
    }
}

static int16_t cpu_contact_mul_low(int16_t a, int16_t b) {
    return (int16_t)(uint16_t)((int32_t)a * (int32_t)b);
}

static int16_t cpu_contact_add(int16_t a, int16_t b) {
    return (int16_t)(uint16_t)((uint16_t)a + (uint16_t)b);
}

static int16_t cpu_contact_sub(int16_t a, int16_t b) {
    return (int16_t)(uint16_t)((uint16_t)a - (uint16_t)b);
}

static bool cpu_actor_in_contact_window(const NbaTipoffActor *actor) {
    int x = fp_round(actor->x_fp), y = fp_round(actor->y_fp);
    return x >= -378 && x < 379 && y >= -208 && y < 209;
}

/* `$86:C302-$C34B`: run after any defender velocity restoration performed
 * by `$86:C261-$C275`; its gate observes those restored values. */
static void cpu_nudge_contact_vertical(NbaTipoffActor *x,
                                       const NbaTipoffActor *y) {
    int16_t vx_delta = cpu_contact_sub(x->velocity_x, y->velocity_x);
    unsigned abs_vx = (unsigned)(vx_delta < 0 ? -vx_delta : vx_delta);
    if (abs_vx >= 0x80u) return;
    int16_t vy = x->velocity_y;
    unsigned abs_vy = (unsigned)(vy < 0 ? -vy : vy);
    if (abs_vy >= 0x100u) return;
    if (fp_round(x->y_fp) >= fp_round(y->y_fp))
        x->velocity_y = (int16_t)(abs_vy + 0x10u);
    else
        x->velocity_y = (int16_t)-(int16_t)(abs_vy + 0x20u);
}

/* `$86:BD41-$BF08` and `$86:BF0B-$C475` share the same signed fixed-point
 * collision projection. Teammates use three arithmetic shifts; opponents
 * use four through `$86:C34C`. `$85:F78B` contributes the wrapped low word
 * of each signed product to these formulas. */
static bool cpu_apply_player_contact_response(NbaTipoff *tipoff,
                                              unsigned x_slot,
                                              unsigned y_slot,
                                              unsigned shift,
                                              bool nudge_x_vertical) {
    NbaTipoffActor *x = &tipoff->actors[x_slot];
    NbaTipoffActor *y = &tipoff->actors[y_slot];
    if (!cpu_actor_in_contact_window(x) || !cpu_actor_in_contact_window(y))
        return false;
    int16_t rel_x = cpu_contact_sub(y->velocity_x, x->velocity_x);
    int16_t rel_y = cpu_contact_sub(y->velocity_y, x->velocity_y);
    if ((uint16_t)rel_x == 0u && (uint16_t)rel_y == 0u) return false;
    int16_t dx = cpu_contact_sub(fp_round(x->x_fp), fp_round(y->x_fp));
    int16_t dy = cpu_contact_sub(fp_round(x->y_fp), fp_round(y->y_fp));
    int16_t dot = cpu_contact_add(cpu_contact_mul_low(dx, rel_x),
                                  cpu_contact_mul_low(dy, rel_y));
    if (dot < 0) return false;

    int16_t radial = nba_gameplay_arithmetic_shift_right(dot, shift);
    int16_t cross = nba_gameplay_arithmetic_shift_right(
        cpu_contact_sub(cpu_contact_mul_low(dy, rel_x),
                        cpu_contact_mul_low(dx, rel_y)), shift);
    int16_t half = nba_gameplay_arithmetic_shift_right(radial, 1u);
    int16_t old_x_vx = x->velocity_x, old_x_vy = x->velocity_y;
    int16_t y_add_x = nba_gameplay_arithmetic_shift_right(
        cpu_contact_add(cpu_contact_mul_low(cross, dy),
                        cpu_contact_mul_low(half, dx)), shift);
    int16_t y_add_y = nba_gameplay_arithmetic_shift_right(
        cpu_contact_sub(cpu_contact_mul_low(half, dy),
                        cpu_contact_mul_low(cross, dx)), shift);
    y->velocity_x = cpu_contact_add(old_x_vx, y_add_x);
    y->velocity_y = cpu_contact_add(old_x_vy, y_add_y);
    x->velocity_x = cpu_contact_add(old_x_vx,
        nba_gameplay_arithmetic_shift_right(
            cpu_contact_mul_low(half, dx), shift));
    x->velocity_y = cpu_contact_add(old_x_vy,
        nba_gameplay_arithmetic_shift_right(
            cpu_contact_mul_low(half, dy), shift));

    /* `$86:C302-$C34B` prevents a nearly parallel opponent response from
     * leaving X sliding through Y on the vertical axis. */
    if (nudge_x_vertical) cpu_nudge_contact_vertical(x, y);
    return true;
}

static bool cpu_player_pose_special_contact(const NbaTipoff *tipoff,
                                            unsigned attacker,
                                            unsigned target) {
    const NbaTipoffActor *a = &tipoff->actors[attacker];
    const NbaTipoffActor *t = &tipoff->actors[target];
    if (a->animation_state != 0x38u ||
        a->upper_animation_phase_raw >= 3u || attacker / 5u == target / 5u ||
        tipoff->live_state_raw >= 0x80u) return false;
    int dy = fp_round(t->y_fp) - fp_round(a->y_fp);
    if (dy < -32 || dy > 32) return false;
    if (t->control_mode == 8u && t->special_contact_raw_56 != -1)
        return false;
    NbaGameplayPosePoint points[2];
    if (!cpu_actor_pose_points(tipoff, attacker, points)) return false;
    int px = points[0].x - fp_round(t->x_fp);
    int py = points[0].y - fp_round(t->y_fp);
    return px >= -7 && px <= 7 && py >= -7 && py <= 7;
}

static void cpu_record_player_contact(NbaTipoff *tipoff,
                                      unsigned actor_a,
                                      unsigned actor_b,
                                      uint32_t routine) {
    ++tipoff->player_contact_count_raw;
    tipoff->player_contact_actor_a_raw = (int8_t)actor_a;
    tipoff->player_contact_actor_b_raw = (int8_t)actor_b;
    tipoff->player_contact_routine_raw = routine;
}

static int16_t cpu_knockdown_velocity(int16_t source, uint8_t jitter) {
    int16_t projected = cpu_contact_add(
        nba_gameplay_arithmetic_shift_right(source, 1u),
        nba_gameplay_arithmetic_shift_right(source, 3u));
    return cpu_contact_add(projected, (int16_t)((int)jitter - 128));
}

/* `$86:C4FE-$C6AC`: normalize the native X=victim/Y=offender register
 * contract into the portable classifier. Its return value never gates the
 * caller's collision physics, but its direct `$07F6` mutation must occur at
 * the original call position. */
static void cpu_classify_player_contact(NbaTipoff *tipoff,
                                        unsigned victim_slot,
                                        unsigned offender_slot,
                                        uint16_t context_tag) {
    if (!tipoff || !tipoff->session || victim_slot >= 10u ||
        offender_slot >= 10u) return;
    const NbaTipoffActor *offender = &tipoff->actors[offender_slot];
    NbaGameplayContactFoulInput input = {
        (uint8_t)offender_slot,
        (uint8_t)victim_slot,
        (uint8_t)(offender_slot / 5u),
        (uint8_t)(offender_slot >= 5u ? 5u : 0u),
        offender->control_mode,
        offender->movement_magnitude_raw,
        offender->movement_boost_timer,
        tipoff->possession_actor,
        (int8_t)tipoff->shot_actor_raw_09c8,
        tipoff->camera_side_group_raw,
        tipoff->live_state_raw,
        tipoff->ball_activity_raw,
        tipoff->period_raw_0926,
        context_tag,
        tipoff->session->config.rules[0],
        tipoff->session->config.rules[1],
        offender->controller_assignment_raw,
        tipoff->session->config.rules[7] != 0u
    };
    if (nba_gameplay_foul_classify_contact(
            &tipoff->fouls, &tipoff->rng, &input,
            &tipoff->rim_raw_13e7)) {
        unsigned persistent = (offender_slot / NBA_MATCH_LINEUP_SIZE) *
            NBA_MATCH_ROSTER_SIZE + offender->roster_slot;
        tipoff->roster_personal_fouls[persistent] =
            tipoff->fouls.personal_fouls[offender_slot];
    }
}

/* `$86:BFBA-$C238` is a separate high-speed knockdown outcome, not part of
 * `$86:C239`'s ordinary elastic response. Register identities are normalized
 * so victim is stationary and hitter is moving. `$86:BFF2` calls the
 * fire-and-forget `$86:C4FE` classifier before any response writes. */
static bool cpu_try_player_knockdown_contact(NbaTipoff *tipoff,
                                             unsigned first_slot,
                                             unsigned second_slot) {
    unsigned victim_slot = first_slot, hitter_slot = second_slot;
    NbaTipoffActor *victim = &tipoff->actors[victim_slot];
    NbaTipoffActor *hitter = &tipoff->actors[hitter_slot];
    if (tipoff->fouls.free_throw_state_raw_0978 != 0u ||
        (victim->behavior_flags_raw & 1u) != 0u) return false;
    if (victim->movement_magnitude_raw != 0u) {
        victim_slot = second_slot;
        hitter_slot = first_slot;
        victim = &tipoff->actors[victim_slot];
        hitter = &tipoff->actors[hitter_slot];
    }
    if ((victim->behavior_flags_raw & 1u) != 0u ||
        victim->movement_magnitude_raw != 0u ||
        (hitter->behavior_flags_raw & 1u) != 0u ||
        hitter->movement_magnitude_raw < 0x0250u ||
        (nba_gameplay_rng_next(&tipoff->rng) & 7u) != 0u)
        return false;

    cpu_classify_player_contact(tipoff, victim_slot, hitter_slot, 0u);

    /* `$86:C476`: cancel receiver/pass state independently of the later
     * knockdown presentation. Mode 16 and active mode-8 records fall back to
     * the ordinary response. */
    if (victim->control_mode == 14u || victim->control_mode == 10u) {
        victim->contact_action_timer_raw_60 = 0u;
        victim->behavior_flags_raw = 0u;
        victim->actor_status_raw_28 = 0u;
        tipoff->pass_receiver_raw = -1;
    }
    if (victim->control_mode == 16u ||
        (victim->control_mode == 8u &&
         victim->special_contact_raw_56 == 0)) return false;

    victim->velocity_x = cpu_knockdown_velocity(
        hitter->velocity_x,
        (uint8_t)nba_gameplay_rng_next(&tipoff->rng));
    victim->velocity_y = cpu_knockdown_velocity(
        hitter->velocity_y,
        (uint8_t)nba_gameplay_rng_next(&tipoff->rng));
    victim->movement_magnitude_raw = actor_distance(
        victim->velocity_x, victim->velocity_y);
    hitter->velocity_x = 0;
    hitter->velocity_y = 0;
    hitter->movement_magnitude_raw = 0u;
    victim->recovery_inhibit_raw = 8u;
    hitter->recovery_inhibit_raw = 20u;

    uint16_t action = 0x35u;
    if (victim->movement_boost_timer != 0u &&
        victim->movement_magnitude_raw >= 0x03F0u &&
        (nba_gameplay_rng_next(&tipoff->rng) & 0x0Fu) == 0u)
        action = 0x36u;
    /* `$86:C0BF/$86:C10C`: both knockdown presentations make the second
     * context-$87 call after jitter/action RNG but before installing state. */
    cpu_classify_player_contact(tipoff, victim_slot, hitter_slot, 0x87u);
    tipoff->rim_raw_13e7 |= 0x0080u;
    victim->action_state = action;
    victim->animation_state = (uint8_t)action;
    victim->upper_animation_tick = 0u;
    victim->upper_animation_phase_raw = 0u;
    victim->contact_action_timer_raw_60 = action == 0x36u ? 174u : 30u;
    victim->contact_inhibit_raw_5a = victim->contact_action_timer_raw_60;
    victim->special_contact_raw_56 = action == 0x36u ? 0 : -1;
    victim->recovery_inhibit_raw = 0u;
    if (tipoff->live_state_raw != 1u) {
        if (tipoff->live_state_raw < 0x80u) tipoff->live_state_raw = 0u;
        tipoff->ball_activity_raw = 0u;
    }
    bool drop_owner = false;
    if (victim_slot == (unsigned)tipoff->possession_actor) {
        /* $86:C0F5-C0FE: action35's nonzero RNG&3 jumps directly to
         * C189, skipping both hitter inhibition and C15F-C186 owner drop.
         * Preserve possession on that branch; an interrupted mode15 pass
         * may retain 09C4/0942/0946 while mode8 executes recovery. C476
         * cancels receiver modes10/14 only, not the passing actor. */
        drop_owner = action == 0x36u ||
            (nba_gameplay_rng_next(&tipoff->rng) & 3u) == 0u;
        if (drop_owner) hitter->contact_inhibit_raw_5a = 10u;
        if (action == 0x36u) tipoff->ball.velocity_z = 480;
    }
    /* $86:C154-C157 gives action36 its vertical impulse even when the
     * victim is not the owner; only C14E-C151's ball impulse is gated. */
    if (action == 0x36u) victim->velocity_z = 600;
    if (drop_owner) {
        tipoff->possession_actor = -1;
        tipoff->possession_team = -1;
        tipoff->deferred_shot_foul_phase_raw_0a02 = 1u;
        tipoff->catch_actor_record_raw_0910 = 0x3EEBu; /* $86:C16B-C16E */
        tipoff->ball.owner_actor = -1;
        tipoff->ball.state = NBA_BALL_LOOSE;
        tipoff->ball.velocity_x = nba_gameplay_arithmetic_shift_right(
            victim->velocity_x, 1u);
        tipoff->ball.velocity_y = nba_gameplay_arithmetic_shift_right(
            victim->velocity_y, 1u);
        tipoff->live_state_raw = 0u;
        /* `$0936=0/$093E=FFFF` returns native dispatch to ownerless-ball
         * pursuit. The host play label must follow that same boundary. */
        cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
    }
    victim->behavior_flags_raw |= 6u;
    victim->control_mode = 8u;
    victim->pass_direction_raw = 0u;
    /* $86:C217 calls F02D (eight directions), NOT F3C3 (fine pass angle). */
    victim->movement_direction = nba_gameplay_contact_facing(
        victim->velocity_x, victim->velocity_y);
    victim->direction = victim->movement_direction;
    cpu_record_player_contact(
        tipoff, victim_slot, hitter_slot, 0x86BFBAu);
    return true;
}

/* `$86:C91E-$CB83`: animation-$38 pose contact. The normal CPU path calls
 * C4FE tag 0, selects CB84/CBA4 impulse physics, and may install action 35/36
 * plus the ROM's probabilistic owner drop. The human-attacker `$17C5` fork
 * skips the first classifier and contact counter; it is represented through
 * the existing controller assignments when CPU-vs-CPU is disabled. */
static void cpu_apply_pose_special_impulse(NbaTipoff *tipoff,
                                           unsigned attacker,
                                           unsigned target) {
    static const int16_t standard_impulse[8][2] = {
        {0, 592}, {418, 418}, {592, 0}, {418, -418},
        {0, -592}, {-418, -418}, {-592, 0}, {-418, 418}
    };
    static const int16_t alternate_impulse[8][2] = {
        {0, 504}, {356, 356}, {504, 0}, {356, -356},
        {0, -504}, {-356, -356}, {-504, 0}, {-356, 356}
    };
    NbaTipoffActor *a = &tipoff->actors[attacker];
    NbaTipoffActor *t = &tipoff->actors[target];
    uint8_t old_mode = t->control_mode;
    bool controls_active = !tipoff->cpu_vs_cpu;
    bool human_attacker = controls_active &&
                          a->controller_assignment_raw >= 0;
    bool alternate = human_attacker;
    if (!human_attacker) {
        cpu_classify_player_contact(tipoff, target, attacker, 0u);
        ++tipoff->team_pose_contact_count_raw[target / 5u];
        if (a->movement_boost_timer != 0u &&
            (nba_gameplay_rng_next(&tipoff->rng) & 0x0Fu) == 0u)
            alternate = true;
    }

    unsigned direction = a->direction & 7u; /* actor facing `+$4E` */
    const int16_t (*table)[2] = alternate ? alternate_impulse :
                                               standard_impulse;
    int16_t base_x = table[direction][0];
    int16_t base_y = table[direction][1];
    t->velocity_x = base_x;
    t->velocity_y = base_y;
    t->movement_magnitude_raw = actor_distance(t->velocity_x, t->velocity_y);
    tipoff->rim_raw_13e7 |= 0x0080u;

    uint16_t action = alternate ? 0x36u : 0x35u;
    bool human_target = controls_active &&
                        t->controller_assignment_raw >= 0;
    if (alternate && human_target) {
        t->velocity_x = cpu_contact_add(t->velocity_x, t->velocity_x);
        t->velocity_y = cpu_contact_add(t->velocity_y, t->velocity_y);
        t->movement_magnitude_raw = actor_distance(
            t->velocity_x, t->velocity_y);
    } else {
        if ((nba_gameplay_rng_next(&tipoff->rng) & 3u) != 0u) return;
        if (alternate) {
            cpu_classify_player_contact(tipoff, target, attacker, 0x87u);
        } else if ((nba_gameplay_rng_next(&tipoff->rng) & 0x0Fu) >= 6u) {
            cpu_classify_player_contact(tipoff, target, attacker, 0x87u);
        }
    }

    t->action_state = action;
    t->animation_state = (uint8_t)action;
    t->upper_animation_tick = 0u;
    t->upper_animation_phase_raw = 0u;
    t->special_contact_raw_56 = alternate ? 0 : -1;
    t->contact_action_timer_raw_60 = alternate ? 174u : 30u;
    t->contact_inhibit_raw_5a = t->contact_action_timer_raw_60;
    t->recovery_inhibit_raw = 0u;
    if (alternate) t->velocity_z = 600;
    if (alternate && target == (unsigned)tipoff->possession_actor) {
        a->contact_inhibit_raw_5a = 10u;
        tipoff->ball.velocity_z = 480;
    } else if (!alternate && attacker == (unsigned)tipoff->possession_actor) {
        a->contact_inhibit_raw_5a = 10u;
    }
    if (tipoff->live_state_raw != 1u) {
        if (tipoff->live_state_raw < 0x80u) tipoff->live_state_raw = 0u;
        tipoff->ball_activity_raw = 0u;
    }

    bool owner_drop =
        (nba_gameplay_rng_next(&tipoff->rng) & 3u) == 0u &&
        target == (unsigned)tipoff->possession_actor;
    if (owner_drop) {
        tipoff->possession_actor = -1;
        tipoff->possession_team = -1;
        tipoff->ball.owner_actor = -1;
        tipoff->ball.state = NBA_BALL_LOOSE;
        tipoff->ball.velocity_x = nba_gameplay_arithmetic_shift_right(
            base_x, 1u);
        tipoff->ball.velocity_y = nba_gameplay_arithmetic_shift_right(
            base_y, 1u);
        tipoff->deferred_shot_foul_phase_raw_0a02 = 1u;
        tipoff->live_state_raw = 0u;
        if (human_attacker) t->velocity_z = cpu_contact_add(
            t->velocity_z, t->velocity_z);
        cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
    }
    t->behavior_flags_raw |= 6u;
    if (old_mode == 14u || old_mode == 10u)
        tipoff->pass_receiver_raw = -1;
    t->control_mode = 8u;
    t->pass_direction_raw = 0u;
    /* $86:CB5E has the same coarse F02D facing contract. */
    t->movement_direction = nba_gameplay_contact_facing(base_x,base_y);
    t->direction = t->movement_direction;
}

/* `$86:C88F-$C91D/$86:CBC4-$CCCC`: return false only when D652 must stop
 * scanning later sorted-X partners. A geometrically visited pair returns
 * true even when its Y/metric gates reject the actual response. */
static bool cpu_process_player_pair(NbaTipoff *tipoff,
                                    unsigned x_slot, unsigned y_slot) {
    NbaTipoffActor *x = &tipoff->actors[x_slot];
    NbaTipoffActor *y = &tipoff->actors[y_slot];
    if (tipoff->live_state_raw == 0x82u &&
        (x_slot == tipoff->inbound_actor_raw ||
         y_slot == tipoff->inbound_actor_raw ||
         x->control_mode == 3u || y->control_mode == 3u)) return false;
    int dx = fp_round(y->x_fp) - fp_round(x->x_fp);
    if (dx < 0) dx = -dx;
    if (dx < 33) {
        if (cpu_player_pose_special_contact(tipoff, x_slot, y_slot)) {
            cpu_apply_pose_special_impulse(tipoff, x_slot, y_slot);
            cpu_record_player_contact(tipoff, x_slot, y_slot, 0x86C91Eu);
            return true;
        }
        if (cpu_player_pose_special_contact(tipoff, y_slot, x_slot)) {
            cpu_apply_pose_special_impulse(tipoff, y_slot, x_slot);
            cpu_record_player_contact(tipoff, y_slot, x_slot, 0x86C91Eu);
            return true;
        }
    }
    int signed_dx = fp_round(y->x_fp) - fp_round(x->x_fp);
    if (signed_dx < -16 || signed_dx > 16) return false;
    int dy = fp_round(y->y_fp) - fp_round(x->y_fp);
    bool opponents = x_slot / 5u != y_slot / 5u;
    if (opponents) {
        if (dy < -16 || dy >= 16 || actor_distance(signed_dx, dy) >= 17u)
            return true;
        /* `$86:BFBA-$BFDB` can swap X/Y while normalizing a moving first
         * record for the knockdown probe. The symmetric ordinary impulse is
         * represented in source order, except that `$86:C23B` must see the
         * swapped defender when it is assigned to the current owner. */
        unsigned response_x_slot = x_slot;
        unsigned response_y_slot = y_slot;
        if ((x->behavior_flags_raw & 1u) == 0u &&
            x->movement_magnitude_raw != 0u &&
            tipoff->possession_actor >= 0 &&
            (unsigned)(y->assignment_current_raw >> 1) ==
                (unsigned)tipoff->possession_actor &&
            y->anchor_distance_raw < 0x90u &&
            (y->control_mode == 2u || y->control_mode == 4u ||
             y->control_mode == 6u)) {
            response_x_slot = y_slot;
            response_y_slot = x_slot;
        }
        if (cpu_try_player_knockdown_contact(tipoff, x_slot, y_slot))
            return true;
        int16_t saved_x[3] = {x->velocity_x, x->velocity_y, x->velocity_z};
        int16_t saved_y[3] = {y->velocity_x, y->velocity_y, y->velocity_z};
        NbaTipoffActor *response_x = &tipoff->actors[response_x_slot];
        NbaTipoffActor *response_y = &tipoff->actors[response_y_slot];
        bool x_defender = response_x->anchor_distance_raw < 0x90u &&
            (response_x->control_mode == 2u ||
             response_x->control_mode == 4u ||
             response_x->control_mode == 6u);
        bool y_defender = response_y->anchor_distance_raw < 0x90u &&
            (response_y->control_mode == 2u ||
             response_y->control_mode == 4u ||
             response_y->control_mode == 6u);
        bool assignment_case = tipoff->possession_actor >= 0 &&
            (unsigned)(response_x->assignment_current_raw >> 1) ==
                (unsigned)tipoff->possession_actor;
        bool responded = false;
        if (assignment_case && x_defender) {
            int16_t preserve[2] = {
                response_x->velocity_x, response_x->velocity_y};
            responded = cpu_apply_player_contact_response(
                tipoff, response_y_slot, response_x_slot, 4u, false);
            response_x->velocity_x = preserve[0];
            response_x->velocity_y = preserve[1];
            if (responded) {
                cpu_nudge_contact_vertical(response_y, response_x);
                response_y->recovery_inhibit_raw = 8u;
                response_x->recovery_inhibit_raw = 2u;
            }
        } else if (assignment_case && y_defender) {
            int16_t preserve[2] = {
                response_y->velocity_x, response_y->velocity_y};
            responded = cpu_apply_player_contact_response(
                tipoff, response_x_slot, response_y_slot, 4u, false);
            response_y->velocity_x = preserve[0];
            response_y->velocity_y = preserve[1];
            if (responded) {
                cpu_nudge_contact_vertical(response_x, response_y);
                response_x->recovery_inhibit_raw = 8u;
                response_y->recovery_inhibit_raw = 2u;
            }
        } else {
            responded = cpu_apply_player_contact_response(
                tipoff, response_x_slot, response_y_slot, 4u, true);
            if (responded) {
                /* `$86:C2C1-$C300` always installs the pair cooldowns.
                 * Even modes below seven use X=8/Y=2; odd or high modes
                 * reverse them to X=2/Y=8. */
                if (response_x->control_mode < 7u &&
                    (response_x->control_mode & 1u) == 0u) {
                    response_x->recovery_inhibit_raw = 8u;
                    response_y->recovery_inhibit_raw = 2u;
                } else {
                    response_x->recovery_inhibit_raw = 2u;
                    response_y->recovery_inhibit_raw = 8u;
                }
            }
        }
        if (responded) {
            cpu_record_player_contact(
                tipoff, response_x_slot, response_y_slot, 0x86BF0Bu);
            if (response_x->controller_assignment_raw >= 0)
                response_x->recovery_inhibit_raw = 0u;
            if (response_y->controller_assignment_raw >= 0)
                response_y->recovery_inhibit_raw = 0u;
        }
        if ((y->behavior_flags_raw & 1u) != 0u) {
            y->velocity_x = saved_y[0]; y->velocity_y = saved_y[1];
            y->velocity_z = saved_y[2];
        }
        if ((x->behavior_flags_raw & 1u) != 0u) {
            x->velocity_x = saved_x[0]; x->velocity_y = saved_x[1];
            x->velocity_z = saved_x[2];
        }
    } else {
        if (dy < -8 || dy >= 8 || actor_distance(signed_dx, dy) >= 8u)
            return true;
        if (cpu_apply_player_contact_response(
                tipoff, x_slot, y_slot, 3u, false)) {
            cpu_record_player_contact(tipoff, x_slot, y_slot, 0x86BD41u);
            x->recovery_inhibit_raw = 8u;
            y->recovery_inhibit_raw = 2u;
        }
    }
    return true;
}

static void cpu_update_player_contacts(NbaTipoff *tipoff) {
    if ((tipoff->simulation_tick & 1u) != 0u) return;
    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];
    cpu_actor_contact_order(tipoff, order);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        for (unsigned j = i + 1u; j < NBA_GAMEPLAY_ACTOR_COUNT; ++j) {
            if (!cpu_process_player_pair(tipoff, order[i], order[j])) break;
        }
    }
}

void nba_tipoff_replay_player_contact_order(NbaTipoff *tipoff,
                                            const uint8_t *order,
                                            unsigned count) {
    if (!tipoff || !order || count > NBA_GAMEPLAY_ACTOR_COUNT) return;
    for (unsigned i = 0; i < count; ++i) {
        if (order[i] >= NBA_GAMEPLAY_ACTOR_COUNT) return;
        for (unsigned j = i + 1u; j < count; ++j) {
            if (order[j] >= NBA_GAMEPLAY_ACTOR_COUNT ||
                !cpu_process_player_pair(tipoff, order[i], order[j])) break;
        }
    }
}

void nba_tipoff_replay_collision_order(NbaTipoff *tipoff,
                                       const uint8_t *order,
                                       unsigned count) {
    if (!tipoff || !order || count > NBA_GAMEPLAY_ACTOR_COUNT) return;
    tipoff->simulation_tick &= ~1u;
    nba_tipoff_replay_player_contact_order(tipoff, order, count);
    bool handled = cpu_try_detached_shot_contact(tipoff);
    if (!handled) handled = cpu_try_owned_ball_contact(tipoff);
    if (handled || tipoff->ball.owner_actor >= 0) return;
    int catcher = -1;
    if (tipoff->ball.state == NBA_BALL_PASS)
        catcher = cpu_first_pass_contact(tipoff);
    else if (tipoff->live_state_raw == 0x82u)
        catcher = cpu_first_inbound_ball_contact(tipoff);
    else
        catcher = cpu_first_loose_ball_contact(tipoff);
    if (catcher >= 0)
        cpu_commit_ball_acquisition(tipoff, (uint8_t)catcher);
}

void nba_tipoff_replay_player_contact_sweep(NbaTipoff *tipoff) {
    if (!tipoff) return;
    tipoff->simulation_tick &= ~1u;
    cpu_update_player_contacts(tipoff);
}

static bool cpu_player_contact_self_test(void) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        state.actors[i].x_fp = (int32_t)(100 + (int)i * 40) * 256;
        state.actors[i].y_fp = 0;
        state.actors[i].controller_assignment_raw = -1;
        state.actors[i].control_mode = 2u;
    }
    state.inbound_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state.possession_actor = -1;
    state.actors[0].x_fp = 0; state.actors[5].x_fp = 8 * 256;
    state.actors[0].velocity_x = 100; state.actors[5].velocity_x = -100;
    if (!cpu_process_player_pair(&state, 0u, 5u) ||
        state.actors[0].velocity_x != 75 ||
        state.actors[0].velocity_y != 16 ||
        state.actors[5].velocity_x != 75 ||
        state.actors[5].velocity_y != 0 ||
        state.actors[0].recovery_inhibit_raw != 8u ||
        state.actors[5].recovery_inhibit_raw != 2u) return false;

    memset(&state, 0, sizeof(state));
    state.inbound_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state.possession_actor = -1;
    state.actors[0].controller_assignment_raw = -1;
    state.actors[5].controller_assignment_raw = -1;
    state.actors[0].control_mode = state.actors[5].control_mode = 2u;
    state.actors[5].y_fp = 8 * 256;
    state.actors[0].velocity_y = 100;
    state.actors[5].velocity_y = -100;
    if (!cpu_process_player_pair(&state, 0u, 5u) ||
        state.actors[0].velocity_x != 0 ||
        state.actors[0].velocity_y != -107 ||
        state.actors[5].velocity_x != 0 ||
        state.actors[5].velocity_y != 75) return false;

    memset(&state, 0, sizeof(state));
    state.inbound_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state.possession_actor = -1;
    state.actors[0].controller_assignment_raw = -1;
    state.actors[1].controller_assignment_raw = -1;
    state.actors[0].control_mode = state.actors[1].control_mode = 2u;
    state.actors[1].x_fp = 7 * 256;
    state.actors[0].velocity_x = 100;
    state.actors[1].velocity_x = -100;
    if (!cpu_process_player_pair(&state, 0u, 1u) ||
        state.actors[0].velocity_x != 23 ||
        state.actors[1].velocity_x != 23) return false;

    memset(&state, 0, sizeof(state));
    state.inbound_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state.possession_actor = -1;
    state.actors[0].controller_assignment_raw = -1;
    state.actors[5].controller_assignment_raw = -1;
    state.actors[0].control_mode = state.actors[5].control_mode = 2u;
    state.actors[5].x_fp = 8 * 256;
    state.actors[0].velocity_x = 100;
    state.actors[5].velocity_x = -100;
    state.actors[0].behavior_flags_raw = 1u;
    if (!cpu_process_player_pair(&state, 0u, 5u) ||
        state.actors[0].velocity_x != 100 ||
        state.actors[0].velocity_y != 0 ||
        state.actors[5].velocity_x != 75) return false;

    memset(&state, 0, sizeof(state));
    state.inbound_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state.possession_actor = -1;
    state.actors[0].x_fp = 0; state.actors[5].x_fp = 17 * 256;
    if (cpu_process_player_pair(&state, 0u, 5u)) return false;
    state.actors[5].x_fp = 16 * 256; state.actors[5].y_fp = 4 * 256;
    if (!cpu_process_player_pair(&state, 0u, 5u)) return false;
    if (state.actors[0].velocity_x != 0 || state.actors[5].velocity_x != 0)
        return false; /* metric 17 visits but rejects response. */
    state.actors[1].x_fp = 7 * 256; state.actors[1].y_fp = 0;
    state.actors[0].velocity_x = 100; state.actors[1].velocity_x = -100;
    if (!cpu_process_player_pair(&state, 0u, 1u) ||
        state.actors[0].velocity_x == 100) return false;
    state.actors[0].velocity_x = 100; state.actors[0].velocity_y = 0;
    state.actors[1].velocity_x = -100; state.actors[1].velocity_y = 0;
    state.actors[1].x_fp = 8 * 256;
    if (!cpu_process_player_pair(&state, 0u, 1u) ||
        state.actors[0].velocity_x != 100) return false;
    state.live_state_raw = 0x82u; state.inbound_actor_raw = 0u;
    if (cpu_process_player_pair(&state, 0u, 1u)) return false;
    if (cpu_knockdown_velocity(640, 0x80u) != 400 ||
        cpu_knockdown_velocity(-320, 0x80u) != -200 ||
        actor_distance(400, -200) != 450u) return false;
    memset(&state, 0, sizeof(state));
    state.camera_side_group_raw = 0u;
    state.possession_actor = -1;
    state.actors[0].control_mode = 8u;
    state.actors[0].special_contact_raw_56 = -1;
    state.actors[0].contact_action_timer_raw_60 = 2u;
    state.actors[0].contact_inhibit_raw_5a = 30u;
    state.actors[0].behavior_flags_raw = 6u;
    state.actors[0].velocity_x = 400;
    state.actors[0].velocity_y = -200;
    if (!cpu_update_knockdown_actor(&state, 0u) ||
        state.actors[0].control_mode != 8u ||
        state.actors[0].contact_action_timer_raw_60 != 0u ||
        state.actors[0].contact_inhibit_raw_5a != 30u) return false;
    if (!cpu_update_knockdown_actor(&state, 0u) ||
        state.actors[0].control_mode != 1u ||
        state.actors[0].behavior_timer != 0x2Fu ||
        state.actors[0].contact_action_timer_raw_60 != 0u ||
        state.actors[0].contact_inhibit_raw_5a != 0u ||
        state.actors[0].behavior_flags_raw != 0u) return false;
    memset(&state, 0, sizeof(state));
    state.possession_actor = -1;
    state.actors[0].control_mode = 8u;
    state.actors[0].special_contact_raw_56 = 0;
    state.actors[0].contact_action_timer_raw_60 = 174u;
    state.actors[0].contact_inhibit_raw_5a = 174u;
    state.actors[0].pass_direction_raw = 0u;
    state.actors[0].velocity_x = 401;
    state.actors[0].velocity_y = -401;
    if (!cpu_update_knockdown_actor(&state, 0u) ||
        state.actors[0].velocity_x != 200 ||
        state.actors[0].velocity_y != -201 ||
        state.actors[0].velocity_z != 0x00F0 ||
        state.actors[0].pass_direction_raw != NBA_GAMEPLAY_UNKNOWN_WORD ||
        state.actors[0].contact_action_timer_raw_60 != 172u ||
        state.actors[0].contact_inhibit_raw_5a != 174u ||
        (state.rim_raw_13e7 & 0x0100u) == 0u) return false;
    state.actors[0].z_fp = 0;
    state.actors[0].velocity_z = 0;
    bool recovery_done = cpu_update_knockdown_actor(&state, 0u) &&
        state.actors[0].velocity_x == 0 &&
        state.actors[0].velocity_y == 0 &&
        state.actors[0].contact_action_timer_raw_60 == 170u;
    if (!recovery_done) return false;

    /* End-to-end `$86:BFBA-$C10C` RNG-order vector. Gate $20 passes,
     * context 0 rejects the $0250 speed, jitters consume $40/$80, and the
     * context-$87 classifier shifts live `$07F6` to $40 and awards code 1. */
    NbaSession session;
    memset(&state, 0, sizeof(state));
    memset(&session, 0, sizeof(session));
    state.session = &session;publish_exhibition_team_ids(&state);
    session.config.rules[0] = 45u;
    session.config.rules[1] = 25u;
    state.inbound_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state.possession_actor = -1;
    state.shot_actor_raw_09c8 = -1;
    state.actors[0].controller_assignment_raw = -1;
    state.actors[5].controller_assignment_raw = -1;
    state.actors[0].control_mode = 2u;
    state.actors[5].control_mode = 1u;
    state.actors[5].velocity_x = 640;
    state.actors[5].movement_magnitude_raw = 0x0250u;
    nba_gameplay_rng_seed(&state.rng, 0x0010u);
    nba_gameplay_foul_init(&state.fouls);
    bool high_speed_foul = cpu_try_player_knockdown_contact(&state, 0u, 5u) &&
        state.rng.state == 0x0040u &&
        state.fouls.foul_event_raw_0964 == NBA_GAMEPLAY_FOUL_DEFENSIVE &&
        state.fouls.team_fouls[1] == 1u &&
        state.fouls.personal_fouls[5] == 1u &&
        state.actors[0].control_mode == 8u;
    if (!high_speed_foul) return false;

    /* `$86:C99D-$CA49` standard impulse-only vector: tag 0 masks the
     * cached RNG to 1, B advances to 2 and skips all action writes. */
    memset(&state, 0, sizeof(state));
    memset(&session, 0, sizeof(session));
    state.session = &session;publish_exhibition_team_ids(&state);
    state.cpu_vs_cpu = true;
    state.possession_actor = -1;
    state.shot_actor_raw_09c8 = -1;
    state.actors[0].direction = 0u;
    state.actors[0].movement_magnitude_raw = 0x0300u;
    state.actors[0].controller_assignment_raw = -1;
    state.actors[5].controller_assignment_raw = -1;
    nba_gameplay_rng_seed(&state.rng, 1u);
    nba_gameplay_foul_init(&state.fouls);
    cpu_apply_pose_special_impulse(&state, 0u, 5u);
    if (state.rng.state != 2u || state.actors[5].velocity_x != 0 ||
        state.actors[5].velocity_y != 592 ||
        state.actors[5].control_mode != 0u ||
        state.actors[5].contact_action_timer_raw_60 != 0u ||
        state.team_pose_contact_count_raw[1] != 1u ||
        (state.rim_raw_13e7 & 0x0080u) == 0u) return false;

    /* Standard action-35/owner-drop vector: B=8, C=16, D=32. The ball
     * receives half the undoubled CB84 base vector. */
    memset(&state, 0, sizeof(state));
    state.session = &session;publish_exhibition_team_ids(&state);
    state.cpu_vs_cpu = true;
    state.possession_actor = 5;
    state.possession_team = 1;
    state.ball.owner_actor = 5;
    state.ball.state = NBA_BALL_ATTACHED;
    state.actors[0].direction = 0u;
    state.actors[0].movement_magnitude_raw = 0x0300u;
    state.actors[0].controller_assignment_raw = -1;
    state.actors[5].controller_assignment_raw = -1;
    nba_gameplay_rng_seed(&state.rng, 4u);
    nba_gameplay_foul_init(&state.fouls);
    cpu_apply_pose_special_impulse(&state, 0u, 5u);
    return state.rng.state == 32u &&
        state.actors[5].velocity_x == 0 &&
        state.actors[5].velocity_y == 592 &&
        state.actors[5].action_state == 0x35u &&
        state.actors[5].control_mode == 8u &&
        state.actors[5].special_contact_raw_56 == -1 &&
        state.actors[5].contact_action_timer_raw_60 == 30u &&
        state.possession_actor == -1 && state.ball.owner_actor == -1 &&
        state.ball.state == NBA_BALL_LOOSE &&
        state.ball.velocity_x == 0 && state.ball.velocity_y == 296 &&
        state.deferred_shot_foul_phase_raw_0a02 == 1u;
}

/* `$86:CCFC-$D548`: contact against an attached ball is opponent-only and
 * has no body-box fallback. The current animation selects a strict 4- or
 * 12-unit pose-point cube; `$86:D035` then consumes the ROM's random gates.
 * A successful `+$3A` roll either installs the candidate through BAA2 or,
 * for animations $32/$33, carries the old owner's velocity into D43E's
 * loose-ball deflection response. */
static bool cpu_try_owned_ball_contact(NbaTipoff *tipoff) {
    if ((tipoff->simulation_tick & 1u) != 0u ||
        tipoff->live_state_raw >= 0x80u ||
        tipoff->ball.state != NBA_BALL_ATTACHED ||
        tipoff->ball.owner_actor < 0 ||
        tipoff->ball.owner_actor >= NBA_GAMEPLAY_ACTOR_COUNT)
        return false;
    uint8_t owner = (uint8_t)tipoff->ball.owner_actor;
    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];
    cpu_actor_contact_order(tipoff, order);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        uint8_t candidate = order[i];
        NbaTipoffActor *candidate_state = &tipoff->actors[candidate];
        if (candidate / 5u == owner / 5u) continue;
        uint8_t threshold = candidate_state->animation_state == 0x13u ?
                            12u : 4u;
        int point = cpu_actor_ball_contact_index(
            tipoff, candidate, false, threshold);
        if (point < 0) continue;
        uint8_t team = team_id_for_context(tipoff, candidate / 5u);
        uint8_t contact_rating = 0u;
        if (!nba_player_gameplay_contact_rating(
                tipoff->assets, team, candidate_state->roster_slot,
                &contact_rating)) continue;
        bool foul_state_clear =
            tipoff->fouls.free_throw_state_raw_0978 == 0u &&
            tipoff->fouls.shooting_foul_raw_09bc == 0u &&
            tipoff->fouls.foul_event_raw_0964 == 0u &&
            tipoff->fouls.whistle_active_raw_09b6 == 0u;
        NbaGameplayOwnedContactResult result =
            nba_gameplay_owned_contact_attempt(
                &tipoff->rng, candidate_state->animation_state,
                (uint8_t)point, contact_rating,
                tipoff->session->config.main_values[2],
                tipoff->session->config.rules[0], foul_state_clear);
        if (result == NBA_GAMEPLAY_OWNED_CONTACT_NONE) continue;
        tipoff->collision_actor_a_raw = (int8_t)candidate;
        tipoff->collision_actor_b_raw = (int8_t)owner;
        if (result == NBA_GAMEPLAY_OWNED_CONTACT_FOUL) {
            tipoff->collision_routine_raw = 0x86D12Du;
            bool recorded = nba_gameplay_foul_record_contact_full(
                &tipoff->fouls, NBA_GAMEPLAY_FOUL_DEFENSIVE,
                candidate, owner, candidate / 5u,
                tipoff->ball_activity_raw != 0u,
                tipoff->period_raw_0926,
                candidate_state->controller_assignment_raw,
                tipoff->session->config.rules[7] != 0u);
            if (recorded) {
                unsigned persistent = (candidate / NBA_MATCH_LINEUP_SIZE) *
                    NBA_MATCH_ROSTER_SIZE + candidate_state->roster_slot;
                tipoff->roster_personal_fouls[persistent] =
                    tipoff->fouls.personal_fouls[candidate];
            }
            if (tipoff->ball_activity_raw != 0u)
                tipoff->rim_raw_13e7 |= 0x2000u;
            return true;
        }

        tipoff->collision_routine_raw = 0x86D1D9u;
        NbaTipoffActor *owner_state = &tipoff->actors[owner];
        owner_state->contact_inhibit_raw_5a = 15u;
        tipoff->ball.velocity_x = owner_state->velocity_x;
        tipoff->ball.velocity_y = owner_state->velocity_y;
        tipoff->ball.velocity_z = owner_state->velocity_z;
        uint8_t owner_group = owner >= 5u ? 5u : 0u;
        owner_state->control_mode =
            owner_group == tipoff->camera_side_group_raw ? 1u : 2u;
        owner_state->reaction_threshold = 0u;
        owner_state->behavior_flags_raw = 0u;
        tipoff->ball.owner_actor = -1;
        tipoff->possession_actor = -1;
        tipoff->pass_actor_raw = -1;
        tipoff->pass_receiver_raw = -1;
        tipoff->pass_active_raw = 0u;
        tipoff->pass_distance_raw = 0u;
        tipoff->ball_activity_raw = 0u;
        if (candidate_state->animation_state == 0x32u ||
            candidate_state->animation_state == 0x33u) {
            candidate_state->contact_inhibit_raw_5a = 15u;
            NbaGameplayRimState deflect = {
                fp_integer_word(tipoff->ball.x_fp),
                fp_integer_word(tipoff->ball.y_fp),
                fp_integer_word(tipoff->ball.z_fp),
                tipoff->ball.velocity_x, tipoff->ball.velocity_y,
                tipoff->ball.velocity_z
            };
            nba_gameplay_ball_apply_deflection(&deflect, &tipoff->rng);
            tipoff->ball.velocity_x = deflect.velocity_x;
            tipoff->ball.velocity_y = deflect.velocity_y;
            tipoff->ball.velocity_z = deflect.velocity_z;
            tipoff->ball.state = NBA_BALL_BOUNCE;
            tipoff->collision_routine_raw = 0x86D43Eu;
            cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
        } else {
            cpu_commit_ball_acquisition(tipoff, candidate);
        }
        return true;
    }
    return false;
}

static void cpu_record_interference_score(NbaTipoff *tipoff,
                                          uint8_t candidate,
                                          uint8_t shooter) {
    unsigned scoring_side = shooter / 5u;
    tipoff->rim_raw_096a = 0u;
    if (!nba_gameplay_foul_record_violation(
            &tipoff->fouls, NBA_GAMEPLAY_VIOLATION_INTERFERENCE,
            candidate, shooter)) return;
    tipoff->session->score[scoring_side] = (uint16_t)(
        tipoff->session->score[scoring_side] + tipoff->shot_value_raw);
    /* `$86:CE51-$CE65` updates the side-leading latch and the corresponding
     * lead-change counter only when the new award crosses the other score. */
    if (tipoff->leading_side_raw_1403 == 0u) {
        if (tipoff->session->score[1] > tipoff->session->score[0]) {
            tipoff->leading_side_raw_1403 = 1u;
            ++tipoff->right_lead_change_count_raw_1407;
        }
    } else if (tipoff->session->score[0] > tipoff->session->score[1]) {
        tipoff->leading_side_raw_1403 = 0u;
        ++tipoff->left_lead_change_count_raw_1405;
    }
    tipoff->shot_result_resolved = true;
    tipoff->collision_actor_a_raw = (int8_t)candidate;
    tipoff->collision_actor_b_raw = (int8_t)shooter;
    tipoff->collision_routine_raw = 0x86CE1Eu;
}

/* `$86:CD97-$D1D6`: an active detached shot on its downward path uses only
 * the strict eight-unit pose points. An opposing actor may trigger code-6
 * basket interference while the ball is above $50, then the same contact
 * continues into `$D078`'s ROM catch test. There is no body-box fallback. */
static bool cpu_try_detached_shot_contact(NbaTipoff *tipoff) {
    if ((tipoff->simulation_tick & 1u) != 0u ||
        tipoff->live_state_raw >= 0x80u ||
        tipoff->ball.state != NBA_BALL_SHOT ||
        tipoff->ball.owner_actor >= 0 ||
        tipoff->ball_activity_raw == 0u ||
        tipoff->ball.velocity_z >= 0 ||
        tipoff->match_clock_raw_0928 < 5u ||
        tipoff->match_clock_raw_0928 >= 0xFF00u ||
        tipoff->shot_actor_raw_09c8 < 0 ||
        tipoff->shot_actor_raw_09c8 >= NBA_GAMEPLAY_ACTOR_COUNT)
        return false;

    uint8_t shooter = (uint8_t)tipoff->shot_actor_raw_09c8;
    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];
    cpu_actor_contact_order(tipoff, order);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        uint8_t candidate = order[i];
        if (candidate / 5u == shooter / 5u) continue;
        int point = cpu_actor_ball_contact_index(
            tipoff, candidate, false, 8u);
        if (point < 0) continue;

        bool event_state_clear =
            tipoff->fouls.free_throw_state_raw_0978 == 0u &&
            tipoff->fouls.shooting_foul_raw_09bc == 0u &&
            tipoff->fouls.foul_event_raw_0964 == 0u &&
            tipoff->fouls.whistle_active_raw_09b6 == 0u;
        if (event_state_clear &&
            tipoff->session->config.rules[5] != 0u &&
            tipoff->rim_raw_096a != 0u &&
            fp_round(tipoff->ball.z_fp) >= 0x50) {
            cpu_record_interference_score(tipoff, candidate, shooter);
        }

        if (!nba_gameplay_detached_shot_contact_attempt(
                &tipoff->rng, (uint8_t)point,
                tipoff->rim_raw_097c != 0u)) continue;
        tipoff->collision_actor_a_raw = (int8_t)candidate;
        tipoff->collision_actor_b_raw = -1;
        tipoff->collision_routine_raw = 0x86D25Au;
        cpu_commit_ball_acquisition(tipoff, candidate);
        return true;
    }
    return false;
}

static bool cpu_generic_loose_contact_due(const NbaTipoff *tipoff) {
    if (!tipoff || (tipoff->simulation_tick & 1u) != 0u) return false;
    /* `$86:CD97-$D1D6` owns an active descending shot. It must finish that
     * classifier before the generic loose-ball `$D25A` acquisition route
     * becomes eligible, even if the host play label has become REBOUND. */
    return !(tipoff->ball.state == NBA_BALL_SHOT &&
             tipoff->ball_activity_raw != 0u &&
             tipoff->ball.velocity_z < 0);
}

static int cpu_first_loose_ball_contact(NbaTipoff *tipoff) {
    if (!cpu_generic_loose_contact_due(tipoff)) return -1;
    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];
    cpu_actor_contact_order(tipoff, order);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        unsigned actor = order[i];
        /* `$86:CF01` doubles the generic loose-ball window from 8 to 16;
         * animation `$13` changes only the owned-ball strip window. */
        /* `$86:CF20/$CF91` also enter CFA0 when a basket changed `$0936`
         * to dead-ball state earlier in this same ordinary update. */
        if (cpu_actor_contacts_ball(tipoff, actor, false, 16u) &&
            cpu_dead_ball_contact_gate(tipoff, actor))
            return (int)actor;
    }
    return -1;
}

/* `$86:CCFC/$CF01/$D25A` classify the ball, not a host play label. A
 * canceled shot may leave DRIVE/ATTACK behind when a later ball detaches. */
static bool cpu_live_loose_ball_contact_due(const NbaTipoff *tipoff) {
    return tipoff->live_state_raw < 0x80u && tipoff->ball.owner_actor < 0 &&
        (tipoff->ball.state == NBA_BALL_LOOSE ||
         tipoff->ball.state == NBA_BALL_BOUNCE) &&
        cpu_generic_loose_contact_due(tipoff);
}

/* `$86:CFA0-$CFDE` runs after a geometric hit and before acquisition. With
 * an ownerless, unlaunched, non-FT dead ball, plays below six accept only the
 * inbound side and replace provisional `$0954`; later plays accept only the
 * already selected `$0954`. BAA2 itself does not own that selector write. */
static bool cpu_dead_ball_contact_gate(NbaTipoff *tipoff, unsigned actor) {
    if (tipoff->live_state_raw != 0x82u ||
        tipoff->possession_actor >= 0 ||
        tipoff->inbound_transfer_raw != 0u ||
        tipoff->fouls.free_throw_state_raw_0978 != 0u)
        return true;
    if (tipoff->play_code >= 6u)
        return actor == tipoff->inbound_actor_raw;
    if (tipoff->actors[actor].team_group_raw_6e !=
        tipoff->inbound_state_raw)
        return false;
    tipoff->inbound_actor_raw = (uint16_t)actor;
    return true;
}

static int cpu_first_receiverless_pass_contact(NbaTipoff *tipoff) {
    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];
    cpu_actor_contact_order(tipoff, order);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        unsigned actor = order[i];
        if (cpu_actor_contacts_ball(tipoff, actor, false, 16u) &&
            cpu_dead_ball_contact_gate(tipoff, actor))
            return (int)actor;
    }
    return -1;
}

/* `$86:CFA0-$CFDE` owns the side/provisional-selector gate before the
 * collision winner becomes current ownership `$093E`. */
static int cpu_first_inbound_ball_contact(NbaTipoff *tipoff) {
    /* `$86:D5DB/D652` is the post-actor-pass 30-Hz collision sweep. */
    if ((tipoff->simulation_tick & 1u) != 0u) return -1;
    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];
    cpu_actor_contact_order(tipoff, order);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        unsigned actor = order[i];
        if (cpu_actor_contacts_ball(tipoff, actor, false, 16u) &&
            cpu_dead_ball_contact_gate(tipoff, actor))
            return (int)actor;
    }
    return -1;
}

static bool cpu_is_raw_pass_receiver(const NbaTipoff *tipoff,
                                     unsigned actor) {
    return tipoff && tipoff->pass_receiver_raw >= 0 &&
           actor == (unsigned)tipoff->pass_receiver_raw;
}

/* `$86:CE88-$D1D6`: a detached pass rejects same-side nonreceivers. The
 * intended receiver uses radius 16; an opposing player uses the radius-12
 * branch at `$86:CEE2-$CEFF`. At Z<24 the ROM computes a full rating into
 * DP $AA but accidentally compares RNG with DP $00, the hit-point index:
 * point 0 can never win and point 1 wins only for RNG byte zero. */
static int cpu_first_pass_contact(NbaTipoff *tipoff) {
    if ((tipoff->simulation_tick & 1u) != 0u ||
        tipoff->ball.state != NBA_BALL_PASS) return -1;
    /* `$86:CEE2-$CF01`: once `$86:A613` has cleared `$0946`, the detached
     * pass is no longer receiver/opponent classified. Every actor instead
     * uses the deterministic generic 16-unit pose/body contact branch. */
    if (tipoff->pass_receiver_raw < 0)
        return cpu_first_receiverless_pass_contact(tipoff);
    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];
    cpu_actor_contact_order(tipoff, order);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        unsigned actor = order[i];
        /* `$0946` is authoritative. `receiver_actor` is a host convenience
         * cache and can intentionally lag while the raw pass state clears. */
        bool receiver = cpu_is_raw_pass_receiver(tipoff, actor);
        if (!receiver && actor / 5u == tipoff->offense_side) continue;
        int point = cpu_actor_ball_contact_index(
            tipoff, actor, receiver, receiver ? 16u : 12u);
        if (point < 0 && receiver) {
            NbaTipoffActor *receiver_state=&tipoff->actors[actor];
            int16_t ball_x=fp_round(tipoff->ball.x_fp);
            int16_t ball_y=fp_round(tipoff->ball.y_fp);
            int16_t ball_z=fp_round(tipoff->ball.z_fp);
            int16_t actor_z=fp_round(receiver_state->z_fp);
            uint16_t height=0u;
            bool coarse=cpu_actor_ball_contact_allowed(receiver_state) &&
                nba_gameplay_ball_coarse_contact(
                    fp_round(receiver_state->x_fp),
                    fp_round(receiver_state->y_fp),actor_z,
                    ball_x,ball_y,ball_z,true);
            /* `$86:CF33-$CF50`: a high intended pass requests upper pose
             * `$37` before the adjusted-height/body test.  The resource and
             * contact-height projection becomes visible on the following
             * actor pass, so this collision pass only installs the reach.
             * Omitting this native continuation makes high tip passes sail
             * through an otherwise correctly selected receiver. */
            if(coarse && cpu_actor_contact_height(tipoff,actor,&height) &&
               ball_z-actor_z>=(int)height &&
               receiver_state->upper_animation_lock_raw_46==0u) {
                receiver_state->exact_jump_animation=true;
                actor_animation_command(tipoff,receiver_state,
                    NBA_ANIMATION_INSTALL_UPPER,0x37u);
                tipoff->tip_reach_mask|=(uint16_t)(1u<<actor);
                continue;
            }
            if(cpu_actor_body_contacts_ball(tipoff,actor,true,16u))point=2;
        }
        if (point < 0) continue;
        if (!cpu_dead_ball_contact_gate(tipoff, actor)) continue;
        if (!receiver && fp_round(tipoff->ball.z_fp) < 24) {
            uint8_t random = (uint8_t)nba_gameplay_rng_next(&tipoff->rng);
            if (random >= (uint8_t)point) continue;
        }
        return (int)actor;
    }
    return -1;
}

static bool cpu_contact_orchestration_self_test(const NbaAssetPack *assets,
                                               NbaSession *session) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    static const int16_t x[10] = {30, -5, 12, -5, 99, 0, 12, -20, 30, 4};
    static const uint8_t expected[10] = {7, 1, 3, 5, 9, 2, 6, 0, 8, 4};
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        state.actors[i].x_fp = (int32_t)x[i] * 256;
    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];
    cpu_actor_contact_order(&state, order);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        if (order[i] != expected[i]) return false;

    /* `$86:CFA0-$CFDE`: the selector write belongs to the contact gate,
     * not to BAA2. Cover both sides of play six and each bypass condition. */
    memset(&state, 0, sizeof(state));
    state.live_state_raw = 0x82u;
    state.possession_actor = -1;
    state.inbound_actor_raw = 2u;
    state.play_code = 5u;
    state.actors[7].team_group_raw_6e = 5u;
    if (cpu_dead_ball_contact_gate(&state, 7u) ||
        state.inbound_actor_raw != 2u ||
        !cpu_dead_ball_contact_gate(&state, 3u) ||
        state.inbound_actor_raw != 3u) return false;
    state.play_code = 6u;
    state.inbound_actor_raw = 2u;
    if (cpu_dead_ball_contact_gate(&state, 3u) ||
        !cpu_dead_ball_contact_gate(&state, 2u) ||
        state.inbound_actor_raw != 2u) return false;
    state.play_code = 5u;
    for (unsigned bypass = 0; bypass < 4u; ++bypass) {
        state.live_state_raw = bypass == 0u ? 0u : 0x82u;
        state.possession_actor = bypass == 1u ? 1 : -1;
        state.inbound_transfer_raw = bypass == 2u ? 1u : 0u;
        state.fouls.free_throw_state_raw_0978 = bypass == 3u ? 1u : 0u;
        if (!cpu_dead_ball_contact_gate(&state, 7u) ||
            state.inbound_actor_raw != 2u) return false;
    }

    /* Integration sentinel: the generic path remains reachable after an
     * inline make changes live state while the host play is REBOUND.
     * This is a C binding test, not an additional native vector. */
    memset(&state, 0, sizeof(state));
    state.assets = assets;
    state.session = session;publish_exhibition_team_ids(&state);
    state.live_state_raw = 0x82u;
    state.possession_actor = -1;
    state.ball.owner_actor = -1;
    state.ball.state = NBA_BALL_BOUNCE;
    state.play_code = 1u;
    state.inbound_actor_raw = 2u;
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        state.actors[i].contact_inhibit_raw_5a = 1u;
    NbaTipoffActor *candidate = &state.actors[1];
    candidate->contact_inhibit_raw_5a = 0u;
    candidate->control_mode = 2u;
    candidate->direction = 6u;
    candidate->exact_jump_animation = true;
    candidate->animation_resources_valid = true;
    candidate->upper_animation_resource_raw_2a = 328u;
    candidate->lower_animation_resource_raw_2c = 1388u;
    NbaGameplayPosePoint points[2];
    if (!cpu_actor_pose_points(&state, 1u, points)) return false;
    state.ball.x_fp = (int32_t)points[0].x * 256;
    state.ball.y_fp = (int32_t)points[0].y * 256;
    state.ball.z_fp = (int32_t)points[0].z * 256;
    /* An earlier geometric hit from the wrong side must not stop scanning. */
    state.actors[0] = *candidate;
    state.actors[0].team_group_raw_6e = 5u;
    if (cpu_first_loose_ball_contact(&state) != 1 ||
        state.inbound_actor_raw != 1u) return false;
    state.play_code = 6u;
    state.inbound_actor_raw = 2u;
    if (cpu_first_loose_ball_contact(&state) != -1 ||
        state.inbound_actor_raw != 2u) return false;
    state.inbound_actor_raw = 1u;
    if (cpu_first_loose_ball_contact(&state) != 1 ||
        state.inbound_actor_raw != 1u) return false;
    state.actors[0].contact_inhibit_raw_5a = 1u;
    state.inbound_actor_raw = 2u;
    state.play_code = 1u;
    candidate->x_fp = 100 * 256;
    if (cpu_first_loose_ball_contact(&state) != -1 ||
        state.inbound_actor_raw != 2u) return false;
    candidate->x_fp = 0;
    state.live_state_raw = 0u;
    if (cpu_first_loose_ball_contact(&state) != 1 ||
        state.inbound_actor_raw != 2u) return false;
    state.live_state_raw = 0x82u;
    state.ball.state = NBA_BALL_SHOT;
    state.ball_activity_raw = 1u;
    state.ball.velocity_z = -1;
    if (cpu_first_loose_ball_contact(&state) != -1 ||
        state.inbound_actor_raw != 2u) return false;
    state.ball.state = NBA_BALL_BOUNCE;
    state.simulation_tick = 1u;
    if (cpu_first_loose_ball_contact(&state) != -1 ||
        state.inbound_actor_raw != 2u) return false;

    NbaTipoffActor actor = {0};
    actor.animation_state = 7u;
    actor.control_mode = 2u;
    if (!cpu_actor_ball_contact_allowed(&actor)) return false;
    actor.animation_state = 0u;
    actor.control_mode = 7u;
    if (cpu_actor_ball_contact_allowed(&actor)) return false;
    actor.control_mode = 8u;
    if (cpu_actor_ball_contact_allowed(&actor)) return false;
    actor.control_mode = 2u;
    actor.contact_inhibit_raw_5a = 1u;
    if (cpu_actor_ball_contact_allowed(&actor)) return false;

    memset(&state, 0, sizeof(state));
    state.simulation_tick = 1u;
    if (cpu_generic_loose_contact_due(&state)) return false;
    state.simulation_tick = 2u;
    if (!cpu_generic_loose_contact_due(&state)) return false;
    state.ball.state = NBA_BALL_SHOT;
    state.ball_activity_raw = 1u;
    state.ball.velocity_z = -1;
    if (cpu_generic_loose_contact_due(&state)) return false;
    state.ball.velocity_z = 0;
    if (!cpu_generic_loose_contact_due(&state)) return false;
    state.ball.velocity_z = -1;
    state.ball_activity_raw = 0u;
    if (!cpu_generic_loose_contact_due(&state)) return false;

    /* `$0946`, rather than the host receiver cache, classifies a pass. */
    state.pass_receiver_raw = 3;
    state.receiver_actor = 7u;
    if (!cpu_is_raw_pass_receiver(&state, 3u) ||
        cpu_is_raw_pass_receiver(&state, 7u)) return false;
    state.pass_receiver_raw = -1;
    if (cpu_is_raw_pass_receiver(&state, 3u)) return false;
    /* `$86:D11F-$D128` compares against point selector DP $00, not the
     * computed chance DP $AA: selector zero never passes; selector one only
     * passes for a zero random byte. */
    return !(0u < 0u) && !(255u < 0u) && (0u < 1u) && !(1u < 1u);
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
}

static NbaShotAction actor_shot_action(const NbaTipoff *tipoff,
                                       const NbaTipoffActor *actor) {
    NbaShotAction s = {0};
    s.animation = actor_animation_channels(actor);
    s.velocity_x = actor->velocity_x; s.velocity_y = actor->velocity_y;
    s.velocity_z = actor->velocity_z; s.mode = actor->control_mode;
    s.speed = actor->movement_speed_raw_4a;
    s.flags = actor->behavior_flags_raw; s.timer = actor->reaction_threshold;
    s.status = actor->actor_status_raw_28; s.behavior_timer = actor->behavior_timer;
    s.activity = tipoff->ball_activity_raw;
    s.bounce_count = tipoff->rim_raw_0920;
    s.bounce_timer = tipoff->shot_bounce_timer_raw_091c;
    return s;
}

static void actor_store_shot_action(NbaTipoff *tipoff, NbaTipoffActor *actor,
                                     const NbaShotAction *s) {
    actor_store_animation_channels(actor, &s->animation);
    actor->velocity_x=s->velocity_x; actor->velocity_y=s->velocity_y;
    actor->velocity_z=s->velocity_z; actor->control_mode=(uint8_t)s->mode;
    actor->movement_speed_raw_4a=s->speed;
    actor->behavior_flags_raw=s->flags; actor->reaction_threshold=s->timer;
    actor->actor_status_raw_28=s->status; actor->behavior_timer=s->behavior_timer;
    tipoff->ball_activity_raw=s->activity; tipoff->rim_raw_0920=s->bounce_count;
    tipoff->shot_bounce_timer_raw_091c=s->bounce_timer;
    NbaPlayerResolvedPose pose={0};
    pose.direction=actor->direction;
    actor->animation_resources_valid=nba_player_resolve_pose(
        tipoff->assets,&s->animation,actor->direction,
        actor->free_throw_launch_half_raw_a8!=0,actor->animation_variant_raw_6c,&pose);
    if (actor->animation_resources_valid) {
        actor->upper_animation_resource_raw_2a=pose.upper_resource;
        actor->lower_animation_resource_raw_2c=pose.lower_resource;
        actor_publish_body_mirror(actor);
    }
}

/* `$86:B625`: evaluate F5E4 again at this call site, then select mode 12/17.
 * The ordinary fallback remains the verified B6D3 startup. */
static bool cpu_start_rom_shot(NbaTipoff *tipoff, unsigned slot) {
    if (!tipoff || slot >= NBA_GAMEPLAY_ACTOR_COUNT) return false;
    NbaTipoffActor *shooter = &tipoff->actors[slot];
    NbaShotAction start=actor_shot_action(tipoff,shooter);
    NbaSpecialShotSelection selection={cpu_lane_to_basket_is_clear(tipoff,slot),
        shooter->movement_magnitude_raw,shooter->anchor_distance_raw,
        shooter->anchor_direction_raw,shooter->direction,shooter->animation_variant_raw_6c,
        shooter->movement_boost_timer!=0,shooter->free_throw_launch_half_raw_a8!=0};
    if (!nba_special_shot_select(tipoff->assets,&start,&shooter->pass_direction_raw,&selection))
        return false;
    ++tipoff->shot_selection_serial;
    const uint16_t observed_selection[8]={selection.lane_result,selection.movement,
        selection.anchor_distance,selection.anchor_direction,selection.facing,
        selection.appearance,start.mode,(uint16_t)slot};
    memcpy(tipoff->shot_selection_inputs,observed_selection,sizeof(observed_selection));
    tipoff->handler_actor = (uint8_t)slot;
    tipoff->shot_origin_x = fp_round(shooter->x_fp);
    tipoff->shot_origin_y = fp_round(shooter->y_fp);
    tipoff->shot_value_raw = 0u;
    /* `$86:B625` preserves the existing `$09C8` shooter chain. It is
     * replaced by the later launch/ownership routines, not shot startup. */
    tipoff->shot_chance_raw = 0u;
    tipoff->shot_inner_veto_raw = false;
    tipoff->shot_miss_index_raw = 0xFFu;
    tipoff->shot_result_resolved = false;
    cpu_enter_play_state(tipoff, NBA_CPU_PLAY_SHOT);
    shooter->exact_shot_animation = true;
    actor_store_shot_action(tipoff,shooter,&start);
    ball_attach_to_actor(tipoff, slot);
    return true;
}

/* Reproducible rare-path fixture. Only prepares inputs; selection, animation,
 * release and ball flight still run through the normal CPU implementations. */
bool nba_tipoff_debug_special_shot(NbaTipoff *tipoff,unsigned slot) {
    if(!tipoff || slot>=10 || tipoff->fouls.free_throw_state_raw_0978) return false;
    NbaTipoff next=*tipoff;
    unsigned side=slot/5;
    int16_t basket=next.team_context[side].anchor_x_raw_0a;
    uint16_t direction=basket<0 ? 6u : 2u;
    for(unsigned i=0;i<10;++i) {
        next.actors[i].x_fp=(int32_t)formation[i].world_x*256;
        next.actors[i].y_fp=(int32_t)formation[i].world_y*256;
        next.actors[i].velocity_x=next.actors[i].velocity_y=next.actors[i].velocity_z=0;
        next.actors[i].z_fp=0;next.actors[i].visible=true;
    }
    NbaTipoffActor *a=&next.actors[slot];
    a->x_fp=(int32_t)(basket+(basket<0 ? 56 : -56))*256;a->y_fp=0;
    a->anchor_distance_raw=56;a->anchor_direction_raw=(uint8_t)(direction*2);
    a->direction=(uint8_t)((direction-(a->animation_variant_raw_6c ? 5u : 2u))&7u);
    a->movement_magnitude_raw=0;a->movement_boost_timer=0;
    a->upper_animation_lock_raw_46=a->lower_animation_lock_raw_48=0;
    a->behavior_flags_raw=0;a->reaction_threshold=0;
    next.phase=NBA_TIPOFF_LIVE;next.cpu_vs_cpu=true;next.live_state_raw=0;
    next.inbound_state_raw=0;next.offense_side=(uint8_t)side;next.camera_side_group_raw=(uint8_t)(side*5);
    next.possession_actor=(int8_t)slot;next.possession_team=(int8_t)side;next.ball.owner_actor=(int8_t)slot;
    if(!cpu_start_rom_shot(&next,slot) || a->control_mode!=17) return false;
    *tipoff=next;
    return true;
}

static int16_t mode13_velocity_component(int32_t delta_fp) {
    int32_t velocity = delta_fp / 40;
    if (velocity > INT16_MAX) velocity = INT16_MAX;
    if (velocity < INT16_MIN) velocity = INT16_MIN;
    return (int16_t)velocity;
}

/* `$86:B34F-$B624`: install the distinct carried-ball close-finish state.
 * The selector/resource tables are ROM data at `$86:B430-$B467`. Pose
 * attachment comes from the asset pack before the 40-tick trajectory is
 * solved, so this does not depend on captured emulator art. */
static bool cpu_start_rom_layup(NbaTipoff *tipoff, unsigned slot) {
    static const uint8_t upper_table[8] = {
        0x18u, 0x1Cu, 0x18u, 0x1Au, 0x19u, 0x1Bu, 0x1Du, 0x1Eu
    };
    NbaTipoffActor *actor = &tipoff->actors[slot];
    if (fp_integer_word(actor->z_fp) != 0 || actor->velocity_z != 0 ||
        actor->anchor_distance_raw >= 0x69u ||
        actor->movement_magnitude_raw < 0x0100u) return false;

    uint8_t team = team_id_for_context(tipoff, slot / 5u);
    uint8_t profile_39 = 0u, unused_3e = 0u;
    if (!nba_player_gameplay_pass_profiles(
            tipoff->assets, team, actor->roster_slot,
            &profile_39, &unused_3e)) return false;
    uint8_t anchor_direction = (uint8_t)((actor->anchor_direction_raw >> 1) & 7u);
    bool force_selector_seven = false;
    if (profile_39 < 0x4Cu) {
        if (anchor_direction == 0u || anchor_direction == 4u) return false;
        force_selector_seven = true;
    } else if (profile_39 < 0x54u) {
        if ((nba_gameplay_rng_next(&tipoff->rng) & 1u) == 0u) {
            if (anchor_direction == 0u || anchor_direction == 4u) return false;
            force_selector_seven = true;
        }
    } else if (profile_39 < 0x5Cu) {
        if ((nba_gameplay_rng_next(&tipoff->rng) & 3u) == 0u) {
            if (anchor_direction == 0u || anchor_direction == 4u) return false;
            force_selector_seven = true;
        }
    }
    uint8_t relative = (uint8_t)((anchor_direction - actor->direction) & 7u);
    if (relative != 0u && relative != 1u && relative != 7u) return false;

    uint8_t variant = (uint8_t)((nba_gameplay_rng_next(&tipoff->rng) & 3u) << 1);
    uint8_t selector;
    for (;;) {
        selector = force_selector_seven ? 7u :
            (uint8_t)(nba_gameplay_rng_next(&tipoff->rng) & 7u);
        force_selector_seven = false;
        /* In the B678 caller DP B2 is team group 0/5, so selector one and
         * cardinal selector seven both execute B47E and reroll. */
        if (selector == 1u ||
            (selector == 7u &&
             (anchor_direction == 0u || anchor_direction == 4u)))
            continue;
        break;
    }

    actor->contact_action_timer_raw_60 = 0x28u;
    actor->reaction_threshold = 0x28u;
    actor->control_mode = 13u;
    actor->special_contact_raw_56 = (int16_t)selector - 1;
    actor->mode13_variant_raw_58 = variant;
    actor->pass_direction_raw = upper_table[selector];
    actor->direction = anchor_direction;
    actor->requested_direction = anchor_direction;
    actor->movement_direction = anchor_direction;
    actor->z_fp = 1;
    actor->velocity_z = 0;
    actor_set_animation(actor, upper_table[selector], 0x1Fu);
    /* `$86:B34F-$B624` configures the actor trajectory but does not move the
     * ball record. The mode-13 executor owns the first pose attachment. */
    actor->velocity_x = mode13_velocity_component(
        (int32_t)(basket_x_for_side(tipoff->offense_side) +
                  (tipoff->offense_side ? -5 : 5)) * 256 -
        tipoff->ball.x_fp);
    /* `$86:B4DF-$B563` solves from two pose points before `$85:F8D9`;
     * its signed half-pixel remainder contributes three velocity units. */
    actor->velocity_x = (int16_t)(actor->velocity_x +
        (tipoff->offense_side ? 3 : -3));
    actor->velocity_y = mode13_velocity_component(-tipoff->ball.y_fp);
    actor->mode13_baseline_velocity_x = actor->velocity_x;
    actor->mode13_baseline_velocity_y = actor->velocity_y;
    actor->movement_magnitude_raw = actor_distance(
        actor->velocity_x, actor->velocity_y);
    actor->behavior_flags_raw |= 0x0006u;
    /* `$87:B7D8`, called inside `$86:B468`, advances the shared ROM RNG
     * while resolving the installed upper pose before B5E2's flag roll. */
    (void)nba_gameplay_rng_next(&tipoff->rng);
    if ((nba_gameplay_rng_next(&tipoff->rng) & 0x0030u) != 0u)
        actor->behavior_flags_raw |= 0x0001u;
    tipoff->live_state_raw = 2u;
    cpu_enter_play_state(tipoff, NBA_CPU_PLAY_SHOT);
    return true;
}

/* `$85:B678-$B8CA`: preserve the ROM's three-way return contract, both
 * clock urgency gates, blocked-lane rating tail, and clear-lane mode-13
 * route. */
static CpuMode11Outcome cpu_dispatch_rom_mode11(
        NbaTipoff *tipoff, unsigned slot, uint8_t *direction) {
    if (!tipoff || slot >= NBA_GAMEPLAY_ACTOR_COUNT)
        return CPU_MODE11_NORMAL_RETURN;
    NbaTipoffActor *actor = &tipoff->actors[slot];
    /* `$85:B684/$B714/$B820` read actor +$04/+$08/+$0C integer words.
     * Render rounding can cross the exact X=$E2 shot-rectangle boundary. */
    int16_t rom_x = fp_integer_word(actor->x_fp);
    int16_t y = fp_integer_word(actor->y_fp);
    int16_t z = fp_integer_word(actor->z_fp);
    unsigned context_side = tipoff->offense_side ? 1u : 0u;

    /* `$85:B67C-$B88A`: context +$3B replaces the ordinary rectangle/policy
     * route. A matching active controller, either late clock, or a clear lane
     * sends the actor toward the team anchor; only the first three conditions
     * start a shot. This path is normally dormant in CPU-vs-CPU play. */
    if (tipoff->controllers.count[context_side] != 0u) {
        for (unsigned control = 0; control < 5u; ++control) {
            if (tipoff->controllers.record[control].group >= 0 &&
                (uint16_t)tipoff->controllers.record[control].group ==
                    tipoff->camera_side_group_raw &&
                (tipoff->controllers.record[control].held & 0x0040u) != 0u) {
                if (z != 0) return CPU_MODE11_NORMAL_RETURN;
                return cpu_start_rom_shot(tipoff, slot) ?
                    CPU_MODE11_SHOT_STARTED : CPU_MODE11_NORMAL_RETURN;
            }
        }
        if (tipoff->rim_raw_092c < 120u ||
            tipoff->match_clock_raw_0928 < 120u) {
            if (z != 0) return CPU_MODE11_NORMAL_RETURN;
            return cpu_start_rom_shot(tipoff, slot) ?
                CPU_MODE11_SHOT_STARTED : CPU_MODE11_NORMAL_RETURN;
        }
        if (!cpu_lane_to_basket_is_clear(tipoff, slot))
            return CPU_MODE11_NORMAL_RETURN;
        if (direction) {
            *direction = nba_gameplay_target_direction(
                (int16_t)(tipoff->team_context[context_side].anchor_x_raw_0a -
                          rom_x), (int16_t)-y, NULL);
        }
        return CPU_MODE11_CONSUMED_ACTION;
    }

    if (rom_x < -338 || rom_x >= 338) return CPU_MODE11_NORMAL_RETURN;

    if (tipoff->rim_raw_092c < 120u ||
        tipoff->match_clock_raw_0928 < 120u) {
        uint16_t random = nba_gameplay_rng_next(&tipoff->rng) & 0x7FFFu;
        if ((random & 0x0008u) == 0u) {
            if (z != 0) return CPU_MODE11_NORMAL_RETURN;
            return cpu_start_rom_shot(tipoff, slot) ?
                CPU_MODE11_SHOT_STARTED : CPU_MODE11_NORMAL_RETURN;
        }
        (void)cpu_formation_route(tipoff, slot, direction);
        return CPU_MODE11_CONSUMED_ACTION;
    }

    int16_t side_anchor = slot < 5u ? -336 : 336;
    bool same_attack_half = (int16_t)(rom_x ^ side_anchor) >= 0;
    if (cpu_lane_to_basket_is_clear(tipoff, slot)) {
        if (actor->anchor_distance_raw >= 0x70u) {
            (void)cpu_formation_route(tipoff, slot, direction);
            return CPU_MODE11_CONSUMED_ACTION;
        }
        if (cpu_start_rom_layup(tipoff, slot))
            return CPU_MODE11_CONSUMED_ACTION;
        uint16_t random = nba_gameplay_rng_next(&tipoff->rng) & 0x7FFFu;
        if ((random & 0x00F8u) == 0u) {
            if (z != 0) return CPU_MODE11_NORMAL_RETURN;
            return cpu_start_rom_shot(tipoff, slot) ?
                CPU_MODE11_SHOT_STARTED : CPU_MODE11_NORMAL_RETURN;
        }
        (void)cpu_formation_route(tipoff, slot, direction);
        return CPU_MODE11_CONSUMED_ACTION;
    }

    if (!same_attack_half) return CPU_MODE11_NORMAL_RETURN;
    if (same_attack_half &&
        nba_gameplay_mode11_shot_rectangle(rom_x, y, 0)) {
        /* `$85:B714-$B731` classifies X/Y before `$85:B820` tests Z.
         * Airborne rectangle entries return without visiting RNG policy. */
        if (z != 0) return CPU_MODE11_NORMAL_RETURN;
        return cpu_start_rom_shot(tipoff, slot) ?
            CPU_MODE11_SHOT_STARTED : CPU_MODE11_NORMAL_RETURN;
    }

    uint8_t team = team_id_for_context(tipoff, slot / 5u);
    uint8_t rating_36 = 0u, rating_37 = 0u, range_49 = 0u;
    if (!nba_player_gameplay_shot_ratings(
            tipoff->assets, team, actor->roster_slot,
            &rating_36, &rating_37) ||
        !nba_player_gameplay_shot_range(
            tipoff->assets, team, actor->roster_slot, &range_49))
        return CPU_MODE11_NORMAL_RETURN;
    NbaGameplayMode11ShotInput decision = {
        .play_step_raw_0998 = tipoff->play_step_raw,
        .play_cycle_raw_09a4 = tipoff->play_cycle_raw,
        .play_hold_raw_09d0 = tipoff->play_hold_raw,
        .dead_ball_raw_0968 = tipoff->dead_ball_raw_0968,
        .shot_clock_rule_raw_17e1 = tipoff->session->config.rules[8],
        .difficulty_raw_17af = tipoff->session->config.main_values[2],
        .assignment_distance_raw_8a = actor->assignment_distance,
        .anchor_distance_raw_8c = actor->anchor_distance_raw,
        .two_point_rating_raw_36 = rating_36,
        .three_point_rating_raw_37 = rating_37,
        .shot_range_raw_49 = range_49,
        .actor_z = z,
        .same_attack_half = same_attack_half
    };
    if (nba_gameplay_mode11_shot_decision(&decision, &tipoff->rng))
        return cpu_start_rom_shot(tipoff, slot) ?
            CPU_MODE11_SHOT_STARTED : CPU_MODE11_NORMAL_RETURN;
    return CPU_MODE11_NORMAL_RETURN;
}

uint8_t nba_tipoff_replay_mode11_dispatch(NbaTipoff *tipoff, uint8_t actor) {
    if (!tipoff || actor >= NBA_GAMEPLAY_ACTOR_COUNT)
        return CPU_MODE11_NORMAL_RETURN;
    uint8_t direction = tipoff->actors[actor].movement_direction;
    CpuMode11Outcome outcome = cpu_dispatch_rom_mode11(
        tipoff, actor, &direction);
    if (outcome == CPU_MODE11_CONSUMED_ACTION &&
        tipoff->actors[actor].control_mode != 13u)
        cpu_owner_accelerate(tipoff, actor, direction);
    return (uint8_t)outcome;
}

/* $86:9D6E/$9DA6-$A476. One shared launch owns the complete persistent
 * release state. Mode 17 skips the ordinary facing/upper-17 installation. */
static void cpu_release_rom_shot(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *shooter=&tipoff->actors[slot];
    NbaGameplayTeamContext *context=&tipoff->team_context[slot/5u];
    NbaShotLaunchInput in={0};
    NbaShotLaunchState s=tipoff->last_shot_launch;
    uint8_t team=team_id_for_context(tipoff, slot / 5u);
    uint32_t roster;
    if(!nba_player_gameplay_shot_ratings(tipoff->assets,team,shooter->roster_slot,
            &in.rating_two,&in.rating_three) ||
       !nba_player_gameplay_shot_range(tipoff->assets,team,shooter->roster_slot,&in.range_49) ||
       !nba_player_gameplay_free_throw_rating(tipoff->assets,team,shooter->roster_slot,&in.rating_free) ||
       !nba_player_gameplay_roster_address(tipoff->assets,team,shooter->roster_slot,&roster)) return;
    in.actor_x=fp_integer_word(shooter->x_fp); in.actor_y=fp_integer_word(shooter->y_fp);
    in.controller=shooter->controller_assignment_raw;
    in.basket_x=context->anchor_x_raw_0a;
    in.origin_x=tipoff->shot_origin_x; in.origin_y=tipoff->shot_origin_y;
    in.team_group=shooter->team_group_raw_6e;
    in.distance_8c=shooter->anchor_distance_raw; in.defense_8a=shooter->assignment_distance;
    in.movement_4c=shooter->movement_magnitude_raw; in.modifier_b2=shooter->shot_modifier_raw_b2;
    in.stamina_18=shooter->shot_stamina_raw_18;
    in.difficulty=tipoff->session->config.main_values[2];
    in.shot_assistance_17bf=tipoff->session->config.options[5];
    in.hot_team_09c0=tipoff->assistance_team_raw_09c0;
    in.free_throw_0978=tipoff->fouls.free_throw_state_raw_0978;
    in.aim_0982=tipoff->free_throw_aim_y_raw_0982; in.power_0980=tipoff->free_throw_aim_x_raw_0980;
    in.clock_0928=tipoff->match_clock_raw_0928; in.period_0926=tipoff->period_raw_0926;
    in.assist_clock_47=context->match_clock_raw_47;
    in.roster_low=(uint16_t)roster; in.roster_bank=(uint16_t)(roster>>16);
    in.special_entry=shooter->control_mode==17u;
    in.boosted=shooter->movement_boost_timer!=0;
    in.alternate_lower=shooter->free_throw_launch_half_raw_a8!=0;
    s.actor=actor_shot_action(tipoff,shooter); s.facing=shooter->direction;
    s.contact_inhibit=shooter->contact_inhibit_raw_5a;
    s.x=(uint16_t)fp_integer_word(tipoff->ball.x_fp); s.x_fraction=(uint16_t)((tipoff->ball.x_fp&255)*256);
    s.y=(uint16_t)fp_integer_word(tipoff->ball.y_fp); s.y_fraction=(uint16_t)((tipoff->ball.y_fp&255)*256);
    s.z=(uint16_t)fp_integer_word(tipoff->ball.z_fp); s.z_fraction=(uint16_t)((tipoff->ball.z_fp&255)*256);
    s.velocity_x=tipoff->ball.velocity_x; s.velocity_y=tipoff->ball.velocity_y; s.velocity_z=tipoff->ball.velocity_z;
    s.owner=(uint16_t)(int16_t)tipoff->ball.owner_actor;
    s.assist_43=context->previous_dead_ball_actor_raw_43;
    s.assist_45=(uint16_t)context->previous_controller_actor_raw_45;
    memcpy(s.player_stats,shooter->shot_statistics,sizeof(s.player_stats));
    s.rng=tipoff->rng;
    if(!nba_shot_launch(tipoff->assets,&in,&s)) {
        fprintf(stderr,"[SHOT] complete ROM launch rejected invalid inputs/assets\n");
        return;
    }
    tipoff->last_shot_launch=s;
    ++tipoff->shot_launch_serial;
    shooter->direction=(uint8_t)s.facing;
    actor_store_shot_action(tipoff,shooter,&s.actor);
    shooter->contact_inhibit_raw_5a=s.contact_inhibit;
    memcpy(shooter->shot_statistics,s.player_stats,sizeof(s.player_stats));
    unsigned persistent_stat = (slot / NBA_MATCH_LINEUP_SIZE) *
        NBA_MATCH_ROSTER_SIZE + shooter->roster_slot;
    memcpy(tipoff->roster_shot_statistics[persistent_stat],
           shooter->shot_statistics, sizeof(shooter->shot_statistics));
    context->previous_dead_ball_actor_raw_43=s.assist_43;
    context->previous_controller_actor_raw_45=(int16_t)s.assist_45;
    tipoff->rng=s.rng;
    tipoff->ball.x_fp=(int32_t)(int16_t)s.x*256+(s.x_fraction>>8);
    tipoff->ball.y_fp=(int32_t)(int16_t)s.y*256+(s.y_fraction>>8);
    tipoff->ball.z_fp=(int32_t)(int16_t)s.z*256+(s.z_fraction>>8);
    tipoff->ball.velocity_x=s.velocity_x; tipoff->ball.velocity_y=s.velocity_y; tipoff->ball.velocity_z=s.velocity_z;
    tipoff->ball.owner_actor=(int8_t)s.owner; tipoff->possession_actor=-1;
    tipoff->ball.state=NBA_BALL_SHOT;
    tipoff->live_state_raw=s.live_state; tipoff->rim_raw_094a=s.attempt_latch;
    tipoff->dead_ball_raw_0966=s.dead_0966; tipoff->dead_ball_raw_0968=s.height_0968;
    tipoff->dead_ball_raw_096c=s.dead_096c; tipoff->rim_raw_0920=s.bounce_0920;
    tipoff->free_throw_flight_timer_raw_0930=s.timeout_0930;
    tipoff->shot_actor_raw_09c8=(int16_t)s.last_owner; tipoff->rim_raw_096a=s.initial_value;
    tipoff->shot_value_raw=s.value; tipoff->shot_inner_veto_raw=s.inner_veto!=0;
    /* $86:9DFB and three-point overrideA5AA retain the attempted value
     * separately from094C, which the make path later clears. */
    tipoff->hud.shot_category_raw_4939=s.value;
    tipoff->shot_chance_raw=(uint8_t)s.chance; tipoff->shot_miss_index_raw=(uint8_t)s.miss_index;
    tipoff->catch_actor_record_raw_0910=s.ball_record;
    if(tipoff->fouls.shooting_foul_raw_09bc && !in.free_throw_0978)
        tipoff->deferred_shot_foul_phase_raw_0a02=1;
}

/* `$86:A9D0-$AA69`: shared terminal close-finish release used by behavior
 * modes 13 and 14. It seeds a two-point detached ball; the ordinary rim
 * engine remains solely responsible for awarding the basket. */
static void cpu_finish_rom_close_shot(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    tipoff->shot_actor_raw_09c8 = (int16_t)slot;
    tipoff->shot_origin_x = fp_round(actor->x_fp);
    tipoff->shot_origin_y = fp_round(actor->y_fp);
    tipoff->shot_value_raw = 2u;
    tipoff->hud.shot_category_raw_4939=2u; /* $86:A9F7 */
    tipoff->rim_raw_096a = 2u;
    tipoff->shot_inner_veto_raw = false;
    tipoff->shot_miss_index_raw = 0xFFu;
    tipoff->shot_result_resolved = false;
    tipoff->live_state_raw = 1u;
    tipoff->ball_activity_raw = 1u;
    tipoff->ball.owner_actor = -1;
    tipoff->ball.state = NBA_BALL_SHOT;
    tipoff->possession_actor = -1;
    cpu_enter_play_state(tipoff, NBA_CPU_PLAY_SHOT);
    actor->control_mode = 1u;
    actor->contact_inhibit_raw_5a = 0x14u;
    actor->behavior_flags_raw = 0u;
    if (actor->special_contact_raw_56 == 6) {
        tipoff->ball.velocity_x = tipoff->ball.x_fp < 0 ? -0x01A0 : 0x01A0;
        tipoff->ball.velocity_y = tipoff->ball.y_fp < 0 ? 0x0080 : -0x0080;
        tipoff->ball.velocity_z = 0x0048;
    } else {
        tipoff->ball.velocity_x = 0;
        tipoff->ball.velocity_y = 0;
        tipoff->ball.velocity_z = (int16_t)0xFE98u;
    }

    /* `$86:986D-$994B`: the terminal close finish has a second actor-state
     * continuation after `$86:A9D0`. Farther animation families simply
     * receive the dead-ball hold; the four close families install the
     * variant-selected landing pose and pin it to attachment point one. */
    int16_t basket_x = (int16_t)tipoff->court_presentation.basket_x_3fef;
    if (basket_x == 0) basket_x = (int16_t)basket_x_for_side(tipoff->offense_side);
    int dx = fp_integer_word(tipoff->ball.x_fp) - basket_x;
    int dy = fp_integer_word(tipoff->ball.y_fp);
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    int distance = dx > dy ? dx + dy / 4 : dy + dx / 4;
    if (distance < 8) {
        if (actor->pass_direction_raw >= 0x1Bu) {
            actor->control_mode = 7u;
            actor->reaction_threshold = 0xB4u;
        } else {
            static const uint8_t landing_table[4] = {
                0x26u, 0x27u, 0x28u, 0x29u
            };
            unsigned variant = (actor->mode13_variant_raw_58 >> 1) & 3u;
            actor_set_animation(actor, landing_table[variant],
                                landing_table[variant]);
            actor->velocity_x = actor->velocity_y = actor->velocity_z = 0;
            actor->control_mode = 16u;
            actor->reaction_threshold = 0x18u;
            uint16_t upper = 0u, lower = 0u;
            int16_t offset_x = 0, offset_y = 0, offset_z = 0;
            if (actor_animation_resources(tipoff, actor, actor->direction,
                                          &upper, &lower) &&
                nba_player_ball_attachment_point_offsets(
                    tipoff->assets, upper, lower, actor->actor_status_raw_28,
                    1u, &offset_x, &offset_y, &offset_z)) {
                actor->upper_animation_resource_raw_2a = upper;
                actor->lower_animation_resource_raw_2c = lower;
                actor->animation_resources_valid = true;
                actor_publish_body_mirror(actor);
                actor->x_fp = fp_replace_integer_word(
                    actor->x_fp,
                    (int16_t)(basket_x - offset_x));
                actor->y_fp = fp_replace_integer_word(
                    actor->y_fp, (int16_t)-offset_y);
                actor->z_fp = fp_replace_integer_word(
                    actor->z_fp, (int16_t)(80 - offset_z));
            }
        }
    }
}

/* `$87:9BD3[13] -> $87:9C49 -> $86:A7DA-$AA69`: carried-ball close
 * finish. It owns movement, vertical physics, pose attachment and both
 * release exits, so generic mode-11 steering must never run over it. */
static bool cpu_update_rom_layup(NbaTipoff *tipoff, unsigned slot) {
    static const int8_t turn_negative[13] = {
        1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 7, 7, 0
    };
    static const int8_t turn_zero[13] = {
        4, 4, 3, 3, 2, 2, 2, 1, 1, 1, 0, 0, 0
    };
    NbaTipoffActor *actor = &tipoff->actors[slot];
    if (tipoff->possession_actor != (int8_t)slot) return true;

    tipoff->ball_activity_raw = 1u;
    bool airborne = fp_integer_word(actor->z_fp) != 0;
    int dx = actor->velocity_x - actor->mode13_baseline_velocity_x;
    int dy = actor->velocity_y - actor->mode13_baseline_velocity_y;
    bool disturbed = dx < -0x50 || dx >= 0x51 ||
                     dy < -0x50 || dy >= 0x51;
    bool wrong_upper = actor->animation_state < 0x18u ||
                       actor->animation_state >= 0x1Fu;
    if (airborne && (wrong_upper ||
        (actor->contact_action_timer_raw_60 >= 0x12u && disturbed))) {
        actor->direction = (uint8_t)((actor->anchor_direction_raw >> 1) & 7u);
        actor_set_animation(actor, 0x17u, 0u);
        ball_attach_to_actor(tipoff, slot);
        tipoff->shot_origin_x = fp_round(actor->x_fp);
        tipoff->shot_origin_y = fp_round(actor->y_fp);
        cpu_release_rom_shot(tipoff, slot);
        cpu_integrate_actor_vertical(actor);
        actor->x_fp += (int32_t)actor->velocity_x * 2;
        actor->y_fp += (int32_t)actor->velocity_y * 2;
        return true;
    }

    ball_attach_to_actor(tipoff, slot);
    uint16_t timer = actor->contact_action_timer_raw_60;
    if ((timer == 0x24u || (timer < 0x24u && !airborne)) &&
        actor->velocity_z == 0) {
        actor->velocity_z =
            (actor->special_contact_raw_56 == 0 ||
             (actor->special_contact_raw_56 == 6 && actor->direction == 3u)) ?
            0x0270 : 0x0264;
    }
    timer = timer >= 2u ? (uint16_t)(timer - 2u) : 0u;
    actor->contact_action_timer_raw_60 = timer;

    if (timer < 0x24u && actor->special_contact_raw_56 <= 0) {
        if (timer <= 8u) {
            actor->direction = actor->special_contact_raw_56 < 0 ?
                actor->requested_direction :
                (uint8_t)(actor->requested_direction ^ 4u);
        } else {
            unsigned index = (unsigned)((timer - 10u) / 2u);
            if (index > 12u) index = 12u;
            int8_t turn = actor->special_contact_raw_56 < 0 ?
                turn_negative[index] : turn_zero[index];
            actor->direction = (uint8_t)((actor->requested_direction + turn) & 7u);
        }
    }

    actor->x_fp += (int32_t)actor->velocity_x * 2;
    actor->y_fp += (int32_t)actor->velocity_y * 2;
    cpu_integrate_actor_vertical(actor);
    ball_attach_to_actor(tipoff, slot);
    tipoff->ball.velocity_z = (int16_t)0xFE98u;
    if (timer != 0u) return true;

    cpu_finish_rom_close_shot(tipoff, slot);
    return true;
}

/* `$87:9BD3[14] -> $87:9C4E -> $86:B154-$B334`: special receiver close
 * finish. Unlike mode 10, this receiver preserves its pass relationship,
 * jumps before the catch, and can finish directly after ownership changes.
 * The executor itself consumes no RNG. */
static bool cpu_update_rom_special_receiver(NbaTipoff *tipoff, unsigned slot) {
    static const uint8_t upper_table[8] = {
        0x18u, 0x1Cu, 0x18u, 0x1Au, 0x19u, 0x1Bu, 0x1Du, 0x1Eu
    };
    static const int8_t facing_table[13] = {
        4, 4, 3, 3, 2, 2, 2, 1, 1, 1, 0, 0, 0
    };
    NbaTipoffActor *actor = &tipoff->actors[slot];

    /* `$85:963D` has already committed planar/Z physics before this native
     * behavior entry. Keeping that work in the caller makes direct B154
     * differential replay and live scheduling share the same boundary. */

    uint16_t timer = (uint16_t)(actor->reaction_threshold - 2u);
    actor->reaction_threshold = timer;
    if (timer == 0u || (timer & 0x8000u) != 0u) {
        if (tipoff->possession_actor == (int8_t)slot) {
            tipoff->ball_activity_raw = 1u;
            ball_position_at_actor(tipoff, slot);
            cpu_finish_rom_close_shot(tipoff, slot);
        } else {
            tipoff->rim_force_raw_1866 = 0u;
            cpu_restore_normal_mode(tipoff, slot);
            cpu_cancel_rom_pass_activity(tipoff);
        }
        return true;
    }

    bool airborne = fp_integer_word(actor->z_fp) != 0;
    if (airborne && tipoff->possession_actor == (int8_t)slot) {
        bool wrong_upper = actor->animation_state < 0x18u ||
                           actor->animation_state >= 0x1Fu;
        int dx = actor->velocity_x - actor->mode13_baseline_velocity_x;
        int dy = actor->velocity_y - actor->mode13_baseline_velocity_y;
        bool disturbed = timer >= 0x12u &&
            (dx < -0x50 || dx > 0x50 || dy < -0x50 || dy > 0x50);
        if (wrong_upper || disturbed) {
            cpu_restore_normal_mode(tipoff, slot);
            actor->direction = (uint8_t)((actor->anchor_direction_raw >> 1) & 7u);
            actor_set_upper_animation(actor, 0x17u);
            ball_attach_to_actor(tipoff, slot);
            tipoff->ball_activity_raw = 0xFFFFu;
            tipoff->shot_origin_x = fp_round(actor->x_fp);
            tipoff->shot_origin_y = fp_round(actor->y_fp);
            cpu_enter_play_state(tipoff, NBA_CPU_PLAY_SHOT);
            cpu_release_rom_shot(tipoff, slot);
            return true;
        }
    }

    if (!airborne) {
        if (timer >= 0x28u) return true;
        if (actor->animation_state < 0x18u || actor->animation_state >= 0x1Fu) {
            int selector = actor->special_contact_raw_56 + 1;
            if (selector < 0 || selector > 7) selector = 0;
            actor->pass_direction_raw = upper_table[selector];
            actor_set_animation(actor, (uint8_t)actor->pass_direction_raw,
                                0x1Fu);
            return true;
        }
        if (actor->animation_state < 0x24u) {
            actor->velocity_z =
                (actor->special_contact_raw_56 == 0 ||
                 (actor->special_contact_raw_56 == 6 &&
                  actor->direction == 3u)) ? 0x0270 : 0x0264;
        }
        return true;
    }

    if (actor->special_contact_raw_56 != 0) {
        actor->direction = actor->requested_direction;
    } else if (timer <= 8u) {
        actor->direction = (uint8_t)(actor->requested_direction ^ 4u);
    } else {
        unsigned index = (unsigned)((timer - 10u) / 2u);
        if (index > 12u) index = 12u;
        actor->direction = (uint8_t)(
            (actor->requested_direction + facing_table[index]) & 7u);
    }

    bool relationship_valid;
    if (tipoff->possession_actor < 0) {
        relationship_valid = tipoff->pass_receiver_raw == (int16_t)slot;
    } else if (tipoff->possession_actor == (int8_t)slot) {
        relationship_valid = true;
    } else {
        unsigned owner = (unsigned)tipoff->possession_actor;
        relationship_valid = owner < NBA_GAMEPLAY_ACTOR_COUNT &&
            tipoff->actors[owner].control_mode == 15u &&
            tipoff->pass_receiver_raw == (int16_t)slot;
    }
    if (!relationship_valid) {
        tipoff->rim_force_raw_1866 = 0u;
        cpu_restore_normal_mode(tipoff, slot);
        cpu_cancel_rom_pass_activity(tipoff);
        return true;
    }
    if (tipoff->possession_actor == (int8_t)slot) {
        tipoff->live_state_raw = 2u;
        if (actor->special_contact_raw_56 != 6)
            tipoff->rim_force_raw_1866 =
                (uint16_t)(0x34EBu + slot * 0x100u);
        ball_attach_to_actor(tipoff, slot);
    }
    return true;
}

bool nba_tipoff_replay_mode13_close_finish(NbaTipoff *tipoff, uint8_t actor) {
    return tipoff && actor < NBA_GAMEPLAY_ACTOR_COUNT &&
           cpu_update_rom_layup(tipoff, actor);
}

bool nba_tipoff_replay_mode14_close_finish(NbaTipoff *tipoff, uint8_t actor) {
    return tipoff && actor < NBA_GAMEPLAY_ACTOR_COUNT &&
           cpu_update_rom_special_receiver(tipoff, actor);
}

bool nba_tipoff_replay_close_finish_start(NbaTipoff *tipoff, uint8_t actor) {
    return tipoff && actor < NBA_GAMEPLAY_ACTOR_COUNT &&
           cpu_start_rom_layup(tipoff, actor);
}

static bool cpu_special_receiver_self_test(const NbaAssetPack *assets) {
    NbaTipoff state;
    NbaSession session;
    uint16_t seed;

    memset(&state, 0, sizeof(state));
    memset(&session, 0, sizeof(session));
    state.session = &session;publish_exhibition_team_ids(&state);
    state.assets = assets;
    state.possession_actor = -1;
    state.pass_actor_raw = 3;
    state.pass_aux_raw = 4;
    state.pass_receiver_raw = 1;
    state.ball_activity_raw = 1u;
    state.rim_raw_094a = 1u;
    state.inbound_transfer_raw = 1u;
    state.actors[0].control_mode = 14u;
    state.actors[0].reaction_threshold = 2u;
    nba_gameplay_rng_seed(&state.rng, 0x9146u);
    seed = state.rng.state;
    if (!cpu_update_rom_special_receiver(&state, 0u) ||
        state.actors[0].control_mode != 1u ||
        state.actors[0].behavior_timer != 0x2Fu ||
        state.pass_actor_raw != -1 || state.pass_aux_raw != -1 ||
        state.pass_receiver_raw != -1 || state.ball_activity_raw != 0u ||
        state.rim_raw_094a != 0u || state.inbound_transfer_raw != 0u ||
        state.rng.state != seed) return false;

    memset(&state, 0, sizeof(state));
    state.session = &session;publish_exhibition_team_ids(&state);
    state.assets = assets;
    state.possession_actor = 0;
    state.actors[0].control_mode = 14u;
    state.actors[0].reaction_threshold = 2u;
    state.actors[0].special_contact_raw_56 = 1;
    state.ball.owner_actor = 0;
    nba_gameplay_rng_seed(&state.rng, 0x9146u);
    seed = state.rng.state;
    if (!cpu_update_rom_special_receiver(&state, 0u) ||
        state.actors[0].control_mode != 1u ||
        state.possession_actor != -1 || state.ball.owner_actor != -1 ||
        state.ball.state != NBA_BALL_SHOT || state.shot_value_raw != 2u ||
        state.ball.velocity_x != 0 || state.ball.velocity_y != 0 ||
        state.ball.velocity_z != (int16_t)0xFE98u ||
        state.rng.state != seed) return false;

    memset(&state, 0, sizeof(state));
    state.session = &session;publish_exhibition_team_ids(&state);
    state.assets = assets;
    state.possession_actor = 0;
    state.actors[0].control_mode = 14u;
    state.actors[0].reaction_threshold = 2u;
    state.actors[0].special_contact_raw_56 = 6;
    state.actors[0].x_fp = -256;
    state.ball.owner_actor = 0;
    if (!cpu_update_rom_special_receiver(&state, 0u) ||
        state.ball.velocity_x != -0x01A0 ||
        state.ball.velocity_y != -0x0080 ||
        state.ball.velocity_z != 0x0048) return false;

    memset(&state, 0, sizeof(state));
    state.session = &session;publish_exhibition_team_ids(&state);
    state.assets = assets;
    state.possession_actor = -1;
    state.pass_receiver_raw = 0;
    state.actors[0].control_mode = 14u;
    state.actors[0].reaction_threshold = 40u;
    state.actors[0].animation_state = 0x18u;
    state.actors[0].special_contact_raw_56 = 0;
    if (!cpu_update_rom_special_receiver(&state, 0u) ||
        state.actors[0].reaction_threshold != 38u ||
        state.actors[0].velocity_z != 0x0270) return false;
    state.actors[0].velocity_z = 0;
    state.actors[0].z_fp = 0;
    state.actors[0].reaction_threshold = 40u;
    state.actors[0].special_contact_raw_56 = 1;
    if (!cpu_update_rom_special_receiver(&state, 0u) ||
        state.actors[0].velocity_z != 0x0264) return false;

    /* `$86:B1E2-$B202`: inclusive +/-80 retains; 81 converts to 9D6E. */
    memset(&state, 0, sizeof(state));
    state.session = &session;publish_exhibition_team_ids(&state);
    state.assets = assets;
    state.possession_actor = 0;
    state.pass_receiver_raw = 0;
    state.actors[0].control_mode = 14u;
    state.actors[0].reaction_threshold = 22u;
    state.actors[0].animation_state = 0x18u;
    state.actors[0].z_fp = 10 * 256;
    state.actors[0].velocity_x = 80;
    state.actors[0].mode13_baseline_velocity_x = 0;
    if (!cpu_update_rom_special_receiver(&state, 0u) ||
        state.actors[0].control_mode != 14u ||
        state.ball.owner_actor != 0) return false;
    state.actors[0].control_mode = 14u;
    state.actors[0].reaction_threshold = 22u;
    state.actors[0].animation_state = 0x18u;
    state.actors[0].z_fp = 10 * 256;
    state.actors[0].velocity_z = 0;
    state.actors[0].velocity_x = 81;
    state.possession_actor = 0;
    state.ball.owner_actor = 0;
    if (!cpu_update_rom_special_receiver(&state, 0u) ||
        state.actors[0].control_mode != 11u ||
        state.ball.owner_actor != -1 || state.ball.state != NBA_BALL_SHOT)
        return false;

    memset(&state, 0, sizeof(state));
    state.session = &session;publish_exhibition_team_ids(&state);
    state.assets = assets;
    state.possession_actor = 1;
    state.pass_receiver_raw = 0;
    state.actors[1].control_mode = 15u;
    state.actors[0].control_mode = 14u;
    state.actors[0].reaction_threshold = 22u;
    state.actors[0].animation_state = 0x18u;
    state.actors[0].z_fp = 10 * 256;
    if (!cpu_update_rom_special_receiver(&state, 0u) ||
        state.actors[0].control_mode != 14u) return false;
    state.actors[0].reaction_threshold = 22u;
    state.actors[0].z_fp = 10 * 256;
    state.pass_receiver_raw = 2;
    if (!cpu_update_rom_special_receiver(&state, 0u) ||
        state.actors[0].control_mode != 1u ||
        state.pass_receiver_raw != -1) return false;
    return true;
}

/* $86:B979-$BAA1: exact special-shot executor and shared launch entry. */
static bool cpu_update_rom_special_shooter(NbaTipoff *tipoff,unsigned slot) {
    NbaTipoffActor *a=&tipoff->actors[slot];
    NbaShotAction action=actor_shot_action(tipoff,a);
    NbaSpecialShotFrame frame={fp_integer_word(a->x_fp),fp_integer_word(a->y_fp),
        fp_integer_word(a->z_fp),a->controller_assignment_raw,a->direction,a->pass_direction_raw,
        a->upper_animation_resource_raw_2a,a->lower_animation_resource_raw_2c,
        2,0,a->team_group_raw_6e,tipoff->camera_side_group_raw,
        tipoff->possession_actor==(int8_t)slot,a->movement_boost_timer!=0,a->free_throw_launch_half_raw_a8!=0};
    NbaSpecialShotBall ball={(uint16_t)fp_integer_word(tipoff->ball.x_fp),
        (uint16_t)fp_integer_word(tipoff->ball.y_fp),(uint16_t)fp_integer_word(tipoff->ball.z_fp),
        tipoff->shot_previous_actor_x_raw_0922,tipoff->live_state_raw,
        tipoff->attached_ball_state_raw_09f6,tipoff->dead_ball_raw_0968,tipoff->ball.velocity_z};
    NbaSpecialShotResult result=nba_special_shot_step(tipoff->assets,&action,&frame,&ball);
    if(result==NBA_SPECIAL_SHOT_ERROR) return true;
    a->direction=(uint8_t)frame.facing;
    actor_store_shot_action(tipoff,a,&action);
    tipoff->ball.x_fp=(int32_t)(int16_t)ball.x*256+(tipoff->ball.x_fp&255);
    tipoff->ball.y_fp=(int32_t)(int16_t)ball.y*256+(tipoff->ball.y_fp&255);
    tipoff->ball.z_fp=(int32_t)(int16_t)ball.z*256+(tipoff->ball.z_fp&255);
    tipoff->shot_previous_actor_x_raw_0922=ball.previous_actor_x;
    tipoff->live_state_raw=ball.live_state;tipoff->attached_ball_state_raw_09f6=ball.attachment_state;
    tipoff->dead_ball_raw_0968=ball.height_latch;tipoff->ball.velocity_z=ball.velocity_z;
    if(result==NBA_SPECIAL_SHOT_RELEASE) {
        cpu_release_rom_shot(tipoff,slot);
        a->reaction_threshold=a->behavior_flags_raw=0;
    } else if(result==NBA_SPECIAL_SHOT_CANCEL || result==NBA_SPECIAL_SHOT_LOST) {
        a->exact_shot_animation=false;
        if(result==NBA_SPECIAL_SHOT_CANCEL) cpu_enter_play_state(tipoff,NBA_CPU_PLAY_DRIVE);
    }
    cpu_integrate_actor_vertical(a);
    a->x_fp+=(int32_t)a->velocity_x*2; a->y_fp+=(int32_t)a->velocity_y*2;
    a->movement_magnitude_raw=actor_distance(a->velocity_x,a->velocity_y);
    return true;
}

/* `$86:B769/$86:B8CA-$B978`: attach the ball to the live pose throughout
 * the shot jump. Common `$85:963D` physics subtracts `$18*$C6` from +$12
 * and integrates with `$C6=2`; release is based on the signed velocity, not
 * a rendered-frame counter. */
static bool cpu_update_rom_shooter(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *shooter = &tipoff->actors[slot];
    NbaShotAction action = actor_shot_action(tipoff,shooter);
    NbaShotOwnerGate owner = nba_shot_action_owner_gate(
        &action,tipoff->possession_actor == (int8_t)slot);
    if (owner == NBA_SHOT_LOST_OWNER) {
        /* `$86:B867`: restore team-relative mode and cooldown, without
         * canceling animation locks or mutating the new owner's ball. */
        cpu_restore_normal_mode(tipoff,slot);
        shooter->exact_shot_animation = false;
        goto shot_physics;
    }
    if (owner == NBA_SHOT_PUMP_WAIT) goto shot_physics;
    if (owner == NBA_SHOT_PUMP_CANCEL) {
        NbaShotCancelBall ball = {tipoff->live_state_raw,
            (uint16_t)fp_integer_word(tipoff->ball.z_fp),
            tipoff->attached_ball_state_raw_09f6,tipoff->dead_ball_raw_0968,
            tipoff->ball.velocity_z};
        if (nba_shot_action_cancel(tipoff->assets,&action,&ball,
                shooter->free_throw_launch_half_raw_a8 != 0)) {
            actor_store_shot_action(tipoff,shooter,&action);
            tipoff->live_state_raw=ball.live_state;
            tipoff->ball.z_fp=(int32_t)(int16_t)ball.ball_z*256 +
                              (tipoff->ball.z_fp & 255);
            tipoff->ball.velocity_z=ball.ball_velocity_z;
            tipoff->attached_ball_state_raw_09f6=ball.attachment_state;
            tipoff->dead_ball_raw_0968=ball.height_latch;
            shooter->exact_shot_animation=false;
            cpu_enter_play_state(tipoff,NBA_CPU_PLAY_DRIVE);
        }
        goto shot_physics;
    }

    shooter->behavior_flags_raw |= 0x0002u;
    tipoff->live_state_raw = 2u;
    ball_attach_to_actor(tipoff, slot);
    NbaShotStage stage = nba_shot_action_delay(&tipoff->ball_activity_raw,2u,
        tipoff->fouls.free_throw_state_raw_0978 != 0);
    if (stage == NBA_SHOT_DELAY) {
        action.flags=shooter->behavior_flags_raw;
        nba_shot_action_windup_button(&action,shooter->controller_assignment_raw,
            tipoff->fouls.free_throw_state_raw_0978,0u);
        shooter->behavior_flags_raw=action.flags;
        goto shot_physics;
    }
    if (stage == NBA_SHOT_JUMP) {
        action=actor_shot_action(tipoff,shooter);
        NbaShotSidestepInput step={fp_integer_word(shooter->x_fp),
            fp_integer_word(shooter->y_fp),(int16_t)basket_x_for_side(tipoff->offense_side),
            shooter->movement_magnitude_raw,shooter->anchor_distance_raw,
            tipoff->fouls.free_throw_state_raw_0978,tipoff->rng.state};
        nba_shot_action_sidestep(&action,&step);
        if (nba_shot_action_jump(tipoff->assets,&action,false,
                shooter->free_throw_launch_half_raw_a8 != 0))
            actor_store_shot_action(tipoff,shooter,&action);
        goto shot_physics;
    }
    /* `$86:B8CA-$B978` turns only once per call once lower +$44 >= $600.
     * Stationary wind-up/sidestep has already returned above; the external
     * ball-launch implementation remains a separate integration boundary. */
    NbaShotGateInput gate={
        fp_integer_word(shooter->x_fp),fp_integer_word(shooter->y_fp),
        fp_integer_word(shooter->z_fp),shooter->velocity_z,-1,
        shooter->lower_animation_accumulator_raw_44,
        tipoff->fouls.free_throw_state_raw_0978,tipoff->rng.state,0,
        shooter->direction,(int16_t)basket_x_for_side(tipoff->offense_side)
    };
    NbaShotGate decision=nba_shot_action_gate(&gate);
    shooter->direction=(uint8_t)gate.facing;
    if (decision==NBA_SHOT_RELEASE) {
        cpu_release_rom_shot(tipoff,slot);
        NbaShotAction cleanup={0};
        nba_shot_action_clear(&cleanup);
        shooter->reaction_threshold=cleanup.timer;
        shooter->behavior_flags_raw=cleanup.flags;
    } else if(decision==NBA_SHOT_GROUNDED) {
        /* `$86:B886` latches the pump fake, not an unconditional release. */
        shooter->behavior_flags_raw |= 0x80u;
    }

shot_physics:
    cpu_integrate_actor_vertical(shooter);
    shooter->x_fp += (int32_t)shooter->velocity_x * 2;
    shooter->y_fp += (int32_t)shooter->velocity_y * 2;
    shooter->movement_magnitude_raw = actor_distance(
        shooter->velocity_x, shooter->velocity_y);
    return true;
}

/* Deterministic mode-17 caller test: the natural CPU trace rarely chooses
 * this facing-dependent action, so exercise selector -> jump -> 9DA6 here. */
static bool cpu_special_shot_self_test(const NbaAssetPack *assets,NbaSession *session) {
    NbaTipoff s={0};
    s.assets=assets;s.session=session;publish_exhibition_team_ids(&s);s.cpu_vs_cpu=true;s.possession_actor=0;
    s.ball.owner_actor=0;s.rng.state=1;s.assistance_team_raw_09c0=0xFFFF;
    s.team_context[0].anchor_x_raw_0a=336;
    for(unsigned i=5;i<10;++i)s.actors[i].x_fp=-1000*256;
    NbaTipoffActor *a=&s.actors[0];
    /* CPU fixture: zero-initialized +16 would mean human pad0. Shot startup
     * correctly preserves controller ownership instead of clearing it. */
    a->controller_assignment_raw=-1;
    a->x_fp=280*256;a->anchor_distance_raw=56;a->anchor_direction_raw=4;
    a->shot_stamina_raw_18=0x7FFF;a->assignment_distance=100;
    a->animation_upper_queue_cursor_raw_18=a->animation_lower_queue_cursor_raw_1a=0xFFFF;
    s.ball.x_fp=280*256+37;s.ball.y_fp=83;s.ball.z_fp=40*256+171;
    if(!cpu_start_rom_shot(&s,0) || a->control_mode!=17 ||
       a->animation_state!=0x14 || a->lower_animation_state!=0x1F ||
       s.ball_activity_raw!=1 || a->pass_direction_raw!=0) return false;
    if(!cpu_update_rom_special_shooter(&s,0) || s.ball_activity_raw!=3 ||
       (s.ball.x_fp&255)!=37 || (s.ball.y_fp&255)!=83 || (s.ball.z_fp&255)!=171) return false;
    if(!cpu_update_rom_special_shooter(&s,0) || s.ball_activity_raw!=0xFFFF ||
       a->velocity_z!=0x228 || a->z_fp<=0) return false;
    a->rom_upper_animation_phase_raw_3a=3;a->reaction_threshold=8;
    int32_t bx=s.ball.x_fp,by=s.ball.y_fp,bz=s.ball.z_fp;
    if(!cpu_update_rom_special_shooter(&s,0) || s.ball.owner_actor!=-1 ||
       a->control_mode!=11 || a->animation_state!=0x14 ||
       s.last_shot_launch.actor.animation.upper_phase!=3 ||
       s.last_shot_launch.player_stats[0]!=1 || s.last_shot_launch.initial_value!=2 ||
       s.ball.x_fp!=bx || s.ball.y_fp!=by || s.ball.z_fp!=bz ||
       a->reaction_threshold!=0 || a->behavior_flags_raw!=0) return false;
    return true;
}

/* Integration guards complement ROM-call replay: they exercise the actual
 * mode-12 dispatcher and persistent ball state, not just the leaf helpers. */
static bool cpu_shot_branches_self_test(const NbaAssetPack *assets,
                                        NbaSession *session) {
    NbaTipoff s={0};
    s.assets=assets; s.session=session;publish_exhibition_team_ids(&s); s.cpu_vs_cpu=true;
    s.possession_actor=0; s.handler_actor=0; s.rng.state=1;
    NbaTipoffActor *a=&s.actors[0];
    a->x_fp=100*256; a->y_fp=0; a->anchor_distance_raw=119;
    a->controller_assignment_raw=-1;
    s.ball.x_fp=37; s.ball.y_fp=83; s.ball.z_fp=171;
    if (!cpu_start_rom_shot(&s,0) || s.ball_activity_raw!=1 ||
        a->velocity_z!=0 || a->lower_animation_state!=0x16 ||
        !a->exact_shot_animation || (s.ball.x_fp&255)!=37 ||
        (s.ball.y_fp&255)!=83 || (s.ball.z_fp&255)!=171) return false;
    for (unsigned i=0;i<14;++i) {
        if (!cpu_update_rom_shooter(&s,0) ||
            s.ball_activity_raw!=(uint16_t)(3+2*i) || a->velocity_z!=0)
            return false;
    }
    uint16_t rng=s.rng.state;
    if (!cpu_update_rom_shooter(&s,0) || s.ball_activity_raw!=0xffff ||
        a->lower_animation_state!=0x32 || a->velocity_z<=0 ||
        (a->velocity_x==0 && a->velocity_y==0) || s.rng.state!=rng)
        return false;

    /* Cancellation cannot fire until both upper animation thresholds pass. */
    a->behavior_flags_raw=0x84; a->rom_upper_animation_phase_raw_3a=3;
    a->upper_animation_accumulator_raw_42=0x600;
    if (!cpu_update_rom_shooter(&s,0) || a->control_mode!=12) return false;
    a->rom_upper_animation_phase_raw_3a=4; a->upper_animation_accumulator_raw_42=0x5ff;
    if (!cpu_update_rom_shooter(&s,0) || a->control_mode!=12) return false;
    a->upper_animation_accumulator_raw_42=0x600;
    a->actor_status_raw_28=0x1234; a->reaction_threshold=9;
    s.ball.z_fp=83*256+171; s.ball.velocity_z=-18;
    s.attached_ball_state_raw_09f6=1;
    if (!cpu_update_rom_shooter(&s,0) || a->control_mode!=11 ||
        a->behavior_flags_raw!=0 || a->reaction_threshold!=0 ||
        a->actor_status_raw_28!=0x9234 || a->upper_animation_lock_raw_46!=0 ||
        a->lower_animation_lock_raw_48!=0 || s.ball_activity_raw!=0 ||
        s.live_state_raw!=0 || s.ball.z_fp!=40*256+171 || s.ball.velocity_z!=0 ||
        s.dead_ball_raw_0968!=40 || s.attached_ball_state_raw_09f6!=2)
        return false;

    /* Lost ownership beats a ready cancel latch and cannot touch the ball. */
    s.possession_actor=1; s.camera_side_group_raw=5;
    s.ball_activity_raw=7; a->control_mode=12; a->team_group_raw_6e=0;
    a->behavior_flags_raw=0x80; a->upper_animation_lock_raw_46=0x16;
    NbaTipoffBall old_ball=s.ball;
    if (!cpu_update_rom_shooter(&s,0) || a->control_mode!=2 ||
        a->behavior_timer!=0x2f || a->behavior_flags_raw!=0 ||
        a->actor_status_raw_28!=0 || a->upper_animation_lock_raw_46!=0x16 ||
        s.ball_activity_raw!=7 || memcmp(&s.ball,&old_ball,sizeof(old_ball)))
        return false;
    s.simulation_tick=2; s.live_state_raw=0; s.ball.owner_actor=-1;
    s.ball.state=NBA_BALL_BOUNCE; s.cpu_play_state=NBA_CPU_PLAY_DRIVE;
    if (!cpu_live_loose_ball_contact_due(&s)) return false;
    s.ball.owner_actor=1;
    if (cpu_live_loose_ball_contact_due(&s)) return false;
    s.ball.owner_actor=-1; s.live_state_raw=0x82;
    if (cpu_live_loose_ball_contact_due(&s)) return false;
    return true;
}

/* `$86:A6B3-$A790`: only the owner advances mode 15's release gate. The
 * ROM reattaches through `$87:B649` while threshold >= actor +$3A, then
 * dispatches `$86:99C4` with family selected by signed actor +$C0. */
bool nba_tipoff_update_rom_passer(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *passer = &tipoff->actors[slot];
    /* `$86:A6B3-$A6C7`: a mode-15 actor that no longer matches ownership
     * `$093E` counts +$60 down at the logical-pass delta. On underflow,
     * A772 dispatches `$86:9846/$9861`: mode 1, +$64=$2F, and complete
     * clearing of +$60/+$7E/+$28. Leaving this actor in mode 15 forever
     * prevents `$85:B271`'s five-player readiness barrier from completing. */
    if (tipoff->possession_actor != (int8_t)slot) {
        passer->reaction_threshold = (uint16_t)(
            passer->reaction_threshold - 2u);
        if ((int16_t)passer->reaction_threshold < 0) {
            passer->control_mode = 1u;
            passer->behavior_timer = 0x2Fu;
            passer->reaction_threshold = 0u;
            passer->behavior_flags_raw = 0u;
            passer->actor_status_raw_28 = 0u;
        }
        return true;
    }
    /* `$86:A6B3` keys ownership from `$093E`; the release gate at `$A749`
     * rereads raw `$0946`. Cached host identities may drive attachment while
     * waiting, but cannot authorize a launch after `$86:A613` canceled it. */
    if (tipoff->handler_actor != slot ||
        tipoff->receiver_actor >= NBA_GAMEPLAY_ACTOR_COUNT)
        return true;
    if (passer->pass_released_raw) return true;
    if (passer->pass_family_raw == 2) {
        /* `$86:A6EC -> $86:A629`: the boosted `$AFC4` family stays attached
         * through its jump. Common actor physics runs before this behavior
         * dispatch; the first descending pass changes +$C0 to 4 and installs
         * `$2C/$24`, then the following family-4 dispatch releases directly
         * through `$A749` without the `$2A-$31` phase table. */
        passer->x_fp += (int32_t)passer->velocity_x * 2;
        passer->y_fp += (int32_t)passer->velocity_y * 2;
        cpu_integrate_actor_vertical(passer);
        ball_attach_to_actor(tipoff, slot);
        if (passer->velocity_z < 0) {
            passer->pass_family_raw = 4;
            if (passer->exact_pass_animation) {
                actor_animation_command(tipoff, passer, NBA_ANIMATION_INSTALL_UPPER, 0x2Cu);
                actor_animation_command(tipoff, passer, NBA_ANIMATION_INSTALL_LOWER, 0x24u);
            } else {
                actor_set_animation(passer, 0x2Cu, 0x24u);
            }
            passer->upper_animation_phase_raw = 3u;
        }
        return true;
    }
    bool immediate_airborne_release = passer->pass_family_raw == 4;
    if (!immediate_airborne_release && passer->upper_animation_phase_raw <=
            passer->pass_release_threshold_raw) {
        ball_attach_to_actor(tipoff, slot);
        return true;
    }
    if (tipoff->pass_receiver_raw < 0) {
        /* `$86:A777-$A78F`: normalize the installed pass executor back to
         * mode 11 and retain `$09C4`; no ball launch occurs. A later mode-11
         * decision may install a fresh receiver and overwrite the metadata. */
        passer->control_mode = 11u;
        passer->reaction_threshold = 0u;
        passer->behavior_flags_raw = 0u;
        passer->actor_status_raw_28 = 0u;
        tipoff->pass_actor_raw = -1;
        tipoff->pass_receiver_raw = -1;
        tipoff->pass_aux_raw = -1;
        if (tipoff->live_state_raw < 0x80u)
            cpu_enter_play_state(tipoff, NBA_CPU_PLAY_ATTACK);
        return true;
    }
    /* `$86:9A07-$9A1D`: signed +$C0 selects all three packed tables:
     * negative `$9C6F`, positive `$9C93`, and zero `$9CB7`. */
    unsigned family = passer->pass_family_raw < 0 ? 0u :
                      passer->pass_family_raw > 0 ? 1u : 2u;
    unsigned band = passer->pass_band_raw / 6u;
    int16_t duration = 0, vertical = 0, opaque = 0;
    if (!nba_assets_gameplay_pass_launch(
            tipoff->assets, (uint8_t)family, (uint8_t)band,
            &duration, &vertical, &opaque) || duration <= 0)
        return true;
    (void)opaque; /* `$86:99C4` does not consume the third record word. */
    uint8_t receiver_slot = (uint8_t)tipoff->pass_receiver_raw;
    const NbaTipoffActor *receiver = &tipoff->actors[receiver_slot];
    tipoff->receiver_actor = receiver_slot;
    int target_x = fp_round(receiver->x_fp) +
        (int)pass_lead_component(receiver->velocity_x, (uint16_t)duration);
    int target_y = fp_round(receiver->y_fp) +
        (int)pass_lead_component(receiver->velocity_y, (uint16_t)duration);
    /* `$86:9A8B-$9AAF` clamps the predicted endpoint before subtracting the
     * current ball position and dividing by the launch scalar. This matters
     * most on baseline inbounds: the hand attachment may sit outside these
     * limits, so omitting the clamp can manufacture a zero-velocity pass
     * which the court-edge routine immediately cancels. */
    if (target_x > 362) target_x = 362;
    if (target_x < -362) target_x = -362;
    if (target_y > 192) target_y = 192;
    if (target_y < -192) target_y = -192;
    /* `$86:A74E` clears `$09C4` immediately before `$86:99C4` detaches the
     * ball. `$09B8` remains the independent inbound-transfer witness. */
    tipoff->pass_active_raw = 0u;
    ball_launch(tipoff, target_x, target_y, (unsigned)duration,
                vertical, NBA_BALL_PASS);
    /* `$86:A761-$A76C`: ordinary families retain a ten-tick post-release
     * actor timer. Airborne families 4/5 bypass this write. */
    if (passer->pass_family_raw < 4)
        passer->reaction_threshold = 0x0Au;
    /* `$86:9B84-$9B8F`: `$99C4` closes the temporary pass-init state 2
     * before normal live ball processing resumes. Leaving it set makes
     * `$85:9D40` reject an otherwise centered basket. */
    if (tipoff->live_state_raw < 0x81u) tipoff->live_state_raw = 0u;
    tipoff->possession_actor = -1;
    passer->pass_released_raw = true;
    return true;
}

static bool cpu_boundary_pass_recovery_self_test(void) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    state.pass_actor_raw = 3;
    state.pass_aux_raw = 5;
    state.pass_receiver_raw = 4;
    state.rim_raw_094a = 7u;
    state.ball_activity_raw = 9u;
    state.inbound_transfer_raw = 1u;
    state.pass_active_raw = 1u;
    state.possession_actor = 3;
    state.ball.owner_actor = 3;
    state.ball.state = NBA_BALL_PASS;
    state.ball.velocity_x = 71;
    state.ball.velocity_y = -37;
    int32_t x_fp = 395 * 256;
    int32_t y_fp = 12 * 256;
    int16_t velocity_x = 64;
    int16_t velocity_y = -11;
    cpu_clamp_ball_to_court(
        &state, &x_fp, &y_fp, &velocity_x, &velocity_y);
    if (x_fp != 394 * 256 || velocity_x != 0 || velocity_y != -11 ||
        state.pass_actor_raw != -1 || state.pass_aux_raw != -1 ||
        state.pass_receiver_raw != -1 || state.rim_raw_094a != 0u ||
        state.ball_activity_raw != 0u || state.inbound_transfer_raw != 0u ||
        state.pass_active_raw != 1u || state.possession_actor != 3 ||
        state.ball.owner_actor != 3 || state.ball.state != NBA_BALL_PASS ||
        state.ball.velocity_x != 71 || state.ball.velocity_y != -37)
        return false;

    /* A player at the same rectangle must not execute the ball-global clear. */
    state.pass_actor_raw = 3;
    state.pass_aux_raw = 5;
    state.pass_receiver_raw = 4;
    state.inbound_transfer_raw = 1u;
    y_fp = -225 * 256;
    velocity_y = -64;
    cpu_clamp_actor_to_court(&x_fp, &y_fp, &velocity_x, &velocity_y);
    if (y_fp != -224 * 256 || velocity_y != 0 ||
        state.pass_actor_raw != 3 || state.pass_aux_raw != 5 ||
        state.pass_receiver_raw != 4 || state.inbound_transfer_raw != 1u)
        return false;

    /* `$86:A749-$A78F`: a prior ball-edge clear makes the release abort
     * without RNG, detachment, or a host-cached receiver launch. */
    memset(&state, 0, sizeof(state));
    state.handler_actor = 3u;
    state.receiver_actor = 4u;
    state.possession_actor = 3;
    state.pass_actor_raw = -1;
    state.pass_aux_raw = -1;
    state.pass_receiver_raw = -1;
    state.pass_active_raw = 1u;
    state.ball.owner_actor = 3;
    state.ball.state = NBA_BALL_ATTACHED;
    state.actors[3].control_mode = 15u;
    state.actors[3].reaction_threshold = 9u;
    state.actors[3].behavior_flags_raw = 0xFFFFu;
    state.actors[3].actor_status_raw_28 = 0xFFFFu;
    state.actors[3].upper_animation_phase_raw = 2u;
    state.actors[3].pass_release_threshold_raw = 1u;
    if (!nba_tipoff_update_rom_passer(&state, 3u) ||
        state.actors[3].control_mode != 11u ||
        state.actors[3].reaction_threshold != 0u ||
        state.actors[3].behavior_flags_raw != 0u ||
        state.actors[3].actor_status_raw_28 != 0u ||
        state.pass_active_raw != 1u || state.ball.owner_actor != 3 ||
        state.ball.state != NBA_BALL_ATTACHED)
        return false;

    /* `$86:A6B3-$A6C7 -> $86:9846`: stale non-owner passers expire. */
    state.possession_actor = 3;
    state.actors[2].control_mode = 15u;
    state.actors[2].reaction_threshold = 2u;
    state.actors[2].behavior_flags_raw = 6u;
    state.actors[2].actor_status_raw_28 = 0xFFFFu;
    if (!nba_tipoff_update_rom_passer(&state, 2u) ||
        state.actors[2].control_mode != 15u ||
        state.actors[2].reaction_threshold != 0u) return false;
    if (!nba_tipoff_update_rom_passer(&state, 2u) ||
        state.actors[2].control_mode != 1u ||
        state.actors[2].behavior_timer != 0x2Fu ||
        state.actors[2].reaction_threshold != 0u ||
        state.actors[2].behavior_flags_raw != 0u ||
        state.actors[2].actor_status_raw_28 != 0u) return false;
    return true;
}

static bool cpu_boosted_pass_self_test(const NbaAssetPack *assets,
                                       NbaSession *session) {
    /* This is an isolated test precondition, not match initialization.
     * Philadelphia's original twelve +$3E profiles are all below $55; a
     * valid selected team must not fail startup merely because this one
     * synthetic boosted-pass case requires a qualifying player. Use the
     * known Orlando test roster without changing the caller's session or
     * any original rating/branch. The full lifecycle assertions stay active. */
    NbaSession fixture = *session;
    fixture.right_team = 18u;
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    state.assets = assets;
    state.session = &fixture;publish_exhibition_team_ids(&state);
    state.possession_actor = 0;
    state.handler_actor = 0u;
    state.receiver_actor = 1u;
    state.pass_actor_raw = -1;
    state.pass_receiver_raw = -1;
    state.actors[0].x_fp = -300 * 256;
    state.actors[0].y_fp = 40 * 256;
    state.actors[1].x_fp = 80 * 256;
    state.actors[1].y_fp = 40 * 256;
    state.actors[0].movement_boost_timer = 20u;
    state.actors[0].lower_animation_state = 0x0Cu;
    state.actors[0].roster_slot = 0u;
    uint8_t profile_39 = 0u, profile_3e = 0u;
    for (uint8_t roster = 0u; roster < 12u; ++roster) {
        if (nba_player_gameplay_pass_profiles(
                assets, team_id_for_context(&state, 0u), roster,
                &profile_39, &profile_3e) && profile_3e >= 0x55u) {
            state.actors[0].roster_slot = roster;
            break;
        }
    }
    if (profile_3e < 0x55u) return false;
    uint16_t distance = 0u;
    uint8_t fine = nba_gameplay_pass_direction(380, 0, &distance);
    if (fine >= 16u || distance < 0x119u) return false;
    state.actors[0].movement_direction =
        (uint8_t)(((fine >> 1) + 4u) & 7u);
    if (!nba_tipoff_begin_rom_pass(&state, 0u, 1u) ||
        state.actors[0].control_mode != 15u ||
        state.actors[1].control_mode != 10u ||
        state.actors[1].reaction_threshold != 0x46u ||
        state.actors[0].pass_family_raw != 2 ||
        state.actors[0].animation_state != 0x18u ||
        state.actors[0].lower_animation_state != 0x1Fu ||
        state.actors[0].velocity_z != 0x0258 ||
        state.pass_active_raw != 1u) return false;
    for (unsigned pass = 0u; pass < 32u &&
            state.actors[0].pass_family_raw == 2; ++pass)
        (void)nba_tipoff_update_rom_passer(&state, 0u);
    if (state.actors[0].pass_family_raw != 4 ||
        state.actors[0].animation_state != 0x2Cu ||
        state.actors[0].lower_animation_state != 0x24u) return false;
    (void)nba_tipoff_update_rom_passer(&state, 0u);
    return state.actors[0].pass_released_raw &&
           state.ball.state == NBA_BALL_PASS &&
           state.ball.owner_actor < 0 && state.pass_active_raw == 0u;
}

static void score_made_basket(NbaTipoff *tipoff) {
    /* `$85:A079-$A345` is the made-basket branch. `$094C` is added to
     * team-record +$26 (`$4711/$4791`), `$0936` becomes `$82`, and
     * `$0952/$0954` seed the dead-ball/inbound path before `$094C` clears. */
    unsigned scoring_side = tipoff->offense_side & 1u;
    unsigned inbound_side = scoring_side ^ 1u;
    uint16_t shot_value = tipoff->shot_value_raw;
    /* $85:A1BC-A1C7 reads the scoring context+43. A negative assist
     * leaves493D alone; do not replace that original retained-state rule. */
    uint16_t assist=tipoff->team_context[scoring_side].previous_dead_ball_actor_raw_43;
    if((int16_t)assist>=0)tipoff->hud.assist_raw_493d=assist;

    /* $85:A081-$A0EA precedes both effect RNG and the score increment. */
    NbaShotMomentum momentum = {0};
    for (unsigned i=0;i<NBA_GAMEPLAY_ACTOR_COUNT;++i) {
        momentum.made_run[i]=tipoff->actors[i].shot_modifier_raw_b2;
        momentum.defensive_run[i]=tipoff->actors[i].defensive_run_raw_b4;
        momentum.team_group[i]=tipoff->actors[i].team_group_raw_6e;
    }
    momentum.assistance_team=tipoff->assistance_team_raw_09c0;
    if (nba_shot_momentum_make(&momentum,(uint16_t)tipoff->shot_actor_raw_09c8,
            tipoff->session->config.options[6],tipoff->match_clock_raw_0928,
            tipoff->session->score[0],tipoff->session->score[1])) {
        for (unsigned i=0;i<NBA_GAMEPLAY_ACTOR_COUNT;++i) {
            tipoff->actors[i].shot_modifier_raw_b2=momentum.made_run[i];
            tipoff->actors[i].defensive_run_raw_b4=momentum.defensive_run[i];
        }
        tipoff->assistance_team_raw_09c0=momentum.assistance_team;
    }

    /* `$85:A0EB-$A115`: the make effect is selected before foul/stat/score
     * bookkeeping. Effect four suppresses the selector entirely; otherwise
     * zero is rejected, so the gameplay LFSR may advance more than once. */
    tipoff->rim_raw_092c = 0x05A0u;
    tipoff->rim_raw_096a = 0u;
    tipoff->inbound_transfer_raw = 0u;
    if (tipoff->rim_effect.effect_raw_401b != 4u) {
        uint16_t selector;
        do {
            selector = (uint16_t)(nba_gameplay_rng_next(&tipoff->rng) & 3u);
        } while (selector == 0u);
        (void)nba_gameplay_effect_start(&tipoff->rim_effect,
                                        (uint16_t)(selector - 1u));
    }
    (void)nba_gameplay_foul_record_made_basket(&tipoff->fouls);
    tipoff->ball_activity_raw = 0u;
    tipoff->rim_raw_094a = 0u;
    tipoff->rim_raw_0962 = 0u;
    tipoff->session->score[scoring_side] = (uint16_t)(
        tipoff->session->score[scoring_side] + shot_value);
    tipoff->last_scoring_side = (uint8_t)scoring_side;
    tipoff->live_state_raw = 0x82u;
    tipoff->inbound_state_raw = (uint16_t)(inbound_side * 5u);
    tipoff->inbound_actor_raw = (uint16_t)(tipoff->inbound_state_raw + 2u);
    tipoff->inbound_layout_raw = 0;
    tipoff->inbound_timer_raw = 300u;
    tipoff->dead_clock_enabled_raw_0a04 = 300u; /* $85:A265-$A268 */
    /* `$85:A219-$A222`: dead-ball scoring requests play `$01`; B128 later
     * preserves it while resetting the stream at the actor-pass boundary. */
    tipoff->play_code = 1u;
    tipoff->play_request_raw = 1u;
    tipoff->offense_side = (uint8_t)inbound_side;
    tipoff->possession_team = (int8_t)inbound_side;
    tipoff->handler_actor = (uint8_t)tipoff->inbound_actor_raw;
    tipoff->receiver_actor = (uint8_t)(inbound_side * 5u + 4u);
    NbaGameplayInboundTarget target;
    int16_t context_anchor =
        tipoff->team_context[inbound_side].anchor_x_raw_0a;
    /* `$85:C450-$C45E` reads the integer words at `$09B0/$09B2` and
     * `$3EEF`. Nearest-pixel rounding can turn a reachable +402.996 edge
     * target into +403, one pixel beyond the native actor cap plus F4F2's
     * asymmetric arrival box. */
    if (nba_gameplay_inbound_target(
            tipoff->inbound_layout_raw, fp_integer_word(tipoff->ball.x_fp),
            fp_integer_word(tipoff->ball.y_fp), context_anchor,
            fp_integer_word(tipoff->ball.x_fp), &tipoff->rng, &target)) {
        tipoff->inbound_target_x_raw = target.x;
        tipoff->inbound_target_y_raw = target.y;
        tipoff->inbound_direction_raw = target.direction;
        if (target.play_requested) {
            tipoff->play_code = target.play_code;
            tipoff->play_request_raw = 1u;
        }
        /* `$85:C5AD-$C5BD` installs the boundary target before the later
         * `$85:AD86-$AD95` formation pass deliberately skips this actor. */
        tipoff->actors[tipoff->inbound_actor_raw].target_x = target.x;
        tipoff->actors[tipoff->inbound_actor_raw].target_y = target.y;
    }
    tipoff->inbound_ready_raw = 0u;
    tipoff->shot_value_raw = 0u;

    /* `$85:A26F-$A292`: ties retain the previous leader. */
    if (tipoff->leading_side_raw_1403 == 0u) {
        if (tipoff->session->score[1] > tipoff->session->score[0]) {
            tipoff->leading_side_raw_1403 = 1u;
            ++tipoff->right_lead_change_count_raw_1407;
        }
    } else if (tipoff->session->score[0] > tipoff->session->score[1]) {
        tipoff->leading_side_raw_1403 = 0u;
        ++tipoff->left_lead_change_count_raw_1405;
    }
    /* `$85:A292-$A336`: these event bits are gameplay-visible presentation
     * state. The side-zero three-point branch also consumes one result whose
     * value is discarded, preserving subsequent RNG cadence. */
    if (shot_value == 3u) {
        tipoff->rim_raw_13e7 |= 0x1000u;
        if (tipoff->camera_side_group_raw == 0u)
            (void)nba_gameplay_rng_next(&tipoff->rng);
    }
    tipoff->rim_raw_13e7 |= 0x0004u;
    hud_request_score(tipoff); /* $85:A346 -> $83:CE36, after score/event stores */
    tipoff->shot_result_resolved = true;
    tipoff->ball.owner_actor = -1;
    tipoff->possession_actor = -1;
}

/* `$87:92AD-$949E -> $87:9B38/$9B41-$9BC8`: violation and ordinary foul
 * branches select an inbound side/layout, then share one `$82` dead-ball
 * initializer. The ROM owner is cleared after its current actor is put back
 * in mode 2; BOUNCE is the port's ownerless equivalent of that record state. */
static void cpu_begin_dead_ball(NbaTipoff *tipoff, uint8_t selected_actor,
                                uint16_t side_group, int16_t layout,
                                bool interference) {
    if (!tipoff || selected_actor >= NBA_GAMEPLAY_ACTOR_COUNT ||
        (side_group != 0u && side_group != 5u)) return;
    unsigned side = side_group / 5u;
    /* `$87:92C5-$92CB` makes code 6 the sole path here that updates both
     * persistent camera side `$093A` and inbound side `$0952`. Ordinary
     * foul branches only write `$0952`, so keep those two words distinct. */
    if (interference) tipoff->camera_side_group_raw = (uint8_t)side_group;
    tipoff->inbound_state_raw = side_group;
    if (interference)
        tipoff->team_context[side].dead_ball_actor_raw_3f = selected_actor;
    tipoff->inbound_layout_raw = layout;
    if (interference) {
        tipoff->dead_ball_raw_096c = 0u;
        tipoff->dead_ball_raw_0966 = NBA_GAMEPLAY_UNKNOWN_WORD;
    }

    tipoff->live_state_raw = 0x82u;
    tipoff->inbound_timer_raw = 300u;
    tipoff->role_rebuild_raw_09d6 = 300u;
    tipoff->rim_raw_092c = 0x05A0u;
    tipoff->shot_clock_mirror_raw_09c6 = 0x05A0u;
    tipoff->dead_ball_raw_0968 = 0u;
    tipoff->rim_raw_096a = 0u;
    tipoff->dead_ball_x_raw_09b0 = fp_integer_word(tipoff->ball.x_fp);
    tipoff->dead_ball_y_raw_09b2 = fp_integer_word(tipoff->ball.y_fp);

    bool had_owner = tipoff->possession_actor >= 0 &&
                     tipoff->possession_actor < NBA_GAMEPLAY_ACTOR_COUNT;
    if (tipoff->possession_actor >= 0 &&
        tipoff->possession_actor < NBA_GAMEPLAY_ACTOR_COUNT) {
        NbaTipoffActor *owner = &tipoff->actors[tipoff->possession_actor];
        if (owner->control_mode != 8u) owner->control_mode = 2u;
    }
    if (had_owner || tipoff->fouls.foul_event_raw_0964 == 3u) {
        /* `$87:9B74-$9B7F` stops both axes for code 3 even without an owner;
         * `$87:9BA9-$9BAC` also stops any owned ball. Other ownerless events
         * preserve velocity. Ownership is signed `$093E`, not a host cache. */
        tipoff->ball.velocity_x = 0;
        tipoff->ball.velocity_y = 0;
    }
    cpu_cancel_rom_pass_activity(tipoff);
    tipoff->ball.owner_actor = -1;
    tipoff->possession_actor = -1;
    tipoff->pass_active_raw = 0u;
    tipoff->rim_raw_097c = 0u;
    if (had_owner) tipoff->ball.state = NBA_BALL_BOUNCE;

}

/* `$85:C37D` runs after the `$87:92A5-$949E` parent boundary. Keep these
 * downstream writes separate so they are not falsely attributed to 9B41. */
static void cpu_finalize_dead_ball_inbound(NbaTipoff *tipoff) {
    uint16_t side_group = tipoff->inbound_state_raw;
    if (side_group != 0u && side_group != 5u) return;
    unsigned side = side_group / 5u;
    tipoff->inbound_actor_raw = (uint16_t)(side_group + 2u);
    tipoff->offense_side = (uint8_t)side;
    tipoff->possession_team = (int8_t)side;
    tipoff->handler_actor = (uint8_t)tipoff->inbound_actor_raw;
    tipoff->receiver_actor = (uint8_t)(side_group + 4u);
    NbaGameplayInboundTarget target;
    int16_t context_anchor = tipoff->team_context[side].anchor_x_raw_0a;
    /* C37D consumes signed integer coordinate words. Keep subpixel fractions
     * out of both the dead-ball source and the live `$3EEF` layout-4 X. */
    if (nba_gameplay_inbound_target(
            tipoff->inbound_layout_raw, tipoff->dead_ball_x_raw_09b0,
            tipoff->dead_ball_y_raw_09b2, context_anchor,
            fp_integer_word(tipoff->ball.x_fp), &tipoff->rng, &target)) {
        tipoff->inbound_target_x_raw = target.x;
        tipoff->inbound_target_y_raw = target.y;
        tipoff->inbound_direction_raw = target.direction;
        /* `$85:C602-$C65B` also publishes the selected play and request.
         * Keeping an old live play here can select the wrong CFA0 carrier
         * gate before the next inbound formation pass. */
        if (target.play_requested) {
            tipoff->play_code = target.play_code;
            tipoff->play_request_raw = 1u;
        }
        /* `$85:C5AD-$C5BD` installs the boundary target before the later
         * `$85:AD86-$AD95` formation pass deliberately skips this actor. */
        tipoff->actors[tipoff->inbound_actor_raw].target_x = target.x;
        tipoff->actors[tipoff->inbound_actor_raw].target_y = target.y;
    }
    tipoff->inbound_ready_raw = 0u;
    tipoff->inbound_transfer_raw = 0u;
}

/* `$87:9426-$9478`: `$09BC` is a deferred shooting-foul event. Phase 2 in
 * `$0A02` bypasses the live-shot wait; otherwise the ROM waits for `$0948`
 * to clear and for an owner to exist or the loose ball to fall below Z=16. */
static bool cpu_resolve_deferred_shooting_foul(NbaTipoff *tipoff) {
    if (!tipoff ||
        (tipoff->fouls.shooting_foul_raw_09bc & 0x7FFFu) !=
            NBA_GAMEPLAY_FOUL_DEFENSIVE)
        return false;
    bool ready = tipoff->deferred_shot_foul_phase_raw_0a02 == 2u ||
        (tipoff->ball_activity_raw == 0u &&
         (tipoff->possession_actor >= 0 ||
          fp_integer_word(tipoff->ball.z_fp) < 16));
    if (!ready) return false;
    uint16_t deferred = tipoff->fouls.shooting_foul_raw_09bc;
    tipoff->ball_activity_raw = 0u;
    tipoff->fouls.free_throw_state_raw_0978 = 1u;
    tipoff->fouls.free_throw_sequence_raw_097a =
        (deferred & 0x8000u) != 0u ? 1u : 2u;
    tipoff->fouls.foul_event_raw_0964 = deferred & 0x7FFFu;
    return true;
}

static void cpu_dispatch_pending_event(NbaTipoff *tipoff) {
    if (!tipoff || tipoff->dead_ball_dispatch_busy_raw_09b4 != 0u) return;

    /* `$87:92ED-$93D9`: synthesize out-of-bounds code 3 only when no
     * existing foul/free-throw/whistle owns this parent pass. */
    if (tipoff->session->config.rules[2] != 0u &&
        tipoff->fouls.free_throw_state_raw_0978 == 0u &&
        tipoff->fouls.shooting_foul_raw_09bc == 0u &&
        tipoff->fouls.foul_event_raw_0964 == 0u &&
        tipoff->fouls.whistle_active_raw_09b6 == 0u &&
        (tipoff->live_state_raw == 0x82u || tipoff->live_state_raw < 0x80u)) {
        int16_t x = 0, y = 0, vx = 0, vy = 0;
        bool eligible = true;
        bool owned = tipoff->possession_actor >= 0 &&
                     tipoff->possession_actor < NBA_GAMEPLAY_ACTOR_COUNT;
        if (owned) {
            NbaTipoffActor *owner = &tipoff->actors[tipoff->possession_actor];
            if (tipoff->live_state_raw == 0x82u ||
                fp_integer_word(owner->z_fp) != 0) eligible = false;
            else {
                x = fp_integer_word(owner->x_fp);
                y = fp_integer_word(owner->y_fp);
            }
        } else {
            x = fp_integer_word(tipoff->ball.x_fp);
            y = fp_integer_word(tipoff->ball.y_fp);
            vx = tipoff->ball.velocity_x; vy = tipoff->ball.velocity_y;
        }
        int16_t layout = 0;
        if (eligible) {
            /* `$87:9340-$9348/$9391-$93AB`: a grounded owned actor is out
             * based on position alone, including an inward-moving actor.
             * `$87:934A-$938E`: only the ownerless ball tests velocity. An
             * outside X with inward velocity returns immediately; it does
             * not fall through to a second Y-boundary test at a corner. */
            if (x >= 378) {
                if (owned || vx >= 0) layout = 1;
            } else if (x < -378) {
                if (owned || vx < 0) layout = 1;
            } else if (y >= 208) {
                if (owned || vy >= 0) layout = 3;
            } else if (y < -208) {
                if (owned || vy < 0) layout = 3;
            }
        }
        if (layout != 0) {
            /* `$87:93BB/$93BE` clears both axes for either edge. */
            tipoff->ball.velocity_x = tipoff->ball.velocity_y = 0;
            tipoff->fouls.foul_event_raw_0964 = 3u;
            /* $87:93C7-$93D5 preserves the last touching actor for DA8C's
             * possession label. The per-frame collision census is transient. */
            tipoff->hud_event_actor_raw_492d =
                tipoff->team_context[tipoff->camera_side_group_raw / 5u].dead_ball_actor_raw_3f;
            tipoff->inbound_layout_raw = layout;
        }
    }
    (void)cpu_resolve_deferred_shooting_foul(tipoff);
    if (tipoff->fouls.foul_event_raw_0964 == 0u) return;
    uint16_t event = tipoff->fouls.foul_event_raw_0964;
    if (event == NBA_GAMEPLAY_VIOLATION_INTERFERENCE) {
        if (tipoff->session->config.rules[5] == 0u ||
            tipoff->fouls.offender_actor_raw < 0) return;
        uint8_t actor = (uint8_t)tipoff->fouls.offender_actor_raw;
        cpu_begin_dead_ball(tipoff, actor, (uint16_t)((actor / 5u) * 5u),
                            0, true);
    } else if (event == 3u || event == 5u || event == 7u) {
        uint8_t selected = tipoff->possession_actor >= 0 ?
            (uint8_t)tipoff->possession_actor :
            (uint8_t)tipoff->team_context[
                tipoff->camera_side_group_raw == 0u ? 0u : 1u]
                .dead_ball_actor_raw_3f;
        int16_t layout = event == 7u ? 2 : 3;
        if (event == 3u && tipoff->inbound_layout_raw != 0)
            layout = tipoff->inbound_layout_raw;
        cpu_begin_dead_ball(tipoff, selected,
            (uint16_t)(tipoff->camera_side_group_raw ^ 5u), layout, false);
    } else {
        /* `$87:9411-$949E`: ordinary and already-seeded bonus contact uses layout 4
         * (3 when `$0966` is nonzero). Code 1 awards the ball opposite the
         * offender; charging/offensive codes 2/13 use `$093A ^ 5`. */
        if ((event != NBA_GAMEPLAY_FOUL_DEFENSIVE &&
             event != NBA_GAMEPLAY_FOUL_CHARGING &&
             event != NBA_GAMEPLAY_FOUL_OFFENSIVE) ||
            tipoff->fouls.offender_actor_raw < 0) return;
        uint8_t offender = (uint8_t)tipoff->fouls.offender_actor_raw;
        uint16_t side_group = event == NBA_GAMEPLAY_FOUL_DEFENSIVE ?
            (uint16_t)(offender < 5u ? 5u : 0u) :
            (uint16_t)(tipoff->camera_side_group_raw ^ 5u);
        uint8_t selected_actor = tipoff->possession_actor >= 0 &&
                tipoff->possession_actor < NBA_GAMEPLAY_ACTOR_COUNT ?
            (uint8_t)tipoff->possession_actor : offender;
        int16_t layout = tipoff->dead_ball_raw_0966 != 0u ? 3 : 4;
        cpu_begin_dead_ball(tipoff, selected_actor, side_group, layout, false);
    }
}

void nba_tipoff_replay_violation_dispatch(NbaTipoff *tipoff) {
    cpu_dispatch_pending_event(tipoff);
}

/* Binding guards for `$87:931E-$93BE`. The native parent witnesses remain
 * the oracle; these cases ensure production does not merge the owned and
 * ownerless branches or discard the first-axis early return. */
static bool cpu_out_of_bounds_dispatch_self_test(void) {
    static const struct {
        bool owned;
        int16_t x, y, vx, vy, layout;
    } cases[] = {
        {true, 378, 0, -64, 17, 1},
        {true, -379, 0, 64, 17, 1},
        {true, 0, 208, 17, -64, 3},
        {true, 0, -209, 17, 64, 3},
        {false, 378, 0, -64, 17, 0},
        {false, -379, 0, 64, 17, 0},
        {false, 378, 209, -64, 64, 0},
        {false, 378, 209, 0, 64, 1},
        {false, -378, -208, -64, -64, 0},
        {false, -379, -209, -64, -64, 1},
        {false, 0, 208, 64, 0, 3},
        {false, 0, -209, 64, -64, 3}
    };
    NbaSession session;
    memset(&session, 0, sizeof(session));
    session.config.rules[2] = 1u;
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        NbaTipoff state;
        memset(&state, 0, sizeof(state));
        state.session = &session;publish_exhibition_team_ids(&state);
        state.possession_actor = cases[i].owned ? 2 : -1;
        state.ball.owner_actor = state.possession_actor;
        state.team_context[0].dead_ball_actor_raw_3f = 2u;
        state.actors[2].x_fp = state.ball.x_fp = cases[i].x * 256;
        state.actors[2].y_fp = state.ball.y_fp = cases[i].y * 256;
        state.actors[2].velocity_x = state.ball.velocity_x = cases[i].vx;
        state.actors[2].velocity_y = state.ball.velocity_y = cases[i].vy;
        cpu_dispatch_pending_event(&state);
        if (cases[i].layout == 0) {
            if (state.fouls.foul_event_raw_0964 != 0u ||
                state.live_state_raw != 0u ||
                state.ball.velocity_x != cases[i].vx ||
                state.ball.velocity_y != cases[i].vy) return false;
        } else if (state.fouls.foul_event_raw_0964 != 3u ||
                   state.live_state_raw != 0x82u ||
                   state.inbound_layout_raw != cases[i].layout ||
                   state.ball.velocity_x != 0 || state.ball.velocity_y != 0 ||
                   state.dead_ball_x_raw_09b0 != cases[i].x ||
                   state.dead_ball_y_raw_09b2 != cases[i].y) return false;
    }
    return true;
}

static void cpu_process_pending_event(NbaTipoff *tipoff) {
    cpu_dispatch_pending_event(tipoff);
    if (tipoff->fouls.foul_event_raw_0964 == 0u) return;
    /* `$87:95A4-$95AB` seeds the C37D target before the 93F5 foul consumer.
     * Keep that order even though the current consumer does not own the
     * coordinate words; later event branches may make the dependency visible. */
    cpu_finalize_dead_ball_inbound(tipoff);
    if(tipoff->fouls.whistle_active_raw_09b6==0u) {
        /* $85:9437/9440 precede the foul consumer's timer/kind replacement. */
        hud_publish(tipoff,0x87BACBu);
        if(tipoff->fouls.whistle_timer_raw_08de>=0)
            hud_publish(tipoff,0x83EBDBu);
    }
    bool consumed=nba_gameplay_foul_consume_pending(
        &tipoff->fouls, tipoff->camera_side_group_raw,
        &tipoff->rim_raw_13e7, &tipoff->inbound_ready_raw, false);
    if(consumed && tipoff->fouls.latched_event_raw_08f0==3u) {
        /* 93F5 replaces the old overlay sequence. Retire its host diagnostic
         * as well so an unsupported statistics page cannot block this call. */
        tipoff->hud.pending_routine=0u;
        tipoff->hud.unsupported_child_pending=false;
    }
}

static bool cpu_deferred_shooting_foul_self_test(void) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    state.possession_actor = -1;
    state.fouls.shooting_foul_raw_09bc = NBA_GAMEPLAY_FOUL_DEFENSIVE;
    state.deferred_shot_foul_phase_raw_0a02 = 1u;
    state.ball_activity_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state.ball.z_fp = 15 * 256;
    if (cpu_resolve_deferred_shooting_foul(&state)) return false;
    state.ball_activity_raw = 0u;
    state.ball.z_fp = 16 * 256;
    if (cpu_resolve_deferred_shooting_foul(&state)) return false;
    state.ball.z_fp = 15 * 256;
    if (!cpu_resolve_deferred_shooting_foul(&state) ||
        state.fouls.foul_event_raw_0964 != NBA_GAMEPLAY_FOUL_DEFENSIVE ||
        state.fouls.free_throw_state_raw_0978 != 1u ||
        state.fouls.free_throw_sequence_raw_097a != 2u) return false;

    memset(&state, 0, sizeof(state));
    state.possession_actor = -1;
    state.fouls.shooting_foul_raw_09bc =
        0x8000u | NBA_GAMEPLAY_FOUL_DEFENSIVE;
    state.deferred_shot_foul_phase_raw_0a02 = 2u;
    state.ball_activity_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state.ball.z_fp = 88 * 256;
    return cpu_resolve_deferred_shooting_foul(&state) &&
           state.ball_activity_raw == 0u &&
           state.fouls.foul_event_raw_0964 ==
               NBA_GAMEPLAY_FOUL_DEFENSIVE &&
           state.fouls.free_throw_state_raw_0978 == 1u &&
           state.fouls.free_throw_sequence_raw_097a == 1u;
}

static bool cpu_dead_ball_dispatch_self_test(void) {
    NbaTipoff state;
    NbaSession session;
    memset(&state, 0, sizeof(state));
    memset(&session, 0, sizeof(session));
    state.session = &session;publish_exhibition_team_ids(&state);
    state.session->config.rules[5] = 1u;
    state.ball.x_fp = 121 * 256;
    state.ball.y_fp = -19 * 256;
    state.ball.z_fp = 88 * 256;
    state.ball.owner_actor = 8;
    state.ball.state = NBA_BALL_ATTACHED;
    state.possession_actor = 8;
    state.actors[8].control_mode = 11u;
    nba_gameplay_rng_seed(&state.rng, 0x9146u);
    nba_gameplay_foul_init(&state.fouls);
    if (!nba_gameplay_foul_record_violation(
            &state.fouls, NBA_GAMEPLAY_VIOLATION_INTERFERENCE, 8u, 2u))
        return false;
    cpu_process_pending_event(&state);
    bool interference_ok = state.live_state_raw == 0x82u &&
           state.camera_side_group_raw == 5u &&
           state.inbound_state_raw == 5u &&
           state.inbound_actor_raw == 7u &&
           state.team_context[1].dead_ball_actor_raw_3f == 8u &&
           state.inbound_layout_raw == 0 &&
           state.inbound_timer_raw == 300u &&
           state.role_rebuild_raw_09d6 == 300u &&
           state.rim_raw_092c == 0x05A0u &&
           state.shot_clock_mirror_raw_09c6 == 0x05A0u &&
           state.dead_ball_raw_0966 == NBA_GAMEPLAY_UNKNOWN_WORD &&
           state.dead_ball_x_raw_09b0 == 121 &&
           state.dead_ball_y_raw_09b2 == -19 &&
           state.possession_actor == -1 && state.ball.owner_actor == -1 &&
           state.ball.state == NBA_BALL_BOUNCE &&
           state.ball.velocity_x == 0 && state.ball.velocity_y == 0 &&
           state.actors[8].control_mode == 2u &&
           state.fouls.foul_event_raw_0964 == 0u &&
           state.fouls.latched_event_raw_08f0 ==
               NBA_GAMEPLAY_VIOLATION_INTERFERENCE &&
           state.fouls.whistle_active_raw_09b6 == 1u;
    if (!interference_ok) return false;

    NbaTipoff defensive;
    memset(&defensive, 0, sizeof(defensive));
    defensive.session = &session;publish_exhibition_team_ids(&defensive);
    defensive.camera_side_group_raw = 0u;
    defensive.possession_actor = 8;
    defensive.ball.owner_actor = 8;
    defensive.ball.state = NBA_BALL_ATTACHED;
    defensive.actors[8].control_mode = 11u;
    nba_gameplay_rng_seed(&defensive.rng, 0x9146u);
    nba_gameplay_foul_init(&defensive.fouls);
    if (!nba_gameplay_foul_record_contact(
            &defensive.fouls, NBA_GAMEPLAY_FOUL_DEFENSIVE,
            2u, 8u, 0u, false, 0u)) return false;
    cpu_process_pending_event(&defensive);
    bool defensive_ok = defensive.live_state_raw == 0x82u &&
        defensive.inbound_state_raw == 5u &&
        defensive.camera_side_group_raw == 0u &&
        defensive.inbound_actor_raw == 7u &&
        defensive.inbound_layout_raw == 4 &&
        defensive.actors[8].control_mode == 2u &&
        defensive.fouls.latched_event_raw_08f0 ==
            NBA_GAMEPLAY_FOUL_DEFENSIVE &&
        defensive.fouls.whistle_active_raw_09b6 == 1u &&
        defensive.fouls.team_fouls[0] == 1u;
    if (!defensive_ok) return false;

    NbaTipoff charging;
    memset(&charging, 0, sizeof(charging));
    charging.session = &session;publish_exhibition_team_ids(&charging);
    charging.camera_side_group_raw = 0u;
    charging.possession_actor = 2;
    charging.ball.owner_actor = 2;
    charging.ball.state = NBA_BALL_ATTACHED;
    charging.actors[2].control_mode = 11u;
    nba_gameplay_rng_seed(&charging.rng, 0x9146u);
    nba_gameplay_foul_init(&charging.fouls);
    if (!nba_gameplay_foul_record_contact(
            &charging.fouls, NBA_GAMEPLAY_FOUL_CHARGING,
            2u, 7u, 0u, false, 0u)) return false;
    cpu_process_pending_event(&charging);
    bool charging_ok = charging.live_state_raw == 0x82u &&
           charging.camera_side_group_raw == 0u &&
           charging.inbound_state_raw == 5u &&
           charging.inbound_actor_raw == 7u &&
           charging.inbound_layout_raw == 4 &&
           charging.actors[2].control_mode == 2u &&
           charging.fouls.latched_event_raw_08f0 ==
               NBA_GAMEPLAY_FOUL_CHARGING &&
           charging.fouls.whistle_active_raw_09b6 == 1u;
    if (!charging_ok) return false;

    /* `$86:C686-$C68F` seeds the bonus before the ordinary event reaches
     * `$87:9411`. It still runs dead-ball placement and consumes `$0964`;
     * `$87:922E` then diverts actors into the free-throw scene. */
    NbaTipoff bonus;
    memset(&bonus, 0, sizeof(bonus));
    bonus.session = &session;publish_exhibition_team_ids(&bonus);
    bonus.camera_side_group_raw = 0u;
    bonus.possession_actor = 8;
    bonus.ball.owner_actor = 8;
    bonus.actors[8].control_mode = 11u;
    nba_gameplay_rng_seed(&bonus.rng, 0x9146u);
    nba_gameplay_foul_init(&bonus.fouls);
    bonus.fouls.team_fouls[0] = 4u;
    if (!nba_gameplay_foul_record_contact(
            &bonus.fouls, NBA_GAMEPLAY_FOUL_DEFENSIVE,
            2u, 8u, 0u, false, 0u)) return false;
    cpu_process_pending_event(&bonus);
    if (!(bonus.live_state_raw == 0x82u &&
           bonus.camera_side_group_raw == 0u &&
           bonus.inbound_state_raw == 5u &&
           bonus.inbound_actor_raw == 7u &&
           bonus.inbound_layout_raw == 4 &&
           bonus.fouls.free_throw_state_raw_0978 == 1u &&
           bonus.fouls.free_throw_sequence_raw_097a == 2u &&
           bonus.fouls.foul_event_raw_0964 == 0u &&
           bonus.fouls.whistle_active_raw_09b6 == 1u)) return false;

    /* Preserve the fractional-coordinate regression, now with the original
     * C39C dispatcher: layout1 takes C50B's edge target. Only layout4 reads
     * live $3EEF through C477. Its +402 integer word remains reachable from
     * the actor cap+394 through F4F2's +8 edge; rounding to403 would not. */
    NbaTipoff fractional;
    memset(&fractional, 0, sizeof(fractional));
    fractional.session = &session;publish_exhibition_team_ids(&fractional);
    fractional.inbound_state_raw = 5u;
    fractional.inbound_layout_raw = 1;
    fractional.team_context[1].anchor_x_raw_0a = 336;
    fractional.ball.x_fp = 402 * 256 + 255;
    fractional.ball.y_fp = -209 * 256 + 127;
    fractional.dead_ball_x_raw_09b0 = 402;
    fractional.dead_ball_y_raw_09b2 = -209;
    nba_gameplay_rng_seed(&fractional.rng, 0x9146u);
    cpu_finalize_dead_ball_inbound(&fractional);
    if (fractional.inbound_target_x_raw != 394 ||
        fractional.inbound_target_y_raw != -160 ||
        fractional.actors[7].target_x != 394 ||
        fractional.actors[7].target_y != -160) return false;
    fractional.inbound_layout_raw = 4;
    nba_gameplay_rng_seed(&fractional.rng, 0x9146u);
    cpu_finalize_dead_ball_inbound(&fractional);
    return fractional.inbound_target_x_raw == 402 &&
           fractional.inbound_target_y_raw == -224 &&
           fractional.actors[7].target_x == 402 &&
           fractional.actors[7].target_y == -224 &&
           fractional.play_code >= 10u && fractional.play_code <= 13u &&
           fractional.play_request_raw == 1u;
}

/* `$85:A4F2-$A5F1`: low-resource owned-ball substep. This path is reached
 * twice per native driver call, including phase<3's pose-height reset and
 * phase>=3's distinct 3/4 vertical rebound. It never copies actor fractions
 * or integrates/clamps the owned X/Y axes. */
static void cpu_update_attached_ball_substep(NbaTipoff *tipoff) {
    NbaTipoffActor *owner = &tipoff->actors[tipoff->possession_actor];
    NbaTipoffBall *ball = &tipoff->ball;
    int16_t offset_x = 0, offset_y = 0, offset_z = 0;
    (void)actor_ball_attachment_offsets(
        tipoff, owner, &offset_x, &offset_y, &offset_z);
    NbaGameplayAttachedVerticalState vertical = {
        .attachment_state_raw_09f6 = tipoff->attached_ball_state_raw_09f6,
        .dead_ball_raw_0968 = tipoff->dead_ball_raw_0968,
        .velocity_z = ball->velocity_z,
        .z_fraction = (uint16_t)((ball->z_fp & 0xFF) << 8),
        .z = fp_integer_word(ball->z_fp),
        .impact_raw_13e5 = tipoff->rim_impact_raw_13e5,
        .event_bits_raw_13e7 = tipoff->rim_raw_13e7
    };
    /* `$85:A50D-$A516` reads actor +$3A, the descriptor phase that also
     * publishes the hand sprite. The compatibility action counter can
     * restart during a 9/11 pose reversal while +$3A is already in flight;
     * reading it here snaps a falling ball back to the hand's height. */
    if (owner->rom_upper_animation_phase_raw_3a < 3u) {
        if (vertical.attachment_state_raw_09f6 == 0u) {
            vertical.attachment_state_raw_09f6 = 1u;
            vertical.dead_ball_raw_0968 = 0u;
        } else if (vertical.attachment_state_raw_09f6 >= 2u) {
            vertical.attachment_state_raw_09f6 = 3u;
        }
        /* `$85:A518-$A52F` uses B953's offset alone, not actor Z. */
        vertical.z = offset_z;
        vertical.z_fraction = 0u;
        vertical.velocity_z = owner->free_throw_launch_half_raw_a8 != 0u ?
            (int16_t)0xFD80u : (int16_t)0xFDE0u;
    } else {
        nba_gameplay_ball_apply_attached_vertical(&vertical);
    }
    tipoff->attached_ball_state_raw_09f6 = vertical.attachment_state_raw_09f6;
    tipoff->dead_ball_raw_0968 = vertical.dead_ball_raw_0968;
    tipoff->rim_impact_raw_13e5 = vertical.impact_raw_13e5;
    tipoff->rim_raw_13e7 = vertical.event_bits_raw_13e7;
    /* `$85:A59A-$A59D` snapshots the old integer Z before A5AC commits
     * this substep. On return from two substeps, 0924 is the intermediate
     * height, not the driver-entry height or the final height. */
    tipoff->ball_previous_z_raw_0924 = (uint16_t)fp_integer_word(ball->z_fp);
    ball->z_fp = (int32_t)vertical.z * 256 + (vertical.z_fraction >> 8);
    ball->velocity_z = vertical.velocity_z;
    /* `$85:A5C2-$A5C7` publishes the owner's integer X before B832 adds
     * the pose offset. This history is later consumed by special shots. */
    tipoff->shot_previous_actor_x_raw_0922 =
        (uint16_t)fp_integer_word(owner->x_fp);
    ball->x_fp = fp_replace_integer_word(ball->x_fp,
        (int16_t)(fp_integer_word(owner->x_fp) + offset_x));
    ball->y_fp = fp_replace_integer_word(ball->y_fp,
        (int16_t)(fp_integer_word(owner->y_fp) + offset_y));
    ball->velocity_x = ball->velocity_y = 0;
}

static NbaGameplayRimResult cpu_update_live_ball(NbaTipoff *tipoff) {
    /* The same `$87:9B0D` 30-Hz logical pass drives `$85:9ACB+` ball
     * collision/integration. Flight-table durations count these due passes. */
    if ((tipoff->simulation_tick & 1u) != 0u)
        return NBA_GAMEPLAY_RIM_FLIGHT;
    /* $0930 is advanced by the independent $85:EE30 60-Hz writer. */
    /* `$85:9A2C-$9A34`: this counter advances by the live scheduler quantum,
     * once before the two free-ball substeps (not once per substep). */
    if (tipoff->rim_raw_094a != 0u)
        tipoff->rim_raw_094a = (uint16_t)(tipoff->rim_raw_094a + 2u);
    /* Oracle frames 1652..1683 write `$0970` 15..0 once per 30-Hz ball
     * pass. Decrement before contact so a newly installed response remains
     * at 15 for its complete first interval. */
    if (tipoff->rim_raw_0970 != 0u) --tipoff->rim_raw_0970;
    NbaTipoffBall *ball = &tipoff->ball;
    /* `$85:9A37-$9A3A` dispatches from signed ownership `$093E`, not from
     * any ball-record presentation label. An ownerless ball therefore runs
     * the complete `$85:9A6A` two-substep core even when the host still calls
     * its last visual state ATTACHED. For the owned path, `$85:9A3C-$9A42`
     * resolves the exact `$093E` actor; a stale handler or provisional
     * `$0954` must not select the attachment pose. */
    int attached_owner = tipoff->possession_actor;
    if (attached_owner < 0) {
        /* Native has only `$093E`; `owner_actor` and ATTACHED are host-side
         * caches. Synchronize both at this authoritative dispatch boundary
         * so later collision/play routing cannot mistake an ownerless native
         * record for a stale owned ball. BOUNCE is the port's neutral label
         * for the shared free-ball core, not a claimed native state write. */
        ball->owner_actor = -1;
        if (ball->state == NBA_BALL_ATTACHED)
            ball->state = NBA_BALL_BOUNCE;
    }
    if (attached_owner >= 0 && attached_owner < NBA_GAMEPLAY_ACTOR_COUNT &&
        tipoff->actors[attached_owner].upper_animation_resource_raw_2a >= 0xF0u) {
        /* `$85:9A43-$9A67`: the resource gate, not a host ball label, owns
         * this direct return. Modes 15/17 retain the complete ball record;
         * other modes compose integer XYZ while preserving every fraction,
         * velocity, attachment latch and controller assignment. */
        uint8_t mode = tipoff->actors[attached_owner].control_mode;
        if (mode != 15u && mode != 17u) {
            /* `$87:B651-$B654` stores the prior BALL X here, unlike the
             * low-resource A5C7 path's owner X. Mode 15/17 writes neither
             * this history nor 0924, so keep it inside the projection gate. */
            tipoff->shot_previous_actor_x_raw_0922 =
                (uint16_t)fp_integer_word(ball->x_fp);
            ball_position_at_actor(tipoff, (unsigned)attached_owner);
        }
        nba_gameplay_effect_step(
            &tipoff->rim_effect, fp_integer_word(ball->y_fp),
            fp_integer_word(ball->z_fp), ball->velocity_z, 2u);
        return NBA_GAMEPLAY_RIM_FLIGHT;
    }
    {
        NbaGameplayRimResult accumulated = NBA_GAMEPLAY_RIM_FLIGHT;
        /* `$85:9A6A/$A7A1`: DP C6 enters as two and drives two complete
         * ownerless substeps. LOOSE, PASS, SHOT and BOUNCE are host labels only;
         * native gravity/rim/ground physics is shared by every free ball. */
        for (unsigned substep = 0; substep < 2u; ++substep) {
        bool made_response = false;
        NbaGameplayRimResult result = NBA_GAMEPLAY_RIM_FLIGHT;

        /* `$85:9A78-$9AC3`: ownerless descending/grounded records release
         * the activity latch, and an intended receiver or live play clears
         * the dead-ball marker before rim classification. */
        int16_t integer_z = fp_integer_word(ball->z_fp);
        bool ownerless = tipoff->possession_actor < 0;
        if (ownerless &&
            ((ball->velocity_z < 0 && integer_z < 64) || integer_z == 0))
            tipoff->ball_activity_raw = 0u;
        if (ownerless &&
            (tipoff->pass_receiver_raw >= 0 || tipoff->live_state_raw == 1u))
            tipoff->dead_ball_raw_0968 = 0u;
        /* `$85:9A99-$9AA0` bypasses rim/script classification for an owned
         * ball whose integer Z is zero. */
        if (!ownerless && integer_z == 0) {
            cpu_update_attached_ball_substep(tipoff);
            continue;
        }

        /* `$85:A009-$A036`: scripted resolution consumes one table entry
         * per substep, retaining all fractional bytes and velocities. */
        if (tipoff->free_throw_resolution_raw_0972 != 0u) {
            static const int16_t scripted_xy[16][2] = {
                {336,0},{336,0},{332,4},{332,4},{332,5},{332,5},
                {328,2},{328,2},{328,2},{328,2},{332,-1},{332,-1},
                {336,-1},{336,-1},{338,1},{338,1}
            };
            unsigned index = (tipoff->free_throw_resolution_raw_0972 >> 1) & 15u;
            int16_t scripted_x = scripted_xy[index][0];
            if (fp_integer_word(ball->x_fp) < 0) scripted_x = (int16_t)-scripted_x;
            ball->x_fp = fp_replace_integer_word(ball->x_fp, scripted_x);
            ball->y_fp = fp_replace_integer_word(ball->y_fp,
                                                  scripted_xy[index][1]);
            ball->z_fp = fp_replace_integer_word(ball->z_fp, 0x53);
            --tipoff->free_throw_resolution_raw_0972;
            continue;
        }

        /* `$85:9C42-$9C5B`: a descending live shot below rim height ends
         * live-shot classification before ordinary free-ball physics. */
        if (integer_z < 73 && tipoff->live_state_raw == 1u &&
            ball->velocity_z < 0) {
            tipoff->ball_activity_raw = 0u;
            tipoff->rim_raw_094a = 0u;
            tipoff->live_state_raw = 0u;
        }
        /* `$85:9A78-$A656`: the current integer position is tested against
         * the rim first. Any reflected velocity and snapped integer axis are
         * then consumed by gravity/integration on this same logical pass. */
        {
            int hoop_x = basket_x_for_side(tipoff->offense_side);
            int hoop_y = basket_y_for_side(tipoff->offense_side);
            bool right_basket = tipoff->offense_side != 0u;
            int16_t before_x = fp_integer_word(ball->x_fp);
            int16_t before_y = fp_integer_word(ball->y_fp);
            int16_t before_z = fp_integer_word(ball->z_fp);
            NbaGameplayRimState rim = {
                before_x, before_y, before_z,
                ball->velocity_x, ball->velocity_y, ball->velocity_z,
                tipoff->rim_raw_092c, tipoff->rim_raw_0962,
                tipoff->rim_raw_096a, tipoff->rim_raw_097c,
                tipoff->rim_raw_096e, tipoff->rim_raw_13e7
            };
            /* `$85:9D65-$9D7B` compares the selected side context-anchor X
             * sign with the ball X sign; it is not an owner/team test. */
            bool correct_side = tipoff->offense_side != 0u ?
                                before_x >= 0 : before_x < 0;
            result = nba_gameplay_rim_world_step(
                &rim, (int16_t)hoop_x, (int16_t)hoop_y,
                tipoff->offense_side != 0u, tipoff->live_state_raw,
                tipoff->rim_force_raw_1866 != 0u,
                tipoff->shot_inner_veto_raw, correct_side);
            NbaGameplayRimContext rim_context = {
                .raw_0920 = tipoff->rim_raw_0920,
                .raw_0936 = tipoff->live_state_raw,
                .raw_0948 = tipoff->ball_activity_raw,
                .raw_094a = tipoff->rim_raw_094a,
                .raw_0970 = tipoff->rim_raw_0970,
                .raw_0978 = tipoff->fouls.free_throw_state_raw_0978,
                .raw_09f8 = tipoff->shot_inner_veto_raw ? 1u : 0u,
                .raw_09b8 = tipoff->inbound_transfer_raw,
                .raw_1866 = tipoff->rim_force_raw_1866,
                .raw_07f6 = tipoff->rng.state,
                .effect_raw_401b = tipoff->rim_effect.effect_raw_401b
            };
            nba_gameplay_rim_apply_inner_response(
                &rim, result, &rim_context, &tipoff->rng);
            if (result == NBA_GAMEPLAY_RIM_MAKE) {
                /* `$85:A079-$A345` runs inline in the detecting substep;
                 * substep two therefore observes live state `$82`. */
                score_made_basket(tipoff);
                rim_context.raw_0936 = tipoff->live_state_raw;
                rim_context.raw_0948 = tipoff->ball_activity_raw;
                rim_context.raw_094a = tipoff->rim_raw_094a;
                rim_context.raw_09b8 = tipoff->inbound_transfer_raw;
                nba_gameplay_rim_apply_made_response(
                    &rim, right_basket, &rim_context);
                /* Score bookkeeping at `$85:A33C-$A342` occurs before the
                 * common physics tail. Keep those freshly raised bits when
                 * the local rim snapshot is copied back below. */
                rim.raw_13e7 = tipoff->rim_raw_13e7;
                made_response = true;
            }
            tipoff->rim_raw_092c = rim.raw_092c;
            tipoff->rim_raw_0962 = rim.raw_0962;
            tipoff->rim_raw_096a = rim.raw_096a;
            tipoff->rim_raw_097c = rim.raw_097c;
            tipoff->rim_raw_096e = rim.raw_096e;
            tipoff->rim_raw_13e7 = rim.raw_13e7;
            tipoff->rim_raw_0920 = rim_context.raw_0920;
            tipoff->live_state_raw = rim_context.raw_0936;
            tipoff->ball_activity_raw = rim_context.raw_0948;
            tipoff->rim_raw_094a = rim_context.raw_094a;
            tipoff->rim_raw_0970 = rim_context.raw_0970;
            tipoff->shot_inner_veto_raw = rim_context.raw_09f8 != 0u;
            tipoff->inbound_transfer_raw = rim_context.raw_09b8;
            if (result == NBA_GAMEPLAY_RIM_MISS)
                (void)nba_gameplay_effect_start(&tipoff->rim_effect, 3u);
            if (result == NBA_GAMEPLAY_RIM_OUTER_CONTACT ||
                result == NBA_GAMEPLAY_RIM_EDGE_CONTACT ||
                result == NBA_GAMEPLAY_RIM_MISS ||
                result == NBA_GAMEPLAY_RIM_MAKE) {
                /* The ROM writes only axes touched by the collision branch;
                 * retain each independent fractional byte. */
                if (result == NBA_GAMEPLAY_RIM_MAKE) {
                    /* `$85:A34A-$A3B3` writes integer X/Y only. */
                    ball->x_fp = fp_replace_integer_word(ball->x_fp, rim.x);
                    ball->y_fp = fp_replace_integer_word(ball->y_fp, rim.y);
                } else if (rim.x != before_x)
                    ball->x_fp = fp_replace_integer_word(ball->x_fp, rim.x);
                if (result != NBA_GAMEPLAY_RIM_MAKE && rim.y != before_y)
                    ball->y_fp = fp_replace_integer_word(ball->y_fp, rim.y);
                if (result != NBA_GAMEPLAY_RIM_MAKE && rim.z != before_z)
                    ball->z_fp = fp_replace_integer_word(ball->z_fp, rim.z);
                ball->velocity_x = rim.velocity_x;
                ball->velocity_y = rim.velocity_y;
                ball->velocity_z = rim.velocity_z;
            }
        }
        if (tipoff->possession_actor >= 0) {
            /* `$85:A3C8-$A3CD` rereads ownership after rim/score response. */
            cpu_update_attached_ball_substep(tipoff);
            continue;
        }
        /* `$85:A3D7-$A3DD` applies gravity on every ownerless-tail pass,
         * including the detecting made-basket substep. */
        ball->velocity_z = (int16_t)(ball->velocity_z - 0x18);
        ball->x_fp += ball->velocity_x;
        ball->z_fp += ball->velocity_z;
        if (nba_gameplay_court_finish_y_step(
                &ball->x_fp, &ball->y_fp,
                &ball->velocity_x, &ball->velocity_y))
            cpu_cancel_rom_pass_activity(tipoff);
        if (ball->z_fp < 0) {
            ball->z_fp = 0;
            {
                /* `$85:A3B7-$A4DA`: gravity precedes integration; ground
                 * impact applies 7/8 vertical restitution (cap $0400) and
                 * impact-only 15/16 lateral damping. */
                NbaGameplayRimState impact = {
                    fp_integer_word(ball->x_fp), fp_integer_word(ball->y_fp),
                    0, ball->velocity_x, ball->velocity_y, ball->velocity_z,
                    0u, 0u, 0u, tipoff->rim_raw_097c, 0u,
                    tipoff->rim_raw_13e7
                };
                nba_gameplay_ball_apply_ground_impact(
                    &impact, &tipoff->rim_impact_raw_13e5);
                ball->velocity_x = impact.velocity_x;
                ball->velocity_y = impact.velocity_y;
                ball->velocity_z = impact.velocity_z;
                tipoff->rim_raw_13e7 = impact.raw_13e7;
                ball->state = NBA_BALL_BOUNCE;
                NbaGameplayRimState settle = {
                    fp_integer_word(ball->x_fp), fp_integer_word(ball->y_fp),
                    fp_integer_word(ball->z_fp), ball->velocity_x,
                    ball->velocity_y, ball->velocity_z, 0u, 0u, 0u,
                    tipoff->rim_raw_097c, 0u, 0u
                };
                NbaGameplaySettleContext settle_context = {
                    .raw_0936 = tipoff->live_state_raw,
                    .raw_0942 = (uint16_t)tipoff->pass_actor_raw,
                    .raw_0944 = (uint16_t)tipoff->pass_aux_raw,
                    .raw_0946 = (uint16_t)tipoff->pass_receiver_raw,
                    .raw_0948 = tipoff->ball_activity_raw,
                    .raw_094a = tipoff->rim_raw_094a,
                    .raw_0978 = tipoff->fouls.free_throw_state_raw_0978,
                    .raw_09b8 = tipoff->inbound_transfer_raw
                };
                if (nba_gameplay_ball_apply_settle(
                        &settle, &settle_context)) {
                    ball->velocity_x = settle.velocity_x;
                    ball->velocity_y = settle.velocity_y;
                    ball->velocity_z = settle.velocity_z;
                    tipoff->rim_raw_097c = settle.raw_097c;
                    tipoff->live_state_raw = settle_context.raw_0936;
                    tipoff->pass_actor_raw =
                        (int16_t)settle_context.raw_0942;
                    tipoff->pass_aux_raw =
                        (int16_t)settle_context.raw_0944;
                    tipoff->pass_receiver_raw =
                        (int16_t)settle_context.raw_0946;
                    tipoff->ball_activity_raw = settle_context.raw_0948;
                    tipoff->rim_raw_094a = settle_context.raw_094a;
                    tipoff->inbound_transfer_raw = settle_context.raw_09b8;
                }
            }
        }
        /* Preserve the first terminal/inner result when substep two merely
         * flies, but allow a later inner result to supersede outer contact. */
        if (result == NBA_GAMEPLAY_RIM_MAKE ||
            (accumulated != NBA_GAMEPLAY_RIM_MAKE &&
             result != NBA_GAMEPLAY_RIM_FLIGHT &&
             (accumulated == NBA_GAMEPLAY_RIM_FLIGHT ||
              result != NBA_GAMEPLAY_RIM_OUTER_CONTACT)))
            accumulated = result;
        }
        /* `$85:A7A8-$A7B5`: all low-resource owned balls, and free balls
         * below 56, clear the rim latch after both substeps. */
        if (tipoff->possession_actor >= 0 || fp_integer_word(ball->z_fp) < 56)
            tipoff->rim_raw_0962 = 0u;
        /* `$87:8F95-$8FA9` schedules `$85:9A24` ball physics before
         * `$87:AA02`, so a miss-started effect receives its first dt=2 step
         * on this same logical pass. */
        nba_gameplay_effect_step(
            &tipoff->rim_effect, fp_integer_word(ball->y_fp),
            fp_integer_word(ball->z_fp), ball->velocity_z, 2u);
        return accumulated;
    }
}

NbaGameplayRimResult nba_tipoff_replay_ball_driver_entry(NbaTipoff *tipoff) {
    if (!tipoff) return NBA_GAMEPLAY_RIM_FLIGHT;
    /* This boundary starts at `$85:9A37`, after the native 094A prefix and
     * the caller's `$87:8EDA-$8EDF` 0970 decrement. */
    if (tipoff->rim_raw_094a != 0u)
        tipoff->rim_raw_094a = (uint16_t)(tipoff->rim_raw_094a - 2u);
    if (tipoff->rim_raw_0970 != 0u) ++tipoff->rim_raw_0970;
    tipoff->simulation_tick &= ~1u;
    return cpu_update_live_ball(tipoff);
}

NbaGameplayRimResult nba_tipoff_replay_ownerless_ball_entry(
        NbaTipoff *tipoff) {
    if (!tipoff) return NBA_GAMEPLAY_RIM_FLIGHT;
    /* The capture enters at `$85:9A6A`, after the wrapper's once-per-pass
     * counters. Back the represented counters up so the production driver
     * reaches that exact state before executing the ownerless core. */
    if (tipoff->free_throw_flight_timer_raw_0930 != 0u)
        ++tipoff->free_throw_flight_timer_raw_0930;
    if (tipoff->rim_raw_094a != 0u)
        tipoff->rim_raw_094a = (uint16_t)(tipoff->rim_raw_094a - 2u);
    if (tipoff->rim_raw_0970 != 0u) ++tipoff->rim_raw_0970;
    tipoff->simulation_tick &= ~1u;
    tipoff->possession_actor = -1;
    tipoff->ball.owner_actor = -1;
    if (tipoff->ball.state == NBA_BALL_ATTACHED ||
        tipoff->ball.state == NBA_BALL_HIDDEN ||
        tipoff->ball.state == NBA_BALL_TOSS)
        tipoff->ball.state = NBA_BALL_BOUNCE;
    return cpu_update_live_ball(tipoff);
}

/* Contact-tick regression for `$85:9A78-$A656`: the negative Y lip reflects
 * velocity before this pass integrates it, while all three fractional bytes
 * survive. This deliberately exercises the live orchestration rather than
 * only the isolated rim classifier. */
static bool cpu_rim_contact_tick_self_test(void) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    nba_gameplay_effect_init(&state.rim_effect);
    state.offense_side = 1u;
    state.handler_actor = 5u;
    state.possession_actor = -1;
    state.live_state_raw = 1u;
    state.rim_raw_096e = 7u;
    state.rim_raw_0970 = 4u;
    state.ball.state = NBA_BALL_SHOT;
    state.ball.x_fp = 344 * 256 + 128;
    state.ball.y_fp = -24 * 256 + 64;
    state.ball.z_fp = 80 * 256 + 96;
    state.ball.velocity_x = 100;
    state.ball.velocity_y = 31;
    state.ball.velocity_z = 150;
    NbaGameplayRimResult result = cpu_update_live_ball(&state);
    return result == NBA_GAMEPLAY_RIM_OUTER_CONTACT &&
           state.ball.x_fp == 345 * 256 + 72 &&
           state.ball.y_fp == -27 * 256 + 34 &&
           state.ball.z_fp == 81 * 256 + 68 &&
           state.ball.velocity_x == 100 &&
           state.ball.velocity_y == -15 &&
           state.ball.velocity_z == 102 &&
           state.rim_raw_096e == 7u &&
           state.rim_raw_0970 == 3u &&
           (state.rim_raw_13e7 & 0x0008u) == 0u;
}

/* Recomp `$85:9A24-$A7B5` orchestration vectors. These lock behavior that
 * cannot be proven by the isolated rim helpers: two substeps, inline score,
 * scripted `$0972`, preliminary live-latch clears, and the final tail. */
static bool cpu_ball_substep_self_test(void) {
    NbaSession session;
    NbaTipoff state;

    /* `$85:9A37` sends negative `$093E` straight to `$85:9A6A`, regardless
     * of the host's last visual ball label. Natural native calls 303 and 333
     * in `.analysis/func-vectors-ownerless-ball-20260826` exercise this
     * state during `$0936=$82`. Starting at Z=33 with VZ=348, two gravity
     * substeps finish at exact fixed Z=9072 and VZ=300; the stale handler's
     * pose must not capture the ball. */
    NbaTipoff ownerless_attached;
    memset(&ownerless_attached, 0, sizeof(ownerless_attached));
    memset(&session, 0, sizeof(session));
    ownerless_attached.session = &session;publish_exhibition_team_ids(&ownerless_attached);
    ownerless_attached.possession_actor = -1;
    ownerless_attached.ball.owner_actor = 7; /* stale host cache, not `$093E` */
    ownerless_attached.ball.state = NBA_BALL_ATTACHED;
    ownerless_attached.ball.x_fp = 39 * 256;
    ownerless_attached.ball.y_fp = -165 * 256;
    ownerless_attached.ball.z_fp = 33 * 256;
    ownerless_attached.ball.velocity_z = 348;
    ownerless_attached.handler_actor = 7u;
    ownerless_attached.inbound_actor_raw = 2u;
    ownerless_attached.live_state_raw = 0x82u;
    ownerless_attached.pass_actor_raw = -1;
    ownerless_attached.pass_aux_raw = -1;
    ownerless_attached.pass_receiver_raw = -1;
    ownerless_attached.actors[7].x_fp = 300 * 256;
    ownerless_attached.actors[7].y_fp = 100 * 256;
    (void)cpu_update_live_ball(&ownerless_attached);
    if (ownerless_attached.possession_actor != -1 ||
        ownerless_attached.ball.owner_actor != -1 ||
        ownerless_attached.ball.state != NBA_BALL_BOUNCE ||
        ownerless_attached.ball.x_fp != 39 * 256 ||
        ownerless_attached.ball.y_fp != -165 * 256 ||
        ownerless_attached.ball.z_fp != 9072 ||
        ownerless_attached.ball.velocity_z != 300)
        return false;

    /* A host LOOSE label must not freeze an ownerless ball above a waiting
     * inbounder. $85:9A6A runs the same substeps as PASS/SHOT/BOUNCE. */
    const uint8_t free_modes[]={NBA_BALL_LOOSE,NBA_BALL_PASS,NBA_BALL_SHOT,NBA_BALL_BOUNCE};
    int32_t expected_z=0;int16_t expected_vz=0;
    for(unsigned i=0;i<sizeof(free_modes);++i) {
        memset(&state,0,sizeof(state));memset(&session,0,sizeof(session));state.session=&session;publish_exhibition_team_ids(&state);
        state.ball.owner_actor=-1;state.possession_actor=-1;state.pass_receiver_raw=-1;
        state.ball.state=free_modes[i];state.ball.z_fp=63*256;state.live_state_raw=0x82;
        state.fouls.whistle_active_raw_09b6=1;
        (void)cpu_update_live_ball(&state);
        if(i==0){expected_z=state.ball.z_fp;expected_vz=state.ball.velocity_z;}
        if(state.ball.z_fp>=63*256 || state.ball.z_fp!=expected_z || state.ball.velocity_z!=expected_vz)return false;
    }

    memset(&session, 0, sizeof(session));
    memset(&state, 0, sizeof(state));
    state.session = &session;publish_exhibition_team_ids(&state);
    state.possession_actor = -1;
    state.ball.owner_actor = -1;
    state.ball.state = NBA_BALL_SHOT;
    state.offense_side = 1u;
    state.live_state_raw = 1u;
    state.shot_value_raw = 2u;
    nba_gameplay_effect_init(&state.rim_effect);
    state.rim_effect.effect_raw_401b = 4u;
    state.ball.x_fp = 336 * 256;
    state.ball.z_fp = 83 * 256;
    NbaGameplayRimResult result = cpu_update_live_ball(&state);
    if (result != NBA_GAMEPLAY_RIM_MAKE || session.score[1] != 2u ||
        state.live_state_raw != 0x82u || state.shot_value_raw != 0u ||
        state.ball.x_fp != 336 * 256 || state.ball.y_fp != 0 ||
        state.ball.z_fp != 82 * 256 + 205 ||
        state.ball.velocity_z != -27 ||
        (state.rim_raw_13e7 & 0x0004u) == 0u) return false;

    memset(&state, 0, sizeof(state));
    state.possession_actor = -1;
    state.ball.owner_actor = -1;
    state.ball.state = NBA_BALL_BOUNCE;
    state.ball.z_fp = 0x0032;
    state.rim_raw_0962 = 0x05A0u;
    (void)cpu_update_live_ball(&state);
    if (state.ball.z_fp != 0 || state.ball.velocity_z != 42 ||
        state.rim_impact_raw_13e5 != 42u ||
        (state.rim_raw_13e7 & 1u) != 0u || state.rim_raw_0962 != 0u)
        return false;

    memset(&state, 0, sizeof(state));
    state.possession_actor = -1;
    state.ball.owner_actor = -1;
    state.ball.state = NBA_BALL_SHOT;
    state.live_state_raw = 1u;
    state.ball_activity_raw = 5u;
    state.rim_raw_094a = 7u;
    state.ball.z_fp = 72 * 256;
    state.ball.velocity_z = -1;
    (void)cpu_update_live_ball(&state);
    if (state.live_state_raw != 0u || state.ball_activity_raw != 0u ||
        state.rim_raw_094a != 0u) return false;

    memset(&state, 0, sizeof(state));
    state.possession_actor = -1;
    state.ball.owner_actor = -1;
    state.ball.state = NBA_BALL_BOUNCE;
    state.ball.z_fp = 55 * 256;
    state.rim_raw_0962 = 0x05A0u;
    (void)cpu_update_live_ball(&state);
    if (state.rim_raw_0962 != 0u) return false;

    memset(&state, 0, sizeof(state));
    state.possession_actor = -1;
    state.ball.owner_actor = -1;
    state.ball.state = NBA_BALL_SHOT;
    state.free_throw_resolution_raw_0972 = 4u;
    state.ball.x_fp = -100 * 256 + 0x55;
    state.ball.y_fp = 9 * 256 + 0x66;
    state.ball.z_fp = 2 * 256 + 0x77;
    state.ball.velocity_x = 11;
    state.ball.velocity_y = -12;
    state.ball.velocity_z = 13;
    (void)cpu_update_live_ball(&state);
    return state.free_throw_resolution_raw_0972 == 2u &&
           state.ball.x_fp == -336 * 256 + 0x55 &&
           state.ball.y_fp == 0 * 256 + 0x66 &&
           state.ball.z_fp == 83 * 256 + 0x77 &&
           state.ball.velocity_x == 11 && state.ball.velocity_y == -12 &&
           state.ball.velocity_z == 13;
}

static void cpu_update_tip_ball(NbaTipoff *tipoff) {
    /* 859A24: the countdown gates the ordinary ownerless driver. */
    if((int16_t)tipoff->tip_toss_countdown_raw_09f2>=0)return;
    tipoff->ball.state=NBA_BALL_TOSS;
    (void)cpu_update_live_ball(tipoff);
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
    if ((uint16_t)tipoff->play_step_raw >= 2u &&
        (tipoff->rng.state & 1u) != 0u &&
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
    /* `$85:B377-$B3A9` does not touch `$09A2`; an active cutter survives a
     * stream rewind, including the made-score request branch. */
    tipoff->play_mirror_raw = tipoff->play_code >= 0x12u ?
        (tipoff->rng.state & 1u) : 0u;
    for (unsigned i = 0; i < 3u; ++i) tipoff->play_selector_raw[i] = -1;
    cpu_advance_play_control(tipoff);
}

static void cpu_reselect_play_control(NbaTipoff *tipoff) {
    ++tipoff->play_consumed_serial;
    /* `$85:B128-$B24B`: `$0994` is consumed only at the completed logical
     * pass boundary. The first RNG result updates the opposite/defending
     * team context at `$46EB/$476B +$30`. */
    uint16_t context_random = nba_gameplay_rng_next(&tipoff->rng);
    unsigned offense = tipoff->offense_side != 0u ? 1u : 0u;
    unsigned defense = offense ^ 1u;
    (void)nba_gameplay_defense_context_reselect(
        tipoff->session->score[offense], tipoff->session->score[defense],
        tipoff->period_raw_0926,
        tipoff->team_context[defense].activity_raw_39, context_random,
        &tipoff->team_context[defense].mode_raw_30);
    tipoff->play_request_raw = 0u;
    tipoff->play_event_wait_raw = 0u;
    tipoff->play_cycle_raw = 0u;
    if (tipoff->live_state_raw == 0x82u) {
        /* `$85:B176/$B245`: made-score state preserves `$0996` and `$09D0`
         * while B377 rewinds the selected stream to record zero. */
        cpu_reset_play_control(tipoff);
        return;
    }

    /* Complete `$85:B18A-$B244`. +$2E values below seven directly select a
     * range. Seven invokes the score/RNG/team table; preserving every rejected
     * LFSR result is essential because this stream is shared with gameplay. */
    NbaGameplayTeamContext *context = &tipoff->team_context[offense];
    uint8_t strategy = (uint8_t)context->strategy_raw_2e;
    uint8_t base = 0u, count = 0u;
    bool hold = false;
    bool range_from_asset = false;
    if (strategy == 7u) {
        int16_t difference = (int16_t)(uint16_t)(
            tipoff->session->score[offense] - tipoff->session->score[defense]);
        if (difference < 0 &&
            (difference == -3 || (nba_gameplay_rng_next(&tipoff->rng) & 3u) == 0u)) {
            strategy = 5u;
        } else if ((nba_gameplay_rng_next(&tipoff->rng) & 7u) == 0u) {
            do strategy = (uint8_t)(nba_gameplay_rng_next(&tipoff->rng) & 7u);
            while (strategy >= 6u);
        } else {
            uint8_t coin = (uint8_t)(nba_gameplay_rng_next(&tipoff->rng) & 1u);
            if (!nba_assets_gameplay_cpu_strategy(
                    tipoff->assets, (uint8_t)context->strategy_team_raw_00,
                    coin, &strategy, &base, &count, &hold))
                return;
            range_from_asset = true;
        }
    }
    if (!range_from_asset) {
        static const uint8_t range_base[7] = {29, 24, 18, 44, 39, 35, 51};
        static const uint8_t range_count[7] = {6, 5, 6, 7, 5, 4, 5};
        if (strategy >= 7u) return;
        base = range_base[strategy];
        count = range_count[strategy];
        hold = strategy == 5u;
    }
    uint8_t offset;
    if (context->strategy_raw_2e != 7u && context->play_selection_raw_56 != 0u) {
        offset = (uint8_t)context->play_selection_raw_56;
    } else {
        do offset = (uint8_t)(nba_gameplay_rng_next(&tipoff->rng) & 7u);
        while (offset >= count);
    }
    if (strategy == 0x33u && (nba_gameplay_rng_next(&tipoff->rng) & 1u) == 0u)
        offset = (uint8_t)(offset + 5u);
    tipoff->play_code = (uint16_t)(base + offset);
    tipoff->play_hold_raw = hold ? 1u : 0u;
    cpu_reset_play_control(tipoff);
}

void nba_tipoff_update_play_control_end_frame(NbaTipoff *tipoff) {
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
                /* `$85:B25E-$B288/$B353`: DP $AA initially holds the
                 * countdown underflow, then becomes the descending actor
                 * loop counter.  The shared exit stores that live scratch
                 * value, not the original underflow. */
                tipoff->play_countdown_raw = (int16_t)(4u - i);
                return;
            }
        }
        tipoff->play_event_wait_raw = 0u; /* `$85:B288` */
    }
    cpu_advance_play_control(tipoff);
}

/* `$86:BAA2-$BAFA`, stopping immediately before `$86:BAFD`: common
 * player-grab prefix shared by the opening tip, pass catches, and rebounds.
 * Live Mesen vectors prove both movement branches: a catcher below `$0080`
 * loses lateral velocity, while a faster catcher retains it verbatim. */
static void cpu_apply_catch_prefix(NbaTipoff *tipoff, uint8_t catcher) {
    NbaTipoffActor *actor = &tipoff->actors[catcher];
    unsigned side = catcher / 5u;
    NbaGameplayTeamContext *context = &tipoff->team_context[side];
    NbaGameplayCatchPrefixState prefix = {
        .catcher = catcher,
        .controller_actor = actor->controller_assignment_raw,
        .velocity_x = actor->velocity_x,
        .velocity_y = actor->velocity_y,
        .movement_magnitude = actor->movement_magnitude_raw,
        .catcher_latch = actor->catcher_latch_raw_ae,
        .rim_force_raw_1866 = tipoff->rim_force_raw_1866,
        .dead_ball_raw_0968 = tipoff->dead_ball_raw_0968,
        .rim_raw_096a = tipoff->rim_raw_096a,
        .context_actor_raw_3f = context->dead_ball_actor_raw_3f,
        .context_controller_raw_41 = context->controller_actor_raw_41,
        .context_previous_actor_raw_43 =
            context->previous_dead_ball_actor_raw_43,
        .context_previous_controller_raw_45 =
            context->previous_controller_actor_raw_45,
        .special_actor_raw_09a2 = (int16_t)tipoff->special_actor_raw,
        .play_aux_raw_09a6 = tipoff->play_aux_selector_raw_09a6,
        .play_selector_raw = {
            tipoff->play_selector_raw[0], tipoff->play_selector_raw[1],
            tipoff->play_selector_raw[2]
        },
        .possession_actor_raw_093e = tipoff->possession_actor,
        .actor_record_raw_0910 = tipoff->catch_actor_record_raw_0910,
        .context_record_raw_0912 = tipoff->catch_context_record_raw_0912,
    };
    nba_gameplay_apply_catch_prefix(&prefix);
    actor->velocity_x = prefix.velocity_x;
    actor->velocity_y = prefix.velocity_y;
    actor->movement_magnitude_raw = prefix.movement_magnitude;
    actor->catcher_latch_raw_ae = prefix.catcher_latch;
    tipoff->rim_force_raw_1866 = prefix.rim_force_raw_1866;
    tipoff->dead_ball_raw_0968 = prefix.dead_ball_raw_0968;
    tipoff->rim_raw_096a = prefix.rim_raw_096a;
    context->dead_ball_actor_raw_3f = prefix.context_actor_raw_3f;
    context->controller_actor_raw_41 = prefix.context_controller_raw_41;
    context->previous_dead_ball_actor_raw_43 =
        prefix.context_previous_actor_raw_43;
    context->previous_controller_actor_raw_45 =
        prefix.context_previous_controller_raw_45;
    tipoff->special_actor_raw = (uint16_t)prefix.special_actor_raw_09a2;
    tipoff->play_aux_selector_raw_09a6 = prefix.play_aux_raw_09a6;
    for (unsigned i = 0; i < 3u; ++i)
        tipoff->play_selector_raw[i] = prefix.play_selector_raw[i];
    tipoff->possession_actor = (int8_t)prefix.possession_actor_raw_093e;
    tipoff->catch_actor_record_raw_0910 = prefix.actor_record_raw_0910;
    tipoff->catch_context_record_raw_0912 = prefix.context_record_raw_0912;
}

static void cpu_apply_catch_mode(NbaTipoff *tipoff, uint8_t catcher) {
    NbaTipoffActor *actor = &tipoff->actors[catcher];
    NbaGameplayTeamContext *context = &tipoff->team_context[catcher / 5u];
    uint16_t mode = actor->control_mode;
    uint16_t timer = actor->reaction_threshold;
    uint16_t flags = actor->behavior_flags_raw;
    nba_gameplay_apply_catch_mode(
        tipoff->match_clock_raw_0928, &context->match_clock_raw_47,
        &mode, &timer, &flags);
    actor->control_mode = (uint8_t)mode;
    actor->contact_action_timer_raw_60 = timer;
    actor->reaction_threshold = timer;
    actor->behavior_flags_raw = flags;
}


static bool cpu_inbound_completion_witness(const NbaTipoff *tipoff) {
    return tipoff->live_state_raw == 0x82u &&
        (tipoff->inbound_transfer_raw != 0u ||
         tipoff->pass_receiver_raw >= 0);
}

static bool cpu_inbound_completion_witness_self_test(void) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    state.live_state_raw = 0x82u;
    state.pass_receiver_raw = -1;
    if (cpu_inbound_completion_witness(&state)) return false;
    state.inbound_transfer_raw = 1u;
    if (!cpu_inbound_completion_witness(&state)) return false;
    state.inbound_transfer_raw = 0u;
    state.pass_receiver_raw = 4;
    if (!cpu_inbound_completion_witness(&state)) return false;
    state.live_state_raw = 0u;
    return !cpu_inbound_completion_witness(&state);
}

static void cpu_apply_ball_acquisition_core(NbaTipoff *tipoff,
                                            uint8_t catcher) {
    /* `$86:BAA2-$BC99` is shared by pass catches and loose rebounds. It
     * installs the collision winner without mass-resetting the other nine
     * actor modes, clears the old play opportunities, and requests the next
     * ROM strategy through `$0994`. */
    unsigned side = catcher / 5u;
    uint16_t side_group = side ? 5u : 0u;
    bool side_change = (int16_t)tipoff->dead_ball_raw_097e >= 0 ?
        tipoff->dead_ball_raw_097e != side_group :
        tipoff->camera_side_group_raw != side_group;
    NbaTipoffActor *catcher_state = &tipoff->actors[catcher];
    cpu_apply_catch_prefix(tipoff, catcher);
    cpu_apply_catch_mode(tipoff, catcher);
    bool special_finish = catcher_state->control_mode == 14u;
    if (side_change) {
        ++tipoff->possession_number;
        /* `$86:BB3B-$BB6E` restores all actor +$74 assignments from +$76
         * and raises `$09D6`; the end-frame AF5C/BC07 pair owns modes. */
        for (unsigned actor = 0;
             actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
            NbaTipoffActor *state = &tipoff->actors[actor];
            state->assignment_current_raw = state->assignment_base_raw;
            state->assignment_actor =
                (uint8_t)(state->assignment_current_raw >> 1);
        }
    }
    /* `$86:BB17-$BB81` reaches the rebuild/clock block only when the
     * catcher changes the active side (or completes the live `$82`
     * inbound). Ordinary same-side passes preserve both clocks and the
     * existing assignments. */
    if (side_change) {
        tipoff->role_rebuild_raw_09d6 = NBA_GAMEPLAY_UNKNOWN_WORD;
        tipoff->play_request_raw = 1u;
        tipoff->rim_raw_092c = 0x05A0u;
        tipoff->free_throw_flight_timer_raw_0930 = 0x0258u;
        tipoff->dead_ball_raw_096c = 0u;
        tipoff->team_context[side].previous_dead_ball_actor_raw_43 =
            NBA_GAMEPLAY_UNKNOWN_WORD;
        tipoff->team_context[side].previous_controller_actor_raw_45 = -1;
    }
    tipoff->offense_side = (uint8_t)side;
    tipoff->possession_team = (int8_t)side;
    tipoff->camera_side_group_raw = (uint8_t)side_group;
    tipoff->possession_frame = 0u;
    tipoff->play_state_frame = 0u;
    tipoff->handler_actor = catcher;
    tipoff->receiver_actor = catcher;
    /* BAA2 installs `$093E` but does not rewrite the ball record. The later
     * common attached-ball tail performs that projection. Preserve the
     * collision-frame position and velocity here while exposing ownership
     * to the host state machine. */
    tipoff->ball.owner_actor = (int8_t)catcher;
    tipoff->ball.state = NBA_BALL_ATTACHED;
    tipoff->cpu_play_state = special_finish ?
        NBA_CPU_PLAY_SHOT : NBA_CPU_PLAY_ATTACK;
    tipoff->shot_result_resolved = false;
    tipoff->shot_inner_veto_raw = false;
    tipoff->shot_miss_index_raw = 0xFFu;
    /* `$86:BC81-$BC90` runs after rebound/stat classification. In particular,
     * BC8A clears the rim context which otherwise traps formation roles 3/4
     * in `$85:AF2A` on the next possession. `$096E` is a separate cooldown
     * and is deliberately preserved. Owner installation makes `$85:A7A8`
     * clear `$0962` at the following common ball tail. */
    tipoff->ball_activity_raw = 0u;
    tipoff->rim_raw_094a = 0u;
    tipoff->shot_value_raw = 0u;
    tipoff->free_throw_resolution_raw_0972 = 0u;
    tipoff->rim_raw_096a = 0u;
    tipoff->rim_raw_097c = 0u;
    tipoff->dead_ball_raw_097e = NBA_GAMEPLAY_UNKNOWN_WORD;
}

static void cpu_commit_ball_acquisition(NbaTipoff *tipoff, uint8_t catcher) {
    /* `$86:D25A-$D349`: publish prior controller, transfer a designated
     * passer controller or select the catcher's own team's round-robin pad.
     * Preserve the designated-pass branch's native lack of a group check;
     * only the round-robin branch explicitly restricts the controller team. */
    controller_read_actors(tipoff);
    if (!nba_controller_acquire(&tipoff->controllers,catcher,
            tipoff->actors[catcher].team_group_raw_6e,
            (uint16_t)tipoff->pass_receiver_raw,(uint16_t)tipoff->pass_aux_raw,
            &tipoff->controller_previous_owner_raw_0a00))
        tipoff->controller_contract_fault=true;
    else controller_write_actors(tipoff);
    /* `$86:D34A-$D3C5`: mark the collision, run the shared BAA2 ownership
     * installer, then clear only the pass globals owned by this continuation.
     * `$09C4/$09DA`, the inbound timer, and the ball record are not reset here. */
    bool completing_tip=tipoff->live_state_raw==0x81u;
    tipoff->rim_raw_13e7 |= 0x0010u;
    cpu_apply_ball_acquisition_core(tipoff, catcher);
    NbaTipCompletion s={tipoff->live_state_raw,tipoff->inbound_transfer_raw,
        (uint16_t)tipoff->pass_receiver_raw,tipoff->fouls.whistle_active_raw_09b6,
        tipoff->play_code,tipoff->play_request_raw,(uint16_t)tipoff->pass_actor_raw,
        (uint16_t)tipoff->pass_aux_raw,(uint16_t)tipoff->ball.velocity_z};
    if(nba_tip_complete_acquisition(&s)) {
        /* `$86:D3C6 -> B04C`: temporary first catch, never final possession. */
        tipoff->tip_contact_actor=(int8_t)catcher;
        tipoff->tip_contact_frame=(uint32_t)tipoff->frame;
        (void)nba_tipoff_select_tip_receiver(tipoff);
        tipoff->cpu_play_state=NBA_CPU_PLAY_PASS;
        return;
    }
    tipoff->live_state_raw=s.live_state;tipoff->play_request_raw=s.request;
    tipoff->pass_actor_raw=(int8_t)s.passer;tipoff->pass_aux_raw=(int8_t)s.aux;
    tipoff->pass_receiver_raw=(int8_t)s.receiver;tipoff->inbound_transfer_raw=s.transfer;
    tipoff->ball.velocity_z=(int16_t)s.ball_vz;
    if(completing_tip && tipoff->tip_possession_frame==0) {
        tipoff->tip_possession_frame=(uint32_t)tipoff->frame;
        tipoff->phase=NBA_TIPOFF_LIVE;
    }
}

void nba_tipoff_replay_ball_acquisition(NbaTipoff *tipoff, uint8_t catcher) {
    if (!tipoff || catcher >= NBA_GAMEPLAY_ACTOR_COUNT) return;
    cpu_commit_ball_acquisition(tipoff, catcher);
}

void nba_tipoff_replay_ball_acquisition_core(NbaTipoff *tipoff,
                                             uint8_t catcher) {
    if (!tipoff || catcher >= NBA_GAMEPLAY_ACTOR_COUNT) return;
    cpu_apply_ball_acquisition_core(tipoff, catcher);
}

static bool cpu_ball_acquisition_self_test(void) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    state.offense_side = 0u;
    state.handler_actor = 1u;
    state.possession_actor = 1;
    state.pass_actor_raw = 1;
    /* This is a CPU catch fixture. $0944=-1 must name no passing pad;
     * historical value3 unintentionally requests the now-implemented
     * designated controller transfer and contradicts the expected -1. */
    state.pass_aux_raw = -1;
    state.pass_receiver_raw = 7;
    state.ball_activity_raw = 0xFFFFu;
    state.rim_raw_094a = 29u;
    state.rim_force_raw_1866 = 1u;
    state.dead_ball_raw_0968 = 2u;
    state.rim_raw_096a = 2u;
    state.play_aux_selector_raw_09a6 = 3;
    state.team_context[1].dead_ball_actor_raw_3f = 9u;
    state.team_context[1].controller_actor_raw_41 = 2;
    state.dead_ball_raw_096c = 3u;
    state.dead_ball_raw_097e = 0u;
    state.actors[7].control_mode = 3u;
    state.actors[7].controller_assignment_raw = -1;
    state.actors[7].velocity_x = 0x30;
    state.actors[7].velocity_y = -0x20;
    state.actors[7].velocity_z = 0x123;
    state.actors[7].movement_magnitude_raw = 0x80u;
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        state.actors[i].team_group_raw_6e = i < 5 ? 0 : 5;
        state.actors[i].controller_assignment_raw = -1;
        state.actors[i].assignment_base_raw = (uint16_t)(i * 2u);
        state.actors[i].assignment_current_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    }
    cpu_commit_ball_acquisition(&state, 7u);
    if (state.possession_actor != 7 || state.ball.owner_actor != 7 ||
        state.actors[7].control_mode != 11u ||
        state.actors[7].velocity_z != 0x123 ||
        state.actors[7].catcher_latch_raw_ae != 1u ||
        state.actors[7].velocity_x != 0x30 ||
        state.actors[7].velocity_y != -0x20 ||
        state.role_rebuild_raw_09d6 != NBA_GAMEPLAY_UNKNOWN_WORD ||
        state.rim_raw_092c != 0x05A0u ||
        state.free_throw_flight_timer_raw_0930 != 0x0258u ||
        state.pass_actor_raw != -1 || state.pass_aux_raw != -1 ||
        state.pass_receiver_raw != -1 || state.ball_activity_raw != 0u ||
        state.rim_raw_094a != 0u || state.rim_raw_096a != 0u ||
        state.rim_force_raw_1866 != 0u ||
        state.dead_ball_raw_0968 != 0u || state.dead_ball_raw_096c != 0u ||
        state.dead_ball_raw_097e != NBA_GAMEPLAY_UNKNOWN_WORD ||
        state.play_aux_selector_raw_09a6 != -1 ||
        state.catch_actor_record_raw_0910 != 0x3BEBu ||
        state.catch_context_record_raw_0912 != 0x476Bu ||
        state.team_context[1].previous_dead_ball_actor_raw_43 !=
            NBA_GAMEPLAY_UNKNOWN_WORD ||
        state.team_context[1].previous_controller_actor_raw_45 != -1 ||
        state.team_context[1].dead_ball_actor_raw_3f != 7u ||
        state.team_context[1].controller_actor_raw_41 != -1)
        return false;
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        if (state.actors[i].assignment_current_raw !=
            state.actors[i].assignment_base_raw) return false;
    NbaTipoff slow;
    memset(&slow, 0, sizeof(slow));
    slow.actors[2].velocity_x = 0x20;
    slow.actors[2].velocity_y = -0x10;
    slow.actors[2].movement_magnitude_raw = 0x7Fu;
    cpu_apply_catch_prefix(&slow, 2u);
    if (slow.actors[2].velocity_x != 0 ||
        slow.actors[2].velocity_y != 0 ||
        slow.actors[2].movement_magnitude_raw != 0u) return false;
    return true;
}

static void cpu_enter_play_state(NbaTipoff *tipoff, NbaCpuPlayState state) {
    tipoff->cpu_play_state = (uint8_t)state;
    tipoff->play_state_frame = 0u;
}

static uint16_t cpu_rom_distance(int16_t dx, int16_t dy) {
    uint16_t ax = dx < 0 ? (uint16_t)(0u - (uint16_t)dx) : (uint16_t)dx;
    uint16_t ay = dy < 0 ? (uint16_t)(0u - (uint16_t)dy) : (uint16_t)dy;
    uint16_t major = ax >= ay ? ax : ay;
    uint16_t minor = ax >= ay ? ay : ax;
    return (uint16_t)(major + (minor >> 2));
}

static void cpu_refresh_pair_geometry(NbaTipoff *tipoff, unsigned actor) {
    NbaTipoffActor *state = &tipoff->actors[actor];
    unsigned paired = state->assignment_current_raw >> 1;
    if (paired >= NBA_GAMEPLAY_ACTOR_COUNT) return;
    NbaTipoffActor *other = &tipoff->actors[paired];
    uint16_t distance = 0u;
    uint8_t direction = nba_gameplay_target_direction(
        (int16_t)(fp_integer_word(other->x_fp) -
                  fp_integer_word(state->x_fp)),
        (int16_t)(fp_integer_word(other->y_fp) -
                  fp_integer_word(state->y_fp)),
        &distance);
    state->assignment_actor = (uint8_t)paired;
    /* `$85:BC6B-$BC7A`: coincident actors (direction 8) retain both prior
     * +$86 direction words while still receiving the zero +$8A distance. */
    if (direction != 8u) state->assignment_direction = direction;
    state->assignment_distance = distance;
    state->pair_distance = distance;
    if (direction != 8u)
        other->assignment_direction = (uint8_t)(direction ^ 4u);
    other->assignment_distance = distance;
    other->pair_distance = distance;
}

static int cpu_select_help_defender(NbaTipoff *tipoff, unsigned owner,
                                    unsigned defense, int16_t anchor,
                                    const uint16_t focal_distance[5]) {
    /* `$85:BB6C-$BB98`: first try owner's +$78 candidate. It must be
     * unassigned and below mode 4; a candidate across the context half is
     * accepted only within $30 of the owner/focal point. */
    unsigned alternate =
        tipoff->actors[owner].assignment_alternate_raw >> 1;
    if (alternate < NBA_GAMEPLAY_ACTOR_COUNT && alternate / 5u == defense) {
        NbaTipoffActor *candidate = &tipoff->actors[alternate];
        bool same_half =
            (int16_t)(anchor ^ fp_round(tipoff->actors[owner].x_fp)) >= 0;
        if ((int16_t)candidate->assignment_current_raw < 0 &&
            candidate->control_mode < 4u &&
            (same_half || focal_distance[alternate % 5u] < 0x30u))
            return (int)alternate;
    }

    /* `$85:BBBF-$BC06`: fallback scans all five defenders and replaces on
     * equality, so the last actor wins a distance tie. */
    int selected = -1;
    uint16_t best = 0x7FFFu;
    for (unsigned i = 0; i < 5u; ++i) {
        unsigned candidate = defense * 5u + i;
        if (tipoff->actors[candidate].control_mode >= 4u) continue;
        if (focal_distance[i] <= best) {
            best = focal_distance[i];
            selected = (int)candidate;
        }
    }
    return selected;
}

static void cpu_install_help_assignment(NbaTipoff *tipoff, unsigned helper,
                                        unsigned owner) {
    /* `$85:BB99-$BBBE` is deliberately one-way. If the helper was assigned,
     * its former offense actor is unbound; the owner's +$74 stays attached
     * to the primary defender. */
    NbaTipoffActor *state = &tipoff->actors[helper];
    unsigned old = state->assignment_current_raw >> 1;
    if (old < NBA_GAMEPLAY_ACTOR_COUNT)
        tipoff->actors[old].assignment_current_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state->assignment_current_raw = (uint16_t)(owner * 2u);
    state->assignment_actor = (uint8_t)owner;
    state->control_mode = 6u;
}

static void cpu_one_way_bind(NbaTipoff *tipoff, unsigned candidate,
                             unsigned target) {
    NbaTipoffActor *state = &tipoff->actors[candidate];
    unsigned old = state->assignment_current_raw >> 1;
    if (old < NBA_GAMEPLAY_ACTOR_COUNT)
        tipoff->actors[old].assignment_current_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state->assignment_current_raw = (uint16_t)(target * 2u);
    state->assignment_actor = (uint8_t)target;
}

static void cpu_symmetric_bind(NbaTipoff *tipoff, unsigned candidate,
                               unsigned target) {
    unsigned old = tipoff->actors[candidate].assignment_current_raw >> 1;
    if (old < NBA_GAMEPLAY_ACTOR_COUNT)
        tipoff->actors[old].assignment_current_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    tipoff->actors[candidate].assignment_current_raw = (uint16_t)(target * 2u);
    tipoff->actors[target].assignment_current_raw = (uint16_t)(candidate * 2u);
    tipoff->actors[candidate].assignment_actor = (uint8_t)target;
    tipoff->actors[target].assignment_actor = (uint8_t)candidate;
}

static bool cpu_try_base_defender(NbaTipoff *tipoff, unsigned target,
                                  unsigned defense, int16_t anchor) {
    /* `$85:BAE4-$BB6B`: prefer target +$78, with the extra opposite-half
     * basket/separation gates preserved exactly. */
    unsigned candidate = tipoff->actors[target].assignment_alternate_raw >> 1;
    if (candidate >= NBA_GAMEPLAY_ACTOR_COUNT || candidate / 5u != defense)
        return false;
    NbaTipoffActor *c = &tipoff->actors[candidate];
    NbaTipoffActor *t = &tipoff->actors[target];
    if ((int16_t)c->assignment_current_raw >= 0 || c->control_mode >= 7u)
        return false;
    if ((int16_t)(anchor ^ fp_round(t->x_fp)) >= 0) {
        cpu_symmetric_bind(tipoff, candidate, target);
        return true;
    }
    if (c->anchor_distance_raw >= t->anchor_distance_raw) {
        if (c->anchor_distance_raw >= 0x38u) return false;
        cpu_symmetric_bind(tipoff, candidate, target);
        return true;
    }
    if (c->anchor_distance_raw < 0x38u) {
        cpu_symmetric_bind(tipoff, candidate, target);
        return true;
    }
    {
        uint16_t separation = cpu_rom_distance(
            (int16_t)(fp_round(t->x_fp) - fp_round(c->x_fp)),
            (int16_t)(fp_round(t->y_fp) - fp_round(c->y_fp)));
        if ((uint16_t)(separation << 1) >= t->anchor_distance_raw)
            return false;
    }
    /* `$85:BB5A -> $85:BBAE`: the far opposite-half candidate is a help
     * assignment, not the target's symmetric primary matchup. */
    cpu_one_way_bind(tipoff, candidate, target);
    return true;
}

static bool cpu_fallback_primary_defender(NbaTipoff *tipoff, unsigned target,
                                          unsigned defense) {
    /* `$85:BA1D-$BAB6`: pass one minimizes +$8E among candidates no farther
     * from the basket than target; pass two minimizes +$8C. Equality replaces,
     * so the last defender wins ties in both passes. */
    int selected = -1;
    uint16_t best = 0x7FFFu;
    for (unsigned i = 0; i < 5u; ++i) {
        unsigned actor = defense * 5u + i;
        NbaTipoffActor *state = &tipoff->actors[actor];
        if (state->control_mode >= 7u ||
            state->anchor_distance_raw > tipoff->actors[target].anchor_distance_raw)
            continue;
        if (state->focal_distance_raw_8e <= best) {
            best = state->focal_distance_raw_8e;
            selected = (int)actor;
        }
    }
    if (selected < 0) {
        best = 0x7FFFu;
        for (unsigned i = 0; i < 5u; ++i) {
            unsigned actor = defense * 5u + i;
            NbaTipoffActor *state = &tipoff->actors[actor];
            if (state->control_mode >= 7u) continue;
            if (state->anchor_distance_raw <= best) {
                best = state->anchor_distance_raw;
                selected = (int)actor;
            }
        }
    }
    if (selected < 0) return false;
    cpu_symmetric_bind(tipoff, (unsigned)selected, target);
    return true;
}

static bool cpu_bind_nearest_unassigned(NbaTipoff *tipoff, unsigned target,
                                        unsigned defense);

/* Direct replay adapters for the callable `$85:B9D2-$BC06` matchup helper
 * family. They preserve each ROM helper boundary; the live BC07 planner uses
 * the same private cores below and adds its caller-owned mode changes later. */
bool nba_tipoff_replay_matchup_helper(NbaTipoff *tipoff, uint16_t entry,
    uint8_t current_actor, uint8_t related_actor, uint8_t context_side,
    uint8_t *selected_actor) {
    if (!tipoff || current_actor >= NBA_GAMEPLAY_ACTOR_COUNT ||
        related_actor >= NBA_GAMEPLAY_ACTOR_COUNT || context_side > 1u)
        return false;
    unsigned defense = context_side;
    int selected = -1;
    bool result = false;
    switch (entry) {
    case 0xB9D2u:
        result = cpu_bind_nearest_unassigned(
            tipoff, related_actor, defense);
        if (result) selected = tipoff->actors[related_actor].assignment_current_raw >> 1;
        break;
    case 0xBA1Du:
        result = cpu_fallback_primary_defender(
            tipoff, related_actor, defense);
        if (result) selected = tipoff->actors[related_actor].assignment_current_raw >> 1;
        break;
    case 0xBAB7u:
        cpu_symmetric_bind(tipoff, current_actor, related_actor);
        selected = current_actor;
        result = true;
        break;
    case 0xBAE4u:
        selected = tipoff->actors[related_actor].assignment_alternate_raw >> 1;
        result = cpu_try_base_defender(
            tipoff, related_actor, defense,
            tipoff->team_context[context_side].anchor_x_raw_0a);
        if (!result || selected >= NBA_GAMEPLAY_ACTOR_COUNT) selected = -1;
        break;
    case 0xBB6Cu: {
        unsigned alternate =
            tipoff->actors[related_actor].assignment_alternate_raw >> 1;
        if (alternate < NBA_GAMEPLAY_ACTOR_COUNT &&
            alternate / 5u == defense) {
            NbaTipoffActor *candidate = &tipoff->actors[alternate];
            int16_t anchor = tipoff->team_context[context_side].anchor_x_raw_0a;
            bool same_half = (int16_t)(anchor ^
                fp_round(tipoff->actors[related_actor].x_fp)) >= 0;
            if ((int16_t)candidate->assignment_current_raw < 0 &&
                candidate->control_mode < 4u &&
                (same_half || candidate->focal_distance_raw_8e < 0x30u)) {
                cpu_one_way_bind(tipoff, alternate, related_actor);
                selected = (int)alternate;
                result = true;
            }
        }
        break;
    }
    case 0xBB99u: {
        cpu_one_way_bind(tipoff, current_actor, related_actor);
        selected = current_actor;
        result = true;
        break;
    }
    case 0xBBBFu: {
        uint16_t best = 0x7FFFu;
        for (unsigned i = 0; i < 5u; ++i) {
            unsigned candidate = defense * 5u + i;
            if (tipoff->actors[candidate].control_mode >= 4u) continue;
            if (tipoff->actors[candidate].focal_distance_raw_8e <= best) {
                best = tipoff->actors[candidate].focal_distance_raw_8e;
                selected = (int)candidate;
            }
        }
        if (selected >= 0) {
            cpu_one_way_bind(tipoff, (unsigned)selected, related_actor);
            result = true;
        }
        break;
    }
    default:
        return false;
    }
    if (selected_actor)
        *selected_actor = selected < 0 ? 0xFFu : (uint8_t)selected;
    return result;
}

static bool cpu_bind_nearest_unassigned(NbaTipoff *tipoff, unsigned target,
                                        unsigned defense) {
    /* `$85:B9D2-$BA1C`: unassigned mode<7, minimum +$8C, first tie wins. */
    int selected = -1;
    uint16_t best = 0x7FFFu;
    for (unsigned i = 0; i < 5u; ++i) {
        unsigned actor = defense * 5u + i;
        NbaTipoffActor *state = &tipoff->actors[actor];
        if ((int16_t)state->assignment_current_raw >= 0 ||
            state->control_mode >= 7u || state->anchor_distance_raw >= best)
            continue;
        best = state->anchor_distance_raw;
        selected = (int)actor;
    }
    if (selected < 0) return false;
    cpu_symmetric_bind(tipoff, (unsigned)selected, target);
    return true;
}

static void cpu_release_defense_assignment(NbaTipoff *tipoff,
                                           unsigned defender) {
    NbaTipoffActor *state = &tipoff->actors[defender];
    unsigned paired = state->assignment_current_raw >> 1;
    state->assignment_current_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state->assignment_actor = 0xFFu;
    if (state->saved_control_mode == 6u ||
        paired >= NBA_GAMEPLAY_ACTOR_COUNT) return;
    tipoff->actors[paired].assignment_current_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    tipoff->actors[paired].assignment_distance = 0x0140u;
    tipoff->actors[paired].pair_distance = 0x0140u;
    state->movement_boost_timer = 0x14u;
}

static bool cpu_defensive_planner_self_test(void) {
    NbaTipoff t;
    memset(&t, 0, sizeof(t));
    uint16_t distances[5] = {0x40u, 0x20u, 0x20u, 0x60u, 0x70u};
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        t.actors[i].assignment_current_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        t.actors[i].assignment_alternate_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        t.actors[i].control_mode = 7u;
    }
    t.actors[7].control_mode = 2u;
    t.actors[7].assignment_current_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    t.actors[2].assignment_alternate_raw = 14u;
    t.actors[2].x_fp = 40 * 256;
    if (cpu_select_help_defender(&t, 2u, 1u, 336, distances) != 7)
        return false;
    t.actors[2].assignment_alternate_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    t.actors[6].control_mode = 2u;
    t.actors[7].control_mode = 2u;
    if (cpu_select_help_defender(&t, 2u, 1u, 336, distances) != 7)
        return false; /* fallback equal distance: last candidate */
    t.actors[7].saved_control_mode = 6u;
    t.actors[7].assignment_current_raw = 4u;
    t.actors[2].assignment_current_raw = 14u;
    cpu_release_defense_assignment(&t, 7u);
    if (t.actors[7].assignment_current_raw != NBA_GAMEPLAY_UNKNOWN_WORD ||
        t.actors[2].assignment_current_raw != 14u)
        return false; /* old mode-6 release is one-way */
    return true;
}

/* `$85:B95C-$B9D1`: every eligible actor visited by the role-rebuild passes
 * loses its prior behavior flags. State `$82` preserves the provisional
 * inbounder's +$60 timer and RNG; every other call measures from the actor's
 * integer position to the physical ball and installs the randomized delay. */
static void cpu_reload_role_reaction(NbaTipoff *tipoff, unsigned actor) {
    NbaTipoffActor *state = &tipoff->actors[actor];
    state->behavior_flags_raw = 0u;
    if (tipoff->live_state_raw == 0x82u &&
        actor == tipoff->inbound_actor_raw) return;
    state->reaction_threshold = nba_gameplay_reaction_threshold(
        &tipoff->rng,
        fp_integer_word(state->x_fp), fp_integer_word(state->y_fp),
        fp_integer_word(tipoff->ball.x_fp),
        fp_integer_word(tipoff->ball.y_fp));
}

void nba_tipoff_refresh_offense_roles_end_frame(NbaTipoff *tipoff) {
    unsigned offense = (tipoff->live_state_raw == 0x82u ?
        tipoff->inbound_state_raw : tipoff->camera_side_group_raw) != 0u ?
        1u : 0u;
    bool scan_predicted_ball = tipoff->live_state_raw == 0x82u ||
        (tipoff->possession_actor < 0 && tipoff->pass_receiver_raw < 0);
    int16_t predicted_ball_x = tipoff->role_focal_x_raw_0918;
    int16_t predicted_ball_y = tipoff->role_focal_y_raw_091a;
    uint16_t nearest_distance = 0x7FFFu;
    unsigned nearest_actor = offense * 5u;
    NbaGameplayLoosePursuitActor offense_actors[5];
    uint8_t offense_modes[5];
    for (unsigned i = 0; i < 5u; ++i) {
        NbaTipoffActor *state = &tipoff->actors[offense * 5u + i];
        uint16_t distance = 0u;
        uint8_t direction = nba_gameplay_pass_direction(
            (int16_t)(tipoff->team_context[offense].anchor_x_raw_0a -
                      fp_integer_word(state->x_fp)),
            (int16_t)(0 - fp_integer_word(state->y_fp)), &distance);
        state->anchor_distance_raw = distance;
        if (direction != 0x10u) state->anchor_direction_raw = direction;
        if (distance < nearest_distance) {
            nearest_distance = distance;
            nearest_actor = offense * 5u + i;
        }
        offense_actors[i] = (NbaGameplayLoosePursuitActor){
            fp_round(state->x_fp), fp_round(state->y_fp), state->control_mode
        };
    }
    tipoff->role_nearest_offense_raw_09de =
        (uint16_t)(0x34EBu + nearest_actor * 0x100u);
    tipoff->role_ownerless_raw_09d8 = scan_predicted_ball ? 1u : 0u;
    (void)nba_gameplay_select_no_owner_pursuer(
        offense_actors, predicted_ball_x, predicted_ball_y, offense_modes);
    if (!scan_predicted_ball) {
        for (unsigned i = 0; i < 5u; ++i)
            offense_modes[i] = offense_actors[i].control_mode < 7u ?
                1u : offense_actors[i].control_mode;
    }
    for (unsigned i = 0; i < 5u; ++i)
        tipoff->actors[offense * 5u + i].control_mode = offense_modes[i];
}

static void cpu_rebuild_role_assignments(NbaTipoff *tipoff,
                                         unsigned offense,
                                         unsigned defense) {
    /* `$85:BD0D-$BE03`: reset offense alternates, then restore both teams'
     * base pairings. Keep the three passes separate because each eligible
     * `$85:B95C` call advances the shared RNG in actor order. */
    for (unsigned i = 0; i < 5u; ++i) {
        unsigned actor = offense * 5u + i;
        NbaTipoffActor *state = &tipoff->actors[actor];
        state->assignment_current_raw = state->assignment_alternate_raw;
        if (state->control_mode < 7u &&
            actor != (unsigned)(uint8_t)tipoff->possession_actor) {
            state->behavior_timer = 0x2Fu;
            cpu_reload_role_reaction(tipoff, actor);
        }
    }
    for (unsigned i = 0; i < 5u; ++i) {
        unsigned actor = defense * 5u + i;
        NbaTipoffActor *state = &tipoff->actors[actor];
        if (state->control_mode < 7u) state->control_mode = 2u;
        state->saved_control_mode = state->control_mode;
        state->assignment_current_raw = state->assignment_base_raw;
        if (state->control_mode < 7u) {
            state->behavior_timer = 0u;
            cpu_reload_role_reaction(tipoff, actor);
        }
    }
    for (unsigned i = 0; i < 5u; ++i) {
        unsigned actor = offense * 5u + i;
        NbaTipoffActor *state = &tipoff->actors[actor];
        if (state->control_mode < 7u) {
            state->control_mode =
                tipoff->live_state_raw == 0x82u &&
                state->team_group_raw_6e == tipoff->inbound_state_raw ?
                1u : 2u;
        }
        state->saved_control_mode = state->control_mode;
        state->assignment_current_raw = state->assignment_base_raw;
        if (state->control_mode < 7u) {
            state->behavior_timer = 0u;
            cpu_reload_role_reaction(tipoff, actor);
        }
    }
    tipoff->role_rebuild_raw_09d6 = 0u;
}

static bool cpu_role_reaction_reload_self_test(void) {
    static const int16_t positions[10][2] = {
        {8, 3}, {-16, -83}, {-24, 80}, {104, -56}, {96, 59},
        {-8, -3}, {16, 83}, {24, -80}, {-104, 56}, {-96, -59}
    };
    static const uint16_t expected[10] = {
        97u, 120u, 143u, 128u, 118u, 7u, 85u, 92u, 0x1234u, 110u
    };
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    state.live_state_raw = 0x81u;
    state.possession_actor = -1;
    state.rng.state = 0x34EAu;
    state.ball.x_fp = -9 * 256;
    state.ball.y_fp = 4 * 256;
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaTipoffActor *a = &state.actors[actor];
        a->x_fp = positions[actor][0] * 256;
        a->y_fp = positions[actor][1] * 256;
        a->control_mode = actor == 8u ? 10u : 2u;
        a->reaction_threshold = actor == 8u ? 0x1234u : 0u;
        a->behavior_flags_raw = actor == 8u ? 0x0200u : 0xFFFFu;
    }
    cpu_rebuild_role_assignments(&state, 1u, 0u);
    if (state.rng.state != 0xC408u) return false;
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        if (state.actors[actor].reaction_threshold != expected[actor] ||
            state.actors[actor].behavior_flags_raw !=
                (actor == 8u ? 0x0200u : 0u)) return false;
    }

    memset(&state, 0, sizeof(state));
    state.live_state_raw = 0x82u;
    state.inbound_state_raw = 0u;
    state.inbound_actor_raw = 2u;
    state.possession_actor = -1;
    state.rng.state = 0x9146u;
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        state.actors[actor].control_mode = 7u;
        state.actors[actor].team_group_raw_6e = actor < 5u ? 0u : 5u;
    }
    state.actors[2].control_mode = 2u;
    state.actors[2].reaction_threshold = 0x4567u;
    state.actors[2].behavior_flags_raw = 0xFFFFu;
    cpu_rebuild_role_assignments(&state, 0u, 1u);
    return state.rng.state == 0x9146u &&
           state.actors[2].reaction_threshold == 0x4567u &&
           state.actors[2].behavior_flags_raw == 0u;
}

void nba_tipoff_refresh_defense_roles_end_frame(NbaTipoff *tipoff) {
    /* `$85:BC07-$C0F5`, entered after `$85:AF5C` has already normalized
     * offense. Keep this boundary independently replayable against Mesen. */
    unsigned offense = (tipoff->live_state_raw == 0x82u ?
        tipoff->inbound_state_raw : tipoff->camera_side_group_raw) != 0u ?
        1u : 0u;
    unsigned defense = offense ^ 1u;
    uint16_t focal_distance[5] = {0};
    unsigned nearest_focal = defense * 5u;
    unsigned nearest_anchor = defense * 5u;
    unsigned nearest_offense_anchor = offense * 5u;
    uint16_t nearest_focal_distance = 0x7FFFu;
    uint16_t nearest_anchor_distance = 0x7FFFu;
    uint16_t nearest_offense_distance = 0x7FFFu;
    int16_t focal_x = tipoff->role_focal_x_raw_0918;
    int16_t focal_y = tipoff->role_focal_y_raw_091a;
    if (tipoff->possession_actor >= 0 &&
        tipoff->possession_actor < NBA_GAMEPLAY_ACTOR_COUNT) {
        focal_x = fp_integer_word(
            tipoff->actors[(unsigned)tipoff->possession_actor].x_fp);
        focal_y = fp_integer_word(
            tipoff->actors[(unsigned)tipoff->possession_actor].y_fp);
    }
    for (unsigned i = 0; i < 5u; ++i) {
        unsigned actor = defense * 5u + i;
        NbaTipoffActor *state = &tipoff->actors[actor];
        uint16_t focal = 0u;
        (void)nba_gameplay_target_direction(
            (int16_t)(fp_integer_word(state->x_fp) - focal_x),
            (int16_t)(fp_integer_word(state->y_fp) - focal_y), &focal);
        focal_distance[i] = focal;
        state->focal_distance_raw_8e = focal;
        if (focal < nearest_focal_distance) {
            nearest_focal_distance = focal;
            nearest_focal = actor;
        }
        cpu_refresh_pair_geometry(tipoff, actor);
    }

    if (tipoff->role_rebuild_raw_09d6 != 0u) {
        tipoff->role_cadence_raw_09d2 = 0x1Eu;
        cpu_rebuild_role_assignments(tipoff, offense, defense);
    } else {
        tipoff->role_cadence_raw_09d2 =
            (uint16_t)(tipoff->role_cadence_raw_09d2 - 2u);
        if ((int16_t)tipoff->role_cadence_raw_09d2 >= 0) return;
        tipoff->role_cadence_raw_09d2 =
            (uint16_t)(tipoff->role_cadence_raw_09d2 + 0x1Eu);
        if ((int8_t)tipoff->camera_side_group_raw < 0) return;
    }

    /* `$85:BE06` deliberately uses the *entry/offense* context +$0A while
     * measuring the five defenders. It is the basket they protect, not the
     * defender context's own anchor. +$88 is untouched in this routine. */
    int16_t defense_basket_x =
        tipoff->team_context[offense].anchor_x_raw_0a;
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaTipoffActor *state = &tipoff->actors[actor];
        unsigned side = actor / 5u;
        if (side == offense &&
            state->anchor_distance_raw < nearest_offense_distance) {
            nearest_offense_distance = state->anchor_distance_raw;
            nearest_offense_anchor = actor;
        }
        if (side == defense) {
            (void)nba_gameplay_target_direction(
                (int16_t)(defense_basket_x -
                          fp_integer_word(state->x_fp)),
                (int16_t)-fp_integer_word(state->y_fp),
                &state->anchor_distance_raw);
            if (state->anchor_distance_raw < nearest_anchor_distance) {
                nearest_anchor_distance = state->anchor_distance_raw;
                nearest_anchor = actor;
            }
            /* `$85:BE70-$BF2B`: dead/inbound states and mode 9 bypass both
             * normalization and assignment cleanup. */
            if (tipoff->live_state_raw >= 0x80u || state->control_mode == 9u)
                continue;
            if (state->control_mode < 7u) state->control_mode = 2u;
            else {
                cpu_release_defense_assignment(tipoff, actor);
                continue;
            }
            if ((int16_t)state->assignment_current_raw < 0) continue;
            if (state->saved_control_mode == 6u) {
                cpu_release_defense_assignment(tipoff, actor);
                continue;
            }
            unsigned paired = state->assignment_current_raw >> 1;
            if (paired >= NBA_GAMEPLAY_ACTOR_COUNT) continue;
            NbaTipoffActor *other = &tipoff->actors[paired];
            /* Opposite anchor sign keeps the pairing immediately. */
            if ((int16_t)(fp_round(other->x_fp) ^ defense_basket_x) < 0 ||
                state->anchor_distance_raw < 0x30u)
                continue;
            if (state->anchor_distance_raw >= other->anchor_distance_raw) {
                cpu_release_defense_assignment(tipoff, actor);
                continue;
            }
            uint16_t delta = (uint16_t)(other->anchor_distance_raw -
                                        state->anchor_distance_raw);
            if (delta < 0x0Cu) state->movement_boost_timer = 0x1Eu;
            if (state->anchor_distance_raw >= 0xA0u) continue;
            uint16_t relative = (uint16_t)(
                (uint16_t)(((state->assignment_direction << 1) ^ 8u) -
                           other->anchor_direction_raw + 3u));
            if ((int16_t)(uint16_t)(relative - 7u) >= 0)
                cpu_release_defense_assignment(tipoff, actor);
        }
    }
    if (tipoff->possession_actor >= 0 &&
        tipoff->possession_actor < NBA_GAMEPLAY_ACTOR_COUNT) {
        NbaTipoffActor *owner =
            &tipoff->actors[(unsigned)tipoff->possession_actor];
        unsigned primary = owner->assignment_current_raw >> 1;
        /* Assignment helpers `$85:BAE4/$BB6C` read the active defense
         * context's own +$0A. This is intentionally the opposite sign from
         * `$BA`, used by the preceding protected-basket cleanup. */
        int16_t anchor = tipoff->team_context[defense].anchor_x_raw_0a;
        /* `$85:BF3F-$BF7F`: a ball owner far from its basket first repairs
         * the nearest offense actor's missing matchup. */
        if (owner->anchor_distance_raw >= 0xF0u &&
            (int16_t)tipoff->actors[nearest_offense_anchor].assignment_current_raw < 0) {
            if (!cpu_try_base_defender(
                    tipoff, nearest_offense_anchor, defense, anchor) &&
                tipoff->actors[nearest_anchor].control_mode < 3u)
                cpu_symmetric_bind(tipoff, nearest_anchor,
                                   nearest_offense_anchor);
        }
        /* `$85:BF89-$BF8E` reloads the owner's +$74 after the repair above;
         * that repair can displace the owner's former primary defender. */
        primary = owner->assignment_current_raw >> 1;
        if (primary >= NBA_GAMEPLAY_ACTOR_COUNT || primary / 5u != defense) {
            if (!cpu_try_base_defender(
                    tipoff, (unsigned)tipoff->possession_actor, defense, anchor) &&
                !cpu_fallback_primary_defender(
                    tipoff, (unsigned)tipoff->possession_actor, defense))
                return; /* `$85:C051`: both primary searches failed. */
            primary = owner->assignment_current_raw >> 1;
        }
        if (primary < NBA_GAMEPLAY_ACTOR_COUNT &&
            primary / 5u == defense) {
            NbaTipoffActor *primary_state = &tipoff->actors[primary];
            if (primary_state->control_mode < 7u)
                primary_state->control_mode = 4u;
            /* `$85:BFC1-$BFDC`: the primary receives +$72=$14 when no
             * closer to its basket than the owner, or while live and the
             * pair distance has reached $10. */
            if (primary_state->anchor_distance_raw >=
                    owner->anchor_distance_raw ||
                (tipoff->live_state_raw < 0x80u &&
                 primary_state->pair_distance >= 0x10u))
                primary_state->movement_boost_timer = 0x14u;

            /* `$85:BFEA-$C04C`: preserve the primary assignment and install
             * a one-way mode-6 help defender only when the team/owner gates
             * request it. Context +$4E is $00A0 in all three ROM gameplay
             * WRAM snapshots (frames 220, 400, and 1800). */
            NbaGameplayTeamContext *context = &tipoff->team_context[defense];
            bool help_due = false;
            if (tipoff->live_state_raw < 0x80u) {
                if (owner->anchor_distance_raw < context->help_distance_raw_4e)
                    help_due = true;
                else if (context->mode_raw_30 == 2u &&
                         (int16_t)(anchor ^ fp_round(owner->x_fp)) < 0)
                    help_due = true;
                else if (owner->help_request_raw_80 != 0u &&
                         (owner->anchor_distance_raw < 0xA0u ||
                          context->mode_raw_30 == 0u))
                    help_due = true;
            }
            if (help_due) {
                int helper = cpu_select_help_defender(
                    tipoff, (unsigned)tipoff->possession_actor, defense,
                    anchor, focal_distance);
                if (helper >= 0) {
                    cpu_install_help_assignment(
                        tipoff, (unsigned)helper,
                        (unsigned)tipoff->possession_actor);
                    NbaTipoffActor *helper_state = &tipoff->actors[helper];
                    if (helper_state->anchor_distance_raw >=
                            owner->anchor_distance_raw ||
                        helper_state->pair_distance >= 0x10u)
                        helper_state->movement_boost_timer = 0x14u;
                }
            }
        }
    } else if (tipoff->pass_receiver_raw < 0) {
        /* `$85:C052-$C0B1`: without an owner, promote the focal-nearest
         * eligible defender; if `$09DA` is busy, fallback is last-tie +$8E. */
        unsigned selected = nearest_focal;
        if (tipoff->actors[selected].control_mode >= 7u) {
            int fallback = -1;
            uint16_t best = 0x7FFFu;
            for (unsigned i = 0; i < 5u; ++i) {
                unsigned actor = defense * 5u + i;
                if (tipoff->actors[actor].control_mode < 7u &&
                    tipoff->actors[actor].focal_distance_raw_8e <= best) {
                    best = tipoff->actors[actor].focal_distance_raw_8e;
                    fallback = (int)actor;
                }
            }
            if (fallback < 0) return;
            selected = (unsigned)fallback;
        }
        if (tipoff->actors[selected].control_mode < 7u)
            tipoff->actors[selected].control_mode = 4u;
    }

    /* `$85:C0B4-$C0F5`: complete missing defense assignments in the
     * context's five-byte actor order. Each target tries its reciprocal base
     * candidate, then the first anchor-nearest unassigned defender. */
    int16_t defense_anchor =
        tipoff->team_context[defense].anchor_x_raw_0a;
    tipoff->pass_distance_raw = 5u; /* `$09DA` is reused as loop counter. */
    for (unsigned i = 0; i < 5u; ++i) {
        unsigned target = tipoff->team_context[defense].actor_order_raw_49[i] >> 1;
        if (target < NBA_GAMEPLAY_ACTOR_COUNT &&
            (int16_t)tipoff->actors[target].assignment_current_raw < 0 &&
            !cpu_try_base_defender(tipoff, target, defense, defense_anchor) &&
            !cpu_bind_nearest_unassigned(tipoff, target, defense))
            return; /* `$85:C051`: retain the remaining counter. */
        --tipoff->pass_distance_raw;
    }
    tipoff->role_rebuild_raw_09d6 = 0u;
}

void nba_tipoff_refresh_team_roles_end_frame(NbaTipoff *tipoff) {
    /* `$87:8FA1-$8FA9`: offense `$85:AF5C`, then defense `$85:BC07`. */
    nba_tipoff_refresh_offense_roles_end_frame(tipoff);
    nba_tipoff_refresh_defense_roles_end_frame(tipoff);
}

static void cpu_update_all_actors(NbaTipoff *tipoff) {
    /* `$87:8F01-$8F8D` updates all ten actors as one logical pass with
     * `$C6/$0938=2`. */
    /* `$87:8EFB-$8F92` is one global 30-Hz pass with `$0938/$C6=2`.
     * Possession and inbound changes do not rephase it. */
    if ((tipoff->simulation_tick & 1u) != 0u) return;
    ++tipoff->actor_update_tick;
    tipoff->actor_pass_executed = true;
    if (tipoff->differential_observer)
        tipoff->differential_observer(tipoff, "actors.begin", tipoff->differential_context);
    /* `$87:9090-$90A0` snapshots all ten +$5E modes into +$84 before any
     * actor dispatch. BC07 relies on this to recognize last frame's mode-6
     * helper after the normalizer has changed +$5E back to mode 2. */
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor)
        tipoff->actors[actor].saved_control_mode =
            tipoff->actors[actor].control_mode;
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaTipoffActor *state = &tipoff->actors[actor];
        cpu_ease_display_direction(state);
        if (state->contact_inhibit_raw_5a != 0u)
            state->contact_inhibit_raw_5a =
                state->contact_inhibit_raw_5a > 2u ?
                (uint16_t)(state->contact_inhibit_raw_5a - 2u) : 0u;
        if (state->recovery_inhibit_raw != 0u)
            state->recovery_inhibit_raw = state->recovery_inhibit_raw > 2u ?
                (uint16_t)(state->recovery_inhibit_raw - 2u) : 0u;
        bool common_committed = cpu_move_actor(tipoff, actor);
        /* The native common commit already owns rectangle/diagonal bounds.
         * A second host clamp would erase stationary-axis and mode-8 rules.
         * Special-mode adopters still using their own integration retain a
         * compatibility guard until their common-prefix ordering is ported. */
        if (!common_committed)
            cpu_clamp_actor_to_court(&state->x_fp, &state->y_fp,
                                     &state->velocity_x, &state->velocity_y);
        cpu_advance_actor_animation(tipoff, state);
        (void)cpu_actor_contact_height(
            tipoff, actor, &state->contact_height_raw_aa);
        /* The renderer resolves the post-dispatch resource phase. Mirror
         * `$87:B649/$B832` against that same phase so an attached mode-15
         * ball cannot lag the visible passing hand by one host tick. */
        if (state->control_mode == 15u &&
            tipoff->ball.owner_actor == (int8_t)actor)
            ball_attach_to_actor(tipoff, actor);
    }
    if (tipoff->differential_observer)
        tipoff->differential_observer(tipoff, "actors.end", tipoff->differential_context);
}

static void cpu_update_actor_behaviors(NbaTipoff *tipoff) {
    /* $87:9075-9086 clears the per-pad latch before the $91xx actor sweep.
     * Human action/movement dispatch is still gated off at match creation;
     * publishing a record here alone does not implement $84:E2AC. */
    nba_controller_begin_sweep(&tipoff->controllers);
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        int pad=tipoff->actors[actor].controller_assignment_raw;
        if(pad>=0 && pad<5 && !tipoff->controllers.record[pad].processed)
            nba_tipoff_publish_controller_input(tipoff,actor,pad==0?tipoff->pad_held_raw:0);
        /* Even inbound owners enter F34F; F3DA selects the separate F43A
         * continuation only after the common flag/pose prefix. */
        cpu_dispatch_normal_actor_behavior(tipoff, actor);
        if(pad>=0 && pad<5 && tipoff->actors[actor].controller_assignment_raw>=0)
            tipoff->controllers.record[pad].processed=1;
    }
}

static void cpu_schedule_actor_behaviors(NbaTipoff *tipoff,
                                         bool acquisition_boundary) {
    /* `$87:9244` normally follows the even-tick physics/global passes. A
     * BAA2 ownership install at that boundary can cross the outer-frame
     * callback: Mesen sees its mode changes on the following odd frame, then
     * the next even pass both consumes and replaces that motion. Preserve
     * that single deferred dispatch without delaying every behavior pass. */
    if ((tipoff->simulation_tick & 1u) == 0u) {
        if (acquisition_boundary) {
            tipoff->actor_behavior_pending = 1u;
            return;
        }
        cpu_update_actor_behaviors(tipoff);
        return;
    }
    if (tipoff->actor_behavior_pending != 0u) {
        tipoff->actor_behavior_pending = 0u;
        cpu_update_actor_behaviors(tipoff);
    }
}

static int cpu_select_inbound_receiver(const NbaTipoff *tipoff,
                                       uint8_t inbounder) {
    NbaGameplayReceiverState actors[NBA_GAMEPLAY_ACTOR_COUNT];
    for (unsigned i=0;i<NBA_GAMEPLAY_ACTOR_COUNT;++i) {
        actors[i].x=fp_integer_word(tipoff->actors[i].x_fp);
        actors[i].y=fp_integer_word(tipoff->actors[i].y_fp);
        actors[i].control_mode=tipoff->actors[i].control_mode;
        actors[i].travel_direction=tipoff->actors[i].assignment_direction;
        actors[i].travel_distance=tipoff->actors[i].pair_distance;
    }
    return nba_gameplay_select_inbound_receiver_cpu(
        inbounder,tipoff->inbound_timer_raw,
        tipoff->team_context[inbounder / 5u].anchor_x_raw_0a,
        tipoff->play_selector_raw,
        actors,NBA_GAMEPLAY_ACTOR_COUNT);
}

static bool cpu_inbound_formation_override_self_test(void) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    state.live_state_raw = 0x82u;
    state.play_code = 6u;
    state.inbound_target_x_raw = -100;
    state.inbound_target_y_raw = 20;
    state.inbound_actor_raw = 9u;
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        state.actors[i].controller_assignment_raw = -1;
        state.actors[i].target_x = (int16_t)(100 + (int)i);
        state.actors[i].target_y = (int16_t)(200 + (int)i);
    }
    /* Role 4 is excluded by `$0954`; role 3 must be the exact descending
     * scan winner on the current (right) side. */
    if (cpu_apply_inbound_formation_override(&state, 5u) != 8 ||
        state.actors[8].target_x != -40 ||
        state.actors[8].target_y != 160 ||
        state.actors[9].target_x != 109 || state.actors[4].target_x != 104)
        return false;
    /* A human role is skipped, and all five native entry predicates fail
     * closed without mutating the prior winner. */
    state.actors[8].controller_assignment_raw = 0;
    state.actors[8].target_x = 108;
    state.actors[8].target_y = 208;
    if (cpu_apply_inbound_formation_override(&state, 5u) != 7 ||
        state.actors[7].target_x != -40 || state.actors[7].target_y != 160)
        return false;
    state.live_state_raw = 2u;
    state.actors[6].target_x = 106;
    if (cpu_apply_inbound_formation_override(&state, 5u) != -1 ||
        state.actors[6].target_x != 106) return false;
    state.live_state_raw = 0x82u;
    state.inbound_target_x_raw = 0;
    return cpu_apply_inbound_formation_override(&state, 5u) == -1;
}

/* `$87:9AA6-$9BCA`: an installed inbounder whose signed `$092E` expires
 * commits the five-second violation/dead-ball recovery. `$9B38` awards the
 * opposite side (`$093A ^ 5`), switches `$0956` to layout 5, restores the
 * clocks, demotes the old owner to mode 2, and clears signed `$093E` before
 * `$85:C37D` seeds the replacement target. */
static void cpu_reset_expired_inbound(NbaTipoff *tipoff) {
    uint8_t previous = tipoff->possession_actor >= 0 &&
            tipoff->possession_actor < NBA_GAMEPLAY_ACTOR_COUNT ?
        (uint8_t)tipoff->possession_actor : 0xFFu;
    NbaGameplayDeadBallReset reset = {
        .camera_side_group = tipoff->camera_side_group_raw,
        .owner_actor = previous < NBA_GAMEPLAY_ACTOR_COUNT ? previous : 0xffffu,
        .ball_x = fp_integer_word(tipoff->ball.x_fp),
        .ball_y = fp_integer_word(tipoff->ball.y_fp),
        .ball_velocity_x = tipoff->ball.velocity_x,
        .ball_velocity_y = tipoff->ball.velocity_y,
        .owner_mode = previous < NBA_GAMEPLAY_ACTOR_COUNT ?
            tipoff->actors[previous].control_mode : 0xffffu
    };
    nba_gameplay_dead_ball_reset(&reset);
    if (previous < NBA_GAMEPLAY_ACTOR_COUNT)
        tipoff->actors[previous].control_mode = (uint8_t)reset.owner_mode;
    tipoff->live_state_raw = reset.live_state;
    tipoff->inbound_timer_raw = reset.inbound_timer;
    tipoff->role_rebuild_raw_09d6 = reset.role_rebuild_timer;
    tipoff->rim_raw_092c = reset.game_clock;
    tipoff->shot_clock_mirror_raw_09c6 = reset.shot_clock_mirror;
    tipoff->dead_ball_raw_0968 = reset.dead_ball;
    tipoff->rim_raw_096a = reset.ball_aux;
    tipoff->dead_ball_x_raw_09b0 = reset.dead_ball_x;
    tipoff->dead_ball_y_raw_09b2 = reset.dead_ball_y;
    tipoff->ball.velocity_x = reset.ball_velocity_x;
    tipoff->ball.velocity_y = reset.ball_velocity_y;
    tipoff->rim_raw_097c = reset.rim_state;
    unsigned side_group = reset.award_side_group == 5u ? 5u : 0u;
    unsigned side = side_group / 5u;
    tipoff->inbound_state_raw = (uint16_t)side_group;
    tipoff->inbound_layout_raw = 5;
    tipoff->inbound_actor_raw = (uint16_t)(side_group + 2u);
    tipoff->possession_actor = -1;
    tipoff->possession_team = (int8_t)side;
    tipoff->offense_side = (uint8_t)side;
    tipoff->handler_actor = (uint8_t)tipoff->inbound_actor_raw;
    tipoff->receiver_actor = (uint8_t)(side_group + 4u);
    tipoff->inbound_ready_raw = 0u;
    tipoff->inbound_transfer_raw = 0u;
    tipoff->ball.owner_actor = -1;
    NbaGameplayInboundTarget target;
    int16_t context_anchor = tipoff->team_context[side].anchor_x_raw_0a;
    if (nba_gameplay_inbound_target(
            5, tipoff->dead_ball_x_raw_09b0,
            tipoff->dead_ball_y_raw_09b2, context_anchor,
            fp_integer_word(tipoff->ball.x_fp), &tipoff->rng,
            &target)) {
        tipoff->inbound_target_x_raw = target.x;
        tipoff->inbound_target_y_raw = target.y;
        tipoff->inbound_direction_raw = target.direction;
        if (target.play_requested) {
            tipoff->play_code = target.play_code;
            tipoff->play_request_raw = 1u;
        }
        tipoff->actors[tipoff->inbound_actor_raw].target_x = target.x;
        tipoff->actors[tipoff->inbound_actor_raw].target_y = target.y;
    }
}

static bool cpu_expired_inbound_self_test(void) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    nba_gameplay_rng_seed(&state.rng, 0x9146u);
    state.camera_side_group_raw = 0u;
    state.inbound_actor_raw = 3u;
    state.actors[3].control_mode = 11u;
    state.ball.x_fp = 394 * 256;
    state.ball.y_fp = -52 * 256;
    state.possession_actor = 3;
    cpu_reset_expired_inbound(&state);
    if (state.actors[3].control_mode != 2u ||
        state.inbound_state_raw != 5u || state.inbound_layout_raw != 5 ||
        state.inbound_actor_raw != 7u || state.inbound_timer_raw != 300u ||
        state.possession_actor != -1 || state.possession_team != 1 ||
        state.inbound_target_x_raw != 394 ||
        state.inbound_target_y_raw != -52 ||
        state.inbound_direction_raw != 6u) return false;

    memset(&state, 0, sizeof(state));
    nba_gameplay_rng_seed(&state.rng, 0x9146u);
    state.camera_side_group_raw = 5u;
    state.inbound_actor_raw = 7u;
    state.actors[7].control_mode = 11u;
    state.ball.x_fp = -394 * 256;
    state.ball.y_fp = 52 * 256;
    state.possession_actor = 7;
    cpu_reset_expired_inbound(&state);
    if (!(state.actors[7].control_mode == 2u &&
           state.inbound_state_raw == 0u && state.inbound_layout_raw == 5 &&
           state.inbound_actor_raw == 2u && state.inbound_timer_raw == 300u &&
           state.possession_actor == -1 && state.possession_team == 0 &&
           state.inbound_target_x_raw == -394 &&
           state.inbound_target_y_raw == 52 &&
           state.inbound_direction_raw == 2u)) return false;

    /* `$87:9B88-$9B91` snapshots integer words before C37D. A positive
     * 361.996 coordinate remains below layout 5's X>=362 edge test. */
    memset(&state, 0, sizeof(state));
    nba_gameplay_rng_seed(&state.rng, 0x9146u);
    state.camera_side_group_raw = 0u;
    state.team_context[1].anchor_x_raw_0a = 336;
    state.ball.x_fp = 361 * 256 + 255;
    state.possession_actor = 3;
    state.actors[3].control_mode = 11u;
    cpu_reset_expired_inbound(&state);
    return state.dead_ball_x_raw_09b0 == 361 &&
           state.inbound_target_x_raw == 361 &&
           state.inbound_target_y_raw == 0;
}

typedef struct {
    int16_t x, y;
    uint8_t direction;
} CpuFreeThrowTarget;

/* `$87:A15C-$A360`: the shooting side occupies four lane records plus the
 * stripe record; each player's +$76 pair occupies the matching defender
 * record.  Only X mirrors when the shooting basket is on the left. */
static void cpu_free_throw_targets(const NbaTipoff *tipoff, uint8_t shooter,
                                   CpuFreeThrowTarget out[10]) {
    static const CpuFreeThrowTarget primary[5] = {
        {298, -56, 0}, {298, 56, 4}, {112, -104, 7},
        {112, 88, 6}, {210, 0, 6}
    };
    static const CpuFreeThrowTarget paired[5] = {
        {330, -56, 0}, {330, 56, 4}, {136, -104, 7},
        {136, 104, 6}, {274, -56, 0}
    };
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        out[i] = (CpuFreeThrowTarget){fp_round(tipoff->actors[i].x_fp),
                                     fp_round(tipoff->actors[i].y_fp),
                                     tipoff->actors[i].direction};
    unsigned base = (shooter / 5u) * 5u;
    unsigned shooter_local = shooter % 5u;
    for (unsigned local = 0; local < 5u; ++local) {
        unsigned actor = base + local;
        unsigned record = local == shooter_local ? 4u :
            (local > shooter_local ? local - 1u : local);
        out[actor] = primary[record];
        unsigned pair = tipoff->actors[actor].assignment_base_raw >> 1;
        if (pair < NBA_GAMEPLAY_ACTOR_COUNT) out[pair] = paired[record];
    }
    if (shooter < 5u) {
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
            out[i].x = (int16_t)-out[i].x;
    }
}

/* `$85:B3AA` with speed argument three, consumed on the 30-Hz actor pass.
 * The ROM's dt=2 makes the largest world-coordinate step six pixels. */
static bool cpu_free_throw_move_actor(NbaTipoffActor *actor,
                                      const CpuFreeThrowTarget *target) {
    int x = fp_round(actor->x_fp), y = fp_round(actor->y_fp);
    int dx = target->x - x, dy = target->y - y;
    int step_x = dx < -6 ? -6 : dx > 6 ? 6 : dx;
    int step_y = dy < -6 ? -6 : dy > 6 ? 6 : dy;
    if (dx == 0 && dy == 0) {
        actor->velocity_x = actor->velocity_y = 0;
        actor->movement_magnitude_raw = 0u;
        actor->direction = target->direction;
        actor->requested_direction = target->direction;
        return true;
    }
    actor->velocity_x = (int16_t)(step_x * 128);
    actor->velocity_y = (int16_t)(step_y * 128);
    actor->x_fp += (int32_t)step_x * 256;
    actor->y_fp += (int32_t)step_y * 256;
    actor->movement_magnitude_raw = actor_distance(step_x, step_y);
    actor->direction = nba_gameplay_target_direction(
        (int16_t)dx, (int16_t)dy, NULL);
    actor->requested_direction = actor->direction;
    return false;
}

static uint8_t cpu_free_throw_rating(const NbaTipoff *tipoff,
                                     uint8_t shooter) {
    uint8_t rating = 0x80u;
    uint8_t team = team_id_for_context(tipoff, shooter / 5u);
    (void)nba_player_gameplay_free_throw_rating(
        tipoff->assets, team, tipoff->actors[shooter].roster_slot, &rating);
    return rating;
}

static bool cpu_free_throw_launch_vector(uint8_t rating, uint8_t roll,
                                         uint8_t choice, uint8_t half,
                                         bool left_basket, int16_t *vx,
                                         int16_t *vy, int16_t *vz) {
    static const int16_t miss[2][4][3] = {
        {{512, 0, 800}, {608, 0, 800}, {576, 0, 848}, {608, 0, 800}},
        {{512, 0, 800}, {512, 0, 800}, {544, 0, 864}, {512, 0, 880}}
    };
    bool missed = roll >= nba_gameplay_free_throw_threshold(rating);
    unsigned table_half = half != 0u;
    unsigned table_choice = choice & 3u;
    *vx = missed ? miss[table_half][table_choice][0] : 512;
    *vy = missed ? miss[table_half][table_choice][1] : 0;
    *vz = missed ? miss[table_half][table_choice][2] : 864;
    if (left_basket) *vx = (int16_t)-*vx;
    return missed;
}

/* The stripe scene shares 9D6E, including A2A7's second RNG draw on BOTH
 * make/miss paths. No separate host probability or launch table remains. */
static void cpu_release_free_throw(NbaTipoff *tipoff, uint8_t shooter) {
    NbaTipoffActor *actor=&tipoff->actors[shooter];
    ball_position_at_actor(tipoff,shooter);
    tipoff->shot_origin_x=fp_integer_word(actor->x_fp);
    tipoff->shot_origin_y=fp_integer_word(actor->y_fp);
    tipoff->shot_result_resolved=false;
    cpu_release_rom_shot(tipoff,shooter);
    actor->reaction_threshold=actor->behavior_flags_raw=0;
}

/* Host launch scaffold shared by the existing CPU path and the dormant human
 * adapter. Native `$87:9F11-$9F5F` additionally clears actor +$4A, cancels
 * both animation channels, and falls through into the state-nine body in the
 * same actor call; those effects remain outside this bounded translation. */
static void free_throw_enter_launch_state(NbaTipoff *tipoff,
                                          NbaTipoffActor *actor) {
    tipoff->fouls.free_throw_state_raw_0978 = 9u;
    tipoff->ball_activity_raw = 1u;
    actor_set_animation(actor, 22u, 22u);
    actor->behavior_flags_raw |= 4u;
    actor->control_mode = 20u;
}

/* `$87:9CBF-$A017`: bounded free-throw scene translation. CPU states
 * 1/2/3/9/10 and the controller-owned 3/4/5 aiming sequence retain their
 * represented gameplay words. Aim artwork `$0988-$098E`, PPU scheduling,
 * and the complete common-launch effects/order described above remain
 * outside this typed gameplay boundary. */
static bool cpu_update_free_throw_scene(NbaTipoff *tipoff) {
    uint16_t *state = &tipoff->fouls.free_throw_state_raw_0978;
    if (*state == 0u) return false;
    if ((tipoff->simulation_tick & 1u) != 0u) return true;
    uint8_t shooter = tipoff->fouls.victim_actor_raw >= 0 &&
                      tipoff->fouls.victim_actor_raw < NBA_GAMEPLAY_ACTOR_COUNT ?
        (uint8_t)tipoff->fouls.victim_actor_raw : tipoff->handler_actor;
    if (shooter >= NBA_GAMEPLAY_ACTOR_COUNT) shooter = 0u;
    tipoff->handler_actor = shooter;
    tipoff->offense_side = shooter / 5u;
    tipoff->possession_team = (int8_t)tipoff->offense_side;

    if (*state == 1u) {
        CpuFreeThrowTarget targets[10];
        cpu_free_throw_targets(tipoff, shooter, targets);
        tipoff->rim_raw_097c = 0u;
        tipoff->pass_receiver_raw = -1;
        tipoff->fouls.whistle_active_raw_09b6 = 0u;
        /* $87:9CDE-9CE6 reaches BC9B for the designated shooter before
         * A15C moves him to the free-throw position. */
        (void)nba_tipoff_transfer_controller(tipoff,shooter);
        if (tipoff->ball.owner_actor >= 0 &&
            tipoff->ball.owner_actor != (int8_t)shooter) {
            ball_position_at_actor(tipoff, (unsigned)tipoff->ball.owner_actor);
            tipoff->ball.owner_actor = -1;
            tipoff->possession_actor = -1;
            tipoff->ball.state = NBA_BALL_BOUNCE;
        }
        bool ready = true;
        for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
            CpuFreeThrowTarget target = targets[actor];
            if (actor == shooter && tipoff->ball.owner_actor < 0) {
                target.x = fp_round(tipoff->ball.x_fp);
                target.y = fp_round(tipoff->ball.y_fp);
            }
            if (!cpu_free_throw_move_actor(&tipoff->actors[actor], &target))
                ready = false;
            cpu_advance_actor_animation(tipoff, &tipoff->actors[actor]);
        }
        if (tipoff->ball.owner_actor < 0) {
            (void)cpu_update_live_ball(tipoff);
            if (cpu_actor_body_contacts_ball(tipoff, shooter, true, 16u)) {
                tipoff->possession_actor = (int8_t)shooter;
                tipoff->ball.owner_actor = (int8_t)shooter;
                tipoff->ball.state = NBA_BALL_ATTACHED;
                tipoff->actors[shooter].control_mode = 11u;
                ball_attach_to_actor(tipoff, shooter);
            }
            ready = false;
        } else if (tipoff->ball.owner_actor == (int8_t)shooter) {
            ball_attach_to_actor(tipoff, shooter);
        }
        if (ready && tipoff->possession_actor >= 0 &&
            tipoff->fouls.whistle_timer_raw_08de < 0) {
            tipoff->free_throw_start_tick_raw_09be =
                (uint16_t)tipoff->simulation_tick;
            tipoff->shot_origin_x = fp_round(tipoff->actors[shooter].x_fp);
            tipoff->shot_origin_y = fp_round(tipoff->actors[shooter].y_fp);
            tipoff->free_throw_upload_raw_180b = 0xA046u;
            tipoff->free_throw_upload_raw_180c = 0x87A0u;
            if (tipoff->controllers.count[tipoff->offense_side] != 0u) {
                NbaGameplayHumanFreeThrowAim human = {
                    .aim_x_raw_0980 = tipoff->free_throw_aim_x_raw_0980,
                    .aim_y_raw_0982 = tipoff->free_throw_aim_y_raw_0982,
                    .accumulator_raw_0984 =
                        tipoff->free_throw_aim_accumulator_raw_0984,
                    .step_raw_0986 = tipoff->free_throw_aim_step_raw_0986
                };
                nba_gameplay_free_throw_human_aim_begin(
                    &human, cpu_free_throw_rating(tipoff, shooter));
                tipoff->free_throw_aim_x_raw_0980 = human.aim_x_raw_0980;
                tipoff->free_throw_aim_accumulator_raw_0984 =
                    human.accumulator_raw_0984;
                tipoff->free_throw_aim_step_raw_0986 = human.step_raw_0986;
            }
            /* `$87:9D6C` publishes transient state two, then `$85:9530`
             * consumes it in the same native actor pass. The retained CPU
             * path selects commentary word $1B; the wider roster-dependent
             * $1F/$23 selector remains outside this completion slice. */
            NbaGameplayFreeThrowCompletion completion = {
                2u, tipoff->fouls.free_throw_sequence_raw_097a,
                tipoff->fouls.whistle_timer_raw_08de,
                tipoff->fouls.whistle_state_raw_08e6,
                tipoff->fouls.whistle_state_mirror_raw_08e8,
                tipoff->free_throw_upload_raw_180b,
                tipoff->free_throw_upload_raw_180c, 0u, 0, 0, 0, 0, 0
            };
            bool clock_changed = tipoff->match_clock_raw_0928 !=
                                 tipoff->free_throw_clock_mirror_raw_493f;
            (void)nba_gameplay_free_throw_presentation_gate(
                &completion, clock_changed, 0x001Bu);
            if (clock_changed)
                tipoff->free_throw_clock_mirror_raw_493f =
                    tipoff->match_clock_raw_0928;
            *state = completion.state_raw_0978;
            tipoff->fouls.whistle_timer_raw_08de =
                completion.whistle_timer_raw_08de;
            tipoff->fouls.whistle_state_raw_08e6 = completion.audio_raw_08e6;
            tipoff->fouls.whistle_state_mirror_raw_08e8 =
                completion.audio_mirror_raw_08e8;
        }
        return true;
    }
    if (*state == 3u || *state == 4u || *state == 5u) {
        NbaTipoffActor *actor = &tipoff->actors[shooter];
        if (*state == 3u) actor_set_upper_animation(actor, 2u);
        bool human_path = actor->controller_assignment_raw >= 0 &&
            tipoff->controllers.count[tipoff->offense_side] != 0u;
        if (*state == 4u || *state == 5u || human_path) {
            NbaGameplayHumanFreeThrowAim human = {
                .state_raw_0978 = *state,
                .aim_x_raw_0980 = tipoff->free_throw_aim_x_raw_0980,
                .aim_y_raw_0982 = tipoff->free_throw_aim_y_raw_0982,
                .accumulator_raw_0984 =
                    tipoff->free_throw_aim_accumulator_raw_0984,
                .step_raw_0986 = tipoff->free_throw_aim_step_raw_0986,
                .controller_assignment_raw_16 =
                    actor->controller_assignment_raw,
                .human_context_raw_3b =
                    tipoff->controllers.count[tipoff->offense_side],
                .shoot_held = actor->controller_assignment_raw >= 0 &&
                    actor->controller_assignment_raw < 5 &&
                    (tipoff->controllers.record[actor->controller_assignment_raw].held & 0xC000u) != 0u
            };
            NbaGameplayHumanFreeThrowResult result =
                nba_gameplay_free_throw_human_aim_step_frame(&human);
            *state = human.state_raw_0978;
            tipoff->free_throw_aim_x_raw_0980 = human.aim_x_raw_0980;
            tipoff->free_throw_aim_y_raw_0982 = human.aim_y_raw_0982;
            tipoff->free_throw_aim_accumulator_raw_0984 =
                human.accumulator_raw_0984;
            tipoff->free_throw_aim_step_raw_0986 = human.step_raw_0986;
            if (result == NBA_HUMAN_FREE_THROW_LAUNCH) {
                free_throw_enter_launch_state(tipoff, actor);
                return true;
            }
            if (result != NBA_HUMAN_FREE_THROW_CPU_FALLBACK) return true;
            /* State four can lose its controller between calls. Native resets
             * it to state three and re-enters the CPU timing branch in the
             * same actor pass. */
            actor_set_upper_animation(actor, 2u);
        }
        /* `$87:9D95-$9DA3` selects the human branch before this CPU-only
         * `$87:9DA6` delay gate. */
        uint16_t elapsed = (uint16_t)(tipoff->simulation_tick -
                                      tipoff->free_throw_start_tick_raw_09be);
        if (elapsed < 120u) return true;
        actor_set_upper_animation(actor, 12u);
        NbaGameplayFreeThrowCompletion completion = {0};
        completion.state_raw_0978 = 3u;
        if (!nba_gameplay_free_throw_cpu_aim_step(
                &completion, &tipoff->rng, elapsed,
                cpu_free_throw_rating(tipoff, shooter),
                &tipoff->free_throw_aim_x_raw_0980,
                &tipoff->free_throw_aim_y_raw_0982)) return true;
        *state = completion.state_raw_0978;
        free_throw_enter_launch_state(tipoff, actor);
        return true;
    }
    if (*state == 9u) {
        NbaTipoffActor *actor = &tipoff->actors[shooter];
        cpu_advance_actor_animation(tipoff, actor);
        if (actor->lower_animation_tick < 6u) {
            ball_attach_to_actor(tipoff, shooter);
            return true;
        }
        cpu_release_free_throw(tipoff, shooter);
        NbaGameplayFreeThrowCompletion completion = {
            9u, tipoff->fouls.free_throw_sequence_raw_097a,
            tipoff->fouls.whistle_timer_raw_08de,
            tipoff->fouls.whistle_state_raw_08e6,
            tipoff->fouls.whistle_state_mirror_raw_08e8,
            tipoff->free_throw_upload_raw_180b,
            tipoff->free_throw_upload_raw_180c, 0u, 0, 0, 0, 0, 0
        };
        (void)nba_gameplay_free_throw_release_complete(&completion);
        *state = completion.state_raw_0978;
        tipoff->fouls.free_throw_sequence_raw_097a =
            completion.attempts_raw_097a;
        tipoff->free_throw_upload_raw_180b = completion.upload_raw_180b;
        tipoff->free_throw_upload_raw_180c = completion.upload_raw_180c;
        return true;
    }
    if (*state == 10u) {
        NbaGameplayRimResult result = cpu_update_live_ball(tipoff);
        /* A make already ran `$85:A079-$A345` inline in its physics
         * substep. Only terminal misses need host-scene acknowledgement. */
        if (!tipoff->shot_result_resolved &&
            (result == NBA_GAMEPLAY_RIM_EDGE_CONTACT ||
             result == NBA_GAMEPLAY_RIM_MISS)) {
            tipoff->shot_result_resolved = true;
        }
        NbaGameplayFreeThrowCompletion completion = {0};
        completion.state_raw_0978 = 10u;
        completion.attempts_raw_097a =
            tipoff->fouls.free_throw_sequence_raw_097a;
        completion.ball_x_raw_3eef = fp_integer_word(tipoff->ball.x_fp);
        completion.ball_y_raw_3ef3 = fp_integer_word(tipoff->ball.y_fp);
        completion.ball_z_raw_3ef7 =
            (uint16_t)fp_integer_word(tipoff->ball.z_fp);
        completion.ball_vx_raw_3ef9 = tipoff->ball.velocity_x;
        completion.ball_vy_raw_3efb = tipoff->ball.velocity_y;
        completion.ball_vz_raw_3efd = tipoff->ball.velocity_z;
        if (tipoff->fouls.free_throw_sequence_raw_097a != 0u) {
            if (!nba_gameplay_free_throw_resolution_step(
                &completion, true, tipoff->ball.owner_actor < 0,
                tipoff->shot_value_raw, tipoff->rim_raw_097c,
                tipoff->free_throw_resolution_raw_0972,
                fp_integer_word(tipoff->actors[shooter].x_fp),
                fp_integer_word(tipoff->actors[shooter].y_fp))) return true;
            *state = completion.state_raw_0978;
            /* `$3EEF/$3EF3/$3EF7/$3EF9/$3EFB/$3EFD` are the gameplay-ball
             * integer position and velocity words. Preserve their fractional
             * bytes while applying the native between-attempt placement. */
            tipoff->ball.x_fp = fp_replace_integer_word(
                tipoff->ball.x_fp, completion.ball_x_raw_3eef);
            tipoff->ball.y_fp = fp_replace_integer_word(
                tipoff->ball.y_fp, completion.ball_y_raw_3ef3);
            tipoff->ball.z_fp = fp_replace_integer_word(
                tipoff->ball.z_fp, (int16_t)completion.ball_z_raw_3ef7);
            tipoff->ball.velocity_x = completion.ball_vx_raw_3ef9;
            tipoff->ball.velocity_y = completion.ball_vy_raw_3efb;
            tipoff->ball.velocity_z = completion.ball_vz_raw_3efd;
            tipoff->ball.owner_actor = -1;
            tipoff->ball.state = NBA_BALL_LOOSE;
            return true;
        }
        (void)nba_gameplay_free_throw_resolution_step(
            &completion, true, tipoff->ball.owner_actor < 0,
            tipoff->shot_value_raw, tipoff->rim_raw_097c,
            tipoff->free_throw_resolution_raw_0972,
            fp_integer_word(tipoff->actors[shooter].x_fp),
            fp_integer_word(tipoff->actors[shooter].y_fp));
        *state = completion.state_raw_0978;
        return true;
    }
    if (*state >= 11u) {
        NbaGameplayFreeThrowCompletion completion = {0};
        completion.state_raw_0978 = *state;
        (void)nba_gameplay_free_throw_resolution_step(
            &completion, true, true, 0u, 0u, 0u, 0, 0);
        *state = completion.state_raw_0978;
        return true;
    }
    return true;
}

static bool cpu_free_throw_scene_self_test(void) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        state.actors[i].assignment_base_raw =
            (uint16_t)(((i + 5u) % NBA_GAMEPLAY_ACTOR_COUNT) * 2u);
    CpuFreeThrowTarget target[10];
    cpu_free_throw_targets(&state, 2u, target);
    static const CpuFreeThrowTarget expected_left[10] = {
        {-298, -56, 0}, {-298, 56, 4}, {-210, 0, 6},
        {-112, -104, 7}, {-112, 88, 6}, {-330, -56, 0},
        {-330, 56, 4}, {-274, -56, 0}, {-136, -104, 7},
        {-136, 104, 6}
    };
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        if (target[i].x != expected_left[i].x ||
            target[i].y != expected_left[i].y ||
            target[i].direction != expected_left[i].direction)
            return false;
    /* The same local-slot layout mirrors to the positive basket; paired
     * defenders occupy the opposite five physical actor records. */
    cpu_free_throw_targets(&state, 7u, target);
    static const CpuFreeThrowTarget expected_right[10] = {
        {330, -56, 0}, {330, 56, 4}, {274, -56, 0},
        {136, -104, 7}, {136, 104, 6}, {298, -56, 0},
        {298, 56, 4}, {210, 0, 6}, {112, -104, 7},
        {112, 88, 6}
    };
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        if (target[i].x != expected_right[i].x ||
            target[i].y != expected_right[i].y ||
            target[i].direction != expected_right[i].direction)
            return false;
    if (nba_gameplay_free_throw_threshold(0x80u) != 130u ||
        nba_gameplay_free_throw_threshold(0xFFu) != 245u) return false;
    if (nba_gameplay_free_throw_human_aim_step(0x00u) != 0x0326u ||
        nba_gameplay_free_throw_human_aim_step(0x7Fu) != 0x0228u ||
        nba_gameplay_free_throw_human_aim_step(0x80u) != 0x0226u ||
        nba_gameplay_free_throw_human_aim_step(0xFFu) != 0x0128u)
        return false;
    NbaGameplayHumanFreeThrowAim human = {
        .state_raw_0978 = 3u,
        .controller_assignment_raw_16 = 0,
        .human_context_raw_3b = 1u
    };
    nba_gameplay_free_throw_human_aim_begin(&human, 0x80u);
    if (human.step_raw_0986 != 0x0226u ||
        nba_gameplay_free_throw_human_aim_step_frame(&human) !=
            NBA_HUMAN_FREE_THROW_WAIT ||
        human.aim_x_raw_0980 != 5u || human.accumulator_raw_0984 != 0u)
        return false;
    human.shoot_held = true;
    if (nba_gameplay_free_throw_human_aim_step_frame(&human) !=
            NBA_HUMAN_FREE_THROW_FIRST_LOCK ||
        human.state_raw_0978 != 4u || human.aim_x_raw_0980 != 0u ||
        human.aim_y_raw_0982 != 10u) return false;
    human.shoot_held = false;
    if (nba_gameplay_free_throw_human_aim_step_frame(&human) !=
            NBA_HUMAN_FREE_THROW_RELEASED_FIRST ||
        human.state_raw_0978 != 5u || human.aim_x_raw_0980 != 5u) return false;
    human.shoot_held = true;
    if (nba_gameplay_free_throw_human_aim_step_frame(&human) !=
            NBA_HUMAN_FREE_THROW_LAUNCH ||
        human.state_raw_0978 != 9u || human.aim_x_raw_0980 != 10u)
        return false;
    /* Exercise the scene adapter, not only the typed helper. In particular,
     * state three must choose human aim before the CPU-only 120-tick gate. */
    NbaTipoff live = {0};
    live.fouls.free_throw_state_raw_0978 = 3u;
    live.fouls.victim_actor_raw = 0;
    live.actors[0].controller_assignment_raw = 0;
    live.controllers.count[0] = 1u;
    live.free_throw_aim_step_raw_0986 = 0x0226u;
    live.simulation_tick = 2u;
    live.free_throw_start_tick_raw_09be = 2u;
    live.controllers.record[0].held = 0x8000u;
    if (!cpu_update_free_throw_scene(&live) ||
        live.fouls.free_throw_state_raw_0978 != 4u ||
        live.free_throw_aim_x_raw_0980 != 0u ||
        live.free_throw_aim_y_raw_0982 != 5u) return false;
    live.controllers.record[0].held = 0u;
    if (!cpu_update_free_throw_scene(&live) ||
        live.fouls.free_throw_state_raw_0978 != 5u ||
        live.free_throw_aim_x_raw_0980 != 5u) return false;
    live.controllers.record[0].held = 0x4000u;
    if (!cpu_update_free_throw_scene(&live) ||
        live.fouls.free_throw_state_raw_0978 != 9u ||
        live.free_throw_aim_x_raw_0980 != 10u ||
        live.actors[0].control_mode != 20u) return false;
    int16_t vx, vy, vz;
    if (cpu_free_throw_launch_vector(
            0xFFu, 0u, 3u, 1u, false, &vx, &vy, &vz) ||
        vx != 512 || vy != 0 || vz != 864) return false;
    if (cpu_free_throw_launch_vector(
            0xFFu, 0u, 0u, 0u, true, &vx, &vy, &vz) ||
        vx != -512 || vy != 0 || vz != 864) return false;
    if (!cpu_free_throw_launch_vector(
            0x80u, 130u, 1u, 0u, false, &vx, &vy, &vz) ||
        vx != 608 || vy != 0 || vz != 800) return false;
    state.fouls.free_throw_state_raw_0978 = 24u;
    state.handler_actor = 0u;
    state.simulation_tick = 2u;
    if (!cpu_update_free_throw_scene(&state) ||
        state.fouls.free_throw_state_raw_0978 != 1u) return false;
    return true;
}

/* `$86:F43A-$F653`: execute the inbound arrival/candidate/pass gates. The
 * surrounding 60-Hz clock decrements `$092E`; this 30-Hz actor pass reloads
 * it to 300 whenever the raw target box has not been reached. */
static void cpu_update_rom_inbound(NbaTipoff *tipoff) {
    /* `$86:F3D2` reaches F43A through the current actor dispatch (`X/$96`).
     * It does not compare that actor with provisional `$0954`. After an
     * A613 boundary cancellation, D353->BAA2 may install a teammate in
     * ownership `$093E`; that mode-11 carrier must be allowed to approach
     * `$0958/$095A` and retry even while `$0954` names the old inbounder. */
    if (tipoff->possession_actor < 0 ||
        tipoff->possession_actor >= NBA_GAMEPLAY_ACTOR_COUNT) return;
    uint8_t inbounder = (uint8_t)tipoff->possession_actor;
    NbaTipoffActor *actor = &tipoff->actors[inbounder];
    /* `$86:F3F6-$F43A` reaches the inbound mode-11 branch only for the
     * collision-installed `$093E` owner. `$0954` starts as provisional slot
     * 2/7 and must not manufacture possession merely by reaching `$0958/A`. */
    if (actor->control_mode != 11u) return;
    actor->target_x = tipoff->inbound_target_x_raw;
    actor->target_y = tipoff->inbound_target_y_raw;
    if ((tipoff->simulation_tick & 1u) != 0u) return;
    if (tipoff->inbound_transfer_raw != 0u &&
        actor->control_mode != 11u) return;
    if (!nba_gameplay_inbound_arrived(
            fp_integer_word(actor->x_fp), fp_integer_word(actor->y_fp),
            tipoff->inbound_target_x_raw, tipoff->inbound_target_y_raw)) {
        tipoff->inbound_timer_raw = 300u;
        return;
    }

    /* `$86:F54F-$F555`: arrival writes 2 to `$0968` and the currently
     * unrepresented `$09F6` before freezing the inbounder. */
    NbaGameplayInboundArrival arrival = {
        .dead_ball_raw_0968 = tipoff->dead_ball_raw_0968,
        .attachment_raw_09f6 = tipoff->attached_ball_state_raw_09f6,
        .behavior_flags_raw_7e = actor->behavior_flags_raw,
        .velocity_x_raw_0e = actor->velocity_x,
        .velocity_y_raw_10 = actor->velocity_y,
        .inbound_ready_raw_09ba = tipoff->inbound_ready_raw,
        .whistle_raw_09b6 = tipoff->fouls.whistle_active_raw_09b6,
        .foul_event_raw_0964 = tipoff->fouls.foul_event_raw_0964,
        .transfer_raw_09b8 = tipoff->inbound_transfer_raw,
        /* `$86:F57A` rereads raw `$0946`, not the host receiver cache.
         * `$86:C48F` can cancel a knocked-down receiver while the cache
         * still names it; F57F must then clear the active `$09B8` transfer
         * so this mode-11 inbound carrier can select another receiver. */
        .receiver_actor_raw_0946 = tipoff->pass_receiver_raw,
        .inbound_direction_raw_095c = tipoff->inbound_direction_raw,
        .draw_direction_raw_4e = actor->direction,
    };
    nba_gameplay_inbound_arrival_prepare(&arrival);
    tipoff->dead_ball_raw_0968 = arrival.dead_ball_raw_0968;
    tipoff->attached_ball_state_raw_09f6 = arrival.attachment_raw_09f6;
    actor->behavior_flags_raw = arrival.behavior_flags_raw_7e;
    actor->velocity_x = arrival.velocity_x_raw_0e;
    actor->velocity_y = arrival.velocity_y_raw_10;
    tipoff->inbound_ready_raw = arrival.inbound_ready_raw_09ba;
    tipoff->fouls.whistle_active_raw_09b6 = arrival.whistle_raw_09b6;
    tipoff->fouls.foul_event_raw_0964 = arrival.foul_event_raw_0964;
    tipoff->inbound_transfer_raw = arrival.transfer_raw_09b8;
    actor->movement_magnitude_raw = 0u;
    actor->direction = arrival.draw_direction_raw_4e;
    /* `$86:F520-$F54E`: a human inbounder uses controller `$090C+$08`
     * through the ROM's direction table before returning. `$86:F58F` then
     * uses signed actor +$16, not actor Z, to enter the CPU selector. */
    if (actor->controller_assignment_raw >= 0) {
        uint16_t held = actor->controller_assignment_raw < 5 ?
            tipoff->controllers.record[actor->controller_assignment_raw].held : 0u;
        actor->special_contact_raw_56 = nba_gameplay_human_inbound_direction(
            actor->controller_assignment_raw, actor->movement_boost_timer,
            nba_controller_host_buttons(held), actor->special_contact_raw_56);
        return;
    }
    if ((int16_t)tipoff->inbound_timer_raw >= 240) return;
    uint16_t random = tipoff->inbound_timer_raw >= 120u ?
                      nba_gameplay_rng_next(&tipoff->rng) : 0u;
    if (!nba_gameplay_inbound_pass_due(
            tipoff->inbound_timer_raw, random)) return;
    int candidate = cpu_select_inbound_receiver(tipoff, inbounder);
    if (candidate < 0 || candidate >= NBA_GAMEPLAY_ACTOR_COUNT) return;
    actor->reaction_threshold = 1u; /* `$86:F60B-$F610` */
    if (nba_tipoff_begin_rom_pass(
            tipoff, inbounder, (unsigned)candidate)) {
        tipoff->receiver_actor = (uint8_t)candidate;
        cpu_enter_play_state(tipoff, NBA_CPU_PLAY_PASS);
    }
}

void nba_tipoff_replay_inbound_continuation(NbaTipoff *tipoff) {
    if (tipoff) cpu_update_rom_inbound(tipoff);
}

static bool cpu_inbound_recovery_carrier_self_test(
        const NbaAssetPack *assets, NbaSession *session) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    state.assets = assets;
    state.session = session;publish_exhibition_team_ids(&state);
    state.live_state_raw = 0x82u;
    state.inbound_actor_raw = 1u; /* provisional `$0954` */
    state.possession_actor = 3;   /* recovered `$093E` carrier */
    state.inbound_target_x_raw = 246;
    state.inbound_target_y_raw = -22;
    state.inbound_direction_raw = 6u;
    state.inbound_timer_raw = 300u;
    state.actors[3].control_mode = 11u;
    state.actors[3].controller_assignment_raw = -1;
    state.actors[3].x_fp = 200 * 256;
    state.actors[3].y_fp = -22 * 256;
    state.actors[3].z_fp = 7 * 256;
    if (!cpu_move_inbound_actor(&state, 3u) ||
        state.actors[3].velocity_x <= 0 ||
        state.actors[3].x_fp != 200 * 256 ||
        state.actors[3].y_fp != -22 * 256 ||
        state.actors[3].z_fp != 7 * 256) return false;
    /* Native F4F2 reads integer word +$04. At 237.255 the target delta is
     * still +9 and therefore outside the [-9,+8] box; nearest-pixel rounding
     * would incorrectly accept it as +8. */
    state.actors[3].x_fp = 237 * 256 + 255;
    state.actors[3].y_fp = -22 * 256;
    state.actors[3].z_fp = 0;
    state.actors[3].velocity_x = 0;
    state.actors[3].velocity_y = 0;
    state.inbound_timer_raw = 100u;
    cpu_update_rom_inbound(&state);
    if (state.inbound_ready_raw != 0u || state.inbound_timer_raw != 300u)
        return false;
    state.actors[3].x_fp = 246 * 256;
    state.actors[3].y_fp = -22 * 256;
    state.actors[3].z_fp = 0;
    state.actors[3].velocity_x = 100;
    state.actors[3].velocity_y = -100;
    cpu_update_rom_inbound(&state);
    return state.inbound_actor_raw == 1u &&
           state.inbound_ready_raw == 1u &&
           state.dead_ball_raw_0968 == 2u &&
           state.actors[3].direction == 6u &&
           state.actors[3].velocity_x == 0 &&
           state.actors[3].velocity_y == 0;
}

static void cpu_install_inbound_carrier(NbaTipoff *tipoff, int contact,
                                        bool first_pickup) {
    /* Initial dead-ball pickup is seeded by the `$85:A262` setup contract.
     * `$86:CFA0-$CFDE` has already applied its conditional `$0954` gate;
     * the later `$86:D353 -> BAA2` ownership install must not unconditionally
     * rewrite that selector, reseed `$092E`, or erase reached `$09BA` state.
     * Reinitializing the timer/latch here permits endless A613 cancellations. */
    NbaTipoffActor *inbounder = &tipoff->actors[contact];
    tipoff->handler_actor = (uint8_t)contact;
    tipoff->possession_actor = (int8_t)contact;
    tipoff->possession_team = (int8_t)(contact / 5);
    tipoff->offense_side = (uint8_t)(contact / 5);
    inbounder->control_mode = 11u;
    inbounder->reaction_threshold = 0u;
    inbounder->behavior_flags_raw &= 0xFFBFu;
    inbounder->target_x = tipoff->inbound_target_x_raw;
    inbounder->target_y = tipoff->inbound_target_y_raw;
    tipoff->inbound_transfer_raw = 0u;
    if (first_pickup) {
        tipoff->inbound_ready_raw = 0u;
        tipoff->inbound_timer_raw = 300u;
    }
}

static bool cpu_try_install_inbound_contact(NbaTipoff *tipoff) {
    if (tipoff->possession_actor >= 0 || tipoff->ball.owner_actor >= 0 ||
        tipoff->inbound_transfer_raw != 0u) return false;
    int contact = cpu_first_inbound_ball_contact(tipoff);
    if (contact < 0) return false;
    cpu_install_inbound_carrier(
        tipoff, contact, tipoff->inbound_ready_raw == 0u);
    return true;
}

static void cpu_update_possession(NbaTipoff *tipoff) {
    /* `$87:923D` globally diverts the actor pass to `$87:9CBF` while
     * `$0978` is active. This must precede the seeded `$0936=$82` inbound
     * path or that dead-ball scaffold steals the free-throw scene. */
    if (cpu_update_free_throw_scene(tipoff)) {
        /* $87:8FA1 -> $85:AF5C/B128 is a frame-level pass, not part of
         * the per-player free-throw diversion at $87:923D. A made stripe
         * basket can request a play even while further attempts remain. */
        if ((tipoff->simulation_tick & 1u)==0u) {
            cpu_cache_predicted_ball_xy(tipoff);
            nba_tipoff_update_play_control_end_frame(tipoff);
            nba_tipoff_refresh_team_roles_end_frame(tipoff);
        }
        ++tipoff->possession_frame;
        ++tipoff->play_state_frame;
        return;
    }
    if (tipoff->live_state_raw == 0x82u) {
        /* `$85:A262-$A268` seeds `$092E/$0A04=300`. `$86:F43A-$F653`
         * owns arrival, its sawtooth reload, receiver selection and AB2D. */
        if (tipoff->inbound_actor_raw < NBA_GAMEPLAY_ACTOR_COUNT) {
            /* `$85:C5AD-$C5BD` owns this target. `$85:AD86-$AD95` skips the
             * provisional `$0954` slot. A different `$093E` carrier restores
             * its own inbound target when F43A dispatches after role refresh. */
            tipoff->actors[tipoff->inbound_actor_raw].target_x =
                tipoff->inbound_target_x_raw;
            tipoff->actors[tipoff->inbound_actor_raw].target_y =
                tipoff->inbound_target_y_raw;
        }
        /* `$85:963D` completes the ten player integrations before the
         * `$86:D5DB/D652` sorted collision sweep. */
        cpu_update_all_actors(tipoff);
        (void)cpu_update_live_ball(tipoff);
        cpu_cache_predicted_ball_xy(tipoff);
        cpu_update_player_contacts(tipoff);
        nba_tipoff_update_play_control_end_frame(tipoff);
        nba_tipoff_refresh_team_roles_end_frame(tipoff);
        cpu_update_actor_behaviors(tipoff);
        if (tipoff->possession_actor >= 0 &&
            tipoff->possession_actor < NBA_GAMEPLAY_ACTOR_COUNT &&
            tipoff->inbound_transfer_raw == 0u) {
            /* The dead-ball carrier is pose-attached visually while the ROM
             * keeps the logical ball owner negative until `$86:AB2D`. */
            ball_position_at_actor(
                tipoff, (unsigned)tipoff->possession_actor);
            tipoff->ball.velocity_x = tipoff->ball.velocity_y = 0;
            tipoff->ball.velocity_z = 0;
            tipoff->ball.owner_actor = -1;
        }
        /* `$86:CCFC-$D43C` pose collision installs the actual dead-ball
         * inbounder after the completed player pass. */
        if (cpu_try_install_inbound_contact(tipoff)) {
            /* The collision installs WRAM `$093E`, not the ball-record owner.
             * Keep that distinction on the installation frame as well as
             * subsequent dead-ball frames. */
            ball_position_at_actor(
                tipoff, (unsigned)tipoff->possession_actor);
            tipoff->ball.velocity_x = tipoff->ball.velocity_y = 0;
            tipoff->ball.velocity_z = 0;
            tipoff->ball.owner_actor = -1;
        }
        /* The inbound transfer still terminates through the shared
         * `$86:BAA2-$BC99` pass-catch acquisition boundary. */
        if (tipoff->cpu_play_state == NBA_CPU_PLAY_PASS &&
            tipoff->ball.owner_actor < 0) {
            int contact = cpu_first_pass_contact(tipoff);
            if (contact >= 0) {
                cpu_commit_ball_acquisition(tipoff, (uint8_t)contact);
                /* `$86:D3C5` returns to the common `$85:A7A8/$87:B649`
                 * owned-ball tail. BAA2 itself deliberately leaves the ball
                 * record at its collision point; the tail projects it. */
                ball_position_at_actor(tipoff, (unsigned)contact);
            }
        }
        if (tipoff->cpu_play_state == NBA_CPU_PLAY_PASS &&
            tipoff->ball.state == NBA_BALL_BOUNCE) {
            cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
        }
        if (tipoff->live_state_raw == 0x82u &&
            tipoff->possession_actor >= 0 &&
            tipoff->inbound_transfer_raw == 0u)
            tipoff->ball.owner_actor = -1;
        /* A completed catch changes `$0936` away from `$82` at D365 and may
         * leave an attached host ball, so that path is done. A pre-release
         * receiver cancellation instead reaches A777 with `$0936=$82`,
         * attached ownership, stale `$09B8=1`, and raw `$0946=-1`; it must
         * continue through F43A/F57F, which clears the transfer. Returning
         * merely because the host ball record is attached deadlocks it. */
        if (tipoff->live_state_raw != 0x82u &&
            tipoff->ball.owner_actor >= 0) {
            ++tipoff->possession_frame;
            ++tipoff->play_state_frame;
            return;
        }
        if (tipoff->inbound_timer_raw > 0u)
            --tipoff->inbound_timer_raw;
        /* `$87:9AA6` is the global `$092E` expiry consumer; it does not
         * require `$093E` to have acquired an inbounder first. */
        if (tipoff->inbound_timer_raw == 0u &&
            tipoff->inbound_transfer_raw == 0u) {
            cpu_reset_expired_inbound(tipoff);
        }
        cpu_update_rom_inbound(tipoff);
        if (tipoff->live_state_raw == 0x82u &&
            tipoff->possession_actor >= 0 &&
            tipoff->inbound_transfer_raw == 0u)
            tipoff->ball.owner_actor = -1;
        ++tipoff->possession_frame;
        ++tipoff->play_state_frame;
        return;
    }
    /* $092C is advanced by $85:EE38, not the possession dispatcher. */
    cpu_update_all_actors(tipoff);
    NbaGameplayRimResult rim_result = cpu_update_live_ball(tipoff);
    cpu_cache_predicted_ball_xy(tipoff);
    int8_t owner_before_contacts = tipoff->ball.owner_actor;
    cpu_update_player_contacts(tipoff);
    bool detached_contact = cpu_try_detached_shot_contact(tipoff);
    if (!detached_contact) (void)cpu_try_owned_ball_contact(tipoff);
    if (cpu_live_loose_ball_contact_due(tipoff)) {
        int catcher = cpu_first_loose_ball_contact(tipoff);
        if (catcher >= 0)
            cpu_commit_ball_acquisition(tipoff, (uint8_t)catcher);
    }
    nba_tipoff_update_play_control_end_frame(tipoff);
    nba_tipoff_refresh_team_roles_end_frame(tipoff);

    switch ((NbaCpuPlayState)tipoff->cpu_play_state) {
        case NBA_CPU_PLAY_BREAK:
        case NBA_CPU_PLAY_DRIVE:
        case NBA_CPU_PLAY_ATTACK:
            /* Debug-only legacy labels. The ROM has no global
             * BREAK/DRIVE/ATTACK distance machine: mode 11 alone chooses
             * B678 shot, AD6B formation, then B50E pass. */
            break;
        case NBA_CPU_PLAY_PASS:
            /* `$86:D5DB/$D652->$86:CCFC-$D1D6`: stable-X collision order,
             * teammate rejection, receiver/opponent pose windows, and the
             * original low-Z DP-$00 interception bug. No floor auto-catch. */
            if (tipoff->ball.owner_actor < 0) {
                int contact = cpu_first_pass_contact(tipoff);
                if (contact >= 0)
                    cpu_commit_ball_acquisition(tipoff, (uint8_t)contact);
            }
            if (tipoff->ball.owner_actor < 0 &&
                tipoff->ball.state == NBA_BALL_BOUNCE)
                cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
            break;
        case NBA_CPU_PLAY_SHOT:
            if (rim_result == NBA_GAMEPLAY_RIM_MAKE) {
                /* Scoring is inline in the detecting physics substep. */
                cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
            } else if (!tipoff->shot_result_resolved &&
                       (rim_result == NBA_GAMEPLAY_RIM_EDGE_CONTACT ||
                        rim_result == NBA_GAMEPLAY_RIM_MISS)) {
                /* `$85:9DAC-$A006` has already applied the distinct edge or
                 * miss impulse and continued through same-tick integration;
                 * mode changes here only hand the result to loose-ball play. */
                tipoff->shot_result_resolved = true;
                tipoff->ball.state = NBA_BALL_BOUNCE;
                cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
            } else if (tipoff->ball.owner_actor < 0 &&
                tipoff->ball.z_fp <= 32 * 256 &&
                tipoff->ball.velocity_z < 0) {
                cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
                tipoff->ball.state = NBA_BALL_BOUNCE;
            }
            break;
        case NBA_CPU_PLAY_REBOUND:
            {
                int catcher = cpu_first_loose_ball_contact(tipoff);
                if (catcher >= 0)
                    cpu_commit_ball_acquisition(tipoff, (uint8_t)catcher);
            }
            break;
    }

    /* `$86:D3C5` completes inside the collision pass, then the caller reaches
     * the shared attached-ball tail. Keep that ordering outside the isolated
     * BAA2/D25A replay functions, whose captured exits precede this write. */
    if (owner_before_contacts < 0 && tipoff->ball.owner_actor >= 0)
        ball_position_at_actor(tipoff,
            (unsigned)tipoff->ball.owner_actor);

    cpu_schedule_actor_behaviors(
        tipoff, (tipoff->rim_raw_13e7 & 0x0010u) != 0u);

    /* A made basket can switch `$0936` to `$82` inside this live-ball pass,
     * after the dispatcher chose the ordinary branch above. If the same
     * collision sweep installs `$093E`, preserve the native separation from
     * the ball-record owner exactly as the dedicated dead-ball branch does. */
    if (tipoff->live_state_raw == 0x82u &&
        tipoff->possession_actor >= 0 &&
        tipoff->possession_actor < NBA_GAMEPLAY_ACTOR_COUNT &&
        tipoff->inbound_transfer_raw == 0u) {
        if (tipoff->inbound_actor_raw != (uint16_t)tipoff->possession_actor ||
            tipoff->actors[tipoff->possession_actor].control_mode != 11u)
            cpu_install_inbound_carrier(
                tipoff, tipoff->possession_actor,
                tipoff->inbound_ready_raw == 0u);
        ball_position_at_actor(tipoff, (unsigned)tipoff->possession_actor);
        tipoff->ball.velocity_x = tipoff->ball.velocity_y = 0;
        tipoff->ball.velocity_z = 0;
        tipoff->ball.owner_actor = -1;
    }

    ++tipoff->possession_frame;
    ++tipoff->play_state_frame;
}

static void cpu_update_camera(NbaTipoff *tipoff) {
    /* `$87:95AC-$95DE`: resolve BEFORE the presentation-clock wait, then
     * copy that record's coordinates when the wait completes. No frame-200
     * camera enable or frame-220 camera-subject substitution. The upstream
     * tip winner/ownership bridge still has its separately documented scope. */
    NbaGameplayCamera *camera = &tipoff->camera;
    if (!camera->caller_waiting) {
        camera->subject_pointer_0940 = nba_gameplay_camera_resolve(tipoff->possession_actor);
        camera->caller_waiting = true;
    }
    if (!nba_gameplay_camera_ready(&camera->presentation_ticks_0564)) return;
    camera->caller_waiting = false;
    NbaCameraInput in = {0};
    if (camera->subject_pointer_0940) {
        unsigned subject = (camera->subject_pointer_0940 - 0x34EBu) / 0x100u;
        in.subject = nba_gameplay_camera_subject(tipoff->actors[subject].x_fp,
                                                tipoff->actors[subject].y_fp);
    } else {
        in.subject = nba_gameplay_camera_subject(tipoff->ball.x_fp, tipoff->ball.y_fp);
    }
    nba_gameplay_camera_copy(camera,camera->subject_pointer_0940,&in.subject);
    /* 8E1C resolves again after the copy; it cannot replace the XY snapshot
     * already chosen by 95BB if the selector changed while waiting. */
    camera->subject_pointer_0940 = nba_gameplay_camera_resolve(tipoff->possession_actor);
    in.ball_height = fp_integer_word(tipoff->ball.z_fp);
    in.side_group = tipoff->camera_side_group_raw == 0xFFu ? -1 : tipoff->camera_side_group_raw;
    in.basket_left = tipoff->team_context[0].anchor_x_raw_0a;
    in.basket_right = tipoff->team_context[1].anchor_x_raw_0a;
    in.alternate_08bc = tipoff->camera_alternate_raw_08bc;
    in.alternate_mode_08cc = tipoff->camera_alternate_mode_raw_08cc;
    in.live_state = tipoff->live_state_raw;
    /* $87:95DB -> $85:8E1C: A9D0 resolves/copies the subject, CC10
     * dispatches one HUD child, then 9192 updates camera. No render-time
     * publication, fixed3/4-frame delay, or extra per-frame child flushing. */
    hud_dispatch(tipoff);
    nba_gameplay_camera_step(camera,&in); /* `$85:9192`: full raw camera input. */
    tipoff->camera_x = tipoff->camera.x;
    tipoff->camera_y = tipoff->camera.y;
    /* 85:8E28 -> 8EDD: camera commits before presentation and streaming. */
    nba_court_presentation_update(&tipoff->court_presentation,
        tipoff->camera_x,tipoff->camera_y,tipoff->period_raw_0926,
        tipoff->team_context[0].anchor_x_raw_0a,
        tipoff->team_context[1].anchor_x_raw_0a);
    (void)nba_court_stream_update(&tipoff->court_stream,tipoff->assets,
        tipoff->camera_x,tipoff->camera_y,tipoff->camera.previous_x,
        tipoff->camera.previous_y,NULL,NULL);
    tipoff->court_stream.scroll_x=tipoff->court_stream.next_scroll_x;
    tipoff->court_stream.scroll_y=tipoff->court_stream.next_scroll_y;
}

static void update_draw_order(NbaTipoff *tipoff, bool full_sort) {
    NbaDrawOrderInput input = {0};
    input.camera_y = (uint16_t)tipoff->camera_y;
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        input.x[actor] = (uint16_t)fp_integer_word(tipoff->actors[actor].x_fp);
        input.y[actor] = (uint16_t)fp_integer_word(tipoff->actors[actor].y_fp);
    }
    input.x[10] = (uint16_t)fp_integer_word(tipoff->ball.x_fp);
    input.y[10] = (uint16_t)fp_integer_word(tipoff->ball.y_fp);
    input.x[11] = tipoff->court_presentation.basket_x_3fef;
    input.y[11] = 0u; /* `$86:DBC2` initializes basket Y to zero. */
    if (!tipoff->draw_order_initialized) {
        if (!nba_draw_order_initialize(&tipoff->draw_order)) return;
        full_sort = true;
    }
    /* FBFF at initial/period placement starts from the carried order.
     * Scheduled OAM passes retain that list and perform one FC80 pass. */
    tipoff->draw_order_initialized = full_sort ?
        nba_draw_order_full_sort(&tipoff->draw_order, &input) :
        nba_draw_order_update(&tipoff->draw_order, &input);
}

static void latch_player_screen_origins(NbaTipoff *tipoff) {
    /* Live Mesen `$87:A47A` and `$87:B649` traces show that player and ball
     * OAM submissions persist across the intervening rendered frame.
     * Reprojecting only the ball against a newer camera moves it away from
     * its retained hand point for one frame. */
    for (unsigned actor=0;actor<NBA_GAMEPLAY_ACTOR_COUNT;++actor) {
        NbaTipoffActor *state=&tipoff->actors[actor];
        int16_t z=fp_integer_word(state->z_fp);
        nba_court_project_actor(fp_integer_word(state->x_fp),
            fp_integer_word(state->y_fp),z,tipoff->camera_x,tipoff->camera_y,
            &tipoff->player_screen_x[actor],&tipoff->player_screen_y[actor]);
        tipoff->player_screen_visible[actor]=nba_court_actor_visible(
            tipoff->player_screen_x[actor],
            (int16_t)((uint16_t)tipoff->player_screen_y[actor]+(uint16_t)z),
            z,state->controller_assignment_raw>=0);
        /* `$87:A5FB-$A609` updates only bit2 after projection/culling admits
         * the actor. Body bit15 remains AB48-AC22's animation result. */
        if (tipoff->player_screen_visible[actor]) {
            uint8_t head_direction = actor_draw_direction(tipoff, actor);
            state->actor_status_raw_28 &= 0xfffbu;
            if (head_direction < 3u) state->actor_status_raw_28 |= 0x0004u;
        }
        memset(&tipoff->player_indicator[actor],0,
               sizeof(tipoff->player_indicator[actor]));
        if (state->controller_assignment_raw>=0 &&
            !tipoff->player_screen_visible[actor])
            nba_court_player_indicator(tipoff->player_screen_x[actor],
                (int16_t)((uint16_t)tipoff->player_screen_y[actor]+(uint16_t)z),
                (uint8_t)state->controller_assignment_raw,
                &tipoff->player_indicator[actor]);
    }
    nba_court_project_actor(fp_integer_word(tipoff->ball.x_fp),
        fp_integer_word(tipoff->ball.y_fp),fp_integer_word(tipoff->ball.z_fp),
        tipoff->camera_x,tipoff->camera_y,
        &tipoff->ball_screen_x,&tipoff->ball_screen_y);
    update_draw_order(tipoff, false);
}

static bool cpu_ball_presentation_latch_self_test(void) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    state.actors[3].x_fp = 100 * 256;
    state.actors[3].y_fp = -50 * 256;
    state.ball.x_fp = 99 * 256;
    state.ball.y_fp = -57 * 256;
    state.ball.z_fp = 67 * 256;
    state.camera_x = -172;
    state.camera_y = -147;
    latch_player_screen_origins(&state);
    int16_t latched_x = state.ball_screen_x;
    int16_t latched_y = state.ball_screen_y;
    int16_t repro_x = 0, repro_y = 0;
    state.camera_x = -166;
    state.camera_y = -149;
    nba_court_project_actor(fp_integer_word(state.ball.x_fp),
        fp_integer_word(state.ball.y_fp),fp_integer_word(state.ball.z_fp),
        state.camera_x,state.camera_y,&repro_x,&repro_y);
    /* The interstitial render retains both OAM coordinates even though a
     * newly projected ball would move by the camera delta. */
    if (state.ball_screen_x != latched_x || state.ball_screen_y != latched_y ||
        (repro_x == latched_x && repro_y == latched_y)) return false;
    latch_player_screen_origins(&state);
    return state.ball_screen_x == repro_x && state.ball_screen_y == repro_y;
}

static void draw_ball(const NbaTipoff *tipoff, NbaRenderer *ren, int x, int y) {
    const NbaAssetItem *item = nba_assets_get(tipoff->assets, NBA_ASSET_TIPOFF_BALL);
    if (!item || !item->data || item->size != 56u ||
        memcmp(item->data, "NBBALL1", 8)) return;
    const uint8_t *data = (const uint8_t *)item->data;
    uint8_t palette[32] = {0};
    memcpy(palette + 5u * 2u, data + 44, 6u * 2u);
    /* `$80:B10C-$B11F` submits resource $081D through B344. Its literal
     * descriptor at `$9B:9C16` places the 8x8 tile at origin (-3,-4).
     * Drawing the extracted tile directly treated the ball's projected
     * center as its top-left corner and visibly separated it from the hand. */
    nba_rom_sprite_resource_render(ren, tipoff->assets, 0x081du,
        palette, x, y, false, 1);
}

bool nba_tipoff_init(NbaTipoff *tipoff, const NbaAssetPack *assets,
                     NbaSession *session) {
    if (!tipoff || !assets || !session) return false;
#define NBA_TIPOFF_REQUIRE(label, expression) do { \
    if (!(expression)) { \
        fprintf(stderr, "[TIPOFF] initialization check failed: %s\n", label); \
        return false; \
    } \
} while (0)
    const uint8_t *ppu_vram = NULL, *ppu_cgram = NULL;
    NBA_TIPOFF_REQUIRE("gameplay RNG", nba_gameplay_rng_self_test());
    NBA_TIPOFF_REQUIRE("SNES Mode-1 compositor", nba_snes_mode1_self_test());
    NBA_TIPOFF_REQUIRE("gameplay AI", nba_gameplay_ai_self_test());
    NBA_TIPOFF_REQUIRE("ball physics", nba_gameplay_ball_self_test());
    NBA_TIPOFF_REQUIRE("effect", nba_gameplay_effect_self_test());
    NBA_TIPOFF_REQUIRE("foul", nba_gameplay_foul_self_test());
    NBA_TIPOFF_REQUIRE("rim contact tick", cpu_rim_contact_tick_self_test());
    NBA_TIPOFF_REQUIRE("two-substep ball physics", cpu_ball_substep_self_test());
    NBA_TIPOFF_REQUIRE("deferred shooting foul", cpu_deferred_shooting_foul_self_test());
    NBA_TIPOFF_REQUIRE("free throw scene", cpu_free_throw_scene_self_test());
    NBA_TIPOFF_REQUIRE("special receiver", cpu_special_receiver_self_test(assets));
    NBA_TIPOFF_REQUIRE("boundary pass recovery", cpu_boundary_pass_recovery_self_test());
    NBA_TIPOFF_REQUIRE("inbound completion", cpu_inbound_completion_witness_self_test());
    NBA_TIPOFF_REQUIRE("inbound formation override",
        cpu_inbound_formation_override_self_test());
    NBA_TIPOFF_REQUIRE("ball acquisition", cpu_ball_acquisition_self_test());
    NBA_TIPOFF_REQUIRE("dead ball dispatch", cpu_dead_ball_dispatch_self_test());
    NBA_TIPOFF_REQUIRE("out-of-bounds dispatch", cpu_out_of_bounds_dispatch_self_test());
    NBA_TIPOFF_REQUIRE("contact orchestration",
        cpu_contact_orchestration_self_test(assets, session));
    NBA_TIPOFF_REQUIRE("player contact", cpu_player_contact_self_test());
    NBA_TIPOFF_REQUIRE("defensive planner", cpu_defensive_planner_self_test());
    NBA_TIPOFF_REQUIRE("role reaction reload", cpu_role_reaction_reload_self_test());
    NBA_TIPOFF_REQUIRE("expired inbound", cpu_expired_inbound_self_test());
    NBA_TIPOFF_REQUIRE("inbound recovery carrier",
        cpu_inbound_recovery_carrier_self_test(assets, session));
    NBA_TIPOFF_REQUIRE("attachment assets", ball_attachment_assets_valid(assets));
    NBA_TIPOFF_REQUIRE("ROM animation cadence", nba_player_animation_self_test(assets));
    NBA_TIPOFF_REQUIRE("latched owner pose/cadence binding", cpu_owner_pose_animation_self_test(assets, session));
    NBA_TIPOFF_REQUIRE("latched ball presentation", cpu_ball_presentation_latch_self_test());
    NBA_TIPOFF_REQUIRE("boosted pass", cpu_boosted_pass_self_test(assets, session));
    NBA_TIPOFF_REQUIRE("shot branches", cpu_shot_branches_self_test(assets, session));
    NBA_TIPOFF_REQUIRE("special shot lifecycle", cpu_special_shot_self_test(assets, session));
    NBA_TIPOFF_REQUIRE("fatigue tables", nba_shot_state_assets_valid(assets));
    NBA_TIPOFF_REQUIRE("indexed gameplay PPU inputs",
        nba_assets_gameplay_ppu_input(assets, session->right_team,
                                      &ppu_vram, &ppu_cgram));
    NBA_TIPOFF_REQUIRE("home-selected court stream map",
        nba_assets_gameplay_court_map(assets, session->right_team));
    NBA_TIPOFF_REQUIRE("tipoff ball asset", nba_assets_get(assets, NBA_ASSET_TIPOFF_BALL));
    NBA_TIPOFF_REQUIRE("complete original HUD lifecycle resource286",
        nba_gameplay_hud_lifecycle_assets_valid(assets));
    NBA_TIPOFF_REQUIRE("original out-of-bounds strings resource289",
        nba_gameplay_hud_oob_assets_valid(assets));
#undef NBA_TIPOFF_REQUIRE
    memset(tipoff, 0, sizeof(*tipoff));
    tipoff->assets = assets;
    tipoff->session = session;
    publish_exhibition_team_ids(tipoff);
    NbaShotMomentum momentum = {0};
    nba_shot_momentum_reset(&momentum); /* $86:DD80 */
    tipoff->assistance_team_raw_09c0=momentum.assistance_team;
    nba_shot_stamina_init(&tipoff->fatigue);
    nba_shot_fatigue_timer_init(&tipoff->fatigue);
    for (unsigned i=0;i<24;++i) {
        uint8_t rating=0;
        if (!nba_player_gameplay_stamina_rating(assets,
                team_id_for_context(tipoff, i / 12u),(uint8_t)(i%12),&rating) || rating<3 || rating>10)return false;
        tipoff->fatigue.rating[i]=rating;
    }
    nba_gameplay_effect_init(&tipoff->rim_effect);
    tipoff->special_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    tipoff->cpu_vs_cpu = true;
    nba_gameplay_rng_seed(&tipoff->rng, 0x9146u);
    tipoff->graphics_scratch.rng=tipoff->rng.state;
    for(unsigned i=0;i<3;++i)tipoff->graphics_scratch.slots[i].record=0xffffu;
    nba_gameplay_foul_init(&tipoff->fouls);
    if(!nba_gameplay_hud_init(&tipoff->hud,assets)) {
        fprintf(stderr,"[HUD] Required original indexed resource286 missing/invalid\n");
        return false;
    }
    /* $87:B99A-B9A9 owns shared08DE/08E6 too, not only HUD-private words. */
    tipoff->fouls.whistle_timer_raw_08de=-1;
    tipoff->fouls.whistle_state_raw_08e6=0xFFFFu;
    tipoff->hud_event_actor_raw_492d=0xFFFFu;
    static const uint8_t context_actor_order[2][5] = {
        {0x10u, 0x0Eu, 0x0Au, 0x12u, 0x0Cu},
        {0x04u, 0x06u, 0x08u, 0x00u, 0x02u}
    };
    for (unsigned side = 0; side < 2u; ++side) {
        tipoff->team_context[side].anchor_x_raw_0a =
            side ? 336 : -336;
        tipoff->team_context[side].score_raw_26 = 0u;
        tipoff->team_context[side].strategy_raw_2e = 7u;
        tipoff->team_context[side].mode_raw_30 = 4u;
        tipoff->team_context[side].flags_raw_32 = 1u;
        tipoff->team_context[side].activity_raw_39 = 1u;
        tipoff->team_context[side].dead_ball_actor_raw_3f =
            NBA_GAMEPLAY_UNKNOWN_WORD;
        tipoff->team_context[side].controller_actor_raw_41 = -1;
        tipoff->team_context[side].previous_dead_ball_actor_raw_43 =
            NBA_GAMEPLAY_UNKNOWN_WORD;
        tipoff->team_context[side].previous_controller_actor_raw_45 = -1;
        tipoff->team_context[side].help_distance_raw_4e = 0x00A0u;
        tipoff->team_context[side].play_selection_raw_56 = 0u;
        for (unsigned i = 0; i < 5u; ++i)
            tipoff->team_context[side].actor_order_raw_49[i] =
                context_actor_order[side][i];
    }
    /* `$86:DBDC-$DBE5/$86:DD2D-$DD44`: the setup quarter choice selects
     * distinct regulation/overtime tables.  Expiry and period advancement
     * remain outside this initialization slice. */
    tipoff->period_raw_0926 = session->match.period_raw_0926;
    tipoff->match_clock_raw_0928 = nba_match_period_clock(session);
    tipoff->possession_actor = -1;
    tipoff->possession_team = -1;
    tipoff->collision_actor_a_raw = -1;
    tipoff->collision_actor_b_raw = -1;
    tipoff->player_contact_actor_a_raw = -1;
    tipoff->player_contact_actor_b_raw = -1;
    session->score[0] = session->score[1] = 0u;
    session->game_clock_ticks = 0u;
    /* Native pre-tip 0936 is 0081, not shot-flight 0001. The distinction
     * selects normal court framing at 85:92F1 instead of ball-height framing. */
    tipoff->live_state_raw = 0x81u;
    tipoff->tip_contact_actor = -1;
    tipoff->camera_side_group_raw = 0xFFu;
    /* Native pre-tip $097E is FFFF: no prior dead-ball side. A zero
     * default falsely makes later same-side acquisition look like a turnover. */
    tipoff->dead_ball_raw_097e = 0xFFFFu;
    /* `$86:E0B0-$E207` leaves pre-tip `$0954` at its cleared zero; FFFF is
     * used for possession/team selectors, not this dormant inbound slot. */
    tipoff->inbound_actor_raw = 0u;
    tipoff->pass_actor_raw = -1;
    tipoff->pass_receiver_raw = -1;
    tipoff->play_aux_selector_raw_09a6 = -1;
    tipoff->shot_actor_raw_09c8 = -1;
    /* `$86:E056-$E0AB`: native object list has ten actor entries before
     * the ball link at34E7. C keeps the link bookkeeping as data; rendering
     * still traverses its own actors, not a captured SNES display list. */
    tipoff->ball_initialization.cursor=0x34D3u+2u*NBA_GAMEPLAY_ACTOR_COUNT;
    nba_tip_ball_initialize(&tipoff->ball_initialization);
    const NbaTipBallInitialization *ball_init=&tipoff->ball_initialization;
    tipoff->ball.x_fp=(int32_t)(int16_t)ball_init->x*256;
    tipoff->ball.y_fp=(int32_t)(int16_t)ball_init->y*256;
    tipoff->ball.z_fp=(int32_t)(int16_t)ball_init->z*256;
    tipoff->ball.velocity_x=(int16_t)ball_init->velocity_x;
    tipoff->ball.velocity_y=(int16_t)ball_init->velocity_y;
    tipoff->ball.velocity_z=(int16_t)ball_init->velocity_z;
    tipoff->catch_actor_record_raw_0910=ball_init->published_record;
    tipoff->context_raw_4933=ball_init->context_4933;
    tipoff->context_raw_4935=ball_init->context_4935;
    tipoff->fouls.latched_event_raw_08f0=ball_init->event_08f0;
    tipoff->ball.owner_actor = -1;
    /* 878DDA-8DE4 starts the first/overtime jump-ball hold at120. */
    tipoff->tip_toss_countdown_raw_09f2=120;
    tipoff->ball.state=NBA_BALL_HIDDEN;
    uint8_t appearance_teams[NBA_PLAYER_APPEARANCE_COUNT];
    uint8_t appearance_roster[NBA_PLAYER_APPEARANCE_COUNT];
    for (unsigned i = 0; i < NBA_PLAYER_APPEARANCE_COUNT; ++i) {
        appearance_teams[i] = team_id_for_context(tipoff, i / 5u);
        appearance_roster[i] =
            session->match.active_lineup[i / NBA_MATCH_LINEUP_SIZE]
                                                [i % NBA_MATCH_LINEUP_SIZE];
    }
    NbaPlayerAppearanceSetup appearance;
    if (!nba_player_appearance_setup(assets, appearance_teams, appearance_roster,
                                     &appearance)) return false;
    NbaPlayerActiveAppearanceInput active_input = {0};
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        uint8_t selector = (uint8_t)(actor % 5u);
        unsigned paired = actor < 5u ? 5u + selector : selector;
        uint8_t team = team_id_for_context(tipoff, paired / 5u);
        uint8_t roster = session->match.active_lineup
            [paired / NBA_MATCH_LINEUP_SIZE][paired % NBA_MATCH_LINEUP_SIZE];
        active_input.lineup_selector[actor] = selector;
        active_input.upper_variant[actor] =
            (uint8_t)appearance.players[actor].upper_variant;
        if (!nba_player_gameplay_shot_ratings(
                assets, team, roster,
                &active_input.appearance_a[actor],
                &active_input.appearance_b[actor])) return false;
    }
    NbaPlayerActiveAppearance active_appearance;
    if (!nba_player_build_active_appearance(
            &active_input, &active_appearance)) return false;
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaTipoffActor *state = &tipoff->actors[actor];
        state->x_fp = (int32_t)formation[actor].world_x * 256;
        state->y_fp = (int32_t)formation[actor].world_y * 256;
        state->direction = formation[actor].direction;
        state->roster_slot = session->match.active_lineup
            [actor / NBA_MATCH_LINEUP_SIZE][actor % NBA_MATCH_LINEUP_SIZE];
        state->requested_direction = formation[actor].direction;
        state->movement_direction = 8u;
        state->saved_control_mode = 0u;
        state->pass_family_raw = -1;
        state->controller_assignment_raw = -1;
        state->shot_stamina_raw_18=0x7FFFu;
        state->lower_animation_state = 0u;
        state->exact_jump_animation=true;
        state->animation_upper_queue_cursor_raw_18 = 0xFFFFu;
        state->animation_lower_queue_cursor_raw_1a = 0xFFFFu;
        /* `$86:D86C-$D89B`: +$76 uses the five-byte lineup selector, not
         * the selected roster record number. The old host shortcut used
         * roster slot 2 for actor zero and incorrectly paired it with actor
         * seven instead of the native same-lineup-position actor five. */
        state->assignment_actor =
            (uint8_t)(active_appearance.assignment_base[actor] >> 1);
        state->team_group_raw_6e = actor < 5u ? 0u : 5u;
        state->assignment_base_raw =
            active_appearance.assignment_base[actor];
        state->assignment_current_raw = state->assignment_base_raw;
        state->assignment_alternate_raw =
            active_appearance.assignment_alternate[actor];
        state->free_throw_launch_half_raw_a8 = appearance.players[actor].alternate_lower;
        state->animation_variant_raw_6c =
            active_appearance.upper_variant[actor];
        state->help_request_raw_80 = active_appearance.help_request[actor];
        /* DF4B-DF84 uses the same already-zero accumulator for +60. The
         * earlier host randomized ten timers here, advancing 07F6 before
         * the first F787/EC32 pass and changing both jumpers' launch branch. */
        state->reaction_threshold = 0;
        state->behavior_timer = 0x2Fu;
        state->control_mode = actor == 0u || actor == 5u ? 4u : 2u;
        state->visible = actor != 4u && actor != 9u;
    }
    publish_appearance_assignment_roles(tipoff->actors, &active_appearance);
    /* Policy remains CPU-only pending complete $84:E2AC action/movement
     * integration. Use a real neutral allocator result, not selected human
     * records with contradictory all-CPU actor assignments. Saved UI choices
     * are retained in the session for the eventual human enable boundary. */
    const uint16_t effective_selection[5]={1,1,1,1,1};
    if (!nba_tipoff_initialize_controllers(tipoff,effective_selection,
            session->controller_flags,0)) return false;
    /* `$86:D8D3-$D8E2`: the reciprocal +$78 values were produced atomically
     * with +$76 above, before mutable +$74 can diverge during live play. */
    /* `$85:BC52-$BC81` and `$85:AFC2-$AFE5`: initialize pair and basket
     * geometry independently of rendered facing and neutral movement intent. */
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaTipoffActor *state = &tipoff->actors[actor];
        unsigned paired_slot = state->assignment_current_raw >> 1;
        if (paired_slot >= NBA_GAMEPLAY_ACTOR_COUNT) continue;
        NbaTipoffActor *paired = &tipoff->actors[paired_slot];
        state->assignment_direction = nba_gameplay_target_direction(
            (int16_t)(fp_round(paired->x_fp) - fp_round(state->x_fp)),
            (int16_t)(fp_round(paired->y_fp) - fp_round(state->y_fp)),
            &state->assignment_distance);
        state->pair_distance = state->assignment_distance;
        int16_t anchor = actor < 5u ? -336 : 336;
        state->anchor_direction_raw = nba_gameplay_pass_direction(
            (int16_t)(anchor - fp_round(state->x_fp)),
            (int16_t)-fp_round(state->y_fp),
            &state->anchor_distance_raw);
    }
    NbaCameraInput initial_camera = {0};
    initial_camera.subject = nba_gameplay_camera_subject(tipoff->ball.x_fp,tipoff->ball.y_fp);
    initial_camera.ball_height = fp_integer_word(tipoff->ball.z_fp);
    initial_camera.side_group = -1;
    initial_camera.basket_left = tipoff->team_context[0].anchor_x_raw_0a;
    initial_camera.basket_right = tipoff->team_context[1].anchor_x_raw_0a;
    initial_camera.live_state = tipoff->live_state_raw;
    nba_gameplay_camera_copy(&tipoff->camera,0,&initial_camera.subject);
    nba_gameplay_camera_place(&tipoff->camera,&initial_camera);
    tipoff->camera_x = tipoff->camera.x;
    tipoff->camera_y = tipoff->camera.y;
    latch_player_screen_origins(tipoff);
    /* Scene entry starts on the due presentation phase. Subsequent credits
     * come from outer updates; pauses do not consume or invent camera ticks. */
    tipoff->camera.presentation_ticks_0564 = 1;
    nba_court_stream_init_home(&tipoff->court_stream,tipoff->camera_x,
        tipoff->camera_y,team_id_for_context(tipoff,0u));
    tipoff->is_initialized = true;
    printf("[TIPOFF] $86:CCFC contact -> $86:B04C receiver -> "
           "$86:99C4 deflection -> $86:D365 possession.\n");
    return true;
}

/* Controlled Mesen witnesses from `$87:95E9` to `$86:DD47`/`$87:985C`.
 * They include the native input acknowledgements used by the capture, so the
 * stage raster remains separate; these replace the old arbitrary 120 ticks. */
#define NBA_MATCH_QTR_PRESENTATION_TICKS       1187u
#define NBA_MATCH_HALFTIME_PRESENTATION_TICKS  1547u
#define NBA_MATCH_OT_PRESENTATION_TICKS        1367u
#define NBA_MATCH_FINAL_PRESENTATION_TICKS     1756u
/* Both confirmed timeout and resume paths pass X=$003C to `$80:86BF` after
 * `$80:CF1B`. The fade/rebuild children remain an explicit placeholder. */
#define NBA_MATCH_PAUSE_TRANSITION_TICKS 60u

bool nba_tipoff_pause_active(const NbaTipoff *tipoff) {
    return tipoff && tipoff->session &&
           tipoff->session->match.pause.state != NBA_MATCH_PAUSE_INACTIVE;
}

bool nba_tipoff_pause_can_enter(const NbaTipoff *tipoff) {
    if (!tipoff || !tipoff->session || !tipoff->is_initialized) return false;
    const NbaMatchLifecycle *match = &tipoff->session->match;
    return match->pause.state == NBA_MATCH_PAUSE_INACTIVE &&
           match->flow_state == NBA_MATCH_FLOW_LIVE &&
           match->final_marker == NBA_MATCH_FINAL_ACTIVE &&
           tipoff->phase == NBA_TIPOFF_LIVE &&
           tipoff->live_state_raw != 0x0080u;
}

static void match_pause_enter(NbaTipoff *tipoff) {
    NbaMatchPauseFlow *pause = &tipoff->session->match.pause;
    pause->saved_live_state_raw_4988 = tipoff->live_state_raw;
    pause->selected_side = context_for_ui_side(tipoff->session->player_one_side);
    pause->selection = tipoff->session->match.timeouts_remaining[
        pause->selected_side] ? NBA_MATCH_PAUSE_SELECT_TIMEOUT :
                               NBA_MATCH_PAUSE_SELECT_RESUME;
    pause->transition_ticks_remaining = 0u;
    pause->state = NBA_MATCH_PAUSE_MENU;
    tipoff->live_state_raw = 0x0080u;
    tipoff->fouls.presentation_gate_raw_08e2=0u; /* $86:834D-8352 */
    /* $86:835D calls CE36 after the live-state80 store. Start came from
     * physical pad0, so requester095E is0 (not the team's context index).
     * This refreshes actual scores even when the temporary panel expired.
     * Existing pause holds continue to return before clock/actor updates. */
    tipoff->hud_requester_raw_095e=0u;
    hud_request_score(tipoff);
}

static void match_pause_begin_resume(NbaTipoff *tipoff) {
    NbaMatchPauseFlow *pause = &tipoff->session->match.pause;
    pause->state = NBA_MATCH_PAUSE_RESUME_TRANSITION;
    pause->transition_ticks_remaining = NBA_MATCH_PAUSE_TRANSITION_TICKS;
}

static void match_pause_confirm_timeout(NbaTipoff *tipoff) {
    NbaMatchPauseFlow *pause = &tipoff->session->match.pause;
    uint8_t side = pause->selected_side ? 1u : 0u;
    /* `$86:83EC/$841E` skip disabled index zero. `$844E` itself does not
     * reject zero, so enforce availability before reaching its decrement. */
    if (tipoff->session->match.timeouts_remaining[side] == 0u) {
        pause->selection = NBA_MATCH_PAUSE_SELECT_RESUME;
        return;
    }
    /* `$86:8453` copies requesting-controller group0/5 from `$08D2`.
     * selected_side is the host array context0/1, never the raw group. */
    tipoff->context_raw_4933 = (uint16_t)(side * NBA_MATCH_LINEUP_SIZE);
    tipoff->context_raw_4935 = tipoff->context_raw_4933;
    --tipoff->session->match.timeouts_remaining[side];
    nba_shot_stamina_fixed_grant(&tipoff->fatigue);
    pause->state = NBA_MATCH_PAUSE_TIMEOUT_TRANSITION;
    pause->transition_ticks_remaining = NBA_MATCH_PAUSE_TRANSITION_TICKS;
}

static void match_pause_step(NbaTipoff *tipoff, const NbaInput *input) {
    NbaMatchPauseFlow *pause = &tipoff->session->match.pause;
    if (pause->state == NBA_MATCH_PAUSE_TIMEOUT_TRANSITION ||
        pause->state == NBA_MATCH_PAUSE_RESUME_TRANSITION) {
        if (pause->transition_ticks_remaining > 0u)
            --pause->transition_ticks_remaining;
        if (pause->transition_ticks_remaining != 0u) return;
        /* $86:84DB/84DF and858E/8592 restore the court HUD on the
         * timeout/resume routes. B99A preserves the old working canvas and
         * generatedCHR; hud_init would incorrectly reset other game state. */
        hud_publish(tipoff,0x87B99Au);
        hud_publish(tipoff,0x87BA54u);
        if (pause->state == NBA_MATCH_PAUSE_TIMEOUT_TRANSITION) {
            pause->state = NBA_MATCH_PAUSE_MENU_AFTER_TIMEOUT;
            pause->selection = NBA_MATCH_PAUSE_SELECT_RESUME;
            return;
        }
        tipoff->live_state_raw = pause->saved_live_state_raw_4988;
        pause->state = NBA_MATCH_PAUSE_INACTIVE;
        pause->transition_ticks_remaining = 0u;
        return;
    }
    if (!input) return;
    uint8_t side = pause->selected_side ? 1u : 0u;
    if (input->pressed & (NBA_BTN_UP | NBA_BTN_DOWN)) {
        pause->selection = pause->selection == NBA_MATCH_PAUSE_SELECT_TIMEOUT ?
            NBA_MATCH_PAUSE_SELECT_RESUME : NBA_MATCH_PAUSE_SELECT_TIMEOUT;
        if (pause->selection == NBA_MATCH_PAUSE_SELECT_TIMEOUT &&
            tipoff->session->match.timeouts_remaining[side] == 0u)
            pause->selection = NBA_MATCH_PAUSE_SELECT_RESUME;
    }
    if (!(input->pressed & NBA_BTN_A)) return;
    if (pause->selection == NBA_MATCH_PAUSE_SELECT_TIMEOUT)
        match_pause_confirm_timeout(tipoff);
    else
        match_pause_begin_resume(tipoff);
}

bool nba_tipoff_match_horn_transition_ready(const NbaTipoff *tipoff) {
    if (!tipoff) return false;
    /* `$87:8EB7-$8ECF`: an owner, a low ball, or non-negative `$0946`
     * resolves the horn.  An ownerless unresolved ball at Z >= 8 stays live. */
    return tipoff->possession_actor >= 0 ||
           fp_integer_word(tipoff->ball.z_fp) < 8 ||
           tipoff->pass_receiver_raw >= 0;
}

static uint16_t match_period_inbound_group(const NbaTipoff *tipoff) {
    uint16_t winner = tipoff->tip_winner_group_raw_0932 == 5u ? 5u : 0u;
    /* The opening-tip loser starts Q2 and Q3; the winner starts Q4.  This is
     * the gameplay owner selected by the native period formation branches
     * following `$86:DDFD`, not presentation-side inference. */
    return tipoff->period_raw_0926 == 1u || tipoff->period_raw_0926 == 2u ?
        (uint16_t)(winner ^ 5u) : winner;
}

static void match_refresh_anchor_geometry(NbaTipoff *tipoff) {
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaTipoffActor *state = &tipoff->actors[actor];
        int16_t anchor = tipoff->team_context[actor / 5u].anchor_x_raw_0a;
        state->anchor_direction_raw = nba_gameplay_pass_direction(
            (int16_t)(anchor - fp_round(state->x_fp)),
            (int16_t)-fp_round(state->y_fp),
            &state->anchor_distance_raw);
    }
}

static void match_restart_period(NbaTipoff *tipoff) {
    NbaSession *session = tipoff->session;
    tipoff->period_raw_0926 = session->match.period_raw_0926;
    tipoff->match_clock_raw_0928 = nba_match_period_clock(session);
    /* `$86:DD47-$DD53`: both clock words and the period-start play selector
     * are written before actor formation is rebuilt. */
    tipoff->rim_raw_092c = 0x05A0u;
    tipoff->shot_clock_mirror_raw_09c6 = 0x05A0u;
    tipoff->play_request_raw = 0x05A0u;
    tipoff->play_code = 1u;
    tipoff->elapsed_shot_clock_raw_13f7 = 0u;
    tipoff->dead_clock_enabled_raw_0a04 = 0u;

    /* `$86:DD56-$DD75`: only the Q3 entry negates both team anchors. */
    if (tipoff->period_raw_0926 == 2u) {
        tipoff->team_context[0].anchor_x_raw_0a =
            (int16_t)-tipoff->team_context[0].anchor_x_raw_0a;
        tipoff->team_context[1].anchor_x_raw_0a =
            (int16_t)-tipoff->team_context[1].anchor_x_raw_0a;
        match_refresh_anchor_geometry(tipoff);
    }

    /* `$86:DD80-$DD86`: release owner and Assistance before rebuilding the
     * period formation. */
    tipoff->possession_actor = -1;
    tipoff->ball.owner_actor = -1;
    tipoff->assistance_team_raw_09c0 = 0xFFFFu;
    tipoff->pass_actor_raw = -1;
    tipoff->pass_receiver_raw = -1;
    tipoff->pass_active_raw = 0u;
    tipoff->ball_activity_raw = 0u;

    if (tipoff->period_raw_0926 >= 4u) {
        /* `$86:DD97-$DDA4` selects the opening formation for overtime. */
        const NbaTipBallInitialization *ball = &tipoff->ball_initialization;
        tipoff->ball.x_fp = (int32_t)(int16_t)ball->x * 256;
        tipoff->ball.y_fp = (int32_t)(int16_t)ball->y * 256;
        tipoff->ball.z_fp = (int32_t)(int16_t)ball->z * 256;
        tipoff->ball.velocity_x = (int16_t)ball->velocity_x;
        tipoff->ball.velocity_y = (int16_t)ball->velocity_y;
        tipoff->ball.velocity_z = (int16_t)ball->velocity_z;
        tipoff->ball.state = NBA_BALL_HIDDEN;
        tipoff->tip_contact_actor = -1;
        tipoff->tip_possession_frame = 0u;
        tipoff->tip_toss_countdown_raw_09f2 = 120u;
        tipoff->live_state_raw = 0x81u;
        tipoff->phase = NBA_TIPOFF_FORMATION;
    } else {
        uint16_t side_group = match_period_inbound_group(tipoff);
        uint8_t inbounder = (uint8_t)(side_group + 2u);
        cpu_begin_dead_ball(tipoff, inbounder, side_group, 0, false);
        /* `$86:F43A-$F668` owns arrival, selector and launch. Do not publish
         * the ready/final state in the same frame as period setup: the actor
         * must first reach the sideline target through the ordinary inbound
         * continuation. Premature finalization leaves mode 3 planted forever. */
        tipoff->play_code = 1u;
        tipoff->play_request_raw = 1u;
        tipoff->phase = NBA_TIPOFF_LIVE;
    }

    tipoff->dead_ball_dispatch_busy_raw_09b4 = 0u;
    tipoff->rim_raw_13e7 &= 0xF7FFu;
    session->match.presentation_ticks_remaining = 0u;
    session->match.flow_state = NBA_MATCH_FLOW_LIVE;
    update_draw_order(tipoff, true);
}

static void match_finish_period(NbaTipoff *tipoff) {
    NbaSession *session = tipoff->session;
    uint16_t old_period = tipoff->period_raw_0926;
    bool tied = session->score[0] == session->score[1];

    /* `$87:96F6-$974B`: common, halftime, then tied-regulation/OT grants. */
    nba_shot_stamina_grant(&tipoff->fatigue, 0x1000u);
    if (old_period == 1u) nba_shot_stamina_grant(&tipoff->fatigue, 0x6000u);
    else if (old_period >= 3u && tied) {
        tipoff->overtime_tie_marker_raw_15bd = 1u;
        nba_shot_stamina_grant(&tipoff->fatigue, 0x3000u);
    }

    /* `$87:9766-$979D`: increment only below raw period four.  Period four
     * is reused for every OT; a non-tie stores five as the final marker. */
    uint16_t next_period = old_period < 4u ? (uint16_t)(old_period + 1u) : old_period;
    bool final = next_period == 4u && !tied;
    if (final) next_period = 5u;
    tipoff->period_raw_0926 = next_period;
    session->match.period_raw_0926 = next_period;
    session->match.final_marker = final ?
        NBA_MATCH_FINAL_CONFIRMED : NBA_MATCH_FINAL_ACTIVE;
    session->match.presentation_ticks_remaining = final ?
        NBA_MATCH_FINAL_PRESENTATION_TICKS :
        old_period == 1u ? NBA_MATCH_HALFTIME_PRESENTATION_TICKS :
        old_period >= 3u && tied ? NBA_MATCH_OT_PRESENTATION_TICKS :
        NBA_MATCH_QTR_PRESENTATION_TICKS;
    session->match.flow_state = final ?
        NBA_MATCH_FLOW_POSTGAME_PRESENTATION_PENDING :
        NBA_MATCH_FLOW_PERIOD_PRESENTATION_PENDING;
}

bool nba_tipoff_step_match_lifecycle(NbaTipoff *tipoff) {
    if (!tipoff || !tipoff->session) return false;
    NbaMatchLifecycle *match = &tipoff->session->match;

    if (match->flow_state == NBA_MATCH_FLOW_FINAL) return true;
    if (match->flow_state == NBA_MATCH_FLOW_PERIOD_PRESENTATION_PENDING ||
        match->flow_state == NBA_MATCH_FLOW_POSTGAME_PRESENTATION_PENDING) {
        if (match->presentation_ticks_remaining > 0u)
            --match->presentation_ticks_remaining;
        if (match->presentation_ticks_remaining == 0u) {
            if (match->flow_state == NBA_MATCH_FLOW_POSTGAME_PRESENTATION_PENDING)
                match->flow_state = NBA_MATCH_FLOW_FINAL;
            else
                match->flow_state = NBA_MATCH_FLOW_PERIOD_RESTART_PENDING;
        }
        return true;
    }
    if (match->flow_state == NBA_MATCH_FLOW_PERIOD_RESTART_PENDING) {
        match_restart_period(tipoff);
        return true;
    }

    /* `$86:97CD-$97F2`: zero and wrapped/sentinel clocks latch the horn.
     * Values one through five are warning values, not an expiry latch. */
    if (match->flow_state == NBA_MATCH_FLOW_LIVE &&
        (tipoff->match_clock_raw_0928 == 0u ||
         tipoff->match_clock_raw_0928 >= 0xF000u)) {
        tipoff->dead_ball_dispatch_busy_raw_09b4 = 1u;
        tipoff->rim_raw_13e7 |= 0x0800u;
        match->flow_state = NBA_MATCH_FLOW_HORN_BALL_LIVE;
    }
    if (match->flow_state != NBA_MATCH_FLOW_HORN_BALL_LIVE) return false;
    if (!nba_tipoff_match_horn_transition_ready(tipoff)) return false;
    match_finish_period(tipoff);
    return true;
}

static bool prepare_substitution_actor_bindings(
    const NbaTipoff *tipoff,
    const uint8_t lineup[NBA_MATCH_TEAM_COUNT][NBA_MATCH_LINEUP_SIZE],
    NbaTipoffActor actors[NBA_GAMEPLAY_ACTOR_COUNT]) {
    uint8_t teams[NBA_PLAYER_APPEARANCE_COUNT];
    uint8_t rosters[NBA_PLAYER_APPEARANCE_COUNT];
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        unsigned side = actor / NBA_MATCH_LINEUP_SIZE;
        teams[actor] = team_id_for_context(tipoff, side);
        rosters[actor] = lineup[side][actor % NBA_MATCH_LINEUP_SIZE];
    }
    NbaPlayerAppearanceSetup appearance;
    if (!nba_player_appearance_setup(tipoff->assets, teams, rosters,
                                     &appearance)) return false;
    NbaPlayerActiveAppearanceInput active_input = {0};
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        uint8_t selector = (uint8_t)(actor % NBA_MATCH_LINEUP_SIZE);
        unsigned paired = actor < NBA_MATCH_LINEUP_SIZE ?
            NBA_MATCH_LINEUP_SIZE + selector : selector;
        uint8_t team = team_id_for_context(tipoff, paired / NBA_MATCH_LINEUP_SIZE);
        uint8_t roster = lineup[paired / NBA_MATCH_LINEUP_SIZE]
                               [paired % NBA_MATCH_LINEUP_SIZE];
        active_input.lineup_selector[actor] = selector;
        active_input.upper_variant[actor] =
            (uint8_t)appearance.players[actor].upper_variant;
        if (!nba_player_gameplay_shot_ratings(
                tipoff->assets, team, roster,
                &active_input.appearance_a[actor],
                &active_input.appearance_b[actor])) return false;
    }
    NbaPlayerActiveAppearance active;
    if (!nba_player_build_active_appearance(&active_input, &active))
        return false;

    memcpy(actors, tipoff->actors, sizeof(tipoff->actors));
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaTipoffActor *state = &actors[actor];
        unsigned side = actor / NBA_MATCH_LINEUP_SIZE;
        state->roster_slot = lineup[side][actor % NBA_MATCH_LINEUP_SIZE];
        state->assignment_actor =
            (uint8_t)(active.assignment_base[actor] >> 1);
        state->assignment_base_raw = active.assignment_base[actor];
        state->assignment_current_raw = state->assignment_base_raw;
        state->assignment_alternate_raw =
            active.assignment_alternate[actor];
        state->free_throw_launch_half_raw_a8 =
            appearance.players[actor].alternate_lower;
        state->animation_variant_raw_6c = active.upper_variant[actor];
        state->help_request_raw_80 = active.help_request[actor];
        if (!nba_player_animation_resources_for_appearance(
                tipoff->assets, state->animation_state,
                state->lower_animation_state, state->direction,
                state->upper_animation_tick, state->lower_animation_tick,
                state->free_throw_launch_half_raw_a8 != 0u,
                state->animation_variant_raw_6c,
                &state->upper_animation_resource_raw_2a,
                &state->lower_animation_resource_raw_2c)) return false;
        state->animation_resources_valid = true;
        actor_publish_body_mirror(state);
    }
    publish_appearance_assignment_roles(actors, &active);
    /* Native actor rebuild restores reciprocal matchup geometry after the
     * lineup/resource transaction, before `$0A08` is cleared. */
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaTipoffActor *state = &actors[actor];
        unsigned paired_slot = state->assignment_current_raw >> 1;
        if (paired_slot >= NBA_GAMEPLAY_ACTOR_COUNT) return false;
        const NbaTipoffActor *paired = &actors[paired_slot];
        state->assignment_direction = nba_gameplay_target_direction(
            (int16_t)(fp_round(paired->x_fp) - fp_round(state->x_fp)),
            (int16_t)(fp_round(paired->y_fp) - fp_round(state->y_fp)),
            &state->assignment_distance);
        state->pair_distance = state->assignment_distance;
        int16_t anchor = actor < NBA_MATCH_LINEUP_SIZE ? -336 : 336;
        state->anchor_direction_raw = nba_gameplay_pass_direction(
            (int16_t)(anchor - fp_round(state->x_fp)),
            (int16_t)-fp_round(state->y_fp),
            &state->anchor_distance_raw);
    }
    return true;
}

bool nba_tipoff_apply_foul_out_substitution(NbaTipoff *tipoff) {
    if (!tipoff || !tipoff->is_initialized || !tipoff->session ||
        !tipoff->assets ||
        tipoff->fouls.substitution_request_raw_0a08 != 1u ||
        tipoff->fouls.foul_out_state_raw_09ca != 8u ||
        tipoff->fouls.substitution_actor_raw_492d < 0 ||
        tipoff->fouls.substitution_actor_raw_492d >=
            NBA_GAMEPLAY_ACTOR_COUNT) return false;
    unsigned outgoing_actor =
        (unsigned)(uint8_t)tipoff->fouls.substitution_actor_raw_492d;
    unsigned side = outgoing_actor / NBA_MATCH_LINEUP_SIZE;
    unsigned lineup_index = outgoing_actor % NBA_MATCH_LINEUP_SIZE;
    NbaMatchLifecycle *match = &tipoff->session->match;
    if (match->active_lineup[side][lineup_index] !=
            tipoff->actors[outgoing_actor].roster_slot)
        return false;
    for (unsigned i = 0; i < NBA_MATCH_LINEUP_SIZE; ++i)
        if (match->roster_order[side][i] != match->active_lineup[side][i])
            return false;

    NbaGameplaySubstitutionInput selection = {0};
    selection.outgoing_lineup_index = (uint8_t)lineup_index;
    memcpy(selection.roster_order, match->roster_order[side],
           sizeof(selection.roster_order));
    memcpy(selection.eligible, match->roster_available[side],
           sizeof(selection.eligible));
    uint8_t outgoing_roster = selection.roster_order[lineup_index];
    selection.eligible[outgoing_roster] = false;
    uint8_t team = team_id_for_context(tipoff, side);
    for (uint8_t roster = 0; roster < NBA_MATCH_ROSTER_SIZE; ++roster)
        if (!nba_player_gameplay_position(
                tipoff->assets, team, roster,
                &selection.position[roster])) return false;
    NbaGameplaySubstitutionResult selected;
    if (!nba_gameplay_select_foul_out_replacement(&selection, &selected))
        return false;

    uint8_t next_lineup[NBA_MATCH_TEAM_COUNT][NBA_MATCH_LINEUP_SIZE];
    memcpy(next_lineup, match->active_lineup, sizeof(next_lineup));
    memcpy(next_lineup[side], selected.roster_order,
           NBA_MATCH_LINEUP_SIZE);
    NbaTipoffActor next_actors[NBA_GAMEPLAY_ACTOR_COUNT];
    if (!prepare_substitution_actor_bindings(
            tipoff, next_lineup, next_actors)) return false;

    uint16_t next_stats[24][5];
    memcpy(next_stats, tipoff->roster_shot_statistics,
           sizeof(next_stats));
    uint8_t next_personal_fouls[24];
    memcpy(next_personal_fouls, tipoff->roster_personal_fouls,
           sizeof(next_personal_fouls));
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        unsigned actor_side = actor / NBA_MATCH_LINEUP_SIZE;
        unsigned persistent = actor_side * NBA_MATCH_ROSTER_SIZE +
            tipoff->actors[actor].roster_slot;
        memcpy(next_stats[persistent], tipoff->actors[actor].shot_statistics,
               sizeof(next_stats[persistent]));
        next_personal_fouls[persistent] =
            tipoff->fouls.personal_fouls[actor];
    }
    NbaShotFatigue next_fatigue = tipoff->fatigue;
    uint8_t next_actor_fouls[NBA_GAMEPLAY_ACTOR_COUNT];
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        unsigned actor_side = actor / NBA_MATCH_LINEUP_SIZE;
        unsigned persistent = actor_side * NBA_MATCH_ROSTER_SIZE +
            next_actors[actor].roster_slot;
        memcpy(next_actors[actor].shot_statistics, next_stats[persistent],
               sizeof(next_actors[actor].shot_statistics));
        next_actor_fouls[actor] = next_personal_fouls[persistent];
        next_fatigue.active_roster[actor] = (uint16_t)persistent;
        next_actors[actor].shot_stamina_raw_18 =
            next_fatigue.stamina[persistent];
    }

    /* Atomic commit in native order: lineup/status, actor identities,
     * appearance/resource bindings, roles/matchups, persistent ownership,
     * then request clear. No operation below can fail. */
    memcpy(match->roster_order[side], selected.roster_order,
           sizeof(match->roster_order[side]));
    memcpy(match->active_lineup, next_lineup, sizeof(match->active_lineup));
    match->roster_available[side][selected.outgoing_roster] = false;
    memcpy(tipoff->actors, next_actors, sizeof(tipoff->actors));
    memcpy(tipoff->roster_shot_statistics, next_stats,
           sizeof(tipoff->roster_shot_statistics));
    memcpy(tipoff->roster_personal_fouls, next_personal_fouls,
           sizeof(tipoff->roster_personal_fouls));
    memcpy(tipoff->fouls.personal_fouls, next_actor_fouls,
           sizeof(tipoff->fouls.personal_fouls));
    tipoff->fatigue = next_fatigue;
    tipoff->role_rebuild_raw_09d6 = 0u;
    tipoff->fouls.foul_out_state_raw_09ca = 0u;
    tipoff->fouls.substitution_actor_raw_492d = -1;
    tipoff->fouls.substitution_request_raw_0a08 = 0u;
    return true;
}

void nba_tipoff_update(NbaTipoff *tipoff, const NbaInput *input) {
    if (!tipoff || !tipoff->is_initialized) return;
    /* This observation is cleared even when pause/lifecycle returns before
     * gameplay. Tick parity alone falsely reported ten physics entries on
     * frozen period frames. It never controls an original gameplay branch. */
    tipoff->actor_pass_executed = false;
    /* `$86:8338-$8341` changes live state before the pause loop. The entire
     * typed pause flow returns before any clock, RNG, actor, ball, camera,
     * audio-event, fatigue or lifecycle writer below can execute. */
    if (nba_tipoff_pause_active(tipoff)) {
        match_pause_step(tipoff, input);
        return;
    }
    if (input && (input->pressed & NBA_BTN_START) &&
        nba_tipoff_pause_can_enter(tipoff)) {
        match_pause_enter(tipoff);
        return;
    }
    /* Period presentation is a hard gameplay boundary.  Its child visuals
     * are still pending, but neither actor physics nor clocks may leak across
     * it. */
    if (tipoff->session &&
        tipoff->session->match.flow_state >=
            NBA_MATCH_FLOW_PERIOD_PRESENTATION_PENDING) {
        (void)nba_tipoff_step_match_lifecycle(tipoff);
        return;
    }
    tipoff->pad_held_raw = input ? nba_controller_native_buttons(input->held) : 0u;
    /* Neutral CPU path $87:9087-908D / formation9622-9628. Input pause is
     * handled above; human dispatch and substitutions remain separate gates. */
    if(tipoff->cpu_vs_cpu) {
        tipoff->hud_requester_raw_095e=0xFFFFu;
        tipoff->hud_dispatch_mode_raw_0960=0xFFFFu;
    }
    /* `$13E7` is an outer-frame event bitfield. Acquisition's bit $0010 is
     * observable for one completed frame, then the next outer pass clears it. */
    tipoff->rim_raw_13e7 &= 0xFFEFu;
    tipoff->collision_actor_a_raw = -1;
    tipoff->collision_actor_b_raw = -1;
    tipoff->collision_routine_raw = 0u;
    tipoff->player_contact_count_raw = 0u;
    tipoff->player_contact_actor_a_raw = -1;
    tipoff->player_contact_actor_b_raw = -1;
    tipoff->player_contact_routine_raw = 0u;
    ++tipoff->frame;
    ++tipoff->simulation_tick;
    ++tipoff->camera.presentation_ticks_0564;
    ++tipoff->session->game_clock_ticks;
    /* The ROM shares07F6 between presentation scheduling and EC32. Run its
     * portable state effect before the actor pass, matching the prior-frame
     * transfer writer observed at F13D. */
    tipoff->graphics_scratch.rng=tipoff->rng.state;
    tipoff->graphics_scratch.scratch_0046=tipoff->scratch_0046;
    if((tipoff->simulation_tick&1u)==0u &&
       nba_graphics_scratch_step(tipoff->assets,&tipoff->graphics_scratch,2u)) {
        tipoff->rng.state=tipoff->graphics_scratch.rng;
        tipoff->scratch_0046=tipoff->graphics_scratch.scratch_0046;
        tipoff->scratch_0047=tipoff->scratch_0046>>8;
    }
    /* $85:EDAC-EDB6: NMI skips negative AND zero. CC10 owns the additional
     * dispatch decrement and zero-to-negative clear transition. */
    nba_gameplay_hud_timer_tick(&tipoff->fouls.whistle_timer_raw_08de);
    /* Native state controls clocks; no frame220 enable or reset. */
    if (tipoff->tip_contact_actor>=0) {
        NbaShotClock clock={tipoff->live_state_raw,tipoff->period_raw_0926,
            tipoff->match_clock_raw_0928,tipoff->rim_raw_092c,
            tipoff->shot_clock_mirror_raw_09c6,tipoff->dead_clock_enabled_raw_0a04,
            tipoff->fatigue.timer,tipoff->free_throw_flight_timer_raw_0930,
            tipoff->session->config.rules[8],tipoff->elapsed_clock_raw_13f9,
            tipoff->elapsed_shot_clock_raw_13f7};
        nba_shot_clock_step(&clock);
        tipoff->match_clock_raw_0928=clock.clock;
        tipoff->rim_raw_092c=clock.shot_clock;
        tipoff->shot_clock_mirror_raw_09c6=clock.shot_clock_mirror;
        tipoff->dead_clock_enabled_raw_0a04=clock.dead_clock_enabled;
        tipoff->fatigue.timer=clock.fatigue_timer;
        tipoff->free_throw_flight_timer_raw_0930=clock.flight_timer;
        tipoff->elapsed_clock_raw_13f9=clock.elapsed_clock;
        tipoff->elapsed_shot_clock_raw_13f7=clock.elapsed_shot_clock;
    }
    tipoff->hud_clock_snapshot_raw_092a=tipoff->match_clock_raw_0928; /* $87:94A5 */
    if (nba_tipoff_step_match_lifecycle(tipoff)) return;
    bool advancing_tip=tipoff->tip_contact_actor>=0;
    if (!advancing_tip) {
        /* 85EE3E-EE43: NMI countdown, saturating atFFFF. The host starts
         * this after setup; ROM video-transfer startup latency is separate. */
        if((int16_t)tipoff->tip_toss_countdown_raw_09f2>=0)
            --tipoff->tip_toss_countdown_raw_09f2;
        cpu_update_all_actors(tipoff);
        cpu_update_tip_ball(tipoff);
        if(!(tipoff->simulation_tick&1u)) {
            for(unsigned slot=0;slot<10;++slot) {
                NbaTipoffActor *a=&tipoff->actors[slot];
                (void)nba_gameplay_target_direction(
                    (int16_t)(fp_integer_word(a->x_fp)-fp_integer_word(tipoff->ball.x_fp)),
                    (int16_t)(fp_integer_word(a->y_fp)-fp_integer_word(tipoff->ball.y_fp)),
                    &a->focal_distance_raw_8e);
            }
        }
    }
    if (!advancing_tip && nba_tipoff_try_tip_contact(tipoff)) {
        cpu_commit_ball_acquisition(tipoff,(uint8_t)tipoff->tip_contact_actor);
    }
    if(!advancing_tip && !(tipoff->simulation_tick&1u))
        cpu_update_actor_behaviors(tipoff);
    if (advancing_tip) {
        /* $87:8EF3 calls fatigue before the due actor pass. All 24 roster
         * records persist; actors only mirror the currently active slots. */
        if ((tipoff->simulation_tick & 1u)==0u) {
            tipoff->fatigue.live_state=tipoff->live_state_raw;
            tipoff->fatigue.enabled=tipoff->session->config.rules[11];
            tipoff->fatigue.quarter=tipoff->session->config.main_values[3];
            for (unsigned i=0;i<NBA_GAMEPLAY_ACTOR_COUNT;++i) {
                tipoff->fatigue.active_roster[i]=(uint16_t)((i<5 ? 0 : 12)+tipoff->actors[i].roster_slot);
                tipoff->fatigue.boost[i]=tipoff->actors[i].movement_boost_timer;
            }
            (void)nba_shot_fatigue_step(tipoff->assets,&tipoff->fatigue);
            for (unsigned i=0;i<NBA_GAMEPLAY_ACTOR_COUNT;++i)
                tipoff->actors[i].shot_stamina_raw_18=
                    tipoff->fatigue.stamina[tipoff->fatigue.active_roster[i]];
        }
        cpu_update_possession(tipoff);
        /* `$86:F357-$F364`: once a detached owner and pending `$09BC` have
         * both been observed, `$0A02` becomes the immediate-resolution phase. */
        if (tipoff->fouls.shooting_foul_raw_09bc != 0u &&
            tipoff->deferred_shot_foul_phase_raw_0a02 != 0u)
            tipoff->deferred_shot_foul_phase_raw_0a02 = 2u;
        /* `$87:92A5-$95E6` performs dead-ball setup before `$85:93F5`
         * consumes the pending event later in the same outer pass. */
        /* $87:9578-9580 consumes the reset notification, not the running
         * shot clock. Without this owner the last icon survives possession
         * resets even when the new clock is above CC10's600 threshold. */
        if(tipoff->shot_clock_mirror_raw_09c6!=0u) {
            tipoff->shot_clock_mirror_raw_09c6=0u;
            hud_publish(tipoff,0x87BACBu);
        }
        cpu_process_pending_event(tipoff);
        /* `$83:ECB0-$ED46`: automatic foul-out continuation is attempted
         * only after the foul has entered the genuine dead-ball consumer.
         * Failure (including no eligible bench player) intentionally leaves
         * `$0A08` pending and preserves every owned state word. */
        if (tipoff->fouls.whistle_active_raw_09b6 != 0u &&
            tipoff->fouls.substitution_request_raw_0a08 != 0u)
            (void)nba_tipoff_apply_foul_out_substitution(tipoff);
    }
    cpu_update_camera(tipoff);
    /* `$87:8EFB`'s due actor pass feeds `$87:A357-$A47A`. OAM retains the
     * previous complete origins on the camera-only frame in between. */
    if ((tipoff->simulation_tick&1u)==0u) latch_player_screen_origins(tipoff);
    /* A61E writes the status of the final projected visible actor to DP47.
     * Preserve the overlapping byte for the next presentation scheduler. */
    if(tipoff->tip_contact_actor<0 && (tipoff->simulation_tick&1u)==0u) {
        unsigned last=visible_submission[7];
        tipoff->scratch_0047=tipoff->actors[last].actor_status_raw_28;
        tipoff->scratch_0046=(uint16_t)((tipoff->scratch_0046&255u)|(tipoff->scratch_0047<<8));
    }
    if (tipoff->tip_possession_frame) tipoff->phase = NBA_TIPOFF_LIVE;
    else if (tipoff->tip_contact_actor >= 0)
        tipoff->phase = NBA_TIPOFF_POSSESSION;
    else if ((int16_t)tipoff->tip_toss_countdown_raw_09f2<0)
        tipoff->phase = NBA_TIPOFF_JUMP_BALL;
}

static void ball_position(const NbaTipoff *tipoff, int *x, int *y) {
    int bx=fp_integer_word(tipoff->ball.x_fp),by=fp_integer_word(tipoff->ball.y_fp);
    int16_t sx,sy;
    nba_court_project_actor((int16_t)bx,(int16_t)by,
        fp_integer_word(tipoff->ball.z_fp),tipoff->camera_x,tipoff->camera_y,
        &sx,&sy);
    *x=sx;*y=sy;
}

static bool actor_visible(unsigned actor) {
    for (unsigned i = 0; i < sizeof(visible_submission); ++i)
        if (visible_submission[i] == actor) return true;
    return false;
}

static uint8_t actor_animation(const NbaTipoff *tipoff, unsigned actor) {
    return tipoff->actors[actor].animation_state;
}

static uint32_t actor_animation_tick(const NbaTipoff *tipoff,unsigned actor) {
    /* Keep contact and rendering on the same existing presentation clock.
     * Native jump-channel startup/cadence is a separate remaining slice. */
    return tipoff->actors[actor].upper_animation_tick;
}

bool nba_tipoff_try_tip_contact(NbaTipoff *tipoff) {
    if(!tipoff || tipoff->tip_contact_actor>=0 || tipoff->live_state_raw!=0x81u ||
       tipoff->ball.owner_actor>=0 || tipoff->ball.state==NBA_BALL_HIDDEN ||
       (tipoff->simulation_tick&1u)!=0u)return false;
    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];cpu_actor_contact_order(tipoff,order);
    for(unsigned i=0;i<NBA_GAMEPLAY_ACTOR_COUNT;++i) {
        unsigned slot=order[i];const NbaTipoffActor *actor=&tipoff->actors[slot];
        if(!cpu_actor_ball_contact_allowed(actor))continue;
        NbaTipContactInput in={0};
        in.actor_id=(uint16_t)slot;in.actor_inhibit=actor->contact_inhibit_raw_5a;
        in.actor_group=slot>=5?5:0;in.upper_state=actor_animation(tipoff,slot);
        in.upper_lock=actor->upper_animation_lock_raw_46;in.live_state=tipoff->live_state_raw;
        in.owner=tipoff->possession_actor;in.receiver=tipoff->pass_receiver_raw;
        in.side_group=tipoff->camera_side_group_raw==255?-1:tipoff->camera_side_group_raw;
        in.hoop_x=tipoff->court_presentation.basket_x_3fef;
        in.actor_x=fp_integer_word(actor->x_fp);in.actor_y=fp_integer_word(actor->y_fp);
        in.actor_z=fp_integer_word(actor->z_fp);
        in.ball_x=fp_integer_word(tipoff->ball.x_fp);in.ball_y=fp_integer_word(tipoff->ball.y_fp);
        in.ball_z=fp_integer_word(tipoff->ball.z_fp);in.ball_vz=tipoff->ball.velocity_z;
        uint16_t upper,lower;uint8_t direction=actor->direction;
        uint8_t team=team_id_for_context(tipoff, slot / 5u);
        if(!actor_animation_resources(tipoff,actor,direction,&upper,&lower) ||
           !nba_player_animation_contact_height(tipoff->assets,team,actor->roster_slot,upper,lower,direction,&in.head_height))continue;
        for(unsigned point=0;point<2;++point) {
            int16_t x,y,z;
            if(!nba_player_ball_attachment_point_offsets(tipoff->assets,upper,lower,
                    direction<3?0x8000u:0u,(uint8_t)point,&x,&y,&z))break;
            in.points[point]=(NbaGameplayPosePoint){(int16_t)(in.actor_x+x),(int16_t)(in.actor_y+y),(int16_t)(in.actor_z+z)};
            ++in.point_count;
        }
        NbaTipContactResult result=nba_tip_contact_geometry(&in);
        if(result.request_reach)tipoff->tip_reach_mask|=(uint16_t)(1u<<slot);
        if(result.route!=NBA_TIP_CONTACT_ACCEPT)continue;
        tipoff->tip_contact_actor=(int8_t)slot;tipoff->tip_contact_frame=(uint32_t)tipoff->frame;
        tipoff->collision_actor_a_raw=(int8_t)slot;tipoff->collision_actor_b_raw=-1;
        tipoff->collision_routine_raw=0x86CCFCu;
        return true;
    }
    return false;
}

bool nba_tipoff_select_tip_receiver(NbaTipoff *tipoff) {
    if(!tipoff || tipoff->tip_contact_actor<0 || tipoff->tip_contact_actor>=10 ||
       tipoff->pass_receiver_raw>=0)return false;
    unsigned slot=(unsigned)tipoff->tip_contact_actor;
    NbaTipoffActor *actor=&tipoff->actors[slot];
    /* D3F2 swaps actor pointers, but DP C2 remains the contact source: the
     * ball record (10). B04C copies C2 to0942; it does not derive it from96. */
    NbaTipReceiver s={0};s.rng=tipoff->rng.state;s.actor_id=10u;
    s.team_group=actor->team_group_raw_6e;s.event_bits=tipoff->tip_event_bits_raw_13e9;
    nba_tip_receiver_select(&s);
    if(s.receiver>=NBA_GAMEPLAY_ACTOR_COUNT)return false;
    tipoff->rng.state=s.rng;tipoff->tip_event=s.event;
    tipoff->pass_actor_raw=(int8_t)s.passer;tipoff->pass_receiver_raw=(int8_t)s.receiver;
    actor->pass_family_raw=(int16_t)s.pass_family;actor->pass_band_raw=s.pass_band;
    tipoff->actors[s.receiver].control_mode=(uint8_t)s.receiver_mode;
    tipoff->handler_actor=tipoff->receiver_actor=(uint8_t)s.receiver;
    tipoff->possession_team=(int8_t)(s.team_group/5);
    tipoff->camera_side_group_raw=(uint8_t)s.team_group;
    tipoff->tip_winner_group_raw_0932=s.team_group;
    tipoff->pass_aux_raw=actor->controller_assignment_raw;
    if(!nba_tipoff_launch_tip_ball(tipoff))return false;
    nba_tip_receiver_finish(&s);
    tipoff->tip_event_bits_raw_13e9=s.event_bits;
    return true;
}

bool nba_tipoff_launch_tip_ball(NbaTipoff *t) {
    if(!t || t->tip_contact_actor<0 || t->tip_contact_actor>=10 ||
       t->pass_receiver_raw<0 || t->pass_receiver_raw>=10)return false;
    unsigned slot=(unsigned)t->tip_contact_actor;
    NbaTipoffActor *p=&t->actors[slot],*r=&t->actors[(unsigned)t->pass_receiver_raw];
    NbaTipLaunch s={0};
    s.ball_x=fp_integer_word(t->ball.x_fp);s.ball_y=fp_integer_word(t->ball.y_fp);s.ball_z=fp_integer_word(t->ball.z_fp);
    s.receiver_x=fp_integer_word(r->x_fp);s.receiver_y=fp_integer_word(r->y_fp);
    s.receiver_vx=r->velocity_x;s.receiver_vy=r->velocity_y;s.passer_z=fp_integer_word(p->z_fp);
    s.pass_family=p->pass_family_raw;s.band=p->pass_band_raw;s.upper_state=actor_animation(t,slot);
    s.passer_mode=p->control_mode;s.receiver_mode=r->control_mode;s.receiver_timer=r->reaction_threshold;
    s.passer_group=p->team_group_raw_6e;s.active_group=t->camera_side_group_raw;
    s.passer_timer=p->reaction_threshold;s.behavior_timer=p->behavior_timer;s.flags=p->behavior_flags_raw;s.status=p->actor_status_raw_28;
    s.live_state=t->live_state_raw;
    /* DP E0/E2 provenance is not yet represented by the host dispatcher.
     * Their copy is replayed in the pure function; no invented ROM pointer. */
    if(!nba_tip_launch(t->assets,&s))return false;
    t->tip_last_launch=s;
    t->ball.x_fp=fp_replace_integer_word(t->ball.x_fp,s.ball_x);
    t->ball.y_fp=fp_replace_integer_word(t->ball.y_fp,s.ball_y);
    t->ball.velocity_x=s.ball_vx;t->ball.velocity_y=s.ball_vy;
    /* `$86:D41B-$D426`: keep BOTH original Z words, then zero VZ. */
    t->ball.velocity_z=0;t->ball.owner_actor=-1;t->ball.state=NBA_BALL_PASS;
    t->possession_actor=-1;t->rim_raw_094a=s.latch;t->catch_actor_record_raw_0910=s.ball_record;
    t->live_state_raw=s.live_state;r->velocity_x=s.receiver_vx;r->velocity_y=s.receiver_vy;
    /* `$86:D3C6-$D43D` publishes the launch with the receiver's exact +$60
     * value.  The normal mode-10 actor pass performs the later two-count
     * decrement.  Keeping that scheduling boundary matters: subtracting
     * here made this wrapper disagree with every native witness (39 -> 37).
     *
     * ROM-COMPATIBILITY QUIRK: +$60 is an odd value for these tip launches;
     * the signed countdown intentionally advances through odd values. */
    r->reaction_threshold=s.receiver_timer;
    p->control_mode=(uint8_t)s.passer_mode;p->reaction_threshold=s.passer_timer;p->behavior_timer=s.behavior_timer;
    p->behavior_flags_raw=s.flags;p->actor_status_raw_28=s.status;p->contact_inhibit_raw_5a=s.inhibit;
    /* `$86:D429-$D43C`: inhibit the opposite center as well. */
    t->actors[t->tip_winner_group_raw_0932?0:5].contact_inhibit_raw_5a=20;
    return true;
}

void nba_tipoff_capture_telemetry(const NbaTipoff *tipoff,
                                  const NbaInput *input,
                                  NbaGameplayTelemetry *telemetry) {
    if (!tipoff || !telemetry) return;
    memset(telemetry, 0, sizeof(*telemetry));
    telemetry->scene_frame = (uint32_t)tipoff->frame;
    telemetry->tip_contact_actor=tipoff->tip_contact_actor;
    telemetry->shot_launch_serial=tipoff->shot_launch_serial;
    telemetry->shot_launch_actor=tipoff->last_shot_launch.last_owner;
    telemetry->shot_launch_value=tipoff->last_shot_launch.initial_value;
    telemetry->tip_contact_frame=tipoff->tip_contact_frame;
    telemetry->tip_possession_frame=tipoff->tip_possession_frame;
    telemetry->tip_reach_mask=tipoff->tip_reach_mask;
    telemetry->tip_toss_countdown_raw=tipoff->tip_toss_countdown_raw_09f2;
    telemetry->jump_scratch_raw=tipoff->scratch_0046;
    telemetry->jump_calls=tipoff->jump_decision_calls;
    telemetry->jump_launches=tipoff->jump_launches;
    telemetry->jump_rejections=tipoff->jump_rejected_contexts;
    telemetry->ball_velocity_raw[0]=(uint16_t)tipoff->ball.velocity_x;
    telemetry->ball_velocity_raw[1]=(uint16_t)tipoff->ball.velocity_y;
    telemetry->ball_velocity_raw[2]=(uint16_t)tipoff->ball.velocity_z;
    telemetry->simulation_tick = tipoff->simulation_tick;
    telemetry->phase = (uint8_t)tipoff->phase;
    /* The native capture counts actual $85:963D entries. Report the C
     * physical actor pass only when cpu_update_all_actors ran this update;
     * a due tick that returned through pause, period or free-throw handling
     * is not an executed pass. Full native scheduling remains separate. */
    telemetry->scheduler_due_raw = (uint8_t)tipoff->actor_pass_executed;
    telemetry->actor_pass_dt_raw = telemetry->scheduler_due_raw ? 2u : 0u;
    telemetry->actor_pass_mask_raw = telemetry->scheduler_due_raw ? 0x03FFu : 0u;
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor)
        telemetry->actor_pass_order_raw[actor] = telemetry->scheduler_due_raw ?
            (uint8_t)actor : 0xFFu;
    telemetry->input_pressed = input ? input->pressed : 0u;
    telemetry->input_held = input ? input->held : 0u;
    telemetry->input_released = input ? input->released : 0u;
    telemetry->pad_held_raw[0] = nba_controller_native_buttons(telemetry->input_held);
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
    telemetry->possession_candidate_raw = tipoff->tip_contact_actor >= 0 ?
                                          tipoff->handler_actor : -1;
    telemetry->play_code_raw = tipoff->tip_contact_actor>=0 ?
                               tipoff->play_code : NBA_GAMEPLAY_UNKNOWN_WORD;
    telemetry->play_step_raw = tipoff->play_step_raw;
    telemetry->play_countdown_raw = tipoff->play_countdown_raw;
    telemetry->play_mirror_raw = tipoff->play_mirror_raw;
    telemetry->play_event_wait_raw = tipoff->play_event_wait_raw;
    telemetry->play_request_raw = tipoff->play_request_raw;
    telemetry->play_consumed_serial = tipoff->play_consumed_serial;
    telemetry->play_cycle_raw = tipoff->play_cycle_raw;
    telemetry->play_hold_raw = tipoff->play_hold_raw;
    telemetry->role_rebuild_raw_09d6 = tipoff->role_rebuild_raw_09d6;
    telemetry->special_actor_raw = tipoff->special_actor_raw;
    for (unsigned i = 0; i < 3u; ++i)
        telemetry->play_selector_raw[i] = tipoff->play_selector_raw[i];
    telemetry->rng_state_raw = tipoff->rng.state;
    /* Historical protocol names are retained until a coordinated schema
     * migration. These are native `$4711` home and `$4791` visitor words;
     * they are not screen-left/screen-right. The HUD uses the opposite
     * display order. Do not silently swap this native projection. */
    telemetry->score_left_raw = tipoff->session->score[0];
    telemetry->score_right_raw = tipoff->session->score[1];
    telemetry->period_raw_0926 = tipoff->period_raw_0926;
    telemetry->match_clock_raw_0928 = tipoff->match_clock_raw_0928;
    for (unsigned side = 0; side < 2u; ++side) {
        telemetry->team_context_mode_raw_30[side] =
            tipoff->team_context[side].mode_raw_30;
        telemetry->team_context_flags_raw_32[side] =
            tipoff->team_context[side].flags_raw_32;
        telemetry->team_context_activity_raw_39[side] =
            tipoff->team_context[side].activity_raw_39;
        telemetry->team_context_dead_ball_actor_raw_3f[side] =
            tipoff->team_context[side].dead_ball_actor_raw_3f;
    }
    telemetry->shot_clock_raw_092c = tipoff->rim_raw_092c;
    telemetry->shot_clock_mirror_raw_09c6 =
        tipoff->shot_clock_mirror_raw_09c6;
    telemetry->shot_value_raw = tipoff->shot_value_raw;
    telemetry->shot_actor_raw_09c8 = tipoff->shot_actor_raw_09c8;
    telemetry->interference_value_raw_096a = tipoff->rim_raw_096a;
    telemetry->shot_chance_raw = tipoff->shot_chance_raw;
    telemetry->shot_miss_index_raw = tipoff->shot_miss_index_raw;
    telemetry->shot_inner_veto_raw = tipoff->shot_inner_veto_raw ? 1u : 0u;
    telemetry->shot_selection_serial=tipoff->shot_selection_serial;
    telemetry->shot_fatigue=tipoff->fatigue;
    telemetry->shot_assistance_team=tipoff->assistance_team_raw_09c0;
    for (unsigned i=0;i<10;++i) {
        telemetry->shot_made_run[i]=tipoff->actors[i].shot_modifier_raw_b2;
        telemetry->shot_defensive_run[i]=tipoff->actors[i].defensive_run_raw_b4;
    }
    memcpy(telemetry->shot_selection_inputs,tipoff->shot_selection_inputs,
           sizeof(telemetry->shot_selection_inputs));
    telemetry->live_state_raw = tipoff->live_state_raw;
    telemetry->inbound_state_raw = tipoff->inbound_state_raw;
    telemetry->inbound_actor_raw = tipoff->inbound_actor_raw;
    telemetry->inbound_timer_raw = tipoff->inbound_timer_raw;
    telemetry->inbound_layout_raw = tipoff->inbound_layout_raw;
    telemetry->inbound_target_x_raw = tipoff->inbound_target_x_raw;
    telemetry->inbound_target_y_raw = tipoff->inbound_target_y_raw;
    telemetry->inbound_direction_raw = tipoff->inbound_direction_raw;
    telemetry->inbound_ready_raw = tipoff->inbound_ready_raw;
    telemetry->inbound_transfer_raw = tipoff->inbound_transfer_raw;
    telemetry->dead_ball_raw_0966 = tipoff->dead_ball_raw_0966;
    telemetry->dead_ball_raw_0968 = tipoff->dead_ball_raw_0968;
    telemetry->dead_ball_raw_096c = tipoff->dead_ball_raw_096c;
    telemetry->dead_ball_x_raw_09b0 = tipoff->dead_ball_x_raw_09b0;
    telemetry->dead_ball_y_raw_09b2 = tipoff->dead_ball_y_raw_09b2;
    telemetry->rim_context_raw_097c = tipoff->rim_raw_097c;
    telemetry->rim_contact_count_raw_0920 = tipoff->rim_raw_0920;
    telemetry->rim_response_raw_0970 = tipoff->rim_raw_0970;
    telemetry->effect_gate_raw_3f33 = tipoff->rim_effect.gate_raw_3f33;
    telemetry->effect_resource_raw_4015 =
        tipoff->rim_effect.resource_raw_4015;
    telemetry->rim_effect_raw_401b = tipoff->rim_effect.effect_raw_401b;
    telemetry->effect_frame_raw_4025 = tipoff->rim_effect.frame_raw_4025;
    telemetry->effect_timer_raw_402d = tipoff->rim_effect.timer_raw_402d;
    telemetry->rim_impact_raw_13e5 = tipoff->rim_impact_raw_13e5;
    telemetry->event_bits_raw_13e7 = tipoff->rim_raw_13e7;
    telemetry->foul_event_raw = tipoff->fouls.foul_event_raw_0964;
    telemetry->shooting_foul_raw = tipoff->fouls.shooting_foul_raw_09bc;
    telemetry->foul_offender_raw = tipoff->fouls.offender_actor_raw;
    telemetry->foul_victim_raw = tipoff->fouls.victim_actor_raw;
    telemetry->team_fouls_raw[0] = tipoff->fouls.team_fouls[0];
    telemetry->team_fouls_raw[1] = tipoff->fouls.team_fouls[1];
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor)
        telemetry->personal_fouls_raw[actor] =
            tipoff->fouls.personal_fouls[actor];
    telemetry->free_throw_state_raw =
        tipoff->fouls.free_throw_state_raw_0978;
    telemetry->free_throw_sequence_raw =
        tipoff->fouls.free_throw_sequence_raw_097a;
    telemetry->free_throw_start_tick_raw_09be =
        tipoff->free_throw_start_tick_raw_09be;
    telemetry->free_throw_aim_x_raw_0980 =
        tipoff->free_throw_aim_x_raw_0980;
    telemetry->free_throw_aim_y_raw_0982 =
        tipoff->free_throw_aim_y_raw_0982;
    telemetry->free_throw_aim_accumulator_raw_0984 =
        tipoff->free_throw_aim_accumulator_raw_0984;
    telemetry->free_throw_aim_step_raw_0986 =
        tipoff->free_throw_aim_step_raw_0986;
    telemetry->free_throw_flight_timer_raw_0930 =
        tipoff->free_throw_flight_timer_raw_0930;
    telemetry->deferred_shot_foul_phase_raw_0a02 =
        tipoff->deferred_shot_foul_phase_raw_0a02;
    telemetry->latched_event_raw_08f0 =
        tipoff->fouls.latched_event_raw_08f0;
    telemetry->whistle_active_raw_09b6 =
        tipoff->fouls.whistle_active_raw_09b6;
    telemetry->whistle_timer_raw_08de =
        (uint16_t)tipoff->fouls.whistle_timer_raw_08de;
    telemetry->presentation_gate_raw_08e2 =
        tipoff->fouls.presentation_gate_raw_08e2;
    telemetry->whistle_presentation_queued_raw =
        tipoff->fouls.whistle_presentation_queued_raw;
    telemetry->hud_sequence_raw_08e6=tipoff->fouls.whistle_state_raw_08e6;
    telemetry->hud_kind_raw_08e8=tipoff->fouls.whistle_state_mirror_raw_08e8;
    telemetry->hud_event_actor_raw_492d=tipoff->hud_event_actor_raw_492d;
    telemetry->hud_clear_raw_08ee=tipoff->hud.clear_raw_08ee;
    telemetry->hud_pending_routine=tipoff->hud.pending_routine;
    telemetry->ball_activity_raw = tipoff->ball_activity_raw;
    telemetry->pass_actor_raw = tipoff->pass_actor_raw;
    telemetry->pass_receiver_raw = tipoff->pass_receiver_raw;
    telemetry->pass_active_raw = tipoff->pass_active_raw;
    telemetry->pass_distance_raw = tipoff->pass_distance_raw;
    telemetry->collision_actor_a = tipoff->collision_actor_a_raw;
    telemetry->collision_actor_b = tipoff->collision_actor_b_raw;
    telemetry->player_contact_count_raw = tipoff->player_contact_count_raw;
    telemetry->player_contact_actor_a_raw = tipoff->player_contact_actor_a_raw;
    telemetry->player_contact_actor_b_raw = tipoff->player_contact_actor_b_raw;
    telemetry->player_contact_routine_raw = tipoff->player_contact_routine_raw;
    telemetry->controller_routine = 0x80CB8Fu;
    telemetry->selection_routine = 0x85C37Du;
    telemetry->collision_routine = tipoff->collision_routine_raw;
    telemetry->possession_routine = (tipoff->rim_raw_13e7&0x10u) ?
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
    telemetry->camera_0874_raw = tipoff->court_stream.destination;
    telemetry->camera_0876_raw = tipoff->court_stream.source;
    telemetry->camera_0878_raw = tipoff->court_stream.next_scroll_x;
    telemetry->camera_087a_raw = tipoff->court_stream.next_scroll_y;
    telemetry->camera_087c_raw = tipoff->court_presentation.window_x_087c;
    telemetry->camera_087e_raw = tipoff->court_presentation.window_y_087e;
    telemetry->camera_0880_raw = tipoff->court_presentation.window_left_0880;
    telemetry->camera_0882_raw = tipoff->court_presentation.window_right_0882;
    telemetry->camera_basket_x_raw = tipoff->court_presentation.basket_x_3fef;
    telemetry->camera_stream_row_bytes = tipoff->court_stream.row_bytes;
    telemetry->camera_initialized_raw = tipoff->camera.initialized_4a54;
    telemetry->camera_pointer_raw = tipoff->camera.subject_pointer_0940;
    telemetry->camera_ticks_raw = tipoff->camera.presentation_ticks_0564;
    telemetry->camera_alternate_raw = tipoff->camera_alternate_raw_08bc;
    telemetry->camera_alternate_mode_raw = tipoff->camera_alternate_mode_raw_08cc;
    telemetry->camera_proxy_raw[0] = tipoff->camera.proxy.x_fraction;
    telemetry->camera_proxy_raw[1] = tipoff->camera.proxy.x_integer;
    telemetry->camera_proxy_raw[2] = tipoff->camera.proxy.y_fraction;
    telemetry->camera_proxy_raw[3] = tipoff->camera.proxy.y_integer;
    telemetry->camera_routine = 0x859192u;

    telemetry->ball.world_x = fp_round(tipoff->ball.x_fp);
    telemetry->ball.world_y = fp_round(tipoff->ball.y_fp);
    telemetry->ball.world_z = fp_round(tipoff->ball.z_fp);
    telemetry->ball.world_x_fp = tipoff->ball.x_fp;
    telemetry->ball.world_y_fp = tipoff->ball.y_fp;
    telemetry->ball.world_z_fp = tipoff->ball.z_fp;
    if (tipoff->tip_contact_actor < 0) {
        nba_court_project_actor(fp_integer_word(tipoff->ball.x_fp),
            fp_integer_word(tipoff->ball.y_fp),fp_integer_word(tipoff->ball.z_fp),
            tipoff->camera_x,tipoff->camera_y,
            &telemetry->ball.screen_x,&telemetry->ball.screen_y);
    } else {
        telemetry->ball.screen_x = tipoff->ball_screen_x;
        telemetry->ball.screen_y = tipoff->ball_screen_y;
    }
    telemetry->ball.velocity_x = fp_round(tipoff->ball.velocity_x);
    telemetry->ball.velocity_y = fp_round(tipoff->ball.velocity_y);
    telemetry->ball.velocity_z = fp_round(tipoff->ball.velocity_z);
    telemetry->ball.owner_actor = tipoff->ball.owner_actor;
    telemetry->ball.state = tipoff->ball.state;
    telemetry->ball.routine = tipoff->tip_contact_actor < 0 ?
        SNES_ADDR_TIPOFF_BALL_INIT :
        tipoff->ball.state == NBA_BALL_ATTACHED ? 0x87B649u : 0x85A518u;
    telemetry->ball.flags_raw = NBA_GAMEPLAY_UNKNOWN_WORD;

    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaGameplayActorTelemetry *out = &telemetry->actors[actor];
        const NbaTipoffActor *state = &tipoff->actors[actor];
        out->index = (uint8_t)actor;
        out->team_side = actor >= 5u;
        out->roster_slot = state->roster_slot;
        bool live = true; /* Initial tip uses the same actor state as live play. */
        out->control = NBA_GAMEPLAY_CONTROL_CPU;
        out->world_x = live ? fp_round(state->x_fp) : formation[actor].world_x;
        out->world_y = live ? fp_round(state->y_fp) : formation[actor].world_y;
        out->world_x_fp = live ? state->x_fp : (int32_t)out->world_x * 256;
        out->world_y_fp = live ? state->y_fp : (int32_t)out->world_y * 256;
        out->world_z = fp_integer_word(state->z_fp);
        out->world_z_fp = live ? state->z_fp : (int32_t)out->world_z * 256;
        if(live) {
            out->screen_x=tipoff->player_screen_x[actor];
            out->screen_y=tipoff->player_screen_y[actor];
        }
        else { out->screen_x=formation[actor].screen_x;
               out->screen_y=(int16_t)(formation[actor].screen_y-out->world_z); }
        out->visible = live ? tipoff->player_screen_visible[actor] :
            actor_visible(actor);
        /* Actor +$0E/+$10 are already signed 8.8 ROM velocity words. Keep
         * them raw so CLI/JSON can compare directly with the Mesen oracle. */
        out->velocity_x = live ? state->velocity_x : 0;
        out->velocity_y = live ? state->velocity_y : 0;
        out->velocity_z = live ? state->velocity_z : 0;
        out->direction = live ? state->direction : formation[actor].direction;
        out->draw_direction_raw = live ? actor_draw_direction(tipoff, actor) :
                                  out->direction;
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
        out->flags_raw = live ? state->actor_status_raw_28 :
                         NBA_GAMEPLAY_UNKNOWN_WORD;
        uint16_t upper_resource = 0u, lower_resource = 0u;
        if (live && actor_animation_resources(tipoff, state, state->direction,
                &upper_resource, &lower_resource)) {
            out->upper_resource_raw = upper_resource;
            out->lower_resource_raw = lower_resource;
        } else {
            out->upper_resource_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
            out->lower_resource_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        }
        out->head_resource_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        uint16_t draw_upper_resource = 0u, draw_lower_resource = 0u;
        if (live && actor_draw_body_resources(tipoff, state,
                (uint8_t)out->draw_direction_raw,
                &draw_upper_resource, &draw_lower_resource)) {
            out->draw_upper_resource_raw = draw_upper_resource;
            out->draw_lower_resource_raw = draw_lower_resource;
        } else {
            out->draw_upper_resource_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
            out->draw_lower_resource_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        }
        if (live && out->draw_upper_resource_raw != NBA_GAMEPLAY_UNKNOWN_WORD &&
            out->draw_lower_resource_raw != NBA_GAMEPLAY_UNKNOWN_WORD) {
            uint8_t team_side = actor >= 5u;
            uint8_t team = team_id_for_context(tipoff, team_side);
            NbaPlayerSpriteDiagnostics appearance = {0};
            bool appearance_valid = nba_player_sprite_diagnose_resources(
                tipoff->assets, team, state->roster_slot, team_side,
                (uint8_t)out->draw_direction_raw, out->draw_upper_resource_raw,
                out->draw_lower_resource_raw, &appearance);
            out->head_resource_raw = appearance.head_resource;
            out->appearance_resource_raw[0] = appearance.lower_resource;
            out->appearance_resource_raw[1] = appearance.upper_resource;
            out->appearance_resource_raw[2] = appearance.head_resource;
            out->appearance_resource_raw[3] = appearance.number_resource;
            out->appearance_opaque_pixels[0] = appearance.lower_opaque_pixels;
            out->appearance_opaque_pixels[1] = appearance.upper_opaque_pixels;
            out->appearance_opaque_pixels[2] = appearance.head_opaque_pixels;
            out->appearance_opaque_pixels[3] = appearance.number_opaque_pixels;
            out->appearance_flags_raw =
                (appearance.player_palette_valid ? 0x01u : 0u) |
                (appearance.number_allowed ? 0x02u : 0u) |
                (appearance.number_composed ? 0x04u : 0u) |
                (appearance.number_palette_valid ? 0x08u : 0u) |
                (appearance.lower_resource_valid ? 0x10u : 0u) |
                (appearance.upper_resource_valid ? 0x20u : 0u) |
                (appearance.head_resource_valid ? 0x40u : 0u) |
                (appearance_valid ? 0x80u : 0u);
        }
        /* `$87:B572-$B648` consumes these exact actor words. Export the
         * represented values instead of hiding them behind FFFF so a Mesen
         * trace can distinguish locomotion-state errors from descriptor
         * phase errors. Historical motion_3c/lower_phase keys remain legacy
         * ticks for existing consumers; animation_rom below exports literal
         * ROM phases/accumulators/resources without conflating the clocks. */
        out->motion_38_raw = live ? state->base_animation_state_raw_38 : 0u;
        out->motion_3a_raw = live ? state->upper_animation_phase_raw : 0u;
        out->motion_3c_raw = live ? state->lower_animation_tick : 0u;
        out->direction_4e_raw = out->direction;
        out->direction_50_raw = state->requested_direction;
        out->direction_52_raw = out->direction;
        out->target_x_56_raw = state->target_x;
        out->target_y_58_raw = state->target_y;
        out->side_group_raw = actor >= 5u ? 5u : 0u;
        out->control_mode_raw = state->control_mode;
        out->mode_saved_62_raw = state->pass_band_raw;
        out->pass_band_62_raw = state->pass_band_raw;
        out->pass_direction_66_raw = state->pass_direction_raw;
        out->control_mode_saved_raw = state->saved_control_mode;
        out->saved_mode_84_raw = state->saved_control_mode;
        out->pass_family_c0_raw = state->pass_family_raw;
        out->pass_release_threshold_raw = state->pass_release_threshold_raw;
        out->pass_released_raw = state->pass_released_raw ? 1u : 0u;
        out->assignment_base_raw = state->assignment_base_raw;
        out->assignment_current_raw = state->assignment_current_raw;
        out->assignment_alternate_raw = state->assignment_alternate_raw;
        out->assignment_distance_raw = state->assignment_distance;
        out->assignment_direction_raw = state->assignment_direction;
        out->anchor_direction_raw_88 = state->anchor_direction_raw;
        out->assignment_role_raw_92 = state->assignment_role_raw_92;
        out->pair_distance_raw = state->pair_distance;
        out->anchor_distance_raw_8c = state->anchor_distance_raw;
        out->reaction_threshold_raw = state->reaction_threshold;
        out->movement_boost_raw = state->movement_boost_timer;
        out->controller_assignment_16_raw =
            state->controller_assignment_raw;
        out->movement_magnitude_4c_raw = state->movement_magnitude_raw;
        bool close_finish = state->control_mode == 13u ||
                            state->control_mode == 14u;
        out->mode13_timer_60_raw = state->control_mode == 13u ?
            state->contact_action_timer_raw_60 :
            state->control_mode == 14u ? state->reaction_threshold : 0u;
        out->mode13_selector_56_raw = close_finish ?
            state->special_contact_raw_56 : -1;
        out->mode13_variant_58_raw = close_finish ?
            state->mode13_variant_raw_58 : 0u;
        out->mode13_baseline_vx_ba_raw = close_finish ?
            state->mode13_baseline_velocity_x : 0;
        out->mode13_baseline_vy_bc_raw = close_finish ?
            state->mode13_baseline_velocity_y : 0;
        out->contact_inhibit_5a_raw = state->contact_inhibit_raw_5a;
        out->contact_height_aa_raw = state->contact_height_raw_aa;
        out->recovery_inhibit_7a_raw = state->recovery_inhibit_raw;
        out->upper_restart_raw = out->lower_restart_raw = 0;
        out->upper_phase_raw = live ? state->upper_animation_phase_raw : 0u;
        out->lower_phase_raw = live ? state->lower_animation_tick : 0u;
        const uint16_t animation_words[10] = {
            state->upper_animation_resource_raw_2a, state->lower_animation_resource_raw_2c,
            state->rom_upper_animation_phase_raw_3a, state->rom_lower_animation_phase_raw_3c,
            state->upper_animation_accumulator_raw_42, state->lower_animation_accumulator_raw_44,
            state->upper_animation_lock_raw_46, state->lower_animation_lock_raw_48,
            state->animation_upper_queue_cursor_raw_18, state->animation_lower_queue_cursor_raw_1a
        };
        memcpy(out->animation_rom_words, animation_words, sizeof(animation_words));
        out->animation_resources_valid = live && state->animation_resources_valid;
        out->animation_phase_target_raw_b0 = state->upper_phase_target_raw_b0;
        out->animation_action_integrated = live &&
            (state->exact_pass_animation ||
             ((state->control_mode == 12u || state->control_mode == 17u) && state->exact_shot_animation));
        out->behavior_flags_raw = state->behavior_flags_raw;
        out->palette_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->actor_routine = live ? 0x85963Du : 0x80AD92u;
        out->ai_routine = nba_gameplay_behavior_routine(state->control_mode);
    }
}

void nba_tipoff_render(const NbaTipoff *tipoff, NbaRenderer *ren) {
    if (!tipoff || !tipoff->is_initialized || !ren) return;
    const uint8_t *gameplay_vram = NULL, *gameplay_cgram = NULL;
    if (!nba_assets_gameplay_ppu_input(
            tipoff->assets, team_id_for_context(tipoff, 0u),
            &gameplay_vram, &gameplay_cgram)) return;
    /* Captured court input contains a historical score panel. Replace only
     * source-owned BG3 map/CHR/palette with the live indexed HUD before any
     * Mode1 layer samples it. The port is single-threaded; never mutate the
     * asset pack or preserve stale captured WEST2/ORLANDO0 presentation. */
    static uint8_t hud_vram[0x10000],hud_cgram[0x200];
    memcpy(hud_vram,gameplay_vram,sizeof(hud_vram));
    memcpy(hud_cgram,gameplay_cgram,sizeof(hud_cgram));
    if(!nba_gameplay_hud_apply(&tipoff->hud,tipoff->assets,hud_vram,hud_cgram))return;
    gameplay_vram=hud_vram;gameplay_cgram=hud_cgram;
    int crop_x,crop_y;
    nba_court_viewport(tipoff->camera_x,tipoff->camera_y,&crop_x,&crop_y);
    uint32_t backdrop = nba_snes_cgram_color(gameplay_cgram, 0, 15, 0, 0, 0);
    if (!nba_snes_mode1_begin(ren, backdrop, true)) return;
    draw_gameplay_court_bg2(tipoff, ren, gameplay_vram, gameplay_cgram,
                            crop_x, crop_y);
    draw_animated_crowd(tipoff, ren, crop_x, crop_y);
    draw_gameplay_goal_bg(tipoff, ren, gameplay_vram, gameplay_cgram);
    draw_gameplay_hud_bg3(ren, gameplay_vram, gameplay_cgram);

    /* The application is single-threaded like the SNES scanout. Reusing one
     * transparent object plane avoids allocating a framebuffer every frame. */
    static NbaRenderer object_plane;
    static bool object_plane_initialized;
    if (!object_plane_initialized) {
        nba_renderer_init(&object_plane);
        object_plane_initialized = true;
    }
    nba_renderer_clear(&object_plane, 0u);
    draw_gameplay_goal_obj(tipoff, &object_plane, gameplay_cgram);
    /* `$87:A7D5-$A81B` submits the basket resource into native OAM slots
     * 33/34 with OBJ priority 3. Priority 2 incorrectly placed rim/net
     * pixels behind BG1's high-priority board edge. */
    submit_argb_object(ren, &object_plane, 3u, 33u);

    uint8_t render_order[NBA_GAMEPLAY_ACTOR_COUNT];
    int16_t screen_x[NBA_GAMEPLAY_ACTOR_COUNT];
    int16_t screen_y[NBA_GAMEPLAY_ACTOR_COUNT];
    unsigned render_count = 0;
    {
        for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
            if(tipoff->tip_contact_actor<0 && !actor_visible(actor))continue;
            screen_x[actor]=tipoff->player_screen_x[actor];
            screen_y[actor]=tipoff->player_screen_y[actor];
            if (tipoff->player_screen_visible[actor])
                render_order[render_count++] = (uint8_t)actor;
        }
        /* `$80:AD92`/`$80:B391` use OAM order, not screen-Y painter sorting.
         * The Mode-1 compositor resolves overlaps from the carried FC80 list. */
    }

    bool ball_in_player = false;
    for (unsigned order = 0; order < render_count; ++order) {
        unsigned actor = render_order[order];
        uint8_t team_side = actor >= 5u;
        uint8_t uniform_side = team_side;
        uint8_t slot = tipoff->actors[actor].roster_slot;
        uint8_t team = team_id_for_context(tipoff, team_side);
        uint8_t state=tipoff->actors[actor].animation_state;
        int jump=0; /* screen_y already contains actual actor Z. */
        uint8_t direction=actor_draw_direction(tipoff, actor);
        uint8_t lower_state=tipoff->actors[actor].lower_animation_state;
        uint32_t upper_tick=tipoff->actors[actor].upper_animation_tick;
        uint32_t lower_tick=tipoff->actors[actor].lower_animation_tick;
        nba_renderer_clear(&object_plane, 0u);
        uint16_t draw_upper_resource = 0u;
        uint16_t draw_lower_resource = 0u;
        uint8_t object_priority = 2u;
        /* D4/D6 are the already-published body pose. A5FA selects the head
         * independently; it does not rotate the torso/legs or ball point. */
        if (actor_draw_body_resources(tipoff, &tipoff->actors[actor], direction,
                &draw_upper_resource, &draw_lower_resource)) {
            uint16_t head_order = 0u, number_resource = 0u;
            uint16_t head_base = 0u, palette_offset = 0u;
            NbaGameplayDrawPreparation prepared = {0};
            bool literal = nba_player_sprite_pose_table_inputs(
                    tipoff->assets, draw_upper_resource,
                    tipoff->actors[actor].direction,
                    &head_order, &number_resource) &&
                nba_player_sprite_pose_identity(tipoff->assets, team, slot,
                    uniform_side, &head_base, &palette_offset);
            if (literal) {
                int16_t world_z = fp_integer_word(tipoff->actors[actor].z_fp);
                NbaGameplayDrawPreparationInput preparation = {
                    .direction = actor_draw_direction_input(tipoff, actor),
                    .status = tipoff->actors[actor].actor_status_raw_28,
                    .upper_resource = draw_upper_resource,
                    .lower_resource = draw_lower_resource,
                    .world_x = fp_integer_word(tipoff->actors[actor].x_fp),
                    .world_y = fp_integer_word(tipoff->actors[actor].y_fp),
                    .world_z = world_z,
                    .screen_x = screen_x[actor],
                    /* The retained origin already includes A620's Z
                     * subtraction. Restore the pre-A620 input word here. */
                    .screen_y = (int16_t)((uint16_t)screen_y[actor] +
                                          (uint16_t)world_z),
                    .head_base = head_base,
                    .palette_offset = palette_offset
                };
                nba_gameplay_prepare_player_draw(&preparation, &prepared);
                object_priority = (uint8_t)((prepared.attribute >> 12) & 3u);
                NbaPlayerSpritePoseInput pose = {
                    .upper_d6 = draw_upper_resource,
                    .lower_d4 = draw_lower_resource,
                    .head_da = prepared.head_resource,
                    .number_d8 = number_resource,
                    .flags_47 = prepared.status,
                    .head_order_51 = head_order,
                    .movement_c0 = tipoff->actors[actor].movement_direction,
                    .attribute_4f = prepared.attribute,
                    /* 0884 is queue work and has no pixel effect. This
                     * adapter neither exports nor claims its live value. */
                    .glyph_work_0884 = 0u,
                    .x = prepared.x,
                    .y = (int16_t)(prepared.y - jump)
                };
                int16_t ball_order = 0;
                bool dribble = tipoff->ball.state != NBA_BALL_HIDDEN &&
                    tipoff->ball.owner_actor == (int8_t)actor &&
                    tipoff->possession_actor == (int8_t)actor &&
                    tipoff->actors[actor].control_mode == 11u &&
                    tipoff->live_state_raw < 0x80u &&
                    tipoff->fouls.free_throw_state_raw_0978 == 0u &&
                    tipoff->rim_effect.resource_raw_4015 < 0x082cu &&
                    draw_upper_resource < 0x00f0u &&
                    nba_player_ball_draw_order(tipoff->assets, draw_upper_resource, &ball_order);
                if (dribble) {
                    /* A47A walks the carried list backwards; visiting the
                     * ball first sets 3F31 before this owner's AF1E call.
                     * Z does not participate in that sort, and equal depths
                     * retain their actual prior order. */
                    NbaPlayerSpriteBallInput ball = {
                        ball_order,
                        actor_oam_index(tipoff, 10u) < actor_oam_index(tipoff, actor) ? 1u : 0u,
                        (uint16_t)((prepared.attribute & 0x3000u) | 0x0c00u),
                        tipoff->ball_screen_x, tipoff->ball_screen_y
                    };
                    literal = nba_player_sprite_render_pose_with_ball(&object_plane,
                        tipoff->assets, team, slot, uniform_side,
                        tipoff->actors[actor].direction, &pose, &ball, 1);
                    ball_in_player = literal;
                } else {
                    literal = nba_player_sprite_render_pose(&object_plane,
                        tipoff->assets, team, slot, uniform_side,
                        tipoff->actors[actor].direction, &pose, 1);
                }
            }
            if (!literal)
                nba_player_sprite_render_resources(
                    &object_plane, tipoff->assets, team, slot, uniform_side,
                    direction, draw_upper_resource, draw_lower_resource,
                    screen_x[actor], screen_y[actor] - jump, 1);
        } else {
            nba_player_sprite_render_split(&object_plane, tipoff->assets, team, slot,
                                           uniform_side, state, lower_state,
                                           direction, upper_tick, lower_tick,
                                           screen_x[actor],
                                           screen_y[actor] - jump, 1);
        }
        submit_argb_object(ren, &object_plane, object_priority, actor_oam_index(tipoff, actor));
    }
    int ball_x, ball_y;
    if (tipoff->tip_contact_actor < 0) {
        ball_position(tipoff, &ball_x, &ball_y);
    } else {
        /* Player and ball OBJ coordinates are one retained OAM submission.
         * Using the current camera here while player origins remain latched
         * creates a one-frame hand/ball separation on camera-only frames. */
        ball_x=tipoff->ball_screen_x;
        ball_y=tipoff->ball_screen_y;
    }
    if (tipoff->ball.state!=NBA_BALL_HIDDEN && !ball_in_player) {
        nba_renderer_clear(&object_plane, 0u);
        draw_ball(tipoff, &object_plane, ball_x, ball_y);
        submit_argb_object(ren, &object_plane, 3u, 0u);
    }

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
    if (nba_tipoff_pause_active(tipoff)) {
        /* Host placeholder only: native menu/fade bitmaps below `$80:CF1B`
         * are not captured. Keep presentation separate from pause behavior. */
        const NbaMatchPauseFlow *pause = &tipoff->session->match.pause;
        nba_renderer_draw_rect(ren, 52, 66, 152, 88, 0xFF081018u);
        nba_renderer_draw_rect(ren, 52, 66, 152, 2, 0xFFFFD760u);
        nba_font_render_text_centered(ren->pixels, NBA_SNES_WIDTH, 74,
            "PAUSED", 0xFFFFFFFFu, 0xFF081018u, 1);
        if (pause->state == NBA_MATCH_PAUSE_TIMEOUT_TRANSITION ||
            pause->state == NBA_MATCH_PAUSE_RESUME_TRANSITION) {
            nba_font_render_text_centered(ren->pixels, NBA_SNES_WIDTH, 102,
                pause->state == NBA_MATCH_PAUSE_TIMEOUT_TRANSITION ?
                    "TIMEOUT" : "RESUMING",
                0xFFFFD760u, 0xFF081018u, 1);
        } else {
            char timeout[40];
            uint8_t side = pause->selected_side ? 1u : 0u;
            snprintf(timeout, sizeof(timeout), "%c TIMEOUT  %u",
                pause->selection == NBA_MATCH_PAUSE_SELECT_TIMEOUT ? '>' : ' ',
                tipoff->session->match.timeouts_remaining[side]);
            nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, 68, 94, timeout,
                pause->selection == NBA_MATCH_PAUSE_SELECT_TIMEOUT ?
                    0xFFFFD760u : 0xFFFFFFFFu, 0xFF081018u, 1);
            nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, 68, 108,
                pause->selection == NBA_MATCH_PAUSE_SELECT_RESUME ?
                    "> RESUME" : "  RESUME",
                pause->selection == NBA_MATCH_PAUSE_SELECT_RESUME ?
                    0xFFFFD760u : 0xFFFFFFFFu, 0xFF081018u, 1);
            nba_font_render_text_centered(ren->pixels, NBA_SNES_WIDTH, 134,
                "UP/DOWN  A SELECT", 0xFF9EF7A9u, 0xFF081018u, 1);
        }
    }
}
