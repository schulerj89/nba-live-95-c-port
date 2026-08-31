#include "nba_receiver_prepare.h"
#include <string.h>

static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0] | ((uint16_t)p[1]<<8)); }
static uint32_t u32(const uint8_t *p) { return u16(p) | ((uint32_t)u16(p+2)<<16); }
static bool neg(uint16_t a,uint16_t b) { return ((uint16_t)(a-b)&0x8000u)!=0; }
static uint16_t magnitude(uint16_t a) { return a&0x8000u ? (uint16_t)(0u-a) : a; }
static uint16_t next_rng(NbaReceiverPrepareState *s) {
    uint16_t old=s->rng_07f6;
    s->rng_07f6=old ? (uint16_t)((uint16_t)(old<<1) ^ (old&0x8000u ? 0x1d87u : 0u)) : 0x9146u;
    return s->rng_07f6;
}
static bool bank_word(const NbaAssetItem *item,uint16_t address,uint16_t *value) {
    const uint8_t *data=item->data;
    uint32_t offset=u32(data+20);
    if(address<0x8000u || address>0xfffeu || offset>item->size ||
       0x8000u>item->size-offset) return false;
    *value=u16(data+offset+address-0x8000u);return true;
}
static bool table_byte(const NbaAssetItem *item,unsigned header,uint16_t resource,
                       unsigned delta,uint16_t *value) {
    const uint8_t *data=item->data;
    uint32_t offset=u32(data+header);
    if(resource>=0x830u || offset>item->size || delta>item->size-offset ||
       resource>=item->size-offset-delta) return false;
    unsigned b=data[offset+delta+resource];
    *value=(uint16_t)(b<0x80u ? b : b+0xff00u);return true;
}

/* $87:B7D8 -> B952, point1. B7DA sets Y=$84 before B7E1 LDA $A8,Y:
 * the original reads DBR:$012C, NOT actor+$A8. The natural right capture
 * observes nonzero $7E:012C with actor+$A8=0; preserve the address quirk.
 * This child contains no RNG call; the intervening roll is $86:AAE1. */
static bool pose(const NbaAssetPack *assets,const NbaReceiverPrepareInput *in,
                 NbaReceiverPrepareState *s,uint16_t upper,uint16_t phase) {
    const NbaAssetItem *item=nba_assets_get(assets,NBA_ASSET_PLAYER_ANIMATIONS);
    const uint8_t *data=item?item->data:NULL;
    if(!item || !item->data || item->size<80u || memcmp(item->data,"NBPANIM1",8) ||
       u32(data+8)!=6u || u32(data+12)!=57u) return false;
    uint16_t lower=(uint16_t)(0x20u+(s->p51>>1)),descriptor,list,lower_resource,upper_resource;
    uint16_t direction_index=(uint16_t)(s->p04*2u+8u);
    s->p1a=phase;s->p49=0x84u;s->p06=s->p04<3u?0xffffu:0u;
    uint16_t lower_table=in->alternate_word_012c?0xc28au:0xc218u;
    if(!bank_word(item,(uint16_t)(lower_table+lower*2u),&descriptor) ||
       !bank_word(item,(uint16_t)(descriptor+direction_index),&list) ||
       !bank_word(item,(uint16_t)(list+s->p14*2u),&lower_resource) ||
       !bank_word(item,(uint16_t)(0xc2fcu+upper*2u),&descriptor) ||
       !bank_word(item,(uint16_t)(descriptor+direction_index),&list) ||
       !bank_word(item,(uint16_t)(list+phase*2u),&upper_resource)) return false;
    uint16_t ly,lz,ux,uy,uz;
    if(!table_byte(item,24,lower_resource,0,&ly) ||
       !table_byte(item,24,lower_resource,0x830,&lz) ||
       !table_byte(item,68,upper_resource,0,&ux) ||
       !table_byte(item,72,upper_resource,0,&uy) ||
       !table_byte(item,76,upper_resource,0,&uz)) return false;
    s->p47=s->p06;
    if(s->p47)ly=(uint16_t)(0u-ly);
    if(s->p06)uy=(uint16_t)(0u-uy);
    uint16_t sum=(uint16_t)(ly+uy),mid=(uint16_t)((sum>>1)|(sum&0x8000u));
    s->p00=(uint16_t)(mid-(uint16_t)(ux*2u));
    s->p02=(uint16_t)(mid+(uint16_t)(ux*2u));
    s->p04=(uint16_t)(ux-(uint16_t)(lz+uz));return true;
}

