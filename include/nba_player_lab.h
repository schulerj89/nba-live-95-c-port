#ifndef NBA_PLAYER_LAB_H
#define NBA_PLAYER_LAB_H

#include "nba_assets.h"
#include "nba_renderer.h"
#include "nba_team_select.h"

#define NBA_PLAYER_ROSTER_SIZE 12
#define NBA_PLAYER_ANIMATION_STATES 57
#define SNES_ROM_TEAM_ROSTER_TABLE 0x84E640
#define SNES_ADDR_PLAYER_POINTERS 0x86D7B8
#define SNES_ADDR_PLAYER_APPEARANCE 0x86D85E
#define SNES_ADDR_PLAYER_SORT 0x86D73E

typedef struct {
    bool is_active;
    uint8_t team;
    uint8_t player;
    uint8_t animation_state;
    uint8_t direction;
    uint32_t animation_tick;
    bool animation_paused;
    bool has_assets;
} NbaPlayerLab;

void nba_player_lab_init(NbaPlayerLab *lab, const NbaAssetPack *assets);
void nba_player_lab_toggle(NbaPlayerLab *lab, const NbaAssetPack *assets);
void nba_player_lab_update(NbaPlayerLab *lab, const NbaAssetPack *assets,
                           const NbaInput *input);
void nba_player_lab_render(const NbaPlayerLab *lab, const NbaAssetPack *assets,
                           NbaRenderer *renderer);
void nba_player_lab_print(const NbaPlayerLab *lab, const NbaAssetPack *assets);

/* Shared ROM-sprite compositor used by Player Lab and live court scenes.
 * origin_x/origin_y are the ROM's lower-body attachment point; scale 1 is
 * native SNES size and scale 2 is the enlarged laboratory view. */
bool nba_player_sprite_render(NbaRenderer *renderer, const NbaAssetPack *assets,
                              uint8_t team, uint8_t roster_slot, uint8_t side,
                              uint8_t upper_state, uint8_t direction,
                              uint32_t animation_tick, int origin_x,
                              int origin_y, int scale);
bool nba_player_sprite_render_split(NbaRenderer *renderer,
                                    const NbaAssetPack *assets, uint8_t team,
                                    uint8_t roster_slot, uint8_t side,
                                    uint8_t upper_state, uint8_t lower_state,
                                    uint8_t direction, uint32_t upper_tick,
                                    uint32_t lower_tick, int origin_x,
                                    int origin_y, int scale);
bool nba_player_sprite_render_resources(NbaRenderer *renderer,
                                    const NbaAssetPack *assets, uint8_t team,
                                    uint8_t roster_slot, uint8_t side,
                                    uint8_t direction,
                                    uint16_t upper_resource,
                                    uint16_t lower_resource, int origin_x,
                                    int origin_y, int scale);
/* Shared `$80:B344-$B498` raw resource compositor. Palette points to sixteen
 * little-endian BGR555 words from the asset pack. */
uint32_t nba_rom_sprite_resource_render(NbaRenderer *renderer,
                                    const NbaAssetPack *assets,
                                    uint16_t resource_id,
                                    const uint8_t palette[32], int origin_x,
                                    int origin_y, bool flip, int scale);
typedef struct {
    int16_t x;
    uint8_t y, tile, attribute, large;
} NbaRomSpriteOamEntry;
typedef struct {
    NbaRomSpriteOamEntry entries[32];
    uint8_t count;
} NbaRomSpriteOamComposition;
/* Portable gameplay-visible output of `$80:B344-$B529`: decode a packed ROM
 * resource into the same clipped low-OAM entries, in submission order. */
bool nba_rom_sprite_resource_compose(const NbaAssetPack *assets,
                                    uint16_t resource_id,
                                    uint16_t caller_attribute,
                                    int16_t origin_x, int16_t origin_y,
                                    NbaRomSpriteOamComposition *composition);
bool nba_player_compose_jersey_number(const NbaAssetPack *assets,
                                      uint8_t jersey, uint8_t direction,
                                      uint8_t side, uint8_t tile[32]);
typedef struct {
    uint16_t upper_resource, lower_resource, head_resource, number_resource;
    uint32_t upper_opaque_pixels, lower_opaque_pixels;
    uint32_t head_opaque_pixels, number_opaque_pixels;
    int8_t upper_attach_x, upper_attach_y;
    int8_t head_attach_x, head_attach_y;
    int8_t number_attach_x, number_attach_y;
    bool lower_resource_valid, upper_resource_valid, head_resource_valid;
    bool number_resource_valid;
    bool player_palette_valid, number_palette_valid;
    bool number_allowed, number_composed;
} NbaPlayerSpriteDiagnostics;

