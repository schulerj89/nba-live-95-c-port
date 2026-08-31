/* Controlled source contracts, not additional natural-game coverage. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_tipoff.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REQUIRE(x,n) do{if(!(x)){fprintf(stderr,"HUD_CONTRACT_FAILURE %u\n",(unsigned)(n));return (n);}}while(0)
static uint16_t w(const uint8_t *p,unsigned a){return (uint16_t)(p[a]|p[a+1u]<<8);}
static uint32_t dw(const uint8_t *p,unsigned a){return (uint32_t)w(p,a)|((uint32_t)w(p,a+2u)<<16);}
static void sd(uint8_t *p,unsigned a,uint32_t v){for(unsigned i=0;i<4u;++i)p[a+i]=(uint8_t)(v>>(i*8u));}
int main(int argc,char **argv) {
    REQUIRE(argc==2,2);NbaAssetPack assets={0};REQUIRE(nba_assets_load(&assets,argv[1]),3);
    NbaGameplayHud h,base;REQUIRE(nba_gameplay_hud_init(&base,&assets),4);
    NbaAssetItem *entry=assets.items+(nba_assets_get(&assets,NBA_ASSET_GAMEPLAY_HUD)-assets.items);
    const uint8_t *resource=(const uint8_t *)entry->data;
    const uint8_t *maps=resource+dw(resource,88u);unsigned timer_cases=0,clock_cases=0,malformed=0;
    for(unsigned value=0;value<65536u;++value) {
        int16_t timer=(int16_t)value;nba_gameplay_hud_timer_tick(&timer);
        REQUIRE((uint16_t)timer==(value>0u && value<32768u?value-1u:value),5);++timer_cases;
    }
    static const uint16_t clocks[10]={0,1,60,599,600,601,3599,3600,43200,65535};
    for(unsigned shot=0;shot<65536u;++shot)for(unsigned c=0;c<10u;++c) {
        h=base;memset(h.visible_map,0x5A,sizeof(h.visible_map));
        NbaGameplayHudInput in={0};in.dispatch_mode_raw_0960=65535u;
        in.presentation_timer_raw_08de=in.presentation_sequence_raw_08e6=65535u;
        in.clock_gate_raw_492b=65535u;in.clock_raw_0928=clocks[c];in.shot_clock_raw_092c=(uint16_t)shot;
        REQUIRE(nba_gameplay_hud_dispatch(&h,&assets,&in),6);
        bool called=shot<600u || shot>=33368u; /* CC6F wrapped subtraction N */
        bool early=shot>0u && shot<32768u && shot>=clocks[c];
        unsigned expected=!called || early?65535u:shot==0u || shot>=32768u?0u:shot/60u+1u;
        REQUIRE(h.clock_frame_raw_08f4==expected,7);
        for(unsigned y=0;y<28u;++y)for(unsigned x=0;x<32u;++x) {
            uint16_t wanted=0x5A5Au;
            if(expected!=65535u && x>=2u && x<6u && y>=20u && y<24u)
                wanted=w(maps+expected*38u,6u+((y-20u)*4u+x-2u)*2u);
            REQUIRE(w(h.visible_map,(y*32u+x)*2u)==wanted,8);
        }
        ++clock_cases;
    }
    for(unsigned c=0;c<3u;++c) {
        h=base;memset(h.visible_map,0x5A,sizeof(h.visible_map));NbaGameplayHudInput in={0};
        in.dispatch_mode_raw_0960=65535u;in.presentation_kind_raw_08e8=1u;
        in.presentation_timer_raw_08de=0u;in.presentation_sequence_raw_08e6=65535u;
        in.clock_gate_raw_492b=65535u;in.clock_raw_0928=c==0u?3599u:c==1u?3600u:43200u;
        in.shot_clock_raw_092c=1440u;REQUIRE(nba_gameplay_hud_dispatch(&h,&assets,&in),9);
        REQUIRE(in.presentation_timer_raw_08de==65535u && in.presentation_sequence_raw_08e6==65535u,10);
        for(unsigned i=0;i<sizeof(h.visible_map);++i) {
            bool cleared=c==0u?((i>=0x480u && i<0x634u)||(i>=0x640u && i<0x674u)):
                                   (i>=0x480u && i<0x680u);
            REQUIRE(h.visible_map[i]==(cleared?0u:0x5Au),11);
        }
    }
    h=base;memset(h.working_map,0xA3,sizeof(h.working_map));memset(h.working_characters,0xC5,sizeof(h.working_characters));
    memset(h.published_characters,0xE7,sizeof(h.published_characters));memset(h.clock_text_raw_4a60,0x19,8);
    h.published_mask=15;h.phase_raw_08e4=7;h.advertisement_counter_raw_4941=13;h.shot_category_raw_4939=3;h.assist_raw_493d=4;
    NbaGameplayHud prior=h;NbaGameplayHudInput in={0};in.presentation_timer_raw_08de=300;in.presentation_sequence_raw_08e6=5;
    REQUIRE(nba_gameplay_hud_publish(&h,&assets,0x87B99Au,&in),12);
    REQUIRE(!memcmp(h.working_map,prior.working_map,sizeof(h.working_map)) &&
            !memcmp(h.working_characters,prior.working_characters,sizeof(h.working_characters)) &&
            !memcmp(h.published_characters,prior.published_characters,sizeof(h.published_characters)) &&
            !memcmp(h.clock_text_raw_4a60,prior.clock_text_raw_4a60,8) && h.published_mask==15 &&
            h.phase_raw_08e4==7 && h.advertisement_counter_raw_4941==13 && h.shot_category_raw_4939==3 && h.assist_raw_493d==4 &&
            in.presentation_timer_raw_08de==65535u && in.presentation_sequence_raw_08e6==65535u,13);
    NbaAssetItem saved=*entry;uint8_t modified[3926];
    for(unsigned section=6u;section<10u;++section)for(unsigned map=0;map<(section==9u?11u:1u);++map)
        for(unsigned field=0;field<3u;++field) {
            memcpy(modified,resource,sizeof(modified));unsigned off=dw(resource,16u+section*8u)+map*38u+field*2u;
            modified[off]^=0x20u;entry->data=modified;
            memset(&h,0xA5,sizeof(h));prior=h;
            REQUIRE(!nba_gameplay_hud_init(&h,&assets) && !memcmp(&h,&prior,sizeof(h)) &&
                    !nba_gameplay_hud_lifecycle_assets_valid(&assets),14);++malformed;
        }
    uint8_t legacy[3500];memcpy(legacy,resource,16);sd(legacy,8,1);sd(legacy,12,9);
    for(unsigned i=0;i<9u;++i){sd(legacy,16+i*8,dw(resource,16+i*8)-8u);sd(legacy,20+i*8,dw(resource,20+i*8));}
    memcpy(legacy+88,resource+96,3412);entry->data=legacy;entry->size=3500;
    REQUIRE(nba_gameplay_hud_init(&h,&assets) && !nba_gameplay_hud_lifecycle_assets_valid(&assets),15);
    NbaTipoff *tip=(NbaTipoff *)malloc(sizeof(*tip)),*old=(NbaTipoff *)malloc(sizeof(*old));NbaSession session;
    REQUIRE(tip && old,16);memset(tip,0xA5,sizeof(*tip));memcpy(old,tip,sizeof(*old));nba_session_init(&session);
    REQUIRE(!nba_tipoff_init(tip,&assets,&session) && !memcmp(tip,old,sizeof(*tip)),17);
    *entry=saved;
    printf("HUD_CONTRACTS timer=%u clock=%u malformed=%u legacy_production_refused=1 reset_preservation=1 clear_boundaries=3\n",timer_cases,clock_cases,malformed);
    free(tip);free(old);nba_assets_free(&assets);return 0;
}
