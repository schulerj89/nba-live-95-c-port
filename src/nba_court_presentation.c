#include "nba_court_presentation.h"
#include <string.h>

static int clamp(int v,int lo,int hi) { return v<lo?lo:v>hi?hi:v; }

/* `$85:8E28-$8EDC`: pure state-writing part of the presentation wrapper.
 * External subject/audio/renderer callees keep independent proof scopes. */
void nba_court_presentation_update(NbaCourtPresentation *s, int16_t x,
    int16_t y, uint16_t period, int16_t left, int16_t right) {
    /* 8E28-8E4A selects the basket record independently from possession. */
    s->basket_x_3fef=(uint16_t)((period<2u ? x>=0 : x<0)?right:left);
    if(x < -310) {
        s->window_x_087c=(uint16_t)(x+442);
        int opposite=-(int16_t)s->window_x_087c;
        s->window_right_0882=(uint16_t)(opposite<0?0:opposite);
        s->window_left_0880=(uint16_t)clamp(opposite+126,0,255);
        s->window_y_087e=(uint16_t)(y+41);
    } else {
        s->window_left_0880=0;
        if(x>=37) {
            s->window_x_087c=(uint16_t)(x+35);
            int distance=504-(int16_t)s->window_x_087c;
            s->window_left_0880=(uint16_t)(distance<256?distance:255);
            s->window_right_0882=(uint16_t)clamp(distance-168,0,255);
            s->window_y_087e=(uint16_t)(y-160);
        } else {
            s->window_right_0882=255;
            s->window_x_087c=128;
            /* 8ED1 intentionally preserves 087E in the middle band. */
        }
    }
}

void nba_court_stream_init(NbaCourtStream *s,int16_t x,int16_t y) {
    memset(s,0,sizeof(*s));
    s->coarse_x=(uint16_t)((x+582)>>3);
    s->coarse_y=(uint16_t)((y+242)>>3);
    s->source=(uint16_t)(0x8006+s->coarse_x*104+s->coarse_y*2);
    s->source_bank=0xa0;
    /* 85:90C4-9191 initializer uses a fresh circular map at word0800. */
    s->destination=0x800;
    s->scroll_x=s->next_scroll_x=(uint16_t)((x+582)&7);
    s->scroll_y=s->next_scroll_y=(uint16_t)((y+242)&7);
}

static void emit(NbaCourtTransfer cb,void *ctx,uint16_t src,uint16_t bank,
    uint16_t bytes,uint16_t dest) { if(cb)cb(ctx,src,bank,bytes,dest); }

/* `$85:8EE6-$90C3`: source/destination row/column walk, including wrap and
 * three-row limit. SNES transfers are represented by portable descriptors. */
