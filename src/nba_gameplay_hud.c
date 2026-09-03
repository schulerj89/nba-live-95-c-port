#include "nba_gameplay_hud.h"
#include "nba_rom_font.h"
#include "nba_gameplay_ai.h"
#include <stdio.h>
#include <string.h>

/* Native `$83:D0AD-$D332`, `$81:A03D-$A241`, `$87:BAF5-$BB58` and
 * `$87:BC6A-$BD2E`. Recomp bank83 D0AD/D157/D1B1/D1FD/D2E0, bank81
 * proportional text/grid/upload wrappers, bank87 clock publisher.
 * State mappings and natural entry/exit vectors are enforced by HUD tests.
 * The source-derived canvas and upload ranges are separate from the parent
 * graphics scheduler; no measured3/4-frame delay is synthesized here. */
typedef struct { unsigned x, y, width, height, tile, character; } HudRegion;
static const HudRegion regions[4] = {
    {26,22,4,2,0x3F,0}, {11,16,14,5,0x47,0x80},
    {25,16,5,5,0x8D,0x4E0}, {11,22,15,2,0xA6,0x670}
};
static const unsigned section_sizes[10] = {1008,30,172,928,288,838,66,44,38,418};

static uint16_t word(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}
static uint32_t dword(const uint8_t *p) {
    return (uint32_t)word(p) | ((uint32_t)word(p+2) << 16);
}
static void put_word(uint8_t *p, uint16_t value) {
    p[0]=(uint8_t)value; p[1]=(uint8_t)(value>>8);
}
static const uint8_t *resource(const NbaAssetPack *assets, unsigned section) {
    const NbaAssetItem *item=nba_assets_get(assets,NBA_ASSET_GAMEPLAY_HUD);
    if (!item || !item->data || item->size < 88u) return NULL;
    const uint8_t *data=item->data;
    unsigned count=dword(data+12),version=dword(data+8);
    if (memcmp(data,"NBHUD001",8) ||
        !((version==1u && count==9u) || (version==2u && count==10u)) ||
        section>=count || item->size<16u+count*8u)
        return NULL;
    size_t next=16u+count*8u;
    for(unsigned i=0;i<count;++i) {
        if(dword(data+16u+i*8u)!=next ||
           dword(data+20u+i*8u)!=section_sizes[i] ||
           next > item->size || section_sizes[i] > item->size-next) return NULL;
        next+=section_sizes[i];
    }
    if(next!=item->size)return NULL;
    /* Fixed resource sections must not turn attacker/malformed dimensions
     * into a larger read. Validate every map before initialization/mutation. */
    for(unsigned i=6u;i<count;++i) {
        const uint8_t *map=data+dword(data+16u+i*8u);
        unsigned maps=i==9u?11u:1u;
        for(unsigned j=0u;j<maps;++j) {
            const uint8_t *m=map+j*38u;
            unsigned width=i==6u?6u:i==7u?19u:4u;
            unsigned height=i==6u?5u:i==7u?1u:4u;
            if(word(m)!=width || word(m+2u)!=height || word(m+4u)!=0x304u)return NULL;
        }
    }
    return data+dword(data+16u+section*8u);
}

bool nba_gameplay_hud_init(NbaGameplayHud *hud,const NbaAssetPack *assets) {
    if(!hud || !resource(assets,0u))return false;
    memset(hud,0,sizeof(*hud));
    /* `$87:B99A-$BA53`: clear the entire original$0380-word map range. */
    hud->clock_mirror_raw_08f6=0xFFFFu;
    hud->clear_raw_08ee=0xFFFFu;
    hud->clock_frame_raw_08f4=0xFFFFu;
    hud->initialized=true;
    return true;
}

static void layout(NbaGameplayHud *hud) {
    memset(hud->working_map,0,sizeof(hud->working_map));
    memset(hud->working_characters,0,0x850u);
    /* `$81:A05F` numbers tiles column-first within each original rectangle. */
    for(unsigned i=0;i<4u;++i) {
        const HudRegion *r=regions+i;
        for(unsigned y=0;y<r->height;++y)
            for(unsigned x=0;x<r->width;++x)
                put_word(hud->working_map+((r->y+y)*32u+r->x+x)*2u,
                    (uint16_t)(0x2000u+r->tile+x*r->height+y));
    }
    /* D14F calls A1E7(selector0): only clock-grid CHR uploads here. */
    memcpy(hud->published_characters,hud->working_characters,0x80u);
    hud->published_mask|=1u;
    hud->clear_raw_08ee=0u;
}

