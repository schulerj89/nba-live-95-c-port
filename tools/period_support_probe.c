#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "period_support.h"
#include <stdio.h>
#include <string.h>
#include <io.h>
static unsigned char raw[131072];
static uint16_t word(unsigned a){return (uint16_t)(raw[a]|(raw[a+1]<<8));}
static void put(unsigned a,uint16_t v){raw[a]=(unsigned char)v;raw[a+1]=(unsigned char)(v>>8);}
static void channels(NbaPlayerAnimationChannels *c,unsigned a,bool save) {
#define FIELD(field,offset) if(save)put(a+offset,c->field);else c->field=word(a+offset)
    FIELD(upper_queue_cursor,0x18);FIELD(lower_queue_cursor,0x1a);
    FIELD(upper_state,0x30);FIELD(lower_state,0x32);FIELD(base_state,0x38);
    FIELD(upper_phase,0x3a);FIELD(lower_phase,0x3c);FIELD(upper_accumulator,0x42);FIELD(lower_accumulator,0x44);
    FIELD(upper_lock,0x46);FIELD(lower_lock,0x48);FIELD(upper_phase_target,0xb0);
    for(unsigned i=0;i<3;i++){FIELD(upper_queue[i],0x1c+i*2);FIELD(lower_queue[i],0x22+i*2);}
#undef FIELD
}
int main(int argc,char **argv) {
    if(argc!=5)return 2;
    FILE *f=fopen(argv[2],"rb");if(!f)return 2;
    size_t n=fread(raw,1,sizeof(raw),f);int extra=fgetc(f);fclose(f);
    if(n!=sizeof(raw)||extra!=EOF)return 2;
    NbaAssetPack assets={0};fflush(stdout);int saved=_dup(_fileno(stdout));
    if(saved<0 || _dup2(_fileno(stderr),_fileno(stdout)))return 2;
    bool loaded=nba_assets_load(&assets,argv[1]);fflush(stdout);
    if(_dup2(saved,_fileno(stdout)))return 2;_close(saved);if(!loaded)return 2;
    bool ok=false;
    if(!strcmp(argv[3],"assignment")) {
        NbaPeriodAssignmentInput in={0};NbaPeriodAssignment s={0};
        for(unsigned side=0;side<2;side++) {
            unsigned context=0x46eb+side*128;
            if(word(context)>255){nba_assets_free(&assets);return 3;}
            in.team[side]=(uint8_t)word(context);
            for(unsigned i=0;i<5;i++) {
                if(word(context+0xe+i*2)>255){nba_assets_free(&assets);return 3;}
                in.roster[side*5+i]=(uint8_t)word(context+0xe+i*2);
                in.selector[side*5+i]=raw[0x159a+side*5+i];
            }
        }
        ok=nba_period_assignment(&assets,&in,&s);
        if(ok)for(unsigned i=0;i<10;i++) {
            unsigned a=0x34eb+i*256;
            put(a+0x6c,s.actor[i].variant);put(a+0x74,s.actor[i].current);
            put(a+0x76,s.actor[i].base);put(a+0x78,s.actor[i].alternate);
            put(a+0x80,s.actor[i].help);put(a+0x92,s.actor[i].role);
            put(0x3435+i*2,s.statistic_pointer[i]);
            put(0x3449+i*4,(uint16_t)s.roster_pointer[i]);
            put(0x344b+i*4,(uint16_t)(s.roster_pointer[i]>>16));
        }
        if(ok)for(unsigned side=0;side<2;side++)for(unsigned i=0;i<5;i++)raw[0x4734+side*128+i]=s.order[side][i];
        if(ok)for(unsigned i=0;i<10;i++)put(0x9da+i*2,s.keys[i]);
    } else if(!strcmp(argv[3],"sort")) {
        NbaPeriodObjectSort s={0};
        for(unsigned i=0;i<11;i++){s.x[i]=(int16_t)word(0x34ef+i*256);s.link[i]=word(0x34ff+i*256);}
        for(unsigned i=0;i<12;i++)s.object[i]=word(0x34d3+i*2);
        ok=nba_period_object_sort(&s);
        if(ok){for(unsigned i=0;i<11;i++)put(0x34ff+i*256,s.link[i]);for(unsigned i=0;i<12;i++)put(0x34d3+i*2,s.object[i]);}
    } else if(!strcmp(argv[3],"attachment")) {
        NbaPeriodAttachment s={0};s.actor=word(0x954);s.owner=word(0x93e);
        if(s.actor>=10){nba_assets_free(&assets);return 3;}
        unsigned a=0x34eb+s.actor*256;s.group=word(a+0x6e);
        for(unsigned i=0;i<5;i++)memcpy(&s.controllers.record[i],raw+0x47eb+i*64,64);
        for(unsigned i=0;i<10;i++)s.controllers.actor_assignment[i]=(int16_t)word(0x3501+i*256);
        for(unsigned side=0;side<2;side++){s.controllers.count[side]=word(0x4726+side*128);s.controllers.cursor[side]=word(0x4728+side*128);}
        channels(&s.channels,a,false);
        s.pose.mirror_flags=word(a+0x28);s.pose.direction=word(a+0x52);
        s.facing=word(a+0x4e);s.alternate_lower=word(a+0xa8);s.variant=word(a+0x6c);s.boost=word(a+0x72);
        s.x=(int16_t)word(a+4);s.y=(int16_t)word(a+8);s.z=(int16_t)word(a+12);
        s.ball_x=(int16_t)word(0x3eef);s.ball_y=(int16_t)word(0x3ef3);s.ball_z=(int16_t)word(0x3ef7);
        s.previous_ball_x=word(0x922);s.scratch_47=word(0x47);
        ok=nba_period_attachment(&assets,&s);
        if(ok) {
            for(unsigned i=0;i<5;i++)memcpy(raw+0x47eb+i*64,&s.controllers.record[i],64);
            for(unsigned i=0;i<10;i++)put(0x3501+i*256,(uint16_t)s.controllers.actor_assignment[i]);
            for(unsigned side=0;side<2;side++){put(0x4726+side*128,s.controllers.count[side]);put(0x4728+side*128,s.controllers.cursor[side]);}
            channels(&s.channels,a,true);put(a+0x28,s.pose.mirror_flags);
            put(a+0x2a,s.pose.upper_resource);put(a+0x2c,s.pose.lower_resource);
            put(a+0x34,s.pose.upper_state);put(a+0x36,s.pose.lower_state);
            put(a+0x3e,s.pose.upper_phase);put(a+0x40,s.pose.lower_phase);put(a+0x52,s.pose.direction);
            put(0x3eef,(uint16_t)s.ball_x);put(0x3ef3,(uint16_t)s.ball_y);put(0x3ef7,(uint16_t)s.ball_z);put(0x922,s.previous_ball_x);
        }
    }
    nba_assets_free(&assets);if(!ok)return 3;
    f=fopen(argv[4],"wb");if(!f)return 2;n=fwrite(raw,1,sizeof(raw),f);int status=fclose(f);
    return n==sizeof(raw)&&status==0?0:2;
}
