#include "nba_human_pass_catch.h"

static bool neg_difference(uint16_t a, uint16_t b) { return ((uint16_t)(a-b)&0x8000u)!=0u; }
static uint16_t magnitude(uint16_t a) { return (a&0x8000u)?(uint16_t)(0u-a):a; }
static bool valid(const NbaHumanPassCatchState *s) { return s && s->source_slot<10u && s->receiver_slot<10u; }
bool nba_human_pass_catch_geometry(NbaHumanPassCatchState *s) {
    if (!valid(s)) return false;
    const NbaHumanPassCatchActor *r=&s->actors[s->receiver_slot];
    s->ba=r->y; s->b6=(uint16_t)(s->basket_x-r->x);
    uint16_t major=magnitude(s->b6),minor=magnitude(r->y);
    if (neg_difference(major,minor)) { uint16_t swap=major;major=minor;minor=swap; }
    s->aa=(uint16_t)(major+(minor>>2u));
    /* AD83 deliberately dereferences [00]+42. Natural left1591/1641/1760
     * read WRAM mirrors00004C/06004C/02004A, words7E50/7E50/7000.
     * Preserve that source pointer; substituting [E0] or [E6] is a port bug. */
    static const uint16_t rating[16]={101,103,104,106,107,109,110,112,114,115,117,118,120,121,123,125}; /* 80:D04A */
    s->ae=(uint16_t)(rating[(s->indirect_word_42&0xffu)>>4u]+0x30u);
    return true;
}
bool nba_human_pass_catch_rng(NbaHumanPassCatchState *s) {
    if (!s) return false;
    uint16_t old=s->rng_07f6;
    s->rng_07f6=old?(uint16_t)((uint16_t)(old<<1u)^((old&0x8000u)?0x1d87u:0u)):0x9146u;
    return true;
}
bool nba_human_pass_catch_direction(NbaHumanPassCatchState *s) {
    if (!s) return false;
    s->b2=(uint16_t)(s->aa|s->ae);
    if (!s->b2) { s->aa=8u;return true; }
    uint16_t x=s->aa,y=s->ae,key=0u;
    if (x&0x8000u) { x=(uint16_t)(0u-x);key|=8u; }
    if (y&0x8000u) { y=(uint16_t)(0u-y);key|=4u; }
    /* F061/F063 has only CMP/BPL. Equality does NOT swap here, unlike
     * F37D/F37F's additional BEQ in F34F. Do not reuse that quantizer. */
    if (neg_difference((uint16_t)(y-1u),x)) { uint16_t swap=x;x=y;y=swap;key|=2u; }
    x=(uint16_t)(x<<1u);
    if (neg_difference((uint16_t)(y-1u),x)) key|=1u;
    static const uint8_t map[16]={0,1,2,1,4,3,2,3,0,7,6,7,4,5,6,5}; /* 85:F09A */
    s->aa=map[key];s->ae=y;s->b2=key;return true;
}
bool nba_human_pass_catch_lane(NbaHumanPassCatchState *s) {
    if (!valid(s)) return false;
    const NbaHumanPassCatchActor *source=&s->actors[s->source_slot];
    if (source->order_cursor<1u || source->order_cursor>11u || s->order[0]!=0xffffu || s->order[12]!=0xffffu) return false;
    for (unsigned i=0;i<13u;++i) if (s->order[i]!=0xffffu && s->order[i]>10u) return false;
    uint16_t x1=source->x,x2=s->basket_x,y1=source->y,y2=0u;
    if (neg_difference(x1,x2)) { x1=(uint16_t)(x1-8u);x2=(uint16_t)(x2+24u); }
    else { x1=(uint16_t)(x1+8u);x2=(uint16_t)(x2-24u); }
    if (y1&0x8000u) { y1=(uint16_t)(y1-24u);y2=24u; }
    else { y1=(uint16_t)(y1+24u);y2=(uint16_t)-24; }
    s->b2=source->team;
    /* F660/F6BD scan the original order around source+14. An X miss at
     * F697 reverses; at F6F4 it returns clear. A Y miss continues. This
     * also preserves volatile92/AC/AE, rather than rescanning all actors. */
    for (unsigned pass=0;pass<2u;++pass) {
        int step=pass?-1:1;
        for (int cursor=(int)source->order_cursor+step;cursor>=0 && cursor<13;cursor+=step) {
            uint16_t slot=s->order[cursor];
            s->candidate_92=slot==0xffffu?0u:(uint16_t)(0x34ebu+slot*0x100u);
            if (slot==0xffffu) break;
            const NbaHumanPassCatchActor *other=&s->actors[slot];
            if (slot==10u || other->team==source->team) continue;
            s->aa=(uint16_t)(other->x-x1);s->ac=magnitude(s->aa);s->ae=(uint16_t)(other->x-x2);
            if (!((s->aa^s->ae)&0x8000u)) break;
            s->aa=(uint16_t)(other->y-y1);s->ae=(uint16_t)(other->y-y2);
            if ((s->aa^s->ae)&0x8000u) { s->aa=1u;return true; }
        }
    }
    s->aa=0u;return true;
}
NbaHumanPassCatchRoute nba_human_pass_catch_attempt(NbaHumanPassCatchState *s) {
    if (!valid(s)) return NBA_HUMAN_PASS_CATCH_INVALID;
    if (neg_difference(s->ae,s->aa) || neg_difference(s->aa,0x10u)) return NBA_HUMAN_PASS_CATCH_AE10;
    s->attempt_0904=0u;
    uint16_t rating=s->profile_word_39&0xffu;
    bool axis_gate=rating<0x4cu;
    if (rating>=0x4cu && rating<0x5cu) {
        nba_human_pass_catch_rng(s);
        axis_gate=(s->rng_07f6&(rating<0x54u?1u:3u))==0u;
    }
    if (axis_gate) {
        uint16_t axis=s->actors[s->source_slot].axis_88>>1u;
        if (axis==0u || axis==4u) return NBA_HUMAN_PASS_CATCH_AE10;
        s->attempt_0904=1u;
    }
    s->aa=s->b6;s->ae=(uint16_t)(0u-s->ba);
    nba_human_pass_catch_direction(s);s->be=s->aa;
    uint16_t saved_source=s->source_slot;s->source_slot=s->receiver_slot;
    bool lane_ok=nba_human_pass_catch_lane(s);s->source_slot=saved_source; /* AE06/AE07 restores96. */
    if (!lane_ok) return NBA_HUMAN_PASS_CATCH_INVALID;
    return s->aa?NBA_HUMAN_PASS_CATCH_AE10:NBA_HUMAN_PASS_CATCH_AF66;
}
NbaHumanPassCatchRoute nba_human_pass_catch_receiver(NbaHumanPassCatchState *s) {
    if (!valid(s)) return NBA_HUMAN_PASS_CATCH_INVALID;
    uint16_t band=s->actors[s->source_slot].pass_band;
    if (band>30u || band%6u) return NBA_HUMAN_PASS_CATCH_INVALID;
    /* AF6E indexes AFA6 by the raw six-byte band. At band30 it reaches
     * AFC4's original opcode bytes A6 8E, not another data row. Preserve
     * that source-derived value; no natural band30 witness is claimed. */
    static const uint16_t bands[6]={20,25,30,40,50,0x8ea6u};
    s->b2=(uint16_t)(bands[band/6u]+0x24u);
    s->actors[s->receiver_slot].timer=s->b2;
    uint16_t old=s->source_slot;s->source_slot=s->receiver_slot;s->receiver_slot=old;
    return NBA_HUMAN_PASS_CATCH_B468; /* AF83 child NOT executed; no synthetic after-state. */
}
NbaHumanPassCatchRoute nba_human_pass_catch_prepare(NbaHumanPassCatchState *s) {
    if (!nba_human_pass_catch_geometry(s)) return NBA_HUMAN_PASS_CATCH_INVALID;
    NbaHumanPassCatchRoute route=nba_human_pass_catch_attempt(s);
    return route==NBA_HUMAN_PASS_CATCH_AF66?nba_human_pass_catch_receiver(s):route;
}