static bool font_for(const NbaAssetPack *assets, bool large, NbaRomFont *font) {
    const NbaAssetItem *item=nba_assets_get(assets,large?
        NBA_ASSET_STARTING_LINEUP_FONT:NBA_ASSET_PLAYER_INTRO_FONT);
    const uint8_t *rows=resource(assets,2u);
    if(!item || !item->data || !rows)return false;
    *font=(NbaRomFont){item->data,item->size,rows,172u,1u,1u,0u};
    return true;
}

/* $83:DA12/DA8C and event-3 branch DB29 use the original proportional font.
 * Strings are extracted from $85:9491, $83:DB9D and $80:D350/D0E2. */
static const uint8_t *oob_strings(const NbaAssetPack *assets) {
    const NbaAssetItem *item=nba_assets_get(assets,NBA_ASSET_GAMEPLAY_OUT_OF_BOUNDS);
    const uint8_t *data=item?(const uint8_t *)item->data:NULL;
    if(!item || !item->data || item->size!=1008u ||
       memcmp(data,"NBOOB001",8) || dword(data+8)!=1u ||
       dword(data+12)!=31u)return NULL;
    for(unsigned i=0;i<31u;++i)
        if(!memchr(data+16u+i*32u,0,32u))return NULL;
    return data+16u;
}

bool nba_gameplay_hud_oob_assets_valid(const NbaAssetPack *assets) {
    return oob_strings(assets)!=NULL;
}

static void violation_layout(NbaGameplayHud *hud) {
    static const HudRegion grids[3]={
        {2,5,28,6,0x47,0}, {2,19,22,2,0xEF,0xA80},
        {2,22,28,2,0x11B,0xD40}};
    memset(hud->working_map,0,sizeof(hud->working_map));
    memset(hud->working_characters,0,0x10C0u);
    for(unsigned i=0;i<3u;++i) {
        const HudRegion *r=grids+i;
        for(unsigned y=0;y<r->height;++y)
            for(unsigned x=0;x<r->width;++x)
                put_word(hud->working_map+((r->y+y)*32u+r->x+x)*2u,
                    (uint16_t)(0x2000u+r->tile+x*r->height+y));
    }
}

static bool out_of_bounds(NbaGameplayHud *hud,const NbaAssetPack *assets,
                          NbaGameplayHudInput *input) {
    const uint8_t *strings=oob_strings(assets);
    NbaRomFont font;
    unsigned side=input->event_actor_raw_492d<5u?1u:0u;
    unsigned team=input->teams[side];
    if(input->latched_event_raw_08f0!=3u || team>=29u || !strings ||
       !font_for(assets,true,&font))return false;
    font.fixed_digit_width=12u;
    uint8_t pixels[224u*48u]={0};
    const uint8_t *team_text=strings+(2u+team)*32u;
    uint16_t title_width,team_width,ball_width;
    if(!nba_rom_font_measure(&font,strings,32u,&title_width) ||
       !nba_rom_font_measure(&font,team_text,32u,&team_width) ||
       !nba_rom_font_measure(&font,strings+32u,32u,&ball_width))return false;
    /* DB61-DB6D centers the team plus a literal $2C suffix reservation.
     * DB86-DB95 right-aligns "Ball" at 256-left. Grid origin is (16,40). */
    int left=(int)((uint16_t)(256u-team_width-44u)>>1);
    if(!nba_rom_font_draw(&font,pixels,224u,224,48,112-(int)(title_width/2u),0,strings,32u) ||
       !nba_rom_font_draw(&font,pixels,224u,224,48,left-16,24,team_text,32u) ||
       !nba_rom_font_draw(&font,pixels,224u,224,48,240-left-(int)ball_width,24,strings+32u,32u))
        return false;
    /* A1E7(0) uploads only grid0: 168 tiles, VRAM byte $2470. */
    memset(hud->working_characters,0,0xA80u);
    for(unsigned y=0;y<48u;++y)
        for(unsigned x=0;x<224u;++x) {
            size_t at=((x/8u)*6u+y/8u)*16u+(y&7u)*2u;
            uint8_t color=pixels[y*224u+x];
            hud->working_characters[at]|=(uint8_t)((color&1u)<<(7u-(x&7u)));
            hud->working_characters[at+1u]|=(uint8_t)(((color>>1)&1u)<<(7u-(x&7u)));
        }
    memcpy(hud->published_characters+0x80u,hud->working_characters,0xA80u);
    hud->violation_characters_published=true;
    /* DB0E-DB1E: $7E:4BB0 -> VRAM word $04E0, $0180 bytes. */
    memcpy(hud->visible_map+0x1C0u,hud->working_map+0x140u,0x180u);
    input->presentation_sequence_raw_08e6=0xFFFFu;
    return true;
}

