#include "nba_shot_action.h"
#include "nba_gameplay_ball.h"

/* `$86:B629-$B6D2`: B625's caller supplies the freshly evaluated F5E4 lane
 * result. Keep the literal appearance-mask test: it is not a boolean gate.
 * The ordinary fallback includes the already verified B6D3 startup. */
bool nba_special_shot_select(const NbaAssetPack *assets, NbaShotAction *state,
                             uint16_t *direction_66,
                             const NbaSpecialShotSelection *in) {
    NbaShotAction next = *state;
    uint16_t direction = *direction_66;
    uint16_t request = 0;
    if (in->lane_result && !in->movement && in->anchor_distance < 0x60u) {
        uint16_t toward = in->anchor_direction >> 1;
        uint16_t relative = (uint16_t)(toward - in->facing) & 7u;
        if (relative >= 2u && relative != 7u) {
            bool clockwise = relative >= 5u ||
                             (relative == 4u && in->appearance != 0u);
            direction = (uint16_t)(toward + (clockwise ? 2 : -2)) & 7u;
            request = clockwise ? 0x14u : 0x15u;
            if (((in->appearance ^ 1u) & request) == 0u) request = 0;
        }
    }
    if (request == 0u) {
        if (!nba_shot_action_start(assets, &next, in->boosted,
                                   in->alternate_lower)) return false;
    } else {
        if (direction < 3u) request ^= 1u;
        if (!nba_player_animation_command(assets, &next.animation,
                NBA_ANIMATION_INSTALL_UPPER, &request, in->boosted,
                in->alternate_lower)) return false;
        request = 0x1Fu;
        if (!nba_player_animation_command(assets, &next.animation,
                NBA_ANIMATION_INSTALL_LOWER, &request, in->boosted,
                in->alternate_lower)) return false;
        next.timer = 0; next.mode = 17; next.speed = 0;
        next.activity = 1; next.bounce_count = 0; next.bounce_timer = 0;
        next.flags |= 6u;
    }
    *state = next;
    *direction_66 = direction;
    return true;
}

/* `$86:B979-$BAA1`, excluding the separately replayed 9DA6 launch itself.
 * B9B4-B9CF only write integer coordinates; fractions remain caller-owned.
 * No RNG is consumed. A phase-3 release does not install ordinary pose 17. */