bool nba_court_stream_update(NbaCourtStream *s,const NbaAssetPack *assets,
    int16_t x,int16_t y,int16_t previous_x,int16_t previous_y,
    NbaCourtTransfer cb,void *ctx) {
    const NbaAssetItem *map=nba_assets_get(assets,NBA_ASSET_GAMEPLAY_COURT_MAP);
    if(!s || !map || !map->data || map->size!=15398u || s->source_bank!=0xa0u ||
       x < -582 || x>328 || y < -242 || y> -53) return false;
    const uint8_t *data=(const uint8_t *)map->data;
    s->row_bytes=0;
    s->next_scroll_x=(uint16_t)(s->scroll_x+x-previous_x);
    s->next_scroll_y=(uint16_t)(s->scroll_y+y-previous_y);
    if(s->next_scroll_x==s->scroll_x && s->next_scroll_y==s->scroll_y)return true;
    uint16_t target_x=(uint16_t)((x+582)>>3),target_y=(uint16_t)((y+242)>>3);
    if(s->coarse_x>113 || s->coarse_y>23)return false;
    /* 8F20-8FD3: all exposed columns, split at circular-map row wrap. */
    while(s->coarse_x!=target_x) {
        uint16_t dest,source;
        if(s->coarse_x>target_x) {
            --s->coarse_x;
            if(!(s->destination&31))s->destination=(uint16_t)((s->destination^0x41f)+1);
            --s->destination;dest=s->destination;
            s->source=(uint16_t)(s->source-104);source=s->source;
        } else {
            ++s->coarse_x;++s->destination;
            if(!(s->destination&31))s->destination=(uint16_t)((s->destination-1)^0x41f);
            dest=s->destination^0x400;
            s->source=(uint16_t)(s->source+104);source=(uint16_t)(s->source+0xd00);
        }
        uint16_t row=dest&0x3e0;
        uint16_t bytes=row<0x60?0x3a:(uint16_t)((32-(row>>5))*2);
        emit(cb,ctx,source,s->source_bank,bytes,(uint16_t)~dest);
        if(bytes!=0x3a)emit(cb,ctx,(uint16_t)(source+bytes),s->source_bank,
            (uint16_t)(0x3a-bytes),(uint16_t)~(dest&0xc1f));
    }
    /* 8FD4-90C3: up to three exposed rows per pass, 33 words each.
     * 498E scratch stores columns in ascending order; native loop reads
     * them in reverse. Callback descriptor Y bit15 encodes column DMA. */
    while(s->coarse_y!=target_y && s->row_bytes<0xc6) {
        uint16_t dest,source;
        if(s->coarse_y>target_y) {
            --s->coarse_y;
            s->destination=(s->destination&0x3e0)?(uint16_t)(s->destination-32):(s->destination|0x3e0);
            dest=s->destination;s->source=(uint16_t)(s->source-2);source=s->source;
        } else {
            ++s->coarse_y;
            uint16_t candidate=(uint16_t)(s->destination+32);
            s->destination=(candidate&0x3e0)?candidate:(s->destination^0x3e0);
            dest=(s->destination&0x3e0)<0x80?(uint16_t)(s->destination+0x380):(uint16_t)(s->destination-0x80);
            s->source=(uint16_t)(s->source+2);source=(uint16_t)(s->source+0x38);
        }
        for(unsigned col=0;col<=32;++col) {
            unsigned offset=(unsigned)source+col*104u-0x8000u;
            if(offset+1>=map->size)return false;
            s->rows[s->row_bytes/2+col]=(uint16_t)(data[offset]|((uint16_t)data[offset+1]<<8));
        }
        uint16_t bytes=(uint16_t)((32-(dest&31))*2);
        emit(cb,ctx,(uint16_t)(0x498e+s->row_bytes),0x7e,bytes,dest);
        emit(cb,ctx,(uint16_t)(0x498e+s->row_bytes+bytes),0x7e,
            (uint16_t)(((dest&31)+1)*2),(uint16_t)((dest&0xfe0)^0x400));
        s->row_bytes=(uint16_t)(s->row_bytes+0x42);
    }
    return true;
}

void nba_court_viewport(int16_t camera_x,int16_t camera_y,int *x,int *y) {
    /* Pixel origin matches ROM scroll registers (one-line display offset).
     * Never crop to the old truncated 114-column panorama. */
    *x=clamp(camera_x+582,0,NBA_COURT_WIDTH-256);
    *y=clamp(camera_y+243,0,NBA_COURT_HEIGHT-224);
}

void nba_court_project_actor(int16_t actor_x,int16_t actor_y,
    int16_t actor_z,int16_t camera_x,int16_t camera_y,
    int16_t *screen_x,int16_t *screen_y) {
    /* `$87:A3BB-$A3DC`: complete native player-origin projection body. */
    /* `$87:A3BB-$A3C9` uses two sign-preserving RORs. C signed division
     * truncates negative values toward zero, which is observably different:
     * world (8,3), camera Y -124 is native screen Y 122, not 123.
     * `$87:A3D1-$A3DC` stores X+Y-camera X; `$87:A620-$A629` subtracts
     * integer Z. The fractional position words are not rounded here. */
    int16_t delta=(int16_t)((uint16_t)actor_y-(uint16_t)actor_x);
    int16_t quarter=delta>=0 ? (int16_t)(delta/4) :
        (int16_t)(-((-(int32_t)delta+3)/4));
    if(screen_x)*screen_x=(int16_t)((uint16_t)actor_x+
        (uint16_t)actor_y-(uint16_t)camera_x);
    if(screen_y)*screen_y=(int16_t)((uint16_t)quarter-
        (uint16_t)camera_y-(uint16_t)actor_z);
}

bool nba_court_actor_visible(int16_t screen_x,int16_t projected_y,
    int16_t actor_z,bool human_controlled) {
    /* `$87:A3DF-$A43B`: player culling and controlled-player routing. */
    /* Controlled actors outside the inner rectangle route through `$87:A846`
     * for an off-screen indicator. CPU actors instead use the wider rectangle
     * and `$87:A42F` writes -50 to +$6A when culled. This helper owns the
     * player visibility result, not the still-separate indicator draw. */
    if(human_controlled)
        return screen_x>=11 && screen_x<245 &&
               projected_y>=11 && projected_y<218;
    if(screen_x < -20 || screen_x>=276 || projected_y < -20)return false;
    if(projected_y<288)return true;
    /* `$87:A423-$A42D` gives a high jumping actor one last test after Z is
     * removed. This path was not reached in the current native trace union,
     * but the retained ROM quirk is explicit rather than optimized away. */
    return (int16_t)((uint16_t)projected_y-(uint16_t)actor_z)<288;
}