typedef enum {
    NBA_PLAYER_SPRITE_HEAD = 0,
    NBA_PLAYER_SPRITE_NUMBER,
    NBA_PLAYER_SPRITE_UPPER,
    NBA_PLAYER_SPRITE_LOWER
} NbaPlayerSpritePartKind;

typedef struct {
    NbaPlayerSpritePartKind kind;
    uint16_t resource;
    int16_t x, y;
    bool flip;
} NbaPlayerSpritePart;

typedef struct {
    NbaPlayerSpritePart parts[4]; /* native `$80:B348` submission order */
    uint8_t count;
} NbaPlayerSpriteComposition;

/* Portable observable output of `$80:AD92-$AEC1`: resolve the four player
 * layers and their attachment origins in native sprite-queue order. */
bool nba_player_compose_sprite_parts(const NbaAssetPack *assets,
                                    uint8_t team, uint8_t roster_slot,
                                    uint8_t side, uint8_t direction,
                                    uint16_t upper_resource,
                                    uint16_t lower_resource,
                                    int16_t lower_x, int16_t lower_y,
                                    NbaPlayerSpriteComposition *composition);
/* Asset-only equivalent of the `$87:A47A` -> `$80:AD92` layer inputs.
 * It does not render or depend on Mesen pixels. Zero-part resources are
 * valid native no-op layers, so validity and opaque pixel counts are exposed
 * separately. */
bool nba_player_sprite_diagnose_resources(const NbaAssetPack *assets,
                                    uint8_t team, uint8_t roster_slot,
                                    uint8_t side, uint8_t direction,
                                    uint16_t upper_resource,
                                    uint16_t lower_resource,
                                    NbaPlayerSpriteDiagnostics *diagnostics);
/* ROM `$87:B572-$B648` preserves a channel phase when the replacement
 * descriptor can represent it. */
/* Literal animation words owned by `$87:B37C-$B571` and `$87:AB72-$AD5A`.
 * Queue cursors are signed (-1 means return to base), with three entries
 * per channel at actor +$1C/+22. No tiles or emulator state live here. */
typedef struct {
    uint16_t upper_queue_cursor, lower_queue_cursor; /* +18/+1A */
    uint16_t upper_state, lower_state, base_state;   /* +30/+32/+38 */
    uint16_t upper_phase, lower_phase;              /* +3A/+3C */
    uint16_t upper_accumulator, lower_accumulator;  /* +42/+44 */
    uint16_t upper_lock, lower_lock;                /* +46/+48 */
    uint16_t upper_queue[3], lower_queue[3];
    uint16_t upper_phase_target;                   /* +B0: target + direction */
} NbaPlayerAnimationChannels;

/* Snapshot resolver: no cadence advance, RNG, or action lock changes. */
typedef struct {
    uint16_t mirror_flags, upper_resource, lower_resource;
    uint16_t upper_state, lower_state, upper_phase, lower_phase, direction;
} NbaPlayerResolvedPose;
bool nba_player_resolve_pose(const NbaAssetPack *assets,
    const NbaPlayerAnimationChannels *channels, uint16_t direction,
    bool alternate_lower, uint16_t variant, NbaPlayerResolvedPose *pose);

#define NBA_PLAYER_APPEARANCE_COUNT 10
typedef struct {
    uint16_t palette_offset, alternate_lower, upper_variant, head_resource;
    uint16_t dirty;
} NbaPlayerAppearance;
typedef struct {
    uint32_t upload_address;
    NbaPlayerAppearance players[NBA_PLAYER_APPEARANCE_COUNT];
} NbaPlayerAppearanceSetup;

typedef struct {
    uint8_t lineup_selector[NBA_PLAYER_APPEARANCE_COUNT];
    uint8_t appearance_a[NBA_PLAYER_APPEARANCE_COUNT];
    uint8_t appearance_b[NBA_PLAYER_APPEARANCE_COUNT];
    uint8_t upper_variant[NBA_PLAYER_APPEARANCE_COUNT];
} NbaPlayerActiveAppearanceInput;

