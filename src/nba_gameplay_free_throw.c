#include "nba_gameplay_free_throw.h"

/* `$87:9DD4-$9DEA`, table `$87:9E29`: eight CPU make thresholds. */
uint8_t nba_gameplay_free_throw_threshold(uint8_t rating) {
    static const uint8_t thresholds[8] = {
        130u, 145u, 160u, 185u, 200u, 215u, 230u, 245u
    };
    unsigned index = rating > 0x80u ? (rating - 0x80u) >> 4 : 0u;
    if (index > 7u) index = 7u;
    return thresholds[index];
}

/* `$87:9D42-$9D57`: rating byte +$38 controls the oscillator quantum. */
uint16_t nba_gameplay_free_throw_human_aim_step(uint8_t rating) {
    uint16_t delta = (uint16_t)((uint16_t)(rating - 0x80u) << 1u);
    return (uint16_t)(0x0226u - delta);
}

void nba_gameplay_free_throw_human_aim_begin(
        NbaGameplayHumanFreeThrowAim *state, uint8_t rating) {
    if (!state) return;
    state->aim_x_raw_0980 = 0u;
    state->accumulator_raw_0984 = 0u;
    state->step_raw_0986 = nba_gameplay_free_throw_human_aim_step(rating);
}

/* `$87:A018-$A045`: add the rating quantum, subtract 110 for every cursor
 * step, and wrap the displayed/launch aim cursor at 112. */
static void human_aim_oscillator_step(NbaGameplayHumanFreeThrowAim *state) {
    state->accumulator_raw_0984 = (uint16_t)(
        state->accumulator_raw_0984 + state->step_raw_0986);
    while (state->accumulator_raw_0984 >= 110u) {
        state->accumulator_raw_0984 = (uint16_t)(
            state->accumulator_raw_0984 - 110u);
        ++state->aim_x_raw_0980;
        if (state->aim_x_raw_0980 >= 112u) state->aim_x_raw_0980 = 0u;
    }
}

NbaGameplayHumanFreeThrowResult nba_gameplay_free_throw_human_aim_step_frame(
        NbaGameplayHumanFreeThrowAim *state) {
    if (!state) return NBA_HUMAN_FREE_THROW_WAIT;
    if (state->state_raw_0978 == 3u) {
        if (state->controller_assignment_raw_16 < 0 ||
            state->human_context_raw_3b == 0u)
            return NBA_HUMAN_FREE_THROW_CPU_FALLBACK;
        human_aim_oscillator_step(state);
        if (!state->shoot_held) return NBA_HUMAN_FREE_THROW_WAIT;
        state->state_raw_0978 = 4u;
        state->aim_y_raw_0982 = state->aim_x_raw_0980;
        state->aim_x_raw_0980 = 0u;
        return NBA_HUMAN_FREE_THROW_FIRST_LOCK;
    }
    if (state->state_raw_0978 == 4u) {
        if (state->controller_assignment_raw_16 < 0) {
            state->state_raw_0978 = 3u;
            return NBA_HUMAN_FREE_THROW_CPU_FALLBACK;
        }
        if (state->shoot_held) return NBA_HUMAN_FREE_THROW_WAIT;
        state->state_raw_0978 = 5u;
        /* Native control falls through into the state-five body in this
         * same actor call, so the second-axis oscillator advances once on
         * the release edge. */
        if (state->human_context_raw_3b == 0u) {
            state->state_raw_0978 = 9u;
            return NBA_HUMAN_FREE_THROW_LAUNCH;
        }
        human_aim_oscillator_step(state);
        return NBA_HUMAN_FREE_THROW_RELEASED_FIRST;
    }
    if (state->state_raw_0978 == 5u) {
        if (state->human_context_raw_3b == 0u) {
            state->state_raw_0978 = 9u;
            return NBA_HUMAN_FREE_THROW_LAUNCH;
        }
        human_aim_oscillator_step(state);
        if (!state->shoot_held) return NBA_HUMAN_FREE_THROW_WAIT;
        state->state_raw_0978 = 9u;
        return NBA_HUMAN_FREE_THROW_LAUNCH;
    }
    return NBA_HUMAN_FREE_THROW_WAIT;
}

/* CPU branch `$87:9DA6-$9E26`; the sibling controller-owned states 3/4/5
 * are implemented above and selected by the scene dispatcher. */