NbaSpecialShotResult nba_special_shot_step(const NbaAssetPack *assets,
    NbaShotAction *state, NbaSpecialShotFrame *frame, NbaSpecialShotBall *ball) {
    if (!frame->owns_ball) {
        nba_shot_action_restore(state, frame->team_group, frame->active_group);
        return NBA_SPECIAL_SHOT_LOST;
    }
    NbaShotAction next = *state;
    NbaSpecialShotBall projected = *ball;
    next.flags |= 2u;
    next.timer = (uint16_t)(next.timer + frame->delta);
    projected.live_state = 2;
    if (next.animation.upper_phase < 3u) {
        int16_t dx, dy, dz;
        if (!nba_player_ball_attachment_point_offsets(assets,
                frame->upper_resource, frame->lower_resource, next.status, 0,
                &dx, &dy, &dz)) return NBA_SPECIAL_SHOT_ERROR;
        projected.x = (uint16_t)(frame->x + dx);
        projected.y = (uint16_t)(frame->y + dy);
        projected.z = (uint16_t)(frame->z + dz);
        projected.previous_actor_x = (uint16_t)frame->x;
    }
    NbaSpecialShotResult result = NBA_SPECIAL_SHOT_HOLD;
    bool cancel = false;
    if ((int16_t)next.activity >= 0) {
        uint16_t activity = (uint16_t)(next.activity + frame->delta);
        if ((int16_t)(uint16_t)(activity - 5u) >= 0) {
            next.activity = 0xFFFFu;
            next.velocity_z = 0x258;
            result = NBA_SPECIAL_SHOT_JUMP;
        } else {
            next.activity = activity;
            cancel = frame->controller >= 0 && !(frame->buttons & 0x80u);
        }
    } else if (frame->z == 0) {
        cancel = true;
    } else {
        if (next.timer >= 10u && frame->facing != frame->direction_66) {
            uint16_t difference = (uint16_t)(frame->facing - frame->direction_66) & 7u;
            frame->facing = (uint16_t)(frame->facing + (difference < 4u ? -1 : 1)) & 7u;
        }
        if (next.animation.upper_phase >= 3u) result = NBA_SPECIAL_SHOT_RELEASE;
    }
    if (cancel) {
        uint16_t request = 0;
        if (!nba_player_animation_command(assets, &next.animation,
                NBA_ANIMATION_CANCEL_UPPER, &request, frame->boosted,
                frame->alternate_lower) ||
            !nba_player_animation_command(assets, &next.animation,
                NBA_ANIMATION_CANCEL_LOWER, &request, frame->boosted,
                frame->alternate_lower)) return NBA_SPECIAL_SHOT_ERROR;
        request = 0x0Cu;
        if (!nba_player_animation_command(assets, &next.animation,
                NBA_ANIMATION_INSTALL_BOTH, &request, frame->boosted,
                frame->alternate_lower)) return NBA_SPECIAL_SHOT_ERROR;
        next.activity = 0; next.mode = 11;
        nba_shot_action_clear(&next);
        projected.live_state = 0; projected.velocity_z = 0;
        projected.height_latch = 40;
        if (projected.attachment_state) projected.attachment_state = 2;
        result = NBA_SPECIAL_SHOT_CANCEL;
    }
    *state = next;
    *ball = projected;
    return result;
}

/* `$85:F02D-$F099` uses strict N-flag comparisons, unlike the nearby
 * target-distance routine's tie handling. Table bytes are `$85:F09A`. */
static uint16_t shot_facing(int16_t dx,int16_t dy) {
    static const uint8_t map[16]={0,1,2,1,4,3,2,3,0,7,6,7,4,5,6,5};
    if (!(dx|dy)) return 8;
    uint16_t x=(uint16_t)dx,y=(uint16_t)dy,key=0;
    if(dx<0){x=(uint16_t)(0u-x);key|=8;}
    if(dy<0){y=(uint16_t)(0u-y);key|=4;}
    if((int16_t)(uint16_t)(y-1u-x)<0){uint16_t swap=x;x=y;y=swap;key|=2;}
    if((int16_t)(uint16_t)(y-1u-(uint16_t)(x*2u))<0)key|=1;
    return map[key];
}

/* `$86:B7F7-$B849`: optional stationary-shot displacement, then B84C.
 * B802 tests N on the wrapped subtraction, B815 tests unsigned carry.
 * ROM table `$86:B745-$B768` has nine XY pairs (including facing 8).
 * These are velocity constants, not sprite data. RNG is only sampled. */
bool nba_shot_action_sidestep(NbaShotAction *state,
                              const NbaShotSidestepInput *in) {
    static const int16_t velocity[9][2] = {
        {0,-80}, {-80,-80}, {-80,0}, {-80,80}, {0,80},
        {80,80}, {80,0}, {80,-80}, {0,-80}
    };
    if (in->movement || (int16_t)(uint16_t)(in->anchor_distance - 0x78u) >= 0 ||
        in->free_throw) return false;
    uint16_t abs_x = in->x < 0 ? (uint16_t)(0u - (uint16_t)in->x) :
                                      (uint16_t)in->x;
    if (abs_x < 0x38u || !(in->rng & 1u)) return false;
    uint16_t facing = shot_facing((int16_t)(in->basket_x - in->x),
                                 (int16_t)(0u - (uint16_t)in->y));
    state->velocity_x = velocity[facing][0];
    state->velocity_y = velocity[facing][1];
    return true;
}

/* `$86:9D7A-$9D98`: release snaps to the basket; unlike B8CA's wind-up,
 * it does not take just one octant step. The same F02D quantizer is used. */
