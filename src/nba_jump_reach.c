#include "nba_tipoff_flow.h"
#include "nba_gameplay_ai.h"
#include <string.h>

void nba_reach_launch(NbaReachLaunch *s) {
    uint16_t x=s->subject_x,y=s->subject_y;
    uint16_t dx=(uint16_t)(x-s->actor_x),dy=(uint16_t)(y-s->actor_y);
    bool far=((uint16_t)(s->subject_distance-32u)&0x8000u)==0;
    if(far) {
        uint16_t vx=(uint16_t)nba_gameplay_arithmetic_shift_right((int16_t)s->subject_vx,4);
        uint16_t vy=(uint16_t)nba_gameplay_arithmetic_shift_right((int16_t)s->subject_vy,4);
        /* EAD0/EAD3 are consecutive ADCs: the first carry is retained. */
        uint32_t sum=(uint32_t)vx+x;
        int16_t lead_x=(int16_t)(uint16_t)(0u-(uint16_t)(sum+s->context_x+(sum>>16)));
        int16_t lead_y=(int16_t)(uint16_t)(0u-(uint16_t)(vy+y));
        x=(uint16_t)(x+(int32_t)lead_x*20/(int16_t)s->subject_distance);
        y=(uint16_t)(y+(int32_t)lead_y*20/(int16_t)s->subject_distance);
        dx=(uint16_t)(x-s->subject_x);dy=(uint16_t)(y-s->subject_y);
    }
    uint16_t direction=nba_gameplay_target_direction((int16_t)dx,(int16_t)dy,NULL);
    if(direction!=8) s->direction_4e=s->direction_50=(uint16_t)(direction^(far?4u:0u));
    s->velocity_x=(uint16_t)((int32_t)(int16_t)(uint16_t)(x-s->actor_x)*256/20);
    s->velocity_y=(uint16_t)((int32_t)(int16_t)(uint16_t)(y-s->actor_y)*256/20);
    s->velocity_z=624;
    ++s->timer_091c;
}

static bool negative(uint16_t a,uint16_t b) {
    return ((uint16_t)(a-b)&0x8000u)!=0;
}

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0]|(uint16_t)p[1]<<8); }
bool nba_graphics_scratch_step(const NbaAssetPack *assets,
    NbaGraphicsScratchState *s,uint16_t delta) {
    const NbaAssetItem *item=nba_assets_get(assets,NBA_ASSET_GAMEPLAY_GRAPHICS_SCRATCH);
    if(!s || !item || !item->data || item->size!=788u ||
       memcmp(item->data,"NBGSCR1",8))return false;
    const uint8_t *b=(const uint8_t *)item->data+8,*selection=b,*table=b+648u;
    NbaGameplayRng rng={s->rng};NbaGraphicsScratchState out=*s;
    for(int slot=2;slot>=0;--slot) {
        NbaGraphicsScratchSlot *q=&out.slots[slot];uint16_t index=0;bool transfer=false;
        if(q->record==0xffffu) {
            unsigned attempts=0;
            do index=nba_gameplay_rng_next(&rng)&31u;while(index>=22u);
            for(;;) {
                if(++attempts>65536u)return false;
                uint16_t ptr=rd16(selection+index*2u);
                if(ptr<0xed61u || ptr>=0xefe9u)return false;
                uint16_t key=rd16(b+(ptr-0xed61u)+6u);bool duplicate=false;
                for(unsigned j=0;j<3;++j) {
                    uint16_t other=out.slots[j].record,other_key;
                    if(other==0xffffu) {
                        /* ROM-COMPATIBILITY QUIRK (retain): F061-F06C scans
                         * all three records, including the empty slot being
                         * populated.  $FFFF+6 wraps to $82:0005; native Mesen
                         * reads $0001 there.  Do not "fix" or skip this slot:
                         * it changes duplicate probing and shared-RNG flow. */
                        other_key=1u;
                    } else {
                        if(other<0xed61u || other>=0xefe9u)return false;
                        other_key=rd16(b+(other-0xed61u)+6u);
                    }
                    if(other_key==key){duplicate=true;break;}
                }
                if(duplicate){index=(uint16_t)((index+1u)%22u);continue;}
                q->record=ptr;q->timer=0;
                if(index>=10u) {
                    uint16_t count=rd16(b+(ptr-0xed61u)+4u);
                    do{q->current=nba_gameplay_rng_next(&rng)&3u;}while(q->current>=count);
                } else q->current=0;
                transfer=true;
                break;
            }
        } else {
            uint16_t ptr=q->record;if(ptr<0xed61u || ptr>=0xefe9u)return false;
            const uint8_t *record=b+(ptr-0xed61u);
            uint16_t count=rd16(record+4u);
            size_t duration=8u+count*2u+q->current*2u;
            if((size_t)(ptr-0xed61u)+duration+2u>648u)return false;
            uint16_t d=rd16(record+duration);
            if(q->timer>=d) {
                q->timer=(uint16_t)(q->timer-d);
                if(ptr>=0xef09u || (uint16_t)(q->current+1u)>=count) {
                    q->timer=q->current=0;q->record=0xffffu;continue;
                }
                ++q->current;
                transfer=true;
            }
        }
        if(q->record==0xffffu)continue;
        if(!transfer){q->timer=(uint16_t)(q->timer+delta);continue;}
        uint16_t ptr=q->record;size_t at=(size_t)(ptr-0xed61u);
        uint16_t selector=(uint16_t)(rd16(b+at+6u)+11u);
        if(selector>=33u)return false;
        uint16_t length=rd16(table+selector*4u+2u);
        out.scratch_0046=(uint16_t)((out.scratch_0046&255u)|(length<<8));
        q->timer=(uint16_t)(q->timer+delta);
    }
    out.rng=rng.state;*s=out;return true;
}

