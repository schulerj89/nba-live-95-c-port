#include "period_render_tail.h"
static unsigned index_of(uint16_t pointer){return (pointer-0x34ebu)/256u;}
static void depth(NbaPeriodRenderTail *s,unsigned i) {
    /* FC69-FC7C: wrapped Y-X, two arithmetic right shifts via CMP/ROR,
     * then wrapped subtraction of camera0860. Z is not consumed here. */
    uint16_t value=(uint16_t)((uint16_t)s->y[i]-(uint16_t)s->x[i]);
    value=(uint16_t)((value>>1)|(value&0x8000u));
    value=(uint16_t)((value>>1)|(value&0x8000u));
    s->depth[i]=(uint16_t)(value-s->camera_y);
}
bool nba_period_render_tail(NbaPeriodRenderTail *state) {
    if(!state || state->leading_sentinel)return false;
    NbaPeriodRenderTail next=*state;unsigned seen=0;
    for(unsigned i=0;i<12;i++) {
        uint16_t p=next.draw_order[i];
        if(p<0x34eb || p>0x3feb || ((p-0x34eb)&255u))return false;
        unsigned index=index_of(p);if(seen&(1u<<index))return false;seen|=1u<<index;
        if(i<11 && next.collision.x[i]!=next.x[i])return false;
    }
    if(!nba_period_object_sort(&next.collision))return false;
    /* FBFF doubles2 to16 then decrements to15. FC0D yields gaps7,3,1.
     * Preserve each source comparison, including no swap on equal keys;
     * a generic sort can change ties and wrapped-sign edge behavior. */
    for(unsigned gap=7;gap;gap>>=1)for(unsigned i=0;i+gap<12;i++) {
        unsigned at=i;
        for(;;) {
            unsigned left=index_of(next.draw_order[at]),right=index_of(next.draw_order[at+gap]);
            depth(&next,left);depth(&next,right);
            uint16_t difference=(uint16_t)(next.depth[right]-next.depth[left]);
            if(difference&0x8000u) {
                uint16_t pointer=next.draw_order[at];next.draw_order[at]=next.draw_order[at+gap];next.draw_order[at+gap]=pointer;
            }
            if(at<gap)break;at-=gap;
        }
    }
    next.frame_low++;if(!next.frame_low)next.frame_high++; /* E1FF-E204 */
    *state=next;return true;
}
