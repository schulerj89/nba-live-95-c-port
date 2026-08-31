#include "nba_draw_order.h"
#include <stdio.h>
#include <string.h>

static uint16_t word(const unsigned char *p) { return (uint16_t)(p[0]|((unsigned)p[1]<<8)); }
int main(int argc,char **argv) {
    if (argc!=2) return 2;
    NbaDrawOrder guard={0},saved={0};NbaDrawOrderInput zero={0};
    if (nba_draw_order_initialize(NULL) || nba_draw_order_project(NULL,&zero) ||
        nba_draw_order_pass(NULL) || nba_draw_order_update(NULL,&zero) ||
        nba_draw_order_project(&guard,NULL) || nba_draw_order_update(&guard,NULL) ||
        memcmp(&guard,&saved,sizeof(guard))) return 6;
    FILE *f=NULL;
    if (fopen_s(&f,argv[1],"rb") || !f) return 3;
    unsigned char b[104];unsigned count=0;
    for (;;) {
        size_t n=fread(b,1,sizeof(b),f);
        if (!n) break;
        if (n!=sizeof(b) || memcmp(b,"DOR1",4) || word(b+4)>3) { fclose(f);return 4; }
        NbaDrawOrder s;NbaDrawOrderInput input;
        for (unsigned i=0;i<12;i++) {
            s.order[i]=word(b+6+i*2);s.depth[i]=word(b+30+i*2);
            input.x[i]=word(b+54+i*2);input.y[i]=word(b+78+i*2);
        }
        input.camera_y=word(b+102);
        unsigned op=word(b+4);
        bool ok=op==0?nba_draw_order_initialize(&s):op==1?nba_draw_order_project(&s,&input):
            op==2?nba_draw_order_pass(&s):nba_draw_order_update(&s,&input);
        printf("{\"index\":%u,\"operation\":%u,\"ok\":%s,\"order\":[",++count,op,ok?"true":"false");
        for (unsigned i=0;i<12;i++) printf("%s%u",i?",":"",s.order[i]);
        printf("],\"depth\":[");
        for (unsigned i=0;i<12;i++) printf("%s%u",i?",":"",s.depth[i]);
        puts("]}");
    }
    bool bad=ferror(f)!=0;fclose(f);
    return bad||!count?5:0;
}
