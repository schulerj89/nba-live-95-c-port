#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "period_appearance.h"
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char raw[131072];
static uint16_t word(unsigned a) { return (uint16_t)(raw[a] | (raw[a+1] << 8)); }
static void put(unsigned a, uint16_t v) { raw[a]=(unsigned char)v;raw[a+1]=(unsigned char)(v>>8); }
int main(int argc,char **argv) {
    if(argc!=4)return 2;
    char *end;long actor=strtol(argv[3],&end,10);
    if(*end || actor<0 || actor>9)return 2;
    FILE *f=fopen(argv[2],"rb");if(!f)return 2;
    size_t n=fread(raw,1,sizeof(raw),f);int extra=fgetc(f);fclose(f);
    if(n!=sizeof(raw)||extra!=EOF)return 2;
    NbaAssetPack assets={0};fflush(stdout);int saved=_dup(_fileno(stdout));
    if(saved<0 || _dup2(_fileno(stderr),_fileno(stdout)))return 2;
    bool loaded=nba_assets_load(&assets,argv[1]);fflush(stdout);
    if(_dup2(saved,_fileno(stdout)))return 2;_close(saved);
    if(!loaded)return 2;
    unsigned a=0x34eb+(unsigned)actor*256;
    NbaPeriodAppearance s={0};NbaPlayerAnimationChannels *c=&s.channels;
#define LOAD(field,off) c->field=word(a+off)
    LOAD(upper_queue_cursor,0x18);LOAD(lower_queue_cursor,0x1a);
    LOAD(upper_state,0x30);LOAD(lower_state,0x32);LOAD(base_state,0x38);
    LOAD(upper_phase,0x3a);LOAD(lower_phase,0x3c);LOAD(upper_accumulator,0x42);LOAD(lower_accumulator,0x44);
    LOAD(upper_lock,0x46);LOAD(lower_lock,0x48);LOAD(upper_phase_target,0xb0);
    for(unsigned i=0;i<3;i++){c->upper_queue[i]=word(a+0x1c+i*2);c->lower_queue[i]=word(a+0x22+i*2);}
#undef LOAD
    s.owner=word(0x93e);s.owner_pointer=word(0x940);s.controller=word(a+0x16);
    s.velocity_x=word(a+0xe);s.velocity_y=word(a+0x10);s.z=word(a+0xc);s.boost=word(a+0x72);s.speed=word(a+0x4a);
    s.direction=word(a+0x4e);s.display_direction=word(a+0x52);s.status=word(a+0x28);
    s.alternate_lower=word(a+0xa8);s.variant=word(a+0x6c);s.catcher_latch=word(a+0xae);
    s.delta=word(0xc6);s.rng=word(0x7f6);s.upper_resource=word(a+0x2a);s.lower_resource=word(a+0x2c);
    if(!nba_period_appearance(&assets,&s)){nba_assets_free(&assets);return 3;}
#define SAVE(field,off) put(a+off,c->field)
    SAVE(upper_queue_cursor,0x18);SAVE(lower_queue_cursor,0x1a);
    SAVE(upper_state,0x30);SAVE(lower_state,0x32);SAVE(base_state,0x38);
    SAVE(upper_phase,0x3a);SAVE(lower_phase,0x3c);SAVE(upper_accumulator,0x42);SAVE(lower_accumulator,0x44);
    SAVE(upper_lock,0x46);SAVE(lower_lock,0x48);SAVE(upper_phase_target,0xb0);
    for(unsigned i=0;i<3;i++){put(a+0x1c+i*2,c->upper_queue[i]);put(a+0x22+i*2,c->lower_queue[i]);}
#undef SAVE
    put(a+0x4e,s.direction);put(a+0x28,s.status);put(a+0x2a,s.upper_resource);put(a+0x2c,s.lower_resource);put(a+0xae,s.catcher_latch);
    printf("{\"rng\":%u,\"owner_pointer\":%u,\"actor\":[",s.rng,s.owner_pointer);
    for(unsigned i=0;i<128;i++)printf("%s%u",i?",":"",word(a+i*2));puts("]}");
    nba_assets_free(&assets);return 0;
}