/* F0AA differs from F3C3 at two equality boundaries. Use its ROM map,
 * preserving wrapped N/Z comparisons rather than host signed ordering. */
static uint16_t facing(const uint8_t *map,uint16_t x,uint16_t y) {
    if(!(x|y))return 16;
    unsigned key=0;
    if(x&0x8000u){x=(uint16_t)(0u-x);key|=16;}
    if(y&0x8000u){y=(uint16_t)(0u-y);key|=8;}
    uint16_t d=(uint16_t)(y-1u-x);
    if(!d || (d&0x8000u)){uint16_t t=x;x=y;y=t;key|=4;}
    if(negative((uint16_t)(y-1u),(uint16_t)(5u*x))) {
        d=(uint16_t)(y-1u-((uint16_t)(3u*x)>>1));
        key|=(!d || (d&0x8000u))?2u:1u;
    }
    return map[key];
}

/* `$86:EC32-$EE75`: parent decision only. See jump-reach-differential.md
 * for native witnesses and the outstanding caller/scratch-state adoption.
 * The result's routine requests do not stand in for EAA8/BD1F side effects. */
bool nba_jump_reach_decide(const NbaAssetPack *assets,
    const NbaJumpReachInput *in,NbaJumpReachResult *result) {
    if(!in || !result)return false;
    const NbaAssetItem *item=nba_assets_get(assets,NBA_ASSET_GAMEPLAY_JUMP_TABLES);
    if(!item || !item->data || item->size!=184u ||
       memcmp(item->data,"NBJUMP1",8))return false;
    const uint8_t *data=item->data;
    NbaJumpReachResult out={in->velocity_x,in->velocity_y,in->velocity_z,in->rng,0,{{0}}};
    NbaGameplayRng rng={in->rng};
    /* EC32-EC9C: grounded actor, lower-channel gate, owned-ball facing. */
    if(in->actor_z || in->lower_state==0x32 || !negative(in->ball_z,0x71))goto done;
    if(in->activity) {
        if((in->owner&0x8000u) || !negative(in->distance,0x28))goto done;
        uint16_t direction=facing(data+152,(uint16_t)(in->actor_x-in->subject_x),
                                  (uint16_t)(in->actor_y-in->subject_y));
        if(((direction-in->subject_direction+3u)&15u)<7u)
            out.requests[out.request_count++]=(NbaJumpReachRequest){0x86EAA8,0};
        goto done;
    }
    if(!(in->receiver&0x8000u))goto done;
    /* EE41-EE75 uses the current RNG bits without advancing the RNG. */
    if(negative(in->ball_z,0x49)) {
        if(negative(in->distance,0x21) &&
           (in->direction^4u)!=in->paired_direction &&
           negative(in->movement,0x40) && !(rng.state&0x38u))
            out.requests[out.request_count++]=(NbaJumpReachRequest){0x86BD1F,0};
        goto done;
    }
    if(!negative(in->distance,0x20))goto done;
    uint16_t height=(uint16_t)(in->subject_z-0x51u);
    if(height&0x8000u)goto done;
    uint16_t rating=((in->raw_0046^in->ball_x)&0x8000u)?in->rating_3d:in->rating_3c;
    rating&=255u;
    uint16_t adjustment=(uint16_t)((((uint16_t)(rating-0x32u)>>3)&0xfffeu)+2u);
    uint16_t index=(uint16_t)((height&0xfffeu)-adjustment);
    if(index&0x8000u)goto done;
    if(index>=144u)return false; /* Reject unsupported context, never clamp. */
    uint16_t threshold=(uint16_t)(data[8+index]|(uint16_t)data[9+index]<<8);
    if(!negative(in->subject_vz,threshold))goto done;
    if(in->live_state==0x81) {
        /* ECA1: slow ball always jumps; otherwise consume exactly one draw. */
        uint16_t speed=in->ball_vz;
        if(speed&0x8000u)speed=(uint16_t)(0u-speed);
        uint16_t launch=600;
        if(speed>=0x40u) {
            uint16_t random=nba_gameplay_rng_next(&rng)&3u;
            if(!random)goto done;
            if(random&2u)launch=528;
        }
        out.velocity_z=launch;
        goto pose;
    }
    if(nba_gameplay_rng_next(&rng)&8u)goto done;
    /* EDAA/EDE2 sign-extend the wrapped delta, multiply by256, divide by20
     * toward zero, then use the low quotient word from F8D9. */
    out.velocity_x=(uint16_t)(((int32_t)(int16_t)(uint16_t)(in->subject_x-in->actor_x)*256)/20);
    out.velocity_y=(uint16_t)(((int32_t)(int16_t)(uint16_t)(in->subject_y-in->actor_y)*256)/20);
    if(rating<0x42u) {
        out.velocity_x=(uint16_t)((out.velocity_x>>1)|(out.velocity_x&0x8000u));
        out.velocity_y=(uint16_t)((out.velocity_y>>1)|(out.velocity_y&0x8000u));
    }
    out.velocity_z=600;
    if(in->block_mode) {
        out.requests[out.request_count++]=(NbaJumpReachRequest){0x87B47A,0x34};
        out.requests[out.request_count++]=(NbaJumpReachRequest){0x87B4DB,0x32};
        goto done;
    }
pose:
    if(in->live_state!=0x81 && (nba_gameplay_rng_next(&rng)&1u)) {
        out.requests[out.request_count++]=(NbaJumpReachRequest){0x87B47A,0x33};
        out.requests[out.request_count++]=(NbaJumpReachRequest){0x87B4DB,0x1f};
    } else out.requests[out.request_count++]=(NbaJumpReachRequest){0x87B3BD,0x32};
done:
    out.rng=rng.state;*result=out;return true;
}
