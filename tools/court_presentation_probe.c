/* ROM witness replay using production camera/presentation functions. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_court_presentation.h"
#include "nba_gameplay_camera.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct { unsigned count; uint16_t v[1024][4]; } Transfers;
static void transfer(void *ctx,uint16_t src,uint16_t bank,uint16_t size,uint16_t dest) {
    Transfers *t=ctx;
    if(t->count>=1024)abort();
    uint16_t *v=t->v[t->count++];v[0]=src;v[1]=bank;v[2]=size;v[3]=dest;
}
static void output(const uint16_t *v,unsigned count) {
    for(unsigned i=0;i<count;++i)printf("%u ",v[i]);
}
int main(int argc,char **argv) {
    NbaAssetPack assets={0};
    if(argc!=2 || !nba_assets_load(&assets,argv[1]))return 2;
    unsigned kind,n;
    while(scanf("%u %u",&kind,&n)==2) {
        unsigned raw[128];
        if(n>128)return 3;
        for(unsigned i=0;i<n;++i)if(scanf("%u",&raw[i])!=1)return 3;
        if(kind==0 && n==10) {
            NbaCourtPresentation p={(uint16_t)raw[5],(uint16_t)raw[6],(uint16_t)raw[7],(uint16_t)raw[8],(uint16_t)raw[9]};
            nba_court_presentation_update(&p,(int16_t)raw[0],(int16_t)raw[1],(uint16_t)raw[2],(int16_t)raw[3],(int16_t)raw[4]);
            uint16_t v[]={p.basket_x_3fef,p.window_x_087c,p.window_y_087e,p.window_left_0880,p.window_right_0882};output(v,5);
        } else if(kind==1 && n==113) {
            NbaCourtStream s={0};
            s.scroll_x=(uint16_t)raw[4];s.scroll_y=(uint16_t)raw[5];
            s.coarse_x=(uint16_t)raw[6];s.coarse_y=(uint16_t)raw[7];s.row_bytes=(uint16_t)raw[8];
            s.destination=(uint16_t)raw[9];s.source=(uint16_t)raw[10];s.next_scroll_x=(uint16_t)raw[11];s.next_scroll_y=(uint16_t)raw[12];s.source_bank=(uint16_t)raw[13];
            for(unsigned i=0;i<99;++i)s.rows[i]=(uint16_t)raw[14+i];
            Transfers t={0};
            if(!nba_court_stream_update(&s,&assets,(int16_t)raw[0],(int16_t)raw[1],(int16_t)raw[2],(int16_t)raw[3],transfer,&t))return 4;
            uint16_t v[]={s.coarse_x,s.coarse_y,s.row_bytes,s.destination,s.source,s.next_scroll_x,s.next_scroll_y};
            output(v,7);output(s.rows,99);printf("%u ",t.count);
            for(unsigned i=0;i<t.count;++i)output(t.v[i],4);
        } else if(kind==2 && n==11) {
            NbaGameplayCamera c={0};c.x=(int16_t)raw[0];c.y=(int16_t)raw[1];c.previous_x=(int16_t)raw[2];c.previous_y=(int16_t)raw[3];
            nba_gameplay_camera_update(&c,(int16_t)raw[5]*256+(raw[4]>>8),(int16_t)raw[7]*256+(raw[6]>>8),(int16_t)raw[8]*256,(uint8_t)raw[9],raw[10]==1);
            uint16_t v[]={(uint16_t)c.x,(uint16_t)c.y,(uint16_t)c.previous_x,(uint16_t)c.previous_y,c.prior_displacement_x,c.prior_displacement_y,c.commanded_step_x,c.commanded_step_y};output(v,8);
        } else return 5;
        puts("");
    }
    nba_assets_free(&assets);return 0;
}
