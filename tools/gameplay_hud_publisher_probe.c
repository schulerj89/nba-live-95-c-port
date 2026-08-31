/* Native caller-state projection replay of production HUD child publishers.
 * Expectations are raw native output canvases, never produced by this probe.
 * Native routine IDs/inputs are supplied on stdin; output files are actual
 * working map/CHR and published map/CHR state at each completed C call. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_gameplay_hud.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool save(const char *directory,unsigned index,const char *extension,
                  const void *data,size_t size) {
    char path[1024];
    if(snprintf(path,sizeof(path),"%s/stage_%02u.%s",directory,index,extension)<0)
        return false;
    FILE *file=fopen(path,"wb");
    if(!file)return false;
    bool ok=fwrite(data,1u,size,file)==size;
    if(fclose(file))ok=false;
    return ok;
}
int main(int argc,char **argv) {
    if(argc!=3)return 2;
    NbaAssetPack assets={0};NbaGameplayHud hud;
    if(!nba_assets_load(&assets,argv[1]))return 3;
    if(!nba_gameplay_hud_init(&hud,&assets))return 4;
    unsigned count=0u;
    for(;;) {
        unsigned values[15];
        int parsed=scanf("%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u",
            values,values+1,values+2,values+3,values+4,values+5,values+6,
            values+7,values+8,values+9,values+10,values+11,values+12,values+13,values+14);
        if(parsed==EOF)break;
        if(parsed!=15 || count>=128u)return 5;
        for(unsigned i=1;i<15u;++i)if(values[i]>65535u)return 6;
        NbaGameplayHudInput input={{(uint16_t)values[1],(uint16_t)values[2]},
            {(uint16_t)values[3],(uint16_t)values[4]},(uint16_t)values[5],
            (uint16_t)values[6],(uint16_t)values[7],(uint16_t)values[8],
            (uint16_t)values[9],(uint16_t)values[10],(uint16_t)values[11],
            (uint16_t)values[12],(uint16_t)values[13],(uint16_t)values[14]};
        if(!nba_gameplay_hud_publish(&hud,&assets,values[0],&input))return 7;
        if(!save(argv[2],count,"map",hud.working_map,sizeof(hud.working_map)) ||
           !save(argv[2],count,"chr",hud.working_characters,sizeof(hud.working_characters)) ||
           !save(argv[2],count,"visible",hud.visible_map,sizeof(hud.visible_map)) ||
           !save(argv[2],count,"published",hud.published_characters,sizeof(hud.published_characters)) ||
           !save(argv[2],count,"clock",hud.clock_text_raw_4a60,sizeof(hud.clock_text_raw_4a60)))return 8;
        printf("HUD_PUBLICATION %u %u %u %u %u %u %u %u %u %u %u %u %u\n",count,values[0],
            hud.publication_count,hud.published_mask,hud.clock_mirror_raw_08f6,
            hud.clear_raw_08ee,input.dead_ball_busy_raw_09b4,input.event_bits_raw_13e7,
            input.presentation_sequence_raw_08e6,hud.clock_frame_raw_08f4,
            input.presentation_timer_raw_08de,input.clock_raw_0928,input.clock_snapshot_raw_092a);
        ++count;
    }
    nba_assets_free(&assets);
    return count?0:9;
}
