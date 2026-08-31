/* Native-entry-only side/back action and B00B/B47A child replay. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_human_pass_action.h"
#include <io.h>
#include "nba_gameplay_ai.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char raw[0x20000],present[0x20000];
static const unsigned ranges[][2]={{0,0x100},{0x500,0x500},{0x1600,0x300},{0x3400,0x1600}};
static uint16_t word(unsigned a) {
    if(a+1>=sizeof(raw)||!present[a]||!present[a+1]){
        fprintf(stderr,"missing raw word %x\n",a);exit(3);
    }
    return (uint16_t)(raw[a]|((uint16_t)raw[a+1]<<8));
}
static void put(unsigned a,uint16_t v){raw[a]=(unsigned char)v;raw[a+1]=(unsigned char)(v>>8);}
static void array(const char *name,unsigned a,unsigned n){
    printf(",\"%s\":[",name);
    for(unsigned i=0;i<n;++i)printf("%s%u",i?",":"",word(a+i*2));
    printf("]");
}
static void load(const char *path){
    FILE *f=fopen(path,"rb");if(!f)exit(2);
    memset(raw,0,sizeof(raw));memset(present,0,sizeof(present));
    for(unsigned i=0;i<sizeof(ranges)/sizeof(ranges[0]);++i){
        unsigned a=ranges[i][0],n=ranges[i][1];
        if(fread(raw+a,1,n,f)!=n)exit(2);
        memset(present+a,1,n);
    }
    if(fgetc(f)!=EOF)exit(2);fclose(f);
}
static uint16_t actor_index(unsigned p){
    return p>=0x34eb&&p<0x3eeb&&(p-0x34eb)%0x100==0?(uint16_t)((p-0x34eb)/0x100):0xffffu;
}
static NbaHumanPassInitState init_input(void) {
    NbaHumanPassInitState s={0};
    s.live_0936=word(0x936);s.passer_0942=word(0x942);s.receiver_0946=word(0x946);
    s.active_09c4=word(0x9c4);s.inbound_transfer_09b8=word(0x9b8);
    s.distance_09da=word(0x9da);s.controller_tag_0944=word(0x944);
    for(unsigned i=0;i<10;++i){unsigned p=0x34eb+i*0x100;
        NbaHumanPassInitActor *a=&s.actors[i];NbaPlayerAnimationChannels *c=&a->animation;
        a->identity=word(p);a->x=(int16_t)word(p+4);a->y=(int16_t)word(p+8);
        a->velocity_x=(int16_t)word(p+0xe);a->velocity_y=(int16_t)word(p+0x10);
        a->movement_direction=word(p+0x4e);a->mode=word(p+0x5e);
        a->timer=word(p+0x60);a->pass_band=word(p+0x62);
        c->upper_queue_cursor=word(p+0x18);c->lower_queue_cursor=word(p+0x1a);
        c->upper_state=word(p+0x30);c->lower_state=word(p+0x32);c->base_state=word(p+0x38);
        c->upper_phase=word(p+0x3a);c->lower_phase=word(p+0x3c);
        c->upper_accumulator=word(p+0x42);c->lower_accumulator=word(p+0x44);
        c->upper_lock=word(p+0x46);c->lower_lock=word(p+0x48);c->upper_phase_target=word(p+0xb0);
        for(unsigned j=0;j<3;++j){c->upper_queue[j]=word(p+0x1c+j*2);c->lower_queue[j]=word(p+0x22+j*2);}
        s.profile_pointers[i][0]=word(0x3449+i*4);s.profile_pointers[i][1]=word(0x344b+i*4);
    }
    return s;
}
static void project_init(const NbaHumanPassInitState *s) {
    put(0x936,s->live_0936);put(0x942,s->passer_0942);put(0x946,s->receiver_0946);
    put(0x9c4,s->active_09c4);put(0x9b8,s->inbound_transfer_09b8);
    put(0x9da,s->distance_09da);put(0x944,s->controller_tag_0944);
    for(unsigned i=0;i<10;++i){unsigned p=0x34eb+i*0x100;
        const NbaHumanPassInitActor *a=&s->actors[i];const NbaPlayerAnimationChannels *c=&a->animation;
        put(p,a->identity);put(p+4,(uint16_t)a->x);put(p+8,(uint16_t)a->y);
        put(p+0xe,(uint16_t)a->velocity_x);put(p+0x10,(uint16_t)a->velocity_y);
        put(p+0x4e,a->movement_direction);
        put(p+0x18,c->upper_queue_cursor);put(p+0x30,c->upper_state);
        put(p+0x3a,c->upper_phase);put(p+0x42,c->upper_accumulator);put(p+0x46,c->upper_lock);
        put(p+0x1a,c->lower_queue_cursor);put(p+0x32,c->lower_state);put(p+0x38,c->base_state);
        put(p+0x3c,c->lower_phase);put(p+0x44,c->lower_accumulator);put(p+0x48,c->lower_lock);
        put(p+0xb0,c->upper_phase_target);
        for(unsigned j=0;j<3;++j){put(p+0x1c+j*2,c->upper_queue[j]);put(p+0x22+j*2,c->lower_queue[j]);}
        put(p+0x5e,a->mode);put(p+0x60,a->timer);put(p+0x62,a->pass_band);
        put(0x3449+i*4,s->profile_pointers[i][0]);put(0x344b+i*4,s->profile_pointers[i][1]);
    }
}
static void state_output(void) {
    array("actor_words",0x34eb,11*128);array("controller_words",0x47eb,160);array("context_words",0x46eb,128);
    array("profile_words",0x3449,20);
    static const unsigned globals[]={0x90c,0x90e,0x910,0x936,0x93a,0x93e,0x942,0x944,0x946,0x978,0x9b8,0x9c4,0x9da};
    printf(",\"global_words\":[");
    for(unsigned i=0;i<sizeof(globals)/sizeof(globals[0]);++i)printf("%s%u",i?",":"",word(globals[i]));
    printf("]");
}
static unsigned char rom[0x180000];
static NbaHumanPassActionState action_input(void) {
    NbaHumanPassActionState s={0};s.common=init_input();
    s.passer_slot=actor_index(word(0x96));s.receiver_slot=actor_index(word(0x8e));
    s.distance_4f=word(0x4f);s.coarse_be=word(0xbe);s.relative_51=word(0x51);
    s.request_00=word(0);s.descriptor_47=word(0x47);s.descriptor_bank_49=word(0x49);
    unsigned address=word(0xe6),bank=word(0xe8)&255;
    unsigned offset=(bank&0x7f)*0x8000+(address&0x7fff)+0x3e;
    if(address<0x8000||offset>=sizeof(rom))exit(3);
    s.profile_3e=rom[offset];
    for(unsigned i=0;i<10;++i){unsigned p=0x34eb+i*0x100;NbaHumanPassActionActor *a=&s.extra[i];
        a->z=word(p+0xc);a->velocity_z=word(p+0x12);a->boost=word(p+0x72);
        a->magnitude=word(p+0x4c);a->family=word(p+0xc0);a->pass_direction=word(p+0x66);a->flags=word(p+0x7e);
    }
    return s;
}
static void project_action(const NbaHumanPassActionState *s) {
    project_init(&s->common);
    put(0,s->request_00);put(0x47,s->descriptor_47);put(0x49,s->descriptor_bank_49);
    put(0x4f,s->distance_4f);put(0xbe,s->coarse_be);put(0x51,s->relative_51);
    for(unsigned i=0;i<10;++i){unsigned p=0x34eb+i*0x100;const NbaHumanPassActionActor *a=&s->extra[i];
        put(p+0xc,a->z);put(p+0x12,a->velocity_z);put(p+0x72,a->boost);
        put(p+0x4c,a->magnitude);put(p+0xc0,a->family);put(p+0x66,a->pass_direction);put(p+0x7e,a->flags);
    }
}
int main(int argc,char **argv) {
    if(argc!=3)return 2;
    NbaAssetPack assets={0};
    int saved=_dup(_fileno(stdout));if(saved<0)return 2;
    if(_dup2(_fileno(stderr),_fileno(stdout))!=0)return 2;
    bool loaded=nba_assets_load(&assets,argv[1]);fflush(stdout);
    if(_dup2(saved,_fileno(stdout))!=0)return 2;_close(saved);if(!loaded)return 3;
    FILE *f=fopen(argv[2],"rb");if(!f)return 2;
    if(fread(rom,1,sizeof(rom),f)!=sizeof(rom)||fgetc(f)!=EOF)return 2;fclose(f);
    char line[8192],mode[16],path[4096];
    while(fgets(line,sizeof(line),stdin)) {
        if(sscanf(line,"%15s %4095[^\r\n]",mode,path)!=2)return 2;
        load(path);NbaHumanPassActionState s=action_input();
        NbaHumanPassActionRoute route=NBA_HUMAN_PASS_ACTION_POSE_AF1D;
        if(!strcmp(mode,"gate"))route=nba_human_pass_action_select(&assets,&s);
        else if(!strcmp(mode,"ground")){if(!nba_human_pass_action_grounded(&assets,&s))return 3;}
        else if(!strcmp(mode,"upper")){if(!nba_human_pass_action_upper(&assets,&s))return 3;}
        else return 2;
        if(route==NBA_HUMAN_PASS_ACTION_INVALID)return 3;
        project_action(&s);printf("{\"route\":%u,\"profile_byte\":%u",(unsigned)route,s.profile_3e);
        static const unsigned addresses[]={0,0x47,0x49,0x4f,0x51,0xbe};
        printf(",\"dp_words\":[");
        for(unsigned i=0;i<sizeof(addresses)/sizeof(addresses[0]);++i)printf("%s%u",i?",":"",word(addresses[i]));
        printf("]");state_output();printf("}\n");
    }
    nba_assets_free(&assets);return ferror(stdin)?1:0;
}