static void unpack_region(const NbaGameplayHud *hud, unsigned region,
                           uint8_t *pixels) {
    const HudRegion *r=regions+region;
    for(unsigned y=0;y<r->height*8u;++y)
        for(unsigned x=0;x<r->width*8u;++x) {
            size_t at=r->character+((x/8u)*r->height+y/8u)*16u+(y&7u)*2u;
            pixels[y*r->width*8u+x]=(uint8_t)(
                ((hud->working_characters[at]>>(7u-(x&7u)))&1u) |
                (((hud->working_characters[at+1]>>(7u-(x&7u)))&1u)<<1));
        }
}
static void pack_region(NbaGameplayHud *hud,unsigned region,const uint8_t *pixels) {
    const HudRegion *r=regions+region;
    memset(hud->working_characters+r->character,0,r->width*r->height*16u);
    for(unsigned y=0;y<r->height*8u;++y)
        for(unsigned x=0;x<r->width*8u;++x) {
            size_t at=r->character+((x/8u)*r->height+y/8u)*16u+(y&7u)*2u;
            uint8_t color=pixels[y*r->width*8u+x];
            hud->working_characters[at]|=(uint8_t)((color&1u)<<(7u-(x&7u)));
            hud->working_characters[at+1]|=(uint8_t)(((color>>1)&1u)<<(7u-(x&7u)));
        }
    /* A1E7 selects this complete grid's contiguous character upload. */
    memcpy(hud->published_characters+r->character,
           hud->working_characters+r->character,r->width*r->height*16u);
    hud->published_mask|=(uint8_t)(1u<<region);
}

static bool names(NbaGameplayHud *hud,const NbaAssetPack *assets,
                   const NbaGameplayHudInput *input) {
    const uint8_t *text=resource(assets,3u);
    NbaRomFont font;
    if(!text || !font_for(assets,true,&font) || input->teams[0]>=29u || input->teams[1]>=29u)
        return false;
    uint8_t canvas[112u*40u];unpack_region(hud,1u,canvas);
    /* D16D reads visitor476B first; D18E reads home46EB second. */
    if(!nba_rom_font_draw(&font,canvas,112u,112,40,0,0,text+input->teams[1]*32u,32u) ||
       !nba_rom_font_draw(&font,canvas,112u,112,40,0,20,text+input->teams[0]*32u,32u))return false;
    pack_region(hud,1u,canvas);return true;
}

static bool scores(NbaGameplayHud *hud,const NbaAssetPack *assets,
                    const NbaGameplayHudInput *input) {
    NbaRomFont font;if(!font_for(assets,true,&font))return false;
    font.fixed_digit_width=12u;
    uint8_t canvas[40u*40u];unpack_region(hud,2u,canvas);
    for(unsigned row=0;row<2u;++row) {
        char digits[6];uint16_t width;
        (void)snprintf(digits,sizeof(digits),"%u",input->scores[row^1u]);
        if(!nba_rom_font_measure(&font,(const uint8_t *)digits,sizeof(digits),&width) ||
           !nba_rom_font_draw(&font,canvas,40u,40,40,40-(int)width,(int)(row*20u),
                               (const uint8_t *)digits,sizeof(digits)))return false;
    }
    pack_region(hud,2u,canvas);return true;
}

