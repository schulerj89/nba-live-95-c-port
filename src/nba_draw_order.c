#include "nba_draw_order.h"

static bool valid(const NbaDrawOrder *state) {
    unsigned seen=0;
    if (!state) return false;
    for (unsigned i=0;i<12;i++) {
        uint16_t p=state->order[i];
        if (p<0x34ebu || p>0x3febu || ((p-0x34ebu)&255u)) return false;
        unsigned bit=1u<<((p-0x34ebu)/256u);
        if (seen&bit) return false;
        seen|=bit;
    }
    return true;
}
bool nba_draw_order_initialize(NbaDrawOrder *state) {
    if (!state) return false;
    /* FBF2/FBF8:12 stores, adding0100 after each. No depth clear. */
    for (unsigned i=0;i<12;i++) state->order[i]=(uint16_t)(0x34ebu+256u*i);
    return true;
}
bool nba_draw_order_project(NbaDrawOrder *state,const NbaDrawOrderInput *input) {
    if (!input || !valid(state)) return false;
    NbaDrawOrder next=*state;
    /* The original traverses the CARRIED order backwards. Each record is
     * visited once. Preserve wrapped16 subtraction and CMP8000/ROR sign;
     * host /4 truncates negative values differently. Z is not consumed. */
    for (unsigned cursor=12;cursor>0;cursor--) {
        unsigned i=(next.order[cursor-1]-0x34ebu)/256u;
        uint16_t v=(uint16_t)(input->y[i]-input->x[i]);
        v=(uint16_t)((v>>1)|(v&0x8000u));
        v=(uint16_t)((v>>1)|(v&0x8000u));
        next.depth[i]=(uint16_t)(v-input->camera_y);
    }
    *state=next;return true;
}
bool nba_draw_order_pass(NbaDrawOrder *state) {
    if (!valid(state)) return false;
    /* FC80 loads22 then FC83/84 subtract2 BEFORE the first comparison.
     * FC90/93 swaps only when(rightDepth-leftDepth)'s wrapped N bit is1.
     * Equal depths do not swap; signed host comparisons lose wrap behavior. */
    for (unsigned cursor=11;cursor>0;cursor--) {
        unsigned at=cursor-1;
        unsigned left=(state->order[at]-0x34ebu)/256u;
        unsigned right=(state->order[at+1]-0x34ebu)/256u;
        if ((uint16_t)(state->depth[right]-state->depth[left])&0x8000u) {
            uint16_t p=state->order[at];
            state->order[at]=state->order[at+1];state->order[at+1]=p;
        }
    }
    return true;
}
bool nba_draw_order_update(NbaDrawOrder *state,const NbaDrawOrderInput *input) {
    if (!input || !valid(state)) return false;
    NbaDrawOrder next=*state;
    if (!nba_draw_order_project(&next,input) || !nba_draw_order_pass(&next)) return false;
    *state=next;return true;
}