bool nba_gameplay_free_throw_cpu_aim_step(
    NbaGameplayFreeThrowCompletion *state, NbaGameplayRng *rng,
    uint16_t elapsed, uint8_t rating, uint16_t *aim_x_raw_0980,
    uint16_t *aim_y_raw_0982) {
    if (!state || !rng || !aim_x_raw_0980 || !aim_y_raw_0982 ||
        state->state_raw_0978 != 3u || elapsed < 120u) return false;
    if (elapsed < 360u &&
        (nba_gameplay_rng_next(rng) & 0x0B2Au) != 0x0B2Au) return false;
    uint8_t roll = (uint8_t)nba_gameplay_rng_next(rng);
    if (roll < nba_gameplay_free_throw_threshold(rating)) {
        *aim_y_raw_0982 = 29u;
        *aim_x_raw_0980 = 16u;
    } else {
        *aim_y_raw_0982 = (uint16_t)((rng->state & 31u) + 12u);
        *aim_x_raw_0980 =
            (uint16_t)((nba_gameplay_rng_next(rng) & 31u) + 12u);
    }
    state->state_raw_0978 = 9u;
    return true;
}

/* `$85:9530-$9597`. The commentary selector upstream of `$08E6/$08E8`
 * remains an explicit input: its player-record predicate has not yet been
 * retained for every roster. The captured CPU paths supply native $1B. */
bool nba_gameplay_free_throw_presentation_gate(
    NbaGameplayFreeThrowCompletion *state, bool clock_changed,
    uint16_t native_audio_word) {
    if (!state || state->whistle_timer_raw_08de >= 0 ||
        state->state_raw_0978 != 2u) return false;
    state->state_raw_0978 = 3u;
    if (!clock_changed) return true;
    state->whistle_timer_raw_08de = 300;
    state->audio_raw_08e6 = native_audio_word;
    state->audio_mirror_raw_08e8 = native_audio_word;
    return true;
}

/* `$87:9F60-$9F73`: launch completion decrements the remaining count and
 * swaps the stripe UI upload words after entering state ten. */
bool nba_gameplay_free_throw_release_complete(
    NbaGameplayFreeThrowCompletion *state) {
    if (!state || state->state_raw_0978 != 9u) return false;
    --state->attempts_raw_097a; /* native DEC deliberately wraps */
    state->state_raw_0978 = 10u;
    state->upload_raw_180c = 0x8080u;
    state->upload_raw_180b = 0x800Cu;
    return true;
}

/* `$87:9F76-$A017`: between-attempt ball setup, 11..24 cadence, and the
 * final make/miss acknowledgement. Ball flight and scoring are upstream;
 * this helper consumes only the native-owned resolution predicates. */
bool nba_gameplay_free_throw_resolution_step(
    NbaGameplayFreeThrowCompletion *state, bool shooter_pass,
    bool ownerless, uint16_t shot_value_raw_094c,
    uint16_t rim_raw_097c, uint16_t resolution_raw_0972,
    int16_t shooter_x, int16_t shooter_y) {
    if (!state || !shooter_pass) return false;
    if (state->state_raw_0978 >= 11u) {
        ++state->state_raw_0978;
        if (state->state_raw_0978 >= 25u) state->state_raw_0978 = 1u;
        return true;
    }
    if (state->state_raw_0978 != 10u) return false;
    if (state->attempts_raw_097a != 0u) {
        if ((rim_raw_097c == 0u && shot_value_raw_094c != 0u) ||
            state->ball_z_raw_3ef7 >= 8u) return false;
        state->ball_x_raw_3eef = shooter_x;
        state->ball_y_raw_3ef3 = shooter_y;
        state->ball_z_raw_3ef7 = 32u;
        state->ball_vx_raw_3ef9 = 0;
        state->ball_vy_raw_3efb = 0;
        state->ball_vz_raw_3efd = 0;
        state->state_raw_0978 = 11u;
        return true;
    }
    if (!ownerless || state->ball_vz_raw_3efd >= 0) return false;
    if (state->ball_z_raw_3ef7 < 24u || rim_raw_097c != 0u ||
        resolution_raw_0972 != 0u) {
        state->state_raw_0978 = 0u;
        return true;
    }
    if (shot_value_raw_094c == 0u) {
        state->state_raw_0978 = 0u;
        return true;
    }
    return false;
}
