#define _CRT_SECURE_NO_WARNINGS
#include "nba_receiver_prepare.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>

static uint8_t rom[0x180000],raw[0x20000];
static uint16_t w(unsigned a){return (uint16_t)(raw[a]|((uint16_t)raw[a+1]<<8));}
static uint16_t rw(uint32_t a){unsigned o=((a>>16)&127u)*32768u+(a&32767u);return (uint16_t)(rom[o]|((uint16_t)rom[o+1]<<8));}
static void emit(unsigned n,unsigned a,uint16_t v){printf("W %u %u %u\n",n,a,v);}
static int guards(NbaAssetPack *pack,const NbaReceiverPrepareInput *in,const NbaReceiverPrepareState *base){
    unsigned count=0;NbaReceiverPrepareState s,before;
#define REFUSE(packarg,inputarg) do { before=s;if(nba_receiver_prepare(packarg,inputarg,&s)||memcmp(&s,&before,sizeof(s)))return 11;++count; }while(0)
    s=*base;REFUSE(NULL,in);s=*base;REFUSE(pack,NULL);s=*base;s.actor.axis_88=16;REFUSE(pack,in);
    if(nba_receiver_prepare(pack,in,NULL))return 11;++count;
    NbaReceiverPassState pass={0},saved;pass.receiver=*base;
    for(unsigned band=0;band<36;++band)if(band>30||band%6){
        pass.passer_band_62=(uint16_t)band;saved=pass;
        if(nba_receiver_pass_prepare(pack,in,&pass)||memcmp(&pass,&saved,sizeof(pass)))return 11;++count;
    }
    NbaAssetPack copy=*pack;unsigned item_index=0;
    while(item_index<copy.item_count&&copy.items[item_index].id!=NBA_ASSET_PLAYER_ANIMATIONS)++item_index;
    if(item_index==copy.item_count)return 11;
    NbaAssetItem original=copy.items[item_index];uint8_t *data=malloc(original.size);
    if(!data)return 11;
    for(unsigned test=0;test<8;++test){
        memcpy(data,original.data,original.size);copy.items[item_index]=original;copy.items[item_index].data=data;
        switch(test){
        case 0:copy.items[item_index].data=NULL;break;
        case 1:copy.items[item_index].size=79;break;
        case 2:data[0]^=1;break;
        case 3:data[8]=5;break;
        case 4:data[12]=56;break;
        case 5:memset(data+20,255,4);break;
        case 6:memset(data+68,255,4);break;
        default:memset(data+72,255,4);break;
        }
        s=*base;REFUSE(&copy,in);
    }
    free(data);printf("GUARDS %u\n",count);return 0;
#undef REFUSE
}
int main(int argc,char **argv){
    if(argc!=4 && argc!=5)return 2;
    bool parent=argc==5&&!strcmp(argv[4],"--af66");bool guard=argc==5&&!strcmp(argv[4],"--guards");
    if(argc==5&&!parent&&!guard)return 2;
    FILE *r=fopen(argv[2],"rb"),*f=fopen(argv[3],"rb");
    if(!r||!f||fread(rom,1,sizeof(rom),r)!=sizeof(rom)||fgetc(r)!=EOF)return 3;
    fclose(r);NbaAssetPack pack={0};fflush(stdout);
    int saved=_dup(_fileno(stdout));if(saved<0||_dup2(_fileno(stderr),_fileno(stdout)))return 4;
    bool loaded=nba_assets_load(&pack,argv[1]);fflush(stdout);
    if(_dup2(saved,_fileno(stdout))||_close(saved)||!loaded)return 5;
    uint32_t count;if(fread(&count,4,1,f)!=1||!count||count>10000u)return 6;
    for(unsigned n=0;n<count;++n){
        if(fread(raw,1,sizeof(raw),f)!=sizeof(raw))return 7;
        unsigned actor=w(parent?0x8e:0x96),anchor=w(0x9e),stats=w(0x3435+w(0xc2)*2u);
        uint32_t profile=w(0xe0)|((uint32_t)raw[0xe2]<<16);
        if(actor<0x34eb||actor>0x3eeb||(actor-0x34eb)%256u||anchor>0xff00u||stats>0xff00u||profile<0x808000u)return 8;
        NbaReceiverPrepareState s={0};NbaReceiverPrepareActor *a=&s.actor;
        NbaReceiverPrepareInput in={rw(profile+0x39u),w(stats+0x18u),w(0x12c),w(0x9c0),w(anchor+8u),w(anchor+10u)};
#define READ(member,offset) a->member=w(actor+offset)
        READ(x_fraction,2);READ(x,4);READ(y_fraction,6);READ(y,8);READ(axis_88,0x88);READ(timer_60,0x60);READ(modifier_b2,0xb2);READ(team_6e,0x6e);
        READ(velocity_x,0xe);READ(velocity_y,0x10);READ(baseline_x_ba,0xba);READ(baseline_y_bc,0xbc);READ(magnitude_4c,0x4c);READ(speed_4a,0x4a);READ(flags_7e,0x7e);READ(facing_4e,0x4e);READ(movement_50,0x50);READ(selector_56,0x56);READ(variant_58,0x58);READ(upper_66,0x66);
#undef READ
#define READ(member,offset) s.member=w(offset)
        READ(rng_07f6,0x7f6);READ(attempt_0904,0x904);READ(live_0936,0x936);READ(timeout_091c,0x91c);
        READ(p00,0);READ(p02,2);READ(p04,4);READ(p06,6);READ(p14,0x14);READ(p18,0x18);READ(p1a,0x1a);READ(p47,0x47);READ(p49,0x49);READ(p4f,0x4f);READ(p51,0x51);
        READ(aa,0xaa);READ(ac,0xac);READ(ae,0xae);READ(b0,0xb0);READ(b2,0xb2);READ(b4,0xb4);READ(b6,0xb6);READ(b8,0xb8);READ(ba,0xba);READ(cc,0xcc);READ(ce,0xce);READ(d0,0xd0);
        READ(math_0806,0x806);READ(math_0808,0x808);READ(math_080a,0x80a);READ(math_080c,0x80c);READ(math_080e,0x80e);READ(math_0810,0x810);READ(sign_0824,0x824);
#undef READ
        if(guard){int result=guards(&pack,&in,&s);fclose(f);nba_assets_free(&pack);return result;}
        uint16_t passer_flags=0,receiver_mode=0;
        if(parent){
            unsigned passer=w(0x96);
            if(passer<0x34eb||passer>0x3eeb||(passer-0x34eb)%256u)return 8;
            NbaReceiverPassState pass={s,w(passer+0x62u),w(passer+0x7eu),w(actor+0x5eu)};
            if(!nba_receiver_pass_prepare(&pack,&in,&pass))return 9;
            s=pass.receiver;passer_flags=pass.passer_flags_7e;receiver_mode=pass.receiver_mode_5e;
        }else if(!nba_receiver_prepare(&pack,&in,&s))return 9;
#define EMIT(member,offset) emit(n,actor+offset,a->member)
        EMIT(x_fraction,2);EMIT(x,4);EMIT(y_fraction,6);EMIT(y,8);EMIT(axis_88,0x88);EMIT(timer_60,0x60);EMIT(modifier_b2,0xb2);EMIT(team_6e,0x6e);
        EMIT(velocity_x,0xe);EMIT(velocity_y,0x10);EMIT(baseline_x_ba,0xba);EMIT(baseline_y_bc,0xbc);EMIT(magnitude_4c,0x4c);EMIT(speed_4a,0x4a);EMIT(flags_7e,0x7e);EMIT(facing_4e,0x4e);EMIT(movement_50,0x50);EMIT(selector_56,0x56);EMIT(variant_58,0x58);EMIT(upper_66,0x66);
#undef EMIT
#define EMIT(member,offset) emit(n,offset,s.member)
        EMIT(rng_07f6,0x7f6);EMIT(attempt_0904,0x904);EMIT(live_0936,0x936);EMIT(timeout_091c,0x91c);
        EMIT(p00,0);EMIT(p02,2);EMIT(p04,4);EMIT(p06,6);EMIT(p14,0x14);EMIT(p18,0x18);EMIT(p1a,0x1a);EMIT(p47,0x47);EMIT(p49,0x49);EMIT(p4f,0x4f);EMIT(p51,0x51);
        EMIT(aa,0xaa);EMIT(ac,0xac);EMIT(ae,0xae);EMIT(b0,0xb0);EMIT(b2,0xb2);EMIT(b4,0xb4);EMIT(b6,0xb6);EMIT(b8,0xb8);EMIT(ba,0xba);EMIT(cc,0xcc);EMIT(ce,0xce);EMIT(d0,0xd0);
        EMIT(math_0806,0x806);EMIT(math_0808,0x808);EMIT(math_080a,0x80a);EMIT(math_080c,0x80c);EMIT(math_080e,0x80e);EMIT(math_0810,0x810);EMIT(sign_0824,0x824);
#undef EMIT
        if(parent){emit(n,w(0x96)+0x7eu,passer_flags);emit(n,actor+0x5eu,receiver_mode);}
    }
    if(fgetc(f)!=EOF)return 10;fclose(f);nba_assets_free(&pack);return 0;
}