static bool clock_update(NbaGameplayHud *hud,const NbaAssetPack *assets,
                          NbaGameplayHudInput *input);

static bool period(NbaGameplayHud *hud,const NbaAssetPack *assets,
                    NbaGameplayHudInput *input) {
    const uint8_t *text=resource(assets,4u);
    unsigned index=input->period_raw_0926;
    if(index && input->phase_raw_08e4==1u)index+=4u;
    NbaRomFont font;
    if(!text || index>=9u || !font_for(assets,false,&font))return false;
    uint8_t canvas[120u*16u];unpack_region(hud,3u,canvas);
    if(!nba_rom_font_draw(&font,canvas,120u,120,16,0,0,text+index*32u,32u))return false;
    pack_region(hud,3u,canvas);hud->clock_mirror_raw_08f6=0xFFFFu;
    /* D24B calls BBE9 before returning; its clock publication and possible
     * 09B4/13E7 writes are part of this native child, not a later cosmetic job. */
    return clock_update(hud,assets,input);
}

static void clock_minutes(NbaGameplayHud *hud,const NbaGameplayHudInput *input) {
    /* BAF5 formats the snapshot092A, not necessarily current0928. */
    unsigned value=input->clock_snapshot_raw_092a;
    if(value==0xFFFFu)value=0u;
    const uint8_t text[8]={'[',(uint8_t)('0'+value/36000u),
        (uint8_t)('0'+value%36000u/3600u),':',
        (uint8_t)('0'+value%3600u/600u),(uint8_t)('0'+value%600u/60u),']',0};
    memcpy(hud->clock_text_raw_4a60,text,sizeof(text));
}

static void clock_tenths(NbaGameplayHud *hud,NbaGameplayHudInput *input) {
    /* $87:BB59-BBE8 preserves the original nonzero final tenth, and signals
     * expiry through the same canonical dead-ball/event fields as gameplay. */
    unsigned original=input->clock_snapshot_raw_092a;
    unsigned value=original==0xFFFFu?0u:original;
    unsigned tenth=(value%60u)/6u;
    if(!tenth) {
        if(original==0u || original>=0xF000u) {
            if(input->dead_ball_busy_raw_09b4==0u) {
                input->dead_ball_busy_raw_09b4=1u;
                input->event_bits_raw_13e7|=0x0800u;
            }
        } else if(original<6u) tenth=1u;
    }
    const uint8_t text[7]={'[',(uint8_t)('0'+value/600u),
        (uint8_t)('0'+value%600u/60u),'.',(uint8_t)('0'+tenth),']',0};
    memcpy(hud->clock_text_raw_4a60,text,sizeof(text));
}

static bool clock_render(NbaGameplayHud *hud,const NbaAssetPack *assets) {
    const uint8_t *descriptor=resource(assets,5u);
    if(!descriptor)return false;
    NbaRomFont font={descriptor,838u,NULL,0u,0u,0u,0u};
    uint8_t canvas[32u*16u]={0}; /* A2D3 clears the clock rectangle first. */
    if(!nba_rom_font_draw(&font,canvas,32u,32,16,0,0,
                         hud->clock_text_raw_4a60,8u))return false;
    pack_region(hud,0u,canvas);return true;
}

static bool clock_update(NbaGameplayHud *hud,const NbaAssetPack *assets,
                          NbaGameplayHudInput *input) {
    /* $87:BBE9-BD2E. Unsigned subtraction/comparison at BC88/BC9D is
     * intentional; replacing the mirror with the current clock changes the
     * original cadence when multiple clock ticks elapse between dispatches. */
    if(input->clock_gate_raw_492b==0xFFFFu)return true;
    if((int16_t)input->presentation_timer_raw_08de>=0) {
        if(input->presentation_kind_raw_08e8!=1u)return true;
    } else if(hud->clear_raw_08ee!=0u) {
        memset(hud->working_map,0,sizeof(hud->working_map));
        const HudRegion *r=regions;
        for(unsigned y=0;y<r->height;++y)
            for(unsigned x=0;x<r->width;++x)
                put_word(hud->working_map+((r->y+y)*32u+r->x+x)*2u,
                    (uint16_t)(0x2000u+r->tile+x*r->height+y));
        memset(hud->working_characters,0,0x80u);
        /* BC66 passes byte count$0080 to8BA1: the last128 map bytes.
         * The helper's X count is bytes, unlike8AD2's word-clear count. */
        memcpy(hud->visible_map+0x600u,hud->working_map+0x580u,0x80u);
        hud->clear_raw_08ee=0u;
    }
    bool minutes;
    if(hud->clock_mirror_raw_08f6==0xFFFFu) {
        hud->clock_mirror_raw_08f6=input->clock_raw_0928;
        minutes=input->clock_raw_0928>=3600u;
    } else {
        minutes=input->clock_raw_0928>=3601u;
        uint16_t next=(uint16_t)(hud->clock_mirror_raw_08f6-
                                (minutes?60u:6u));
        if(next<input->clock_raw_0928)return true;
        hud->clock_mirror_raw_08f6=next;
    }
    if(minutes)clock_minutes(hud,input);else clock_tenths(hud,input);
    return clock_render(hud,assets);
}