uint16_t nba_shot_action_release_facing(int16_t x, int16_t y, int16_t basket_x) {
    return shot_facing((int16_t)(basket_x-x),(int16_t)(0u-(uint16_t)y));
}

/* `$86:B769-$B790`: ownership takes priority over the pump-fake latch.
 * The latch waits for BOTH upper phase >= 4 and accumulator >= $600. */
NbaShotOwnerGate nba_shot_action_owner_gate(const NbaShotAction *state,
                                            bool owns_ball) {
    if (!owns_ball) return NBA_SHOT_LOST_OWNER;
    if (!(state->flags & 0x80u)) return NBA_SHOT_CONTINUE;
    return state->animation.upper_phase >= 4u &&
           state->animation.upper_accumulator >= 0x600u ?
           NBA_SHOT_PUMP_CANCEL : NBA_SHOT_PUMP_WAIT;
}

/* `$86:B86C-$B88F/$B8C9`: CPU wind-up holds without reading controller
 * buttons. Only a non-free-throw human releasing B sets the $80 latch. */
void nba_shot_action_windup_button(NbaShotAction *state, int16_t controller,
                                   uint16_t free_throw, uint16_t buttons) {
    if (controller >= 0 && !free_throw && !(buttons & 0x80u))
        state->flags |= 0x80u;
}

/* `$86:B890-$B8C8`: cancel both animation channels, lower the ball to 40,
 * clear its vertical speed and action flags. Preserve actor +$28 and the
 * ball's fractional Z word; $09F6 becomes 2 only when previously nonzero. */
bool nba_shot_action_cancel(const NbaAssetPack *assets, NbaShotAction *state,
                            NbaShotCancelBall *ball, bool alternate_lower) {
    NbaShotAction next = *state;
    uint16_t request = 0;
    if (!nba_player_animation_command(assets, &next.animation,
            NBA_ANIMATION_CANCEL_UPPER, &request, false, alternate_lower) ||
        !nba_player_animation_command(assets, &next.animation,
            NBA_ANIMATION_CANCEL_LOWER, &request, false, alternate_lower))
        return false;
    next.activity = 0;
    next.mode = 11;
    nba_shot_action_clear(&next);
    *state = next;
    ball->live_state = 0;
    ball->ball_velocity_z = 0;
    ball->ball_z = ball->height_latch = 0x28;
    if (ball->attachment_state) ball->attachment_state = 2;
    return true;
}

/* `$86:B6D3-$B744`: ordinary shot entry after the special-shot selector.
 * Cancel both locks, arithmetic-halve both velocities, then distinguish a
 * stationary wind-up from an already-moving jump. No RNG is consumed. */
bool nba_shot_action_start(const NbaAssetPack *assets, NbaShotAction *state,
                           bool boosted, bool alternate_lower) {
    if (!state) return false;
    NbaShotAction s = *state;
    uint16_t request = 0;
    if (!nba_player_animation_command(assets, &s.animation, NBA_ANIMATION_CANCEL_UPPER,
            &request, boosted, alternate_lower) ||
        !nba_player_animation_command(assets, &s.animation, NBA_ANIMATION_CANCEL_LOWER,
            &request, boosted, alternate_lower)) return false;
    s.mode = 12; s.speed = 0;
    s.velocity_x = nba_gameplay_arithmetic_shift_right(s.velocity_x, 1);
    s.velocity_y = nba_gameplay_arithmetic_shift_right(s.velocity_y, 1);
    request = 0x16;
    s.bounce_count = 0;
    if (!(s.velocity_x | s.velocity_y)) {
        s.activity = 1;
        if (!nba_player_animation_command(assets,&s.animation,NBA_ANIMATION_INSTALL_BOTH,
                &request,boosted,alternate_lower)) return false;
    } else {
        s.activity = 0xFFFF; s.bounce_timer = 0; s.velocity_z = 0x210;
        if (!nba_player_animation_command(assets,&s.animation,NBA_ANIMATION_INSTALL_UPPER,
                &request,boosted,alternate_lower)) return false;
        request = 0x32;
        if (!nba_player_animation_command(assets,&s.animation,NBA_ANIMATION_INSTALL_LOWER,
                &request,boosted,alternate_lower)) return false;
    }
    s.flags |= 4;
    *state = s;
    return true;
}