/* $86:AA6A-AB0C. E0/C2 still identify the inherited passer at AF66,
 * whereas $96+B2 and +6E identify the receiver after pointer exchange. */
static void accuracy(const NbaReceiverPrepareInput *in,NbaReceiverPrepareState *s) {
    static const uint16_t errors[16]={0,5,3,3,5,0,3,0xfffd,0,0xfffb,0xfffd,0xfffd,0xfffb,0,0xfffd,3};
    uint16_t chance=(uint16_t)((in->profile_word_39&255u)+150u);
    uint16_t fatigue=(uint16_t)(((uint16_t)(0x7fffu-in->stamina_word_18)>>8)>>2);
    chance=(uint16_t)(chance-fatigue);if(chance&0x8000u)chance=0;
    if(!neg(s->actor.modifier_b2,3))chance=(uint16_t)(chance+(chance>>2));
    if(!(in->hot_team_09c0&0x8000u))chance=in->hot_team_09c0==s->actor.team_6e?
        (uint16_t)(chance+35u):(uint16_t)(chance>>1);
    if(neg(chance,5))chance=5;else if(!neg(chance,255))chance=255;
    if((next_rng(s)&255u)>=chance) {
        unsigned index=(s->p4f^4u)*2u;
        s->ac=(uint16_t)(s->ac+errors[index]);s->b0=(uint16_t)(s->b0+errors[index+1]);
    }
}

/* $85:F8D9/F867: low signed quotient and UNSIGNED remainder-low X.
 * Denominator0 returns quotient0 with the untouched magnitude remainder.
 * Preserve wrapped low quotient, including overflow; never saturate. */
static void divide(NbaReceiverPrepareState *s,uint16_t lo,uint16_t hi,
                   uint16_t divisor,uint16_t *quotient,uint16_t *remainder) {
    uint32_t value=lo|((uint32_t)hi<<16);s->sign_0824=0;
    if(hi&0x8000u){value=0u-value;s->sign_0824=1;}
    uint16_t den=divisor;
    if(den&0x8000u){den=(uint16_t)(0u-den);s->sign_0824=(uint16_t)(s->sign_0824-1u);}
    s->ce=(uint16_t)(value>>16);s->d0=den;
    uint32_t q=den?value/den:0u,r=den?value%den:value;
    s->math_0806=(uint16_t)r;s->math_0808=(uint16_t)(r>>16);
    s->math_080a=(uint16_t)(den>>1);s->math_080c=0;
    s->math_080e=(uint16_t)q;s->math_0810=(uint16_t)(q>>16);
    s->cc=s->sign_0824?(uint16_t)(0u-(uint16_t)q):(uint16_t)q;
    *quotient=s->cc;*remainder=(uint16_t)r;
}