static bool tilemap(uint8_t *map,const uint8_t *source,unsigned x,unsigned y) {
    if(!source || word(source+4)!=0x304u)return false;
    unsigned width=word(source),height=word(source+2);
    if(x+width>32u || y+height>28u)return false;
    for(unsigned row=0;row<height;++row)
        memcpy(map+((y+row)*32u+x)*2u,source+6u+row*width*2u,width*2u);
    return true;
}

bool nba_gameplay_hud_lifecycle_assets_valid(const NbaAssetPack *assets) {
    /* Legacy v1 remains valid for its frozen leaf tests, but cannot support
     * the production BA5E child. Check the complete v2 payload up front. */
    return resource(assets,9u)!=NULL;
}

static bool clear_panel(NbaGameplayHud *hud,const NbaGameplayHudInput *input) {
    /* $83:EBDB-ED46: this is the map/palette projection. Substitution and
     * shared task1850 bookkeeping remain their existing explicit owners. */
    if ((int16_t)input->dispatch_mode_raw_0960<0 &&
        input->presentation_kind_raw_08e8==1u) {
        if(input->clock_raw_0928<3600u) {
            memset(hud->visible_map+0x480u,0,0x1B4u); /* ED04:0640/00DA words */
            memset(hud->visible_map+0x640u,0,0x34u);  /* ED15:0720/001A words */
            hud->clear_raw_08ee=0u;
        } else {
            memset(hud->visible_map+0x480u,0,0x200u); /* ED47/ED5D kind1 */
            hud->clear_raw_08ee=0xFFFFu;
        }
        return true;
    }
    if((int16_t)input->dispatch_mode_raw_0960<0) {
        static const uint16_t kinds[7]={6,10,13,27,31,35,39};
        static const uint16_t starts[7]={0x660,0x6C0,0x620,0x660,0x6A0,0x660,0x6A0};
        static const uint16_t counts[7]={0xE0,0x80,0x120,0xE0,0xA0,0xE0,0xA0};
        /* Literal $83:ED47/ED5D entries1..7. These clear the actual
         * selected overlay even when its drawing child is still pending. */
        for(unsigned i=0;i<7u;++i)if(input->presentation_kind_raw_08e8==kinds[i]) {
            memset(hud->visible_map+(starts[i]-0x400u)*2u,0,counts[i]*2u);
            hud->clear_raw_08ee=0xFFFFu;return true;
        }
        switch(input->presentation_kind_raw_08e8) {
            case 17:
                /* EC60/EC8B/ECA4 -> EC46 without foul-out/injury pages.
                 * ED47/ED5D entry8 clears $04E0 for $0260 words. */
                if(input->latched_event_raw_08f0==3u &&
                   input->contact_context_raw_497f==0u &&
                   input->foul_out_state_raw_09ca==0u && input->injury_state_raw_09cc==0u) {
                    memset(hud->visible_map+0x1C0u,0,0x4C0u);
                    hud->clear_raw_08ee=0xFFFFu;return true;
                }
                /* fall through: explicit untranslated continuation */
            case 22:
                hud->pending_routine=0x83EC60u;return false;
            default:break;
        }
    }
    memset(hud->visible_map+0x80u,0,0x600u); /* ECEF:0440/0300 words */
    hud->clear_raw_08ee=0xFFFFu;
    return true;
}

