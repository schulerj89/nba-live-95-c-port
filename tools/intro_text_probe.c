#include "nba_intro_text.h"
#include "nba_snes_ppu.h"
#include <stdio.h>

int main(int argc,char **argv) {
    if(argc!=3)return 2;
    NbaAssetPack assets={0};
    NbaRenderer renderer;
    nba_renderer_init(&renderer);
    if(!nba_assets_load(&assets,argv[1]))return 3;
    for(int legal=0;legal<2;legal++)for(int brightness=0;brightness<16;brightness++) {
        if(!nba_intro_text_render(&assets,&renderer,legal!=0,brightness))return 4;
        char path[2048];
        if(snprintf(path,sizeof(path),"%s/%s-%02d.rgb",argv[2],legal?"legal":"license",brightness)<0)return 5;
        FILE *f=fopen(path,"wb");if(!f)return 5;
        for(int i=0;i<NBA_SNES_WIDTH*NBA_SNES_HEIGHT;i++) {
            uint32_t color=renderer.pixels[i];
            uint8_t rgb[3]={(uint8_t)(color>>16),(uint8_t)(color>>8),(uint8_t)color};
            if(fwrite(rgb,1,3,f)!=3)return 6;
        }
        if(fclose(f))return 6;
    }
    nba_snes_mode1_release(&renderer);
    nba_assets_free(&assets);
    puts("PASS: 32 native-font raster outputs generated (not a parity verdict).");
    return 0;
}
