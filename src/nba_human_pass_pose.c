#include "nba_human_pass_pose.h"
#include <string.h>

static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static uint32_t u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static const NbaAssetItem *animation(const NbaAssetPack *assets) {
    const NbaAssetItem *item = nba_assets_get(assets, NBA_ASSET_PLAYER_ANIMATIONS);
    if (!item || !item->data || item->size < 80u || memcmp(item->data, "NBPANIM1", 8u)) return NULL;
    const uint8_t *d = item->data;
    if (u32(d + 8u) != 6u || u32(d + 12u) != 57u) return NULL;
    return item;
}
static bool bank_word(const uint8_t *bank, uint32_t address, uint16_t *out) {
    if (address < 0x8000u || address > 0xfffeu) return false;
    *out = u16(bank + address - 0x8000u); return true;
}
bool nba_human_pass_pose_resolve(const NbaAssetPack *assets, NbaHumanPassPoseState *s) {
    if (!s || s->actor.upper_30 >= 57u || s->actor.lower_32 >= 57u || s->actor.facing_4e > 8u) return false;
    const NbaAssetItem *item = animation(assets); if (!item) return false;
    const uint8_t *data = item->data;
    uint32_t offset = u32(data + 20u);
    if (offset > item->size || 0x8000u > item->size - offset) return false;
    const uint8_t *bank = data + offset;
    NbaHumanPassPoseState next = *s; NbaHumanPassPoseActor *a = &next.actor;
    uint16_t descriptor, frame_list;
    next.bank_49 = 0x84u;
    a->flags_28 = (uint16_t)((a->flags_28 & 0x7fffu) | (a->facing_4e < 3u ? 0x8000u : 0u));
    /* AEFE/AEFF: ASL supplies carry to ADC #8. For the supported native
     * facing0..8 domain carry is zero; no phase modulo/count clamp exists. */
    next.direction_index_ac = (uint16_t)(a->facing_4e * 2u + 8u);
    uint32_t lower_table = a->alternate_lower_a8 ? 0xc28au : 0xc218u;
    if (!bank_word(bank, lower_table + a->lower_32 * 2u, &descriptor) ||
        !bank_word(bank, (uint32_t)descriptor + next.direction_index_ac, &frame_list) ||
        !bank_word(bank, (uint32_t)frame_list + (uint16_t)(a->phase_3c * 2u), &a->lower_2c) ||
        !bank_word(bank, 0xc2fcu + a->upper_30 * 2u, &descriptor) ||
        !bank_word(bank, (uint32_t)descriptor + next.direction_index_ac, &frame_list) ||
        !bank_word(bank, (uint32_t)frame_list + (uint16_t)(a->phase_3a * 2u), &a->upper_2a)) return false;
    next.pointer_47 = frame_list; /* AF26 retains the upper frame-list pointer. */
    if (a->upper_2a < 0xf0u && (uint16_t)(a->variant_6c ^ (a->facing_4e < 3u ? 1u : 0u)) == 0u)
        a->upper_2a = (uint16_t)(a->upper_2a + 0x28u);
    a->previous_upper_34 = a->upper_30; a->previous_lower_36 = a->lower_32;
    a->previous_phase_3e = a->phase_3a; a->previous_phase_40 = a->phase_3c;
    if (a->facing_4e != 8u) a->resolved_facing_52 = a->facing_4e; /* AF6F skips +52 for8. */
    *s = next; return true;
}
static bool table_byte(const NbaAssetItem *item, unsigned header, uint16_t resource,
                       unsigned delta, uint16_t *out) {
    if (resource >= 0x830u) return false;
    const uint8_t *data = item->data;
    uint32_t offset = u32(data + header);
    if (offset > item->size || delta > item->size - offset || resource >= item->size - offset - delta) return false;
    unsigned byte = data[offset + delta + resource];
    *out = (uint16_t)(byte >= 0x80u ? byte + 0xff00u : byte); return true;
}
bool nba_human_pass_pose_offset(const NbaAssetPack *assets, NbaHumanPassPoseState *s) {
    if (!s) return false;
    const NbaAssetItem *item = animation(assets); if (!item) return false;
    uint16_t lower_y, lower_z, upper_x, upper_y, upper_z;
    unsigned header = s->scratch_00 == 0u ? 56u : 68u;
    if (!table_byte(item, 24u, s->actor.lower_2c, 0u, &lower_y) ||
        !table_byte(item, 24u, s->actor.lower_2c, 0x830u, &lower_z) ||
        !table_byte(item, header, s->actor.upper_2a, 0u, &upper_x) ||
        !table_byte(item, header + 4u, s->actor.upper_2a, 0u, &upper_y) ||
        !table_byte(item, header + 8u, s->actor.upper_2a, 0u, &upper_z)) return false;
    uint16_t flags = s->actor.flags_28;
    if (flags & 0x8000u) flags ^= 3u; /* B83B/B83D: sign toggles both mirror masks. */
    s->pointer_47 = (flags & 2u) ? 0xffffu : 0u;
    s->scratch_06 = (flags & 1u) ? 0xffffu : 0u;
    /* B869/B86C and B8AA/B8AD negate AFTER sign extension in 16-bit A.
     * Preserve word width, including the source-derived -128 -> +128 edge;
     * no controlled/native extreme witness is asserted by this translation. */
    if (flags & 2u) lower_y = (uint16_t)(0u - lower_y);
    if (flags & 1u) upper_y = (uint16_t)(0u - upper_y);
    uint16_t sum = (uint16_t)(lower_y + upper_y);
    uint16_t midpoint = (uint16_t)((sum >> 1u) | (sum & 0x8000u)); /* B8B1/B8B4 signed ROR floor. */
    uint16_t twice_x = (uint16_t)(upper_x * 2u);
    s->scratch_00 = (uint16_t)(midpoint - twice_x);
    s->scratch_02 = (uint16_t)(midpoint + twice_x);
    s->scratch_04 = (uint16_t)(upper_x - (uint16_t)(lower_z + upper_z));
    return true;
}
bool nba_human_pass_pose_attach(const NbaAssetPack *assets, NbaHumanPassPoseState *s) {
    if (!s) return false; NbaHumanPassPoseState next = *s;
    next.scratch_00 = 0u; /* B64B always requests point0. */
    if (!nba_human_pass_pose_offset(assets, &next)) return false;
    next.previous_ball_x_0922 = next.ball_x; /* B651/B654 preserve old X, not new attached X. */
    next.ball_x = (uint16_t)(next.actor.x + next.scratch_00);
    next.ball_y = (uint16_t)(next.actor.y + next.scratch_02);
    *s = next; return true;
}
bool nba_human_pass_pose_prefix(const NbaAssetPack *assets, NbaHumanPassPoseState *s) {
    if (!s) return false; NbaHumanPassPoseState next = *s;
    if (!nba_human_pass_pose_resolve(assets, &next) || !nba_human_pass_pose_attach(assets, &next)) return false;
    next.ball_z = (uint16_t)(next.actor.z + next.scratch_04); /* AF25-AF2D owns Z separately. */
    *s = next; return true;
}
bool nba_human_pass_pose_commit(NbaHumanPassPoseState *s) {
    if (!s) return false;
    s->actor.mode_5e = 15u; s->actor.flags_7e |= 6u;
    if ((uint16_t)(s->live_0936 - 0x80u) & 0x8000u) s->live_0936 = 2u; /* AF42/AF45 CMP/BPL, not signed operands. */
    return true;
}
bool nba_human_pass_pose_prepare(const NbaAssetPack *assets, NbaHumanPassPoseState *s) {
    if (!nba_human_pass_pose_prefix(assets, s)) return false;
    return nba_human_pass_pose_commit(s); /* Stop BEFORE AF4D's PLA epilogue. */
}
