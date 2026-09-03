/* Exercise the production dispatcher, both possession branches and retirement.
 * The Python verifier owns the native expectations; this exports actual bytes. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_gameplay_hud.h"
#include <stdio.h>
#include <string.h>

static bool save(const char *dir,const char *name,const void *data,size_t size) {
    char path[1024];
    if(snprintf(path,sizeof(path),"%s/%s",dir,name)<0)return false;
    FILE *f=fopen(path,"wb");if(!f)return false;
    bool ok=fwrite(data,1,size,f)==size;
    return fclose(f)==0 && ok;
}

int main(int argc,char **argv) {
    if(argc!=3)return 2;
    NbaAssetPack assets={0};
    if(!nba_assets_load(&assets,argv[1]) || !nba_gameplay_hud_oob_assets_valid(&assets))return 3;
    for(unsigned team=0;team<29u;++team)for(unsigned side=0;side<2u;++side) {
        NbaGameplayHud h;NbaGameplayHudInput in={0};
        if(!nba_gameplay_hud_init(&h,&assets))return 4;
        memset(h.working_map,0xA5,sizeof(h.working_map));
        memset(h.working_characters,0xA5,sizeof(h.working_characters));
        in.teams[0]=(uint16_t)(side?9u:team);in.teams[1]=(uint16_t)(side?team:17u);
        in.latched_event_raw_08f0=3u;in.event_actor_raw_492d=(uint16_t)(side?0u:9u);
        in.presentation_sequence_raw_08e6=in.presentation_kind_raw_08e8=17u;
        in.presentation_timer_raw_08de=300u;in.dispatch_mode_raw_0960=0xFFFFu;
        in.clock_raw_0928=in.clock_snapshot_raw_092a=43200u;
        in.shot_clock_raw_092c=1440u;
        if(!nba_gameplay_hud_dispatch(&h,&assets,&in) || in.presentation_sequence_raw_08e6!=18u)return 5;
        if(team==0u && side==0u &&
           (!save(argv[2],"layout.map",h.working_map,sizeof(h.working_map)) ||
            !save(argv[2],"layout.chr",h.working_characters,sizeof(h.working_characters))))return 6;
        if(!nba_gameplay_hud_dispatch(&h,&assets,&in) || in.presentation_sequence_raw_08e6!=0xFFFFu ||
           in.presentation_kind_raw_08e8!=17u || h.pending_routine)return 7;
        char name[80];uint8_t vram[65536]={0},cgram[512]={0};
        if(!nba_gameplay_hud_apply(&h,&assets,vram,cgram))return 8;
        (void)snprintf(name,sizeof(name),"team_%02u_side_%u.vram",team,side);
        if(!save(argv[2],name,vram,sizeof(vram)))return 9;
        if(team==9u && side==0u &&
           (!save(argv[2],"native-text.chr",h.working_characters,sizeof(h.working_characters)) ||
            !save(argv[2],"palette.cgram",cgram,sizeof(cgram))))return 10;
        in.presentation_timer_raw_08de=0u;
        if(!nba_gameplay_hud_dispatch(&h,&assets,&in) || in.presentation_timer_raw_08de!=0xFFFFu ||
           in.presentation_sequence_raw_08e6!=0xFFFFu || h.clear_raw_08ee!=0xFFFFu)return 11;
        for(unsigned i=0;i<sizeof(h.visible_map);++i)if(h.visible_map[i])return 12;
        uint8_t retired[65536]={0};
        if(!nba_gameplay_hud_apply(&h,&assets,retired,cgram) ||
           memcmp(retired+0x2470,vram+0x2470,0xA80))return 13;
        /* Following score uploads must replace overlapping violation CHR. */
        NbaGameplayHud score;
        if(!nba_gameplay_hud_init(&score,&assets))return 14;
        in.presentation_kind_raw_08e8=1u;in.presentation_timer_raw_08de=300u;
        static const uint32_t children[]={0x83D0AD,0x83D157,0x83D1B1,0x83D1FD,0x83D2E0};
        for(unsigned i=0;i<5u;++i)
            if(!nba_gameplay_hud_publish(&h,&assets,children[i],&in) ||
               !nba_gameplay_hud_publish(&score,&assets,children[i],&in))return 15;
        memset(vram,0,sizeof(vram));memset(retired,0,sizeof(retired));
        if(!nba_gameplay_hud_apply(&h,&assets,vram,cgram) ||
           !nba_gameplay_hud_apply(&score,&assets,retired,cgram) ||
           memcmp(vram+0x800,retired+0x800,0x700) ||
           memcmp(vram+0x23F0,retired+0x23F0,0x850))return 16;
    }
    for(unsigned gate=0;gate<4u;++gate) {
        NbaGameplayHud h;NbaGameplayHudInput in={0};
        if(!nba_gameplay_hud_init(&h,&assets))return 17;
        in.dispatch_mode_raw_0960=0xFFFFu;in.presentation_kind_raw_08e8=17u;
        in.latched_event_raw_08f0=3u;
        if(gate==0u) {in.latched_event_raw_08f0=1u;in.presentation_sequence_raw_08e6=17u;in.presentation_timer_raw_08de=300u;}
        if(gate==1u)in.contact_context_raw_497f=1u;
        if(gate==2u)in.foul_out_state_raw_09ca=1u;
        if(gate==3u)in.injury_state_raw_09cc=1u;
        if(nba_gameplay_hud_dispatch(&h,&assets,&in) ||
           h.pending_routine!=(gate?0x83EC60u:0x83DA12u))return 18;
    }
    puts("OOB_CONTRACT 58 possession cases, 58 retirements, 58 following scoreboards, 4 continuation guards");
    nba_assets_free(&assets);return 0;
}