static bool shot_clock(NbaGameplayHud *hud,const NbaAssetPack *assets,
                       const NbaGameplayHudInput *input) {
    uint16_t value=input->shot_clock_raw_092c;
    unsigned frame=0;
    if(value!=0u && (int16_t)value>=0) {
        if(value>=input->clock_raw_0928)return true;
        /* Original endpoint quirk at $87:BA6D/BA72: increments BEFORE
         * subtraction and repeats for zero. In particular 60 displays2. */
        do { ++frame;value=(uint16_t)(value-60u); } while((int16_t)value>=0);
    }
    if(frame==hud->clock_frame_raw_08f4)return true;
    const uint8_t *maps=resource(assets,9u);
    if(!maps || frame>=11u)return false; /* caller's <600 domain */
    hud->clock_frame_raw_08f4=(uint16_t)frame;
    return tilemap(hud->visible_map,maps+frame*38u,2u,20u);
}

void nba_gameplay_hud_timer_tick(int16_t *timer) {
    /* $85:EDAC-EDB6 is separate from CC1A. Zero is retained here so that
     * CC1A performs the signed transition and calls EBDB exactly once. */
    if(timer && *timer>0)--*timer;
}

bool nba_gameplay_hud_dispatch(NbaGameplayHud *hud,const NbaAssetPack *assets,
                               NbaGameplayHudInput *input) {
    static const uint32_t children[44]={ /* original long table $83:CC7B */
        0x83EBDB,0x83D0AD,0x83D157,0x83D1B1,0x83D1FD,0x83D2E0,
        0x83D333,0x83D3AD,0x83D407,0x83D8DA,0x83D910,0x83D973,
        0x83D9F7,0x83E81D,0x83D3AD,0x83E897,0x83E99E,0x83DA12,
        0x83DA8C,0x83DD7F,0x83DDEF,0x83DF07,0x83DF3D,0x83DA8C,
        0x83DFCE,0x83E060,0x83E0BE,0x83E0F4,0x83E157,0x83E1C7,
        0x83E38D,0x83E3C3,0x83E426,0x83E496,0x83E563,0x83E599,
        0x83E5FC,0x83E66C,0x83E7E7,0x83E9D4,0x83EA37,0x83EAA7,
        0x83EBA2,0x83DBA2};
    if(!hud || !hud->initialized || !input)return false;
    if((int16_t)input->dispatch_mode_raw_0960>=0)return true;
    if((int16_t)input->presentation_timer_raw_08de>=0) {
        --input->presentation_timer_raw_08de;
        if((int16_t)input->presentation_timer_raw_08de<0) {
            bool ok=clear_panel(hud,input);
            input->presentation_sequence_raw_08e6=0xFFFFu;
            if(!ok)return false;
            hud->pending_routine=0u;hud->unsupported_child_pending=false;
        } else if((int16_t)input->presentation_sequence_raw_08e6>=0) {
            /* An untranslated child has no invented return/poststate.
             * The host keeps playing and ticking the canonical timer, but
             * cannot step to another overlay child until retirement. */
            if(hud->unsupported_child_pending)return false;
            unsigned seq=input->presentation_sequence_raw_08e6;
            if(seq>=44u) { hud->pending_routine=0x83CC43u;return false; }
            ++input->presentation_sequence_raw_08e6;
            bool oob=input->presentation_kind_raw_08e8==17u &&
                     input->latched_event_raw_08f0==3u && (seq==17u || seq==18u);
            if(seq>=6u && !oob) {
                hud->pending_routine=children[seq];hud->unsupported_child_pending=true;
                return false;
            }
            return nba_gameplay_hud_publish(hud,assets,children[seq],input);
        } else if(input->presentation_kind_raw_08e8!=1u)return true;
    }
    if((int16_t)input->presentation_timer_raw_08de>=0 || input->clock_raw_0928<3600u)
        if(!clock_update(hud,assets,input))return false;
    /* CC6F/CC72 uses subtraction's N flag, not unsigned magnitude. */
    if((int16_t)input->presentation_timer_raw_08de<0 &&
       (int16_t)(uint16_t)(input->shot_clock_raw_092c-600u)<0)
        return shot_clock(hud,assets,input);
    return true;
}

