#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "period_render_tail.h"
#include <stdio.h>
static unsigned char raw[131072];
static uint16_t word(unsigned a){return (uint16_t)(raw[a]|(raw[a+1]<<8));}
static void put(unsigned a,uint16_t v){raw[a]=(unsigned char)v;raw[a+1]=(unsigned char)(v>>8);}
int main(int argc,char **argv) {
    if(argc!=3)return 2;FILE*f=fopen(argv[1],"rb");if(!f)return 2;
    size_t size=fread(raw,1,sizeof(raw),f);int extra=fgetc(f);fclose(f);if(size!=sizeof(raw)||extra!=EOF)return 2;
    NbaPeriodRenderTail s={0};
    for(unsigned i=0;i<12;i++) {
        unsigned a=0x34eb+i*256;s.x[i]=(int16_t)word(a+4);s.y[i]=(int16_t)word(a+8);s.depth[i]=word(a+0x68);
        s.draw_order[i]=word(0x7e44+i*2);s.collision.object[i]=word(0x34d3+i*2);
        if(i<11){s.collision.x[i]=s.x[i];s.collision.link[i]=word(a+0x14);}
    }
    s.camera_y=word(0x860);s.leading_sentinel=word(0x34d1);s.frame_low=word(0x84a);s.frame_high=word(0x84c);
    if(!nba_period_render_tail(&s))return 3;
    for(unsigned i=0;i<12;i++) {
        put(0x34eb+i*256+0x68,s.depth[i]);put(0x7e44+i*2,s.draw_order[i]);put(0x34d3+i*2,s.collision.object[i]);
        if(i<11)put(0x34eb+i*256+0x14,s.collision.link[i]);
    }
    put(0x84a,s.frame_low);put(0x84c,s.frame_high);
    f=fopen(argv[2],"wb");if(!f)return 2;size=fwrite(raw,1,sizeof(raw),f);int closed=fclose(f);
    return size==sizeof(raw)&&closed==0?0:2;
}
