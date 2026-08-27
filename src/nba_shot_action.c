#include "nba_shot_action.h"
#include "nba_gameplay_ai.h"
#include "nba_gameplay_ball.h"

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
