/* Before-only original WRAM/VRAM adapter. No native after-state is read. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_gameplay_hud.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t w(const uint8_t *p,unsigned a) { return (uint16_t)(p[a]|p[a+1u]<<8); }
static void sw(uint8_t *p,unsigned a,uint16_t v) {p[a]=(uint8_t)v;p[a+1u]=(uint8_t)(v>>8);}
int main(int argc,char **argv) {
    if(argc!=4)return 2;
    NbaAssetPack assets={0};
    if(!nba_assets_load(&assets,argv[1]))return 3;
    FILE *f=fopen(argv[2],"rb"),*o=fopen(argv[3],"wb");
    uint32_t count;
    if(!f || !o || fread(&count,4,1,f)!=1 || !count || count>20000u)return 4;
    if(fwrite(&count,4,1,o)!=1)return 5;
    for(uint32_t index=0;index<count;++index) {
        uint8_t raw[0x20000],vram[0x10000],cg[0x200];uint32_t pc;
        if(fread(&pc,4,1,f)!=1 || fread(raw,1,sizeof(raw),f)!=sizeof(raw) ||
           fread(vram,1,sizeof(vram),f)!=sizeof(vram) || fread(cg,1,sizeof(cg),f)!=sizeof(cg))return 6;
        NbaGameplayHud h;NbaGameplayHudInput in={0};
        if(!nba_gameplay_hud_init(&h,&assets))return 7;
        memcpy(h.working_map,raw+0x4A70,sizeof(h.working_map));
        memcpy(h.working_characters,raw+0x5070,sizeof(h.working_characters));
        memcpy(h.clock_text_raw_4a60,raw+0x4A60,8);
        memcpy(h.visible_map,vram+0x800,sizeof(h.visible_map));
        memcpy(h.published_characters,vram+0x23F0,sizeof(h.published_characters));h.published_mask=15;
        h.clock_mirror_raw_08f6=w(raw,0x8F6);h.clear_raw_08ee=w(raw,0x8EE);
        h.clock_frame_raw_08f4=w(raw,0x8F4);h.phase_raw_08e4=w(raw,0x8E4);
        h.advertisement_counter_raw_4941=w(raw,0x4941);h.late_statistics_raw_4931=w(raw,0x4931);
        h.assist_raw_493d=w(raw,0x493D);h.shot_category_raw_4939=w(raw,0x4939);
        h.statistics_kind_raw_08ea=w(raw,0x8EA);h.statistics_index_raw_08ec=w(raw,0x8EC);
        h.scratch_raw_00aa=w(raw,0xAA);h.canvas_state_raw_7a70=w(raw,0x7A70);
        in.teams[0]=w(raw,0x46EB);in.teams[1]=w(raw,0x476B);
        in.scores[0]=w(raw,0x4711);in.scores[1]=w(raw,0x4791);
        in.period_raw_0926=w(raw,0x926);in.phase_raw_08e4=h.phase_raw_08e4;
        in.clock_raw_0928=w(raw,0x928);in.clock_snapshot_raw_092a=w(raw,0x92A);
        in.clock_gate_raw_492b=w(raw,0x492B);in.presentation_timer_raw_08de=w(raw,0x8DE);
        in.presentation_kind_raw_08e8=w(raw,0x8E8);in.presentation_sequence_raw_08e6=w(raw,0x8E6);
        in.dead_ball_busy_raw_09b4=w(raw,0x9B4);in.event_bits_raw_13e7=w(raw,0x13E7);
        in.dispatch_mode_raw_0960=w(raw,0x960);in.requester_raw_095e=w(raw,0x95E);
        in.shot_clock_raw_092c=w(raw,0x92C);in.style_raw_17ab=w(raw,0x17AB);
        in.presentation_gate_raw_08e2=w(raw,0x8E2);in.rng_raw_07f6=w(raw,0x7F6);
        bool ok;
        if(pc==0x83CC10u)ok=nba_gameplay_hud_dispatch(&h,&assets,&in);
        else if(pc==0x83CE36u)ok=nba_gameplay_hud_request_score(&h,&assets,&in);
        else if(pc==0x85EDACu) {int16_t timer=(int16_t)in.presentation_timer_raw_08de;
            nba_gameplay_hud_timer_tick(&timer);in.presentation_timer_raw_08de=(uint16_t)timer;ok=true;}
        else ok=nba_gameplay_hud_publish(&h,&assets,pc,&in);
        memcpy(raw+0x4A70,h.working_map,sizeof(h.working_map));
        memcpy(raw+0x5070,h.working_characters,sizeof(h.working_characters));
        memcpy(raw+0x4A60,h.clock_text_raw_4a60,8);
        sw(raw,0x8F6,h.clock_mirror_raw_08f6);sw(raw,0x8EE,h.clear_raw_08ee);
        sw(raw,0x8F4,h.clock_frame_raw_08f4);sw(raw,0x8E4,h.phase_raw_08e4);
        sw(raw,0x4941,h.advertisement_counter_raw_4941);sw(raw,0x4931,h.late_statistics_raw_4931);
        sw(raw,0x493D,h.assist_raw_493d);sw(raw,0x4939,h.shot_category_raw_4939);
        sw(raw,0x8EA,h.statistics_kind_raw_08ea);sw(raw,0x8EC,h.statistics_index_raw_08ec);
        sw(raw,0xAA,h.scratch_raw_00aa);sw(raw,0x7A70,h.canvas_state_raw_7a70);
        sw(raw,0x8DE,in.presentation_timer_raw_08de);sw(raw,0x8E6,in.presentation_sequence_raw_08e6);
        sw(raw,0x8E8,in.presentation_kind_raw_08e8);sw(raw,0x8E2,in.presentation_gate_raw_08e2);
        sw(raw,0x9B4,in.dead_ball_busy_raw_09b4);sw(raw,0x13E7,in.event_bits_raw_13e7);
        sw(raw,0x7F6,in.rng_raw_07f6);
        if(!nba_gameplay_hud_apply(&h,&assets,vram,cg))return 8;
        uint32_t complete=ok?1u:0u;
        if(fwrite(&complete,4,1,o)!=1 || fwrite(&h.pending_routine,4,1,o)!=1 ||
           fwrite(raw,1,sizeof(raw),o)!=sizeof(raw) || fwrite(vram,1,sizeof(vram),o)!=sizeof(vram) ||
           fwrite(cg,1,sizeof(cg),o)!=sizeof(cg))return 9;
    }
    if(fgetc(f)!=EOF || ferror(f) || fclose(f) || fclose(o))return 10;
    printf("HUD_NATIVE %u\n",count);nba_assets_free(&assets);return 0;
}
