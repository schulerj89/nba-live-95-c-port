/* Controlled source tests: no capture or native snapshot is an input. */
#define main frozen_probe_main
#include "../build/sound-init-audit-v1/source/tools/setup_sound_init_probe.c"
#undef main

int main(int argc,char **argv)
{
    static Bus b;NbaSetupSoundInit s,saved,clone;NbaCodecBusCycle c;
    const uint8_t banks[]={0,0x80,0x82,0xbf,0x7e,0x7f},guards[]={0,1,2,127,128,255},highs[]={0,0x12,0x80,255};
    unsigned bi,gi,hi,i,cases=0,clones=0,refusals=0;
    if(argc!=2||!file_read(argv[1],b.rom,sizeof(b.rom)))return 2;
    for(bi=0;bi<6;bi++)for(gi=0;gi<6;gi++)for(hi=0;hi<4;hi++) {
        NbaCodecWorkEntry e={(uint16_t)((uint16_t)highs[hi]<<8|0x34u),0x5678,0x9abc,0x1fe0,banks[bi],1};
        uint64_t cycles=0;unsigned clears=0,guard_writes=0;uint8_t cb=bi>=4?0x80:banks[bi];
        memset(b.wram,0xa5,sizeof(b.wram));b.valid=true;b.wram[0x53]=guards[gi];b.wram[0x33]=(uint8_t)(0x81+hi);b.wram[0x34]=(uint8_t)(0x43+hi);
        if(!nba_setup_sound_init_begin(&s,&e,10000))return 3;
        while(nba_setup_sound_init_peek(&s,&c)&&s.stop==NBA_SOUND_INIT_NONE) {
            uint8_t value=c.kind==NBA_CODEC_READ?read_bus(&b,c.address):0;clone=s;
            if(c.kind==NBA_CODEC_WRITE) {
                if(c.source_pc==0x809b93) {
                    if(clears>=452||c.address!=((uint32_t)cb<<16|0x7edu-clears)||c.value!=0)return 4;
                    clears++;
                }
                if(c.address==0x53) {
                    if(guard_writes>=2||c.source_pc!=(guard_writes?0x809bedu:0x809b8au)||c.value!=(uint8_t)(guards[gi]-guard_writes-1u))return 5;
                    guard_writes++;
                }
                write_bus(&b,c.address,c.value);
            }
            if(!nba_setup_sound_init_accept(&s,value)||!nba_setup_sound_init_accept(&clone,value)||memcmp(&s,&clone,sizeof(s)))return 6;
            cycles++;clones++;
        }
        if(!b.valid||clears!=452||guard_writes!=2||cycles!=4703u+(bi==4?3u:bi==5?6u:0u)||s.stop!=NBA_SOUND_INIT_SPC_RESPONSE||s.boundary_pc!=0x80aacd)return 7;
        if(s.bus.registers.value!=((uint16_t)highs[hi]<<8|255u)||s.bus.registers.symbol!=255||s.bus.registers.stream_cursor!=7||s.bus.registers.stack_pointer!=0x1fd8||s.bus.registers.data_bank!=0x80)return 8;
        for(i=0;i<sizeof(b.wram);i++) {
            uint8_t expected=0xa5;
            if(i>=0x1fd8&&i<=0x1fe0)continue; /* Real stack bytes have separate source/native comparison. */
            if(i>=0x62a&&i<=0x7ed)expected=(i==0x7cd||i==0x7ce||i==0x7cf||i==0x749)?255:i==0x62d?128:i==0x62f?127:0;
            if(i==0x53)expected=(uint8_t)(guards[gi]-2u);
            if(i==0x33||i==0x5a)expected=(uint8_t)(0x81+hi);
            if(i==0x34||i==0x5b)expected=(uint8_t)(0x43+hi);
            if(b.wram[i]!=expected)return 9;
        }
        if(!nba_setup_sound_init_peek(&s,&c)||c.kind!=NBA_CODEC_READ||c.address!=0x802140||!c.instruction_end)return 10;
        saved=s;for(i=0;i<256;i++){if(nba_setup_sound_init_accept(&s,(uint8_t)i)||memcmp(&s,&saved,sizeof(s)))return 11;refusals++;}
        cases++;
    }
    printf("{\"passed\":true,\"source_cases\":%u,\"cloned_cycles\":%u,\"refused_responses\":%u}\n",cases,clones,refusals);return 0;
}