typedef struct {
    uint16_t assignment_base[NBA_PLAYER_APPEARANCE_COUNT];
    uint16_t assignment_alternate[NBA_PLAYER_APPEARANCE_COUNT];
    uint16_t upper_variant[NBA_PLAYER_APPEARANCE_COUNT];
    uint16_t help_request[NBA_PLAYER_APPEARANCE_COUNT];
    uint16_t sorted_key[2][5];
    uint16_t sorted_actor_offset[2][5];
} NbaPlayerActiveAppearance;

bool nba_player_build_active_appearance(
    const NbaPlayerActiveAppearanceInput *input,
    NbaPlayerActiveAppearance *output);
bool nba_player_appearance_setup(const NbaAssetPack *assets,
    const uint8_t teams[NBA_PLAYER_APPEARANCE_COUNT],
    const uint8_t roster[NBA_PLAYER_APPEARANCE_COUNT],
    NbaPlayerAppearanceSetup *setup);

typedef enum {
    NBA_ANIMATION_INSTALL_BOTH,
    NBA_ANIMATION_INSTALL_UPPER,
    NBA_ANIMATION_INSTALL_LOWER,
    NBA_ANIMATION_CANCEL_UPPER,
    NBA_ANIMATION_CANCEL_LOWER,
    NBA_ANIMATION_REVERSE_BOTH
} NbaPlayerAnimationCommand;

bool nba_player_animation_command(const NbaAssetPack *assets,
    NbaPlayerAnimationChannels *channels, NbaPlayerAnimationCommand command,
    uint16_t *requested_state, bool boosted, bool alternate_lower);
/* Same command, exposing the literal DP47 descriptor write when it occurs.
 * Early exits preserve the caller's scratch word. */
bool nba_player_animation_command_scratch(const NbaAssetPack *assets,
    NbaPlayerAnimationChannels *channels, NbaPlayerAnimationCommand command,
    uint16_t *requested_state, bool boosted, bool alternate_lower,
    uint16_t * scratch_47);
/* $86:E545-E592, including B37C reversal; writes +4E, never display +52.
 * Current resource IDs remain untouched until the next animation cadence. */
bool nba_player_owner_unlatched_pose(const NbaAssetPack *assets,
    NbaPlayerAnimationChannels *channels, int16_t vx, int16_t vy,
    uint16_t requested_direction, uint16_t *facing,
    bool boosted, bool alternate_lower);
/* Common/action cadence plus mode-2 states 7/13/18, using the ROM $07F6 LFSR.
 * Invalid inputs fail atomically without changing channels/RNG/resources. */
bool nba_player_animation_step_channels(const NbaAssetPack *assets,
    NbaPlayerAnimationChannels *channels, uint16_t direction, uint16_t speed,
    uint16_t delta, bool alternate_lower, uint16_t variant, uint16_t *rng,
    uint16_t *upper_resource, uint16_t *lower_resource);

bool nba_player_animation_frame_count(const NbaAssetPack *assets,
                                      bool upper, uint8_t state,
                                      bool alternate_lower,
                                      uint16_t *count);
uint8_t nba_player_locomotion_state(uint8_t base_state, bool stationary,
                                    bool boosted, bool owns_ball,
                                    bool airborne);
/* Common descriptor cadence in `$87:AAB2-$AD5A`. Accumulators and phases are
 * the actor's literal +$42/+$44 and +$3A/+$3C words. */
bool nba_player_animation_rom_step(const NbaAssetPack *assets,
                                   uint8_t upper_state, uint8_t lower_state,
                                   uint8_t direction, uint16_t speed_raw_4a,
                                   bool alternate_lower, uint16_t variant_raw_6c,
                                   uint16_t *upper_accumulator,
                                   uint16_t *lower_accumulator,
                                   uint16_t *upper_phase,
                                   uint16_t *lower_phase,
                                   uint16_t *upper_resource,
                                   uint16_t *lower_resource);
bool nba_player_animation_resources(const NbaAssetPack *assets,
                                    uint8_t upper_state, uint8_t lower_state,
                                    uint8_t direction, uint32_t upper_tick,
                                    uint32_t lower_tick,
                                    uint16_t *upper_resource,
                                    uint16_t *lower_resource);
/* Snapshot form of `$87:AC76-$AC95/$87:AD38-$AD57`. Unlike the legacy
 * lab helper above, this applies the active player's tall-body lower table
 * and roster +$08 upper-body variant exactly as the live actor resolver. */
bool nba_player_animation_resources_for_appearance(
                                    const NbaAssetPack *assets,
                                    uint8_t upper_state, uint8_t lower_state,
                                    uint8_t direction, uint32_t upper_tick,
                                    uint32_t lower_tick,
                                    bool alternate_lower,
                                    uint16_t variant_raw_6c,
                                    uint16_t *upper_resource,
                                    uint16_t *lower_resource);