bool nba_receiver_prepare(const NbaAssetPack *assets,const NbaReceiverPrepareInput *in,
                          NbaReceiverPrepareState *state) {
    static const uint16_t upper[8]={0x18,0x1c,0x18,0x1a,0x19,0x1b,0x1d,0x1e};
    static const uint16_t phase[8]={5,4,5,7,6,5,7,3};
    if(!assets || !in || !state || (state->actor.axis_88>>1)>7u)return false;
    NbaReceiverPrepareState s=*state;
    s.p04=s.p4f=(uint16_t)(s.actor.axis_88>>1);s.p51=(uint16_t)((next_rng(&s)&3u)*2u);
    for(;;) {
        s.p18=s.attempt_0904?7u:(uint16_t)(next_rng(&s)&7u);
        if(s.p18==7u && (s.p4f==0u || s.p4f==4u)){s.attempt_0904=0;continue;}
        s.ba=(uint16_t)(s.p18*2u);
        if(s.p18==1u){
            /* B4B3/B4B6 tests wrapped CMP N, not signed operands. AF66
             * supplies the pass-band timer here, not team group0/5. */
            if(!neg(s.b2,0x40u)){s.attempt_0904=0;continue;}
            s.p04^=4u;
        }
        break;
    }
    s.p00=1;s.p02=upper[s.p18];s.p14=2;
    if(!pose(assets,in,&s,upper[s.p18],phase[s.p18]))return false;
    uint32_t x=(in->basket_x_fraction|((uint32_t)in->basket_x<<16))-
               (s.actor.x_fraction|((uint32_t)s.actor.x<<16));
    uint32_t y=0u-(s.actor.y_fraction|((uint32_t)s.actor.y<<16));
    s.aa=s.b4=(uint16_t)x;s.ac=(uint16_t)(x>>16);
    s.ae=s.b8=(uint16_t)y;s.b0=(uint16_t)(y>>16);
    s.b2=magnitude(s.ac);s.b6=magnitude(s.b0);
    if(neg(s.b2,s.b6)){uint16_t t=s.b2;s.b2=s.b6;s.b6=t;}
    s.b6>>=2;s.b2=(uint16_t)(s.b2+s.b6);
    accuracy(in,&s);
    s.ac=(uint16_t)(s.ac-s.p00);s.b0=(uint16_t)(s.b0-s.p02);
    if(s.p18==7u)s.b0=(uint16_t)(s.b0+((s.actor.y&0x8000u)?0xfff8u:8u));
    s.b2=(uint16_t)((s.actor.timer_60&255u)<<8);
    divide(&s,s.aa,s.ac,s.b2,&s.aa,&s.ac);
    divide(&s,s.ae,s.b0,s.b2,&s.ae,&s.b0);
    s.actor.velocity_x=s.actor.baseline_x_ba=s.aa;
    s.actor.velocity_y=s.actor.baseline_y_bc=s.ae;
    s.aa=magnitude(s.aa);s.ae=magnitude(s.ae);
    if(neg(s.aa,s.ae)){uint16_t t=s.aa;s.aa=s.ae;s.ae=t;}
    s.ae>>=2;s.aa=(uint16_t)(s.aa+s.ae);
    s.actor.magnitude_4c=s.aa;s.actor.speed_4a=(uint16_t)(s.aa*2u);
    if(next_rng(&s)&0x30u)s.actor.flags_7e|=1u;
    s.actor.facing_4e=s.actor.movement_50=s.p4f;s.actor.flags_7e|=6u;
    s.live_0936=2;s.timeout_091c=0;
    s.actor.selector_56=(uint16_t)(s.p18-1u);s.actor.variant_58=s.p51;s.actor.upper_66=upper[s.p18];
    *state=s;return true;
}

bool nba_receiver_pass_prepare(const NbaAssetPack *assets,const NbaReceiverPrepareInput *in,
                               NbaReceiverPassState *state) {
    static const uint16_t flight[6]={20,25,30,40,50,0x8ea6};
    if(!state || state->passer_band_62>30u || state->passer_band_62%6u)return false;
    NbaReceiverPassState s=*state;uint16_t saved51=s.receiver.p51;
    /* AF6E uses the raw six-byte band. Band30 reaches AFC4's A6 8E opcode
     * bytes; preserve the original word, not an invented sixth data row. */
    s.receiver.b2=(uint16_t)(flight[s.passer_band_62/6u]+0x24u);
    s.receiver.actor.timer_60=s.receiver.b2;
    if(!nba_receiver_prepare(assets,in,&s.receiver))return false;
    /* AF87-AF8D restores the caller's original pointers. The typed API never
     * changes their identities; mode belongs to receiver, flags to passer. */
    s.receiver_mode_5e=14u;s.passer_flags_7e|=4u;s.receiver.p51=saved51;
    *state=s;return true;
}
