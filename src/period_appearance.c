#include "period_appearance.h"

bool nba_period_appearance(const NbaAssetPack *assets, NbaPeriodAppearance *state) {
    if (!state || state->owner != UINT16_MAX || state->controller != UINT16_MAX ||
        state->velocity_x || state->velocity_y || state->z || state->boost ||
        state->speed || state->display_direction >= 8u || state->channels.base_state >= 19u)
        return false;
    NbaPeriodAppearance s = *state;
    NbaPlayerAnimationChannels *c = &s.channels;
    s.owner_pointer = 0; /* A9D0/A9DF, owner already cleared at DD83. */
    c->base_state = nba_player_locomotion_state((uint8_t)c->base_state,
                                               true, false, false, false);
    /* B572 writes +38 as well as unlocked channels. The parent reset does
     * not erase +30/+32/+38; derive them from the carried original base. */
    uint16_t count;
    if (!c->upper_lock && c->upper_state != c->base_state) {
        c->upper_state = c->base_state; c->upper_accumulator = 0;
        if (!nba_player_animation_frame_count(assets, true, (uint8_t)c->base_state,
                                              false, &count)) return false;
        if (c->upper_phase >= count) c->upper_phase = 0;
    }
    if (!c->lower_lock && c->lower_state != c->base_state) {
        c->lower_state = c->base_state; c->lower_accumulator = 0;
        /* Original B630 checks C218 even for an alternate-lower actor;
         * retain that table choice. AB5F selects the alternate for cadence. */
        if (!nba_player_animation_frame_count(assets, false, (uint8_t)c->base_state,
                                              false, &count)) return false;
        if (c->lower_phase >= count) c->lower_phase = 0;
    }
    if (s.direction >= 8u) s.direction = s.display_direction; /* AB38..AB45 */
    s.status = (uint16_t)((s.status & 0x7fffu) |
                         (s.display_direction < 3u ? 0x8000u : 0u));
    /* ABA5 XBA consumes the original C6 word, with 16-bit wrap. This is
     * an input contract, not an inferred cadence or elapsed-time model. */
    uint16_t delta = (uint16_t)((s.delta << 8) | (s.delta >> 8));
    if (!nba_player_animation_step_channels(assets, c, s.display_direction,
            s.speed, delta, s.alternate_lower != 0, s.variant, &s.rng,
            &s.upper_resource, &s.lower_resource)) return false;
    *state = s;
    return true;
}