bool nba_gameplay_hud_request_score(NbaGameplayHud *hud,const NbaAssetPack *assets,
                                    NbaGameplayHudInput *input) {
    if(!hud || !hud->initialized || !input)return false;
    if((int16_t)input->requester_raw_095e>=0) {
        /* CE36-CE73's synchronous human/pause request. The host pause
         * caller is a separate integration boundary; this leaf is usable. */
        if((int16_t)input->presentation_timer_raw_08de>=0) {
            input->presentation_gate_raw_08e2=0u; /* EBD8 before EBDB */
            if(!clear_panel(hud,input))return false;
        }
        input->presentation_timer_raw_08de=300u;
        hud->assist_raw_493d=0xFFFFu;hud->clock_mirror_raw_08f6=0xFFFFu;
        input->presentation_sequence_raw_08e6=input->presentation_kind_raw_08e8=1u;
        ++input->presentation_gate_raw_08e2; /* $83:CE5C, wrapping word */
        for(unsigned i=0;i<5u;++i) {
            static const uint32_t pc[5]={0x83D0ADu,0x83D157u,0x83D1B1u,0x83D1FDu,0x83D2E0u};
            if(!nba_gameplay_hud_publish(hud,assets,pc[i],input))return false;
        }
        hud->pending_routine=0u;hud->unsupported_child_pending=false;return true;
    }
    /* CE84-CEB6 preserves Arcade's skipped advertisement selector1.
     * ROM task[EE]+28 upload is explicitly pending, never drawn from guesses. */
    do { ++hud->advertisement_counter_raw_4941; }
    while((hud->advertisement_counter_raw_4941&3u)==1u && input->style_raw_17ab!=2u);
    hud->advertisement_upload_pending=true;
    if((int16_t)input->presentation_timer_raw_08de>=0)return true;
    if(!nba_gameplay_hud_publish(hud,assets,0x87BACBu,input))return false;
    input->presentation_timer_raw_08de=300u;
    uint16_t selected=1u;
    bool statistics=false;
    if(input->clock_raw_0928>=1800u) {
        if((int16_t)(uint16_t)(input->period_raw_0926-3u)>=0 &&
           input->clock_raw_0928<7200u && hud->late_statistics_raw_4931==0u) {
            hud->late_statistics_raw_4931=13u;selected=13u;statistics=true;
        }
        else if(hud->phase_raw_08e4!=0u) {
            if(hud->phase_raw_08e4==1u && input->period_raw_0926==2u) {
                selected=10u;statistics=true;
            }
            else if(input->presentation_kind_raw_08e8==1u) {
                /* $83:CF21/CF7A/CFB3 call the shared CEFD rejection loop.
                 * Keep all rejected draws and the AA store, even when the
                 * selected statistics drawing child remains untranslated. */
                NbaGameplayRng rng={input->rng_raw_07f6};
                uint16_t random;
                do {
                    hud->scratch_raw_00aa=(uint16_t)(nba_gameplay_rng_next(&rng)&0x7FFFu);
                    random=hud->scratch_raw_00aa&0x7Fu;
                } while(random>=100u);
                input->rng_raw_07f6=rng.state;
                if(random>=70u) {
                    uint16_t category=hud->shot_category_raw_4939,index=0u;
                    if((int16_t)(uint16_t)(category-2u)<0) {
                        index=random<80u?6u:random<90u?7u:8u;
                        selected=6u;statistics=true;
                    } else if(random<75u) {
                        if((int16_t)hud->assist_raw_493d>=0) {
                            index=9u;selected=6u;statistics=true;
                        }
                    } else if(random<90u) {
                        index=(uint16_t)((category==2u?0u:3u)+(random-75u)/5u);
                        selected=6u;statistics=true;
                    } else { selected=39u;statistics=true; }
                    if(statistics && selected==6u)hud->statistics_index_raw_08ec=index;
                }
            }
        }
    }
    input->presentation_sequence_raw_08e6=input->presentation_kind_raw_08e8=selected;
    if(statistics)hud->statistics_kind_raw_08ea=selected; /* CF6C, not CFD5 */
    ++hud->phase_raw_08e4;input->phase_raw_08e4=hud->phase_raw_08e4;
    hud->assist_raw_493d=0xFFFFu;hud->pending_routine=0u;hud->unsupported_child_pending=false;
    return true;
}