/* `$86:B84C-$B866`: launch after the wind-up/sidestep decision. The free
 * throw path preserves vertical velocity, and installs only the lower body. */
bool nba_shot_action_jump(const NbaAssetPack *assets, NbaShotAction *state,
                          bool free_throw, bool alternate_lower) {
    if (!state) return false;
    NbaShotAction s=*state;
    uint16_t request=0;
    if (!nba_player_animation_command(assets,&s.animation,NBA_ANIMATION_CANCEL_LOWER,
            &request,false,alternate_lower)) return false;
    if (!free_throw) s.velocity_z=0x210;
    request=0x32;
    if (!nba_player_animation_command(assets,&s.animation,NBA_ANIMATION_INSTALL_LOWER,
            &request,false,alternate_lower)) return false;
    *state=s; return true;
}

/* `$86:9846-$986C`: team identity is actor +$6E, not its host array slot. */
void nba_shot_action_restore(NbaShotAction *s,uint16_t group,uint16_t active) {
    s->mode=group==active ? 1u : 2u;
    s->behavior_timer=0x2F;
    s->timer=s->flags=s->status=0;
}

/* `$86:B8C0-$B8C8`: post-release cleanup does NOT clear actor +$28. */
void nba_shot_action_clear(NbaShotAction *s) { s->timer=s->flags=0; }

/* `$86:B7CD-$B7F6`: signed activity countdown boundary. A negative activity
 * is already airborne; the stationary counter starts at 1 and advances by C6. */
NbaShotStage nba_shot_action_delay(uint16_t *activity,uint16_t delta,bool free_throw) {
    if ((int16_t)*activity<0) return NBA_SHOT_AIRBORNE;
    uint16_t next=(uint16_t)(*activity+delta);
    if ((int16_t)(uint16_t)(next-0x1E)<0) {
        *activity=next; return NBA_SHOT_DELAY;
    }
    *activity=0xFFFF;
    return free_throw ? NBA_SHOT_AIRBORNE : NBA_SHOT_JUMP;
}

/* `$86:B8CA-$B950/$B958-$B970/$B978`: facing and release decision only;
 * stops BEFORE the `$86:9D6E` launch call. It samples $07F6 without stepping
 * the RNG. Human-button and CPU branches are separate, even in this CPU port. */
NbaShotGate nba_shot_action_gate(NbaShotGateInput *s) {
    if (!s->free_throw && !(s->z | s->velocity_z)) return NBA_SHOT_GROUNDED;
    if (s->lower_accumulator >= 0x600) {
        uint16_t target=shot_facing((int16_t)(s->basket_x-s->x),
                                   (int16_t)(0u-(uint16_t)s->y));
        uint16_t difference=(uint16_t)(s->facing-target);
        if (difference) {
            difference &= 7;
            s->facing=(uint16_t)((s->facing+(difference<4 ? -1 : 1)) & 7);
        }
    }
    if (s->controller<0) {
        if (s->velocity_z<0) return NBA_SHOT_RELEASE;
        if (s->velocity_z>=0x60) return NBA_SHOT_WAIT;
        return s->free_throw || !(s->rng&0x70) ? NBA_SHOT_RELEASE : NBA_SHOT_WAIT;
    }
    if (s->free_throw || (int16_t)(uint16_t)((uint16_t)s->velocity_z-0xFE81u)<0)
        return NBA_SHOT_RELEASE;
    return s->buttons&0x80 ? NBA_SHOT_WAIT : NBA_SHOT_RELEASE;
}