/* Descriptor-driven actor +$3A/+$3C frame phases used by native action
 * gates. These are frame indices, not elapsed logical-pass counters. */
bool nba_player_animation_phases(const NbaAssetPack *assets,
                                 uint8_t upper_state, uint8_t lower_state,
                                 uint32_t upper_tick, uint32_t lower_tick,
                                 uint16_t *upper_phase,
                                 uint16_t *lower_phase);
bool nba_player_animation_contact_height(const NbaAssetPack *assets,
                                         uint8_t team, uint8_t roster_slot,
                                         uint16_t upper_resource,
                                         uint16_t lower_resource,
                                         uint8_t direction,
                                         uint16_t *height);
bool nba_player_animation_contact_height_from_resources(
    const NbaAssetPack *assets, uint16_t upper_resource,
    uint16_t lower_resource, uint16_t *height);
bool nba_player_gameplay_animation_variant(const NbaAssetPack *assets,
    uint8_t team, uint8_t roster_slot, uint16_t *variant_raw_6c);
bool nba_player_animation_self_test(const NbaAssetPack *assets);
/* Exact `$87:B832-$B952` ball offset composition from ROM animation-resource
 * tables. `mirror_flags_raw` is actor `+$28`; no host directional offsets. */
bool nba_player_ball_attachment_offsets(const NbaAssetPack *assets,
                                        uint16_t upper_resource,
                                        uint16_t lower_resource,
                                        uint16_t mirror_flags_raw,
                                        int16_t *x, int16_t *y, int16_t *z);
/* `$86:D549-$D5DA` evaluates both `$87:B832` pose points. Selector zero is
 * the normal held-ball point; selector one uses `$AC:CC2F/$BF4B/$C397`. */
bool nba_player_ball_attachment_point_offsets(const NbaAssetPack *assets,
                                        uint16_t upper_resource,
                                        uint16_t lower_resource,
                                        uint16_t mirror_flags_raw,
                                        uint8_t point_selector,
                                        int16_t *x, int16_t *y, int16_t *z);
bool nba_player_gameplay_roster_address(const NbaAssetPack *assets,
    uint8_t team, uint8_t roster_slot, uint32_t *address);
/* Roster bytes3C/3D used by the EC32/ECF9 jump decision. */
bool nba_player_gameplay_jump_ratings(const NbaAssetPack *assets,
    uint8_t team,uint8_t slot,uint8_t *rating_3c,uint8_t *rating_3d);
bool nba_player_gameplay_stamina_rating(const NbaAssetPack *assets,
    uint8_t team,uint8_t roster_slot,uint8_t *rating);
bool nba_player_gameplay_shot_ratings(const NbaAssetPack *assets,
                                      uint8_t team, uint8_t roster_slot,
                                      uint8_t *two_point, uint8_t *three_point);
bool nba_player_gameplay_shot_range(const NbaAssetPack *assets,
                                    uint8_t team, uint8_t roster_slot,
                                    uint8_t *range_49);
bool nba_player_gameplay_free_throw_rating(const NbaAssetPack *assets,
                                           uint8_t team,
                                           uint8_t roster_slot,
                                           uint8_t *rating);
bool nba_player_gameplay_free_throw_launch_half(const NbaAssetPack *assets,
                                                uint8_t team,
                                                uint8_t roster_slot,
                                                uint8_t *half);
bool nba_player_gameplay_decision_profiles(const NbaAssetPack *assets,
                                           uint8_t team, uint8_t roster_slot,
                                           uint8_t *profile_3f,
                                           uint8_t *profile_40);
bool nba_player_gameplay_movement_profile(const NbaAssetPack *assets,
                                          uint8_t team, uint8_t roster_slot,
                                          uint8_t *profile_42);
bool nba_player_gameplay_pass_profiles(const NbaAssetPack *assets,
                                       uint8_t team, uint8_t roster_slot,
                                       uint8_t *profile_39,
                                       uint8_t *profile_3e);
bool nba_player_gameplay_contact_rating(const NbaAssetPack *assets,
                                        uint8_t team, uint8_t roster_slot,
                                        uint8_t *rating_3a);
bool nba_player_gameplay_position(const NbaAssetPack *assets,
                                  uint8_t team, uint8_t roster_slot,
                                  uint8_t *position_raw_92);

#endif