bool nba_gameplay_hud_publish(NbaGameplayHud *hud,const NbaAssetPack *assets,
                              uint32_t native_routine,NbaGameplayHudInput *input) {
    if(!hud || !hud->initialized || !input)return false;
    bool ok=false;
    switch(native_routine) {
        case 0x87B99Au:
            /* Pause/timeout return $86:84DB/$858E, unlike new-game init:
             * preserve working canvas/text, generatedCHR, phase/counters,
             * category and assist. Only these original owners are reset. */
            hud->clear_raw_08ee=hud->clock_mirror_raw_08f6=0xFFFFu;
            hud->clock_frame_raw_08f4=0xFFFFu;hud->canvas_state_raw_7a70=0u;
            input->presentation_timer_raw_08de=0xFFFFu;
            input->presentation_sequence_raw_08e6=0xFFFFu;
            memset(hud->visible_map,0,sizeof(hud->visible_map));
            hud->pending_routine=0u;hud->unsupported_child_pending=false;
            ok=true;break;
        case 0x87BA54u:
            input->presentation_sequence_raw_08e6=0xFFFFu;
            hud->clear_raw_08ee=0xFFFFu;ok=true;break;
        case 0x83EBDBu:ok=clear_panel(hud,input);break;
        case 0x87BA5Eu:ok=shot_clock(hud,assets,input);break;
        case 0x83D0ADu:layout(hud);ok=true;break;
        case 0x83DA12u:violation_layout(hud);ok=true;break;
        case 0x83DA8Cu:ok=out_of_bounds(hud,assets,input);break;
        case 0x83D157u:ok=names(hud,assets,input);break;
        case 0x83D1B1u:ok=scores(hud,assets,input);break;
        case 0x83D1FDu:ok=period(hud,assets,input);break;
        case 0x87BAF5u:clock_minutes(hud,input);ok=true;break;
        case 0x87BB59u:clock_tenths(hud,input);ok=true;break;
        case 0x87BBE9u:ok=clock_update(hud,assets,input);break;
        case 0x87BACBu:
            if((int16_t)input->presentation_timer_raw_08de>=0)ok=true;
            else {
                /* $87:BAD3-$BAD6 also retires the pending frame state. */
                hud->clock_frame_raw_08f4=0xFFFFu;
                ok=tilemap(hud->visible_map,resource(assets,8u),2u,20u);
            }
            break;
        case 0x83D2E0u:
            /* `$D2E0`: source4E70 (map row16) -> VRAM0640 (row18). */
            memcpy(hud->visible_map+0x480u,hud->working_map+0x400u,0x200u);
            ok=tilemap(hud->visible_map,resource(assets,6u),4u,18u) &&
               tilemap(hud->visible_map,resource(assets,7u),11u,23u);
            /* $83:D32C-$D32F: parent dispatcher must consume this completion. */
            if(ok)input->presentation_sequence_raw_08e6=0xFFFFu;
            break;
        default:return false; /* Unsupported overlays are never substituted. */
    }
    if(ok)++hud->publication_count;
    return ok;
}

bool nba_gameplay_hud_apply(const NbaGameplayHud *hud,const NbaAssetPack *assets,
                            uint8_t *vram,uint8_t *cgram) {
    if(!hud || !hud->initialized || !vram || !cgram)return false;
    const uint8_t *characters=resource(assets,0u),*palette=resource(assets,1u);
    if(!characters || !palette)return false;
    memcpy(vram+0x800u,hud->visible_map,sizeof(hud->visible_map));
    memcpy(vram+0x2000u,characters,1008u);
    for(unsigned region=0;region<4u;++region)
        if(hud->published_mask&(1u<<region)) {
            const HudRegion *r=regions+region;
            memcpy(vram+0x23F0u+r->character,hud->published_characters+r->character,
                   r->width*r->height*16u);
        }
    if(hud->violation_characters_published)
        memcpy(vram+0x2470u,hud->published_characters+0x80u,0xA80u);
    memcpy(cgram+2u,palette,30u);
    return true;
}
