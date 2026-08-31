#include "nba_human_pass_launch.h"

static uint16_t magnitude16(uint16_t value) {
    return (value&0x8000u)?(uint16_t)(0u-value):value;
}
static uint16_t low(uint32_t value) { return (uint16_t)value; }
static uint16_t high(uint32_t value) { return (uint16_t)(value>>16); }
static uint32_t join(uint16_t lo,uint16_t hi) { return lo|((uint32_t)hi<<16); }
static bool cmp_negative(uint16_t a,uint16_t b) { return ((uint16_t)(a-b)&0x8000u)!=0; }

NbaHumanPassLaunchRegisters nba_human_pass_launch_multiply(
        NbaHumanPassLaunchMath *m,NbaHumanPassLaunchRegisters in) {
    uint16_t a=magnitude16(in.a),x=magnitude16(in.x);
    uint32_t product=(uint32_t)a*x;
    /* F7C9-F820 keeps the magnitude low word at0820. Its overlapping821
     * stores leave the upper byte of the cross-product sum at0822, not
     * the final product high word returned inX. Both magnitudes<=8000,
     * so the byte cross sum cannot overflow16 bits on this signed path. */
    uint32_t cross=((uint32_t)(a&255u)*(x&255u)>>8)+
                   (uint32_t)(a>>8)*(x&255u)+(uint32_t)(a&255u)*(x>>8);
    m->count_085a=(uint16_t)(m->count_085a+1u);
    m->product_low_0820=low(product);m->cross_high_0822=(uint16_t)(cross>>8);
    if((in.a^in.x)&0x8000u) {
        product=0u-product;
        /* F7A5/F7A8 EORFFFF / BEQ skipsINC when magnitude low isFFFF.
         * Preserve the source's one-low-word decrement from normal negation.
         * This edge is source-derived; natural reachability is not claimed. */
        if(m->product_low_0820==0xffffu)--product;
    }
    /* F7D4 SEP30 sets the8-bit index flag and clears BOTH index high bytes,
     * includingY without a LDY instruction. LaterREP30 does not restore it.
     * All natural first99C4 multiply calls witness actor-pointerY ->00EB. */
    in.a=low(product);in.x=high(product);in.y&=0xffu;return in;
}

NbaHumanPassLaunchRegisters nba_human_pass_launch_divide(
        NbaHumanPassLaunchMath *m,NbaHumanPassLaunchRegisters in) {
    uint32_t numerator=join(in.a,in.x);bool negative_numerator=(in.x&0x8000u)!=0;
    bool negative_denominator=(in.y&0x8000u)!=0;
    if(negative_numerator)numerator=0u-numerator;
    uint16_t denominator=magnitude16(in.y);
    m->cc=low(numerator);m->ce=high(numerator);m->d0=denominator;
    m->sign_0824=(uint16_t)((negative_numerator?1u:0u)-(negative_denominator?1u:0u));
    /* F867-F8D8 performs unsigned long division. Its zero-denominator branch
     * F87A->F8CF returns quotient0 and the original magnitude as remainder.
     * Preserve that native result instead of invoking undefined C division. */
    uint32_t quotient=denominator?numerator/denominator:0;
    uint32_t remainder=denominator?numerator%denominator:numerator;
    m->remainder_0806=low(remainder);m->remainder_high_0808=high(remainder);
    m->divisor_080a=(uint16_t)(denominator>>1);m->divisor_high_080c=0;
    m->quotient_080e=low(quotient);m->quotient_high_0810=high(quotient);
    m->cc=m->sign_0824?(uint16_t)(0u-low(quotient)):low(quotient);
    /* F8D5 places remainder low inX; F8D9's wrapper never replacesX/Y.
     * X is NOT the signed quotient's high word, despite99C4 storing it
     * alongsideA into its temporary B6/B8 or BA/BC pair. */
    in.a=m->cc;in.x=m->remainder_0806;in.y=m->quotient_high_0810;return in;
}

