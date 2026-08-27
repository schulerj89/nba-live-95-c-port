#define _CRT_SECURE_NO_WARNINGS
#include "nba_gameplay_camera.h"
#include <stdio.h>
#include <string.h>
static void print_words(const uint16_t *v,unsigned n){for(unsigned i=0;i<n;++i)printf("%u ",v[i]);}
int main(void) {
    unsigned kind,n,v[64];
    while(scanf("%u %u",&kind,&n)==2) {
        if(n>64)return 2;
        for(unsigned i=0;i<n;++i)if(scanf("%u",v+i)!=1)return 2;
        NbaGameplayCamera c={0};
        if((kind==0 && n==18)||(kind==1 && n==23)) {
            c.x=(int16_t)v[0];c.y=(int16_t)v[1];c.previous_x=(int16_t)v[2];c.previous_y=(int16_t)v[3];
            c.commanded_step_x=(uint16_t)v[4];c.commanded_step_y=(uint16_t)v[5];c.initialized_4a54=(uint16_t)v[6];
            NbaCameraInput in={{(uint16_t)v[7],(uint16_t)v[8],(uint16_t)v[9],(uint16_t)v[10]},
                (int16_t)v[11],(int16_t)v[12],(int16_t)v[13],(int16_t)v[14],(uint16_t)v[15],(uint16_t)v[16],(uint16_t)v[17]};
            if(kind==1) {
                in.subject=(NbaCameraSubject){(uint16_t)v[19],(uint16_t)v[20],(uint16_t)v[21],(uint16_t)v[22]};
                nba_gameplay_camera_copy(&c,(uint16_t)v[18],&in.subject);
                nba_gameplay_camera_place(&c,&in);
            } else nba_gameplay_camera_step(&c,&in);
            uint16_t result[]={(uint16_t)c.x,(uint16_t)c.y,(uint16_t)c.previous_x,(uint16_t)c.previous_y,
                c.prior_displacement_x,c.prior_displacement_y,c.commanded_step_x,c.commanded_step_y,c.initialized_4a54};
            print_words(result,9);
            if(kind==1){uint16_t xy[]={c.proxy.x_fraction,c.proxy.x_integer,c.proxy.y_fraction,c.proxy.y_integer};print_words(xy,4);}
        } else if(kind==2 && n==2)printf("%u ",nba_gameplay_camera_resolve((int16_t)v[0]));
        else if(kind==3 && n==5) {
            NbaCameraSubject s={(uint16_t)v[1],(uint16_t)v[2],(uint16_t)v[3],(uint16_t)v[4]};
            nba_gameplay_camera_copy(&c,(uint16_t)v[0],&s);
            uint16_t xy[]={c.proxy.x_fraction,c.proxy.x_integer,c.proxy.y_fraction,c.proxy.y_integer};print_words(xy,4);
        } else if(kind==4 && n>=5 && n==v[3]+4) {
            uint16_t ptr=nba_gameplay_camera_resolve((int16_t)v[0]);bool ready=false;uint16_t ticks=0;
            for(unsigned i=0;i<v[3];++i){if(ready)return 3;ticks=(uint16_t)v[4+i];ready=nba_gameplay_camera_ready(&ticks);}
            if(!ready)return 4;
            printf("%u %u ",ptr,ticks);
        } else return 5;
        puts("");
    }
    return 0;
}
