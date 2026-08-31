/* Bounded pass receiver selection and F1C1 distance from native entry only. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_human_pass_init.h"
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
static NbaHumanPassInput selection_input(void) {
    NbaHumanPassInput s={0};
    unsigned context=word(0x9e),record=word(0x9a);
    if(context!=0x46eb&&context!=0x476b)exit(3);
    if(record<0x47eb||record>=0x492b||(record-0x47eb)%0x40)exit(3);
    s.actor=actor_index(word(0x96));s.group_first=actor_index(word(context+4));
    s.context_group=word(context+0xc);s.direction=word(record+6);s.controller_id_090e=word(0x90e);
    for(unsigned i=0;i<10;++i){unsigned p=0x34eb+i*0x100;
        s.actors[i].x=(int16_t)word(p+4);s.actors[i].y=(int16_t)word(p+8);
        s.actors[i].mode=word(p+0x5e);s.actors[i].anchor_distance_8c=word(p+0x8c);
    }
    return s;
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
static bool initialize_mode(const char *mode) {
    if(!strcmp(mode,"fine")) {
        uint16_t distance=0;uint16_t fine=nba_gameplay_pass_direction((int16_t)word(0xaa),(int16_t)word(0xae),&distance);
        printf("{\"distance\":%u,\"fine\":%u}\n",distance,fine);return true;
    }
    if(strcmp(mode,"cancel")&&strcmp(mode,"prefix")&&strcmp(mode,"geometry")&&strcmp(mode,"chain"))return false;
    NbaHumanPassInitState s=init_input();unsigned passer=actor_index(word(0x96)),receiver=actor_index(word(0x8e));
    if(passer>=10)exit(3);
    NbaHumanPassInitPrefix p={0};NbaHumanPassInitGeometry g={0};NbaHumanPassSelection selected={0};
    NbaHumanPassRoute route=NBA_HUMAN_PASS_CONTINUE_INITIALIZER;
    if(!strcmp(mode,"cancel")) {
        uint16_t request=0;
        if(!nba_player_animation_command_scratch(NULL,&s.actors[passer].animation,NBA_ANIMATION_CANCEL_UPPER,&request,false,false,NULL))exit(3);
    }else if(!strcmp(mode,"prefix")) {
        if(!nba_human_pass_init_prefix(&s,(uint16_t)passer,word(0xc2),(uint16_t)receiver,word(0xaa),&p))exit(3);
    }else if(!strcmp(mode,"geometry")) {
        if(!nba_human_pass_init_geometry(&s,(uint16_t)passer,(uint16_t)receiver,&g))exit(3);
    }else {
        NbaHumanPassInput select=selection_input();
        route=nba_human_pass_prepare(&select,word(0xc2),&s,&selected,&p,&g);
        if(route==NBA_HUMAN_PASS_INVALID)exit(3);
        receiver=selected.receiver_slot;
    }
    project_init(&s);printf("{\"route\":%u",(unsigned)route);
    printf(",\"prefix_words\":[");
    if(!strcmp(mode,"prefix")||(!strcmp(mode,"chain")&&route==NBA_HUMAN_PASS_CONTINUE_INITIALIZER))
        printf("%u,%u,%u",p.profile_lo_e6,p.profile_hi_e8,0x34eb+p.receiver_slot*0x100);
    printf("],\"geometry_words\":[");
    if(!strcmp(mode,"geometry")||(!strcmp(mode,"chain")&&route==NBA_HUMAN_PASS_CONTINUE_INITIALIZER))
        printf("%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",(uint16_t)g.dx,(uint16_t)g.dy,g.fine_direction,g.coarse_direction,g.distance,g.band,g.distance,g.relative,g.relative,0x34eb+receiver*0x100);
    printf("]");state_output();printf("}\n");return true;
}
int main(void){
    char line[8192],mode[16],path[4096];
    while(fgets(line,sizeof(line),stdin)){
        if(sscanf(line,"%15s %4095[^\r\n]",mode,path)!=2)return 2;
        load(path);
        if(initialize_mode(mode))continue;
        if(!strcmp(mode,"metric")){
            printf("{\"distance\":%u}\n",nba_human_pass_distance((int16_t)word(0xaa),(int16_t)word(0xae)));
            continue;
        }
        if(strcmp(mode,"pass"))return 2;
        unsigned context=word(0x9e),record=word(0x9a);
        if(context!=0x46eb&&context!=0x476b)return 3;
        if(record<0x47eb||record>=0x492b||(record-0x47eb)%0x40)return 3;
        NbaHumanPassInput s={0};
        s.actor=actor_index(word(0x96));s.group_first=actor_index(word(context+4));
        s.context_group=word(context+0xc);s.direction=word(record+6);s.controller_id_090e=word(0x90e);
        for(unsigned i=0;i<10;++i){
            unsigned p=0x34eb+i*0x100;
            s.actors[i].x=(int16_t)word(p+4);s.actors[i].y=(int16_t)word(p+8);
            s.actors[i].mode=word(p+0x5e);s.actors[i].anchor_distance_8c=word(p+0x8c);
        }
        NbaHumanPassSelection out=nba_human_pass_select(&s);
        if(out.route==NBA_HUMAN_PASS_INVALID){fprintf(stderr,"invalid native pass input %s\n",path);return 3;}
        put(0x944,out.controller_tag_0944);
        printf("{\"route\":%u,\"score\":%u,\"handoff_words\":[",(unsigned)out.route,out.score);
        if(out.route==NBA_HUMAN_PASS_CONTINUE_INITIALIZER)printf("%u,%u",out.receiver_identity,0x34eb+out.receiver_slot*0x100);
        printf("]");
        array("actor_words",0x34eb,11*128);array("controller_words",0x47eb,160);array("context_words",0x46eb,128);
        static const unsigned globals[]={0x90c,0x90e,0x910,0x936,0x93a,0x93e,0x944,0x946,0x978};
        printf(",\"global_words\":[");
        for(unsigned i=0;i<sizeof(globals)/sizeof(globals[0]);++i)printf("%s%u",i?",":"",word(globals[i]));
        printf("]}\n");
    }
    return 0;
}