static uint16_t floor_shift16(uint16_t value,unsigned count) {
    uint16_t shifted=(uint16_t)(value>>count);
    if(value&0x8000u)shifted|=(uint16_t)(0xffffu<<(16u-count));
    return shifted;
}
static uint16_t scaled_delta(NbaHumanPassLaunchMath *math,uint16_t end,uint16_t start,uint16_t duration) {
    uint16_t delta=(uint16_t)(end-start);
    /* 9AB7-9AD1 sign-extends the wrapped16-bit delta, then shifts its byte
     * representation by8. It does not subtract full host coordinates. */
    uint32_t fixed=(uint32_t)delta<<8;
    if(delta&0x8000u)fixed|=0xff000000u;
    NbaHumanPassLaunchRegisters in={low(fixed),high(fixed),duration};
    return nba_human_pass_launch_divide(math,in).a;
}
static uint16_t predict(NbaHumanPassLaunchMath *math,uint16_t position,uint16_t velocity,uint16_t duration) {
    NbaHumanPassLaunchRegisters in={duration,velocity,0};
    NbaHumanPassLaunchRegisters out=nba_human_pass_launch_multiply(math,in);
    uint32_t product=join(out.a,out.x);
    uint32_t shifted=product>>8;
    if(product&0x80000000u)shifted|=0xff000000u;
    return (uint16_t)(position+low(shifted));
}

bool nba_human_pass_launch(const NbaHumanPassLaunchTables *tables,NbaHumanPassLaunchState *state) {
    if(!tables||!state||state->source_index>=10||state->receiver_index>=10)return false;
    NbaHumanPassLaunchState s=*state;
    NbaHumanPassLaunchActor *source=&s.actors[s.source_index],*receiver=&s.actors[s.receiver_index];
    if(source->band_62>30 || source->band_62%6)return false;
    unsigned family=(source->family_c0&0x8000u)?0u:source->family_c0?1u:2u;
    unsigned band=source->band_62/2u;
    uint16_t duration=tables->family[family][band];
    s.profile_0914=s.profile_e0;s.profile_0916=s.profile_e2;
    s.released_094a=1;s.owner_093e=0xffff;source->delay_5a=20;
    s.pointer_92=s.ball_pointer_0910=0x3eeb;s.duration_b2=duration;
    uint16_t target_x=predict(&s.math,receiver->x,receiver->velocity_x,duration);
    uint16_t target_y=predict(&s.math,receiver->y,receiver->velocity_y,duration);
    /* 9A8D/9A95/9A9F/9AA7 branch on wrapped CMP sign. Clamping also changes
     * the RECEIVER's actual velocity at9BF5/9C3F; a host endpoint clamp
     * without those writes would discard original gameplay behavior. */
    bool clamp_x=false,clamp_y=false;
    if(!cmp_negative(target_x,0x016b)) { target_x=0x016a;clamp_x=true; }
    else if(cmp_negative(target_x,0xfe96)) { target_x=0xfe96;clamp_x=true; }
    if(clamp_x)receiver->velocity_x=scaled_delta(&s.math,target_x,receiver->x,duration);
    if(!cmp_negative(target_y,0x00c1)) { target_y=0x00c0;clamp_y=true; }
    else if(cmp_negative(target_y,0xff40)) { target_y=0xff40;clamp_y=true; }
    if(clamp_y)receiver->velocity_y=scaled_delta(&s.math,target_y,receiver->y,duration);
    s.ball_velocity_x=scaled_delta(&s.math,target_x,s.ball_x,duration);
    s.ball_velocity_y=scaled_delta(&s.math,target_y,s.ball_y,duration);
    s.ball_velocity_z=tables->family[family][band+1];
    if(source->z>=16)s.ball_velocity_z=(uint16_t)(s.ball_velocity_z-0x80u);
    /* 9C45 arithmetic shifts6 times, then a seventh unless upper30 is2B/2C.
     * This changes integer position only; fractional positions stay intact. */
    unsigned shift=(source->upper_30==0x2b||source->upper_30==0x2c)?6u:7u;
    s.ball_x=(uint16_t)(s.ball_x+floor_shift16(s.ball_velocity_x,shift));
    s.ball_y=(uint16_t)(s.ball_y+floor_shift16(s.ball_velocity_y,shift));
    s.ball_z=(uint16_t)(s.ball_z+floor_shift16(s.ball_velocity_z,shift));
    if(receiver->mode_5e!=14)receiver->timer_60=(uint16_t)(duration+15u);
    if(source->mode_5e!=15) {
        source->mode_5e=source->group_6e==s.offense_093a?1:2;
        source->behavior_64=47;source->timer_60=0;source->flags_7e=0;source->flags_28=0;
    }
    if(cmp_negative(s.live_0936,0x81))s.live_0936=0;
    *state=s;return true;
}
