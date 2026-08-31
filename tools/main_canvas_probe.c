/* Controlled canvas-only adapter. Expected native bytes are never supplied. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_setup_screen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc,char **argv) {
    if(argc!=3)return 2;
    NbaAssetPack assets={0};NbaSession session;
    NbaSetupScreen *setup=calloc(1,sizeof(*setup));
    uint8_t *canvas=malloc(0x10000u);int result=0;
    if(!setup||!canvas||!nba_assets_load(&assets,argv[1])){result=3;goto done;}
    nba_session_init(&session);
    nba_setup_screen_init(setup,&assets,&session.config);
    NbaInput input={0};
    for(unsigned i=0;i<400u;++i)(void)nba_setup_screen_update(setup,&input);
    for(unsigned i=0;i<4u;++i) {
        unsigned value;
        if(scanf("%u",&value)!=1||value>(i==0u||i==3u?3u:2u)){result=4;goto done;}
        setup->working_main[i]=(uint16_t)value;
    }
    char extra;if(scanf(" %c",&extra)!=EOF){result=4;goto done;}
    memcpy(canvas,setup->vram,0x10000u);
    if(!nba_setup_screen_build_main_value_canvas(setup,canvas)){result=5;goto done;}
    FILE *out=fopen(argv[2],"wb");
    if(!out){result=6;goto done;}
    size_t count=fwrite(canvas,1,0x10000u,out);int closed=fclose(out);
    if(count!=0x10000u||closed)result=7;
done:
    nba_assets_free(&assets);free(canvas);free(setup);return result;
}
