/* Independent bounded source edge checks. Reuse only frozen bus file I/O. */
#define main frozen_probe_main
#include "../build/sound-prefix-audit-v1/source/tools/setup_sound_prefix_probe.c"
#undef main

static void seed(Bus *b)
{
    memset(b->wram,0,sizeof(b->wram)); b->valid=true;
    b->wram[0x5a]=0x82; b->wram[0x7cd]=b->wram[0x7ce]=b->wram[0x7cf]=255;
}

int main(int argc,char **argv)
{
    static Bus b; NbaSetupSoundPrefix s,clone,saved; NbaCodecBusCycle c,again;
    NbaCodecWorkEntry entry={0x5500,0x80,5,0x1fe0,0x80,0x13};
    const uint8_t banks[]={0,0x80,0x82,0xbf};
    const uint16_t words[]={0,255,32767,65535};
    unsigned channel,bank,w,count=0,clones=0,refusals=0,i;
    if(argc!=2||!file_read(argv[1],b.rom,sizeof(b.rom)))return 2;
    for(channel=0;channel<8;channel++)for(bank=0;bank<4;bank++)for(w=0;w<4;w++) {
        uint64_t cycles=0;unsigned writes=0;uint16_t target=(uint16_t)(0x7ba+2*channel),next=(uint16_t)(words[w]+1u);
        seed(&b);b.wram[0x5a]=banks[bank];b.wram[0x73a+channel]=10;b.wram[0x742+channel]=15;
        b.wram[target]=(uint8_t)words[w];b.wram[target+1]=(uint8_t)(words[w]>>8);
        b.wram[0x76a+channel*2]=0x34;b.wram[0x76b+channel*2]=0x12;
        b.wram[0x78a+channel*2]=0x78;b.wram[0x78b+channel*2]=0x56;
        if(!nba_setup_sound_prefix_begin(&s,&entry,10000))return 3;
        while(nba_setup_sound_prefix_peek(&s,&c)&&s.stop==NBA_SOUND_PREFIX_NONE) {
            uint8_t value=c.kind==NBA_CODEC_READ?read_bus(&b,c.address):0;
            clone=s;if(!nba_setup_sound_prefix_peek(&clone,&again)||memcmp(&c,&again,sizeof(c)))return 4;
            if(c.kind==NBA_CODEC_WRITE) {
                if(c.source_pc==0x80a29f) {
                    uint32_t expected=(uint32_t)banks[bank]<<16|target+(writes?0u:1u);
                    uint8_t byte=(uint8_t)(next>>(writes?0:8));
                    if(writes>=2||c.address!=expected||c.value!=byte)return 5;
                    writes++;
                }
                write_bus(&b,c.address,c.value);
            }
            if(!nba_setup_sound_prefix_accept(&s,value)||!nba_setup_sound_prefix_accept(&clone,value)||memcmp(&s,&clone,sizeof(s)))return 6;
            cycles++;clones++;
        }
        /* 193 cycles from literal source sequence; each empty channel's
         * 13 instructions add 3+4+4+2+3+3+2+2+3+2+2+2+3 = 35. */
        if(!b.valid||cycles!=193+35*channel||writes!=2||s.stop!=NBA_SOUND_PREFIX_SPC_RESPONSE||s.boundary_pc!=0x80aae6||s.bus.registers.symbol!=2*channel||s.bus.registers.stream_cursor!=channel||s.bus.registers.stack_pointer!=0x1fda)return 7;
        if(b.wram[0x58]!=0x34||b.wram[0x59]!=0x12||b.wram[0x6c]!=0x78||b.wram[0x6d]!=0x56||b.wram[target]!=(uint8_t)next||b.wram[target+1]!=(uint8_t)(next>>8))return 8;
        if(!nba_setup_sound_prefix_peek(&s,&c)||c.address!=((uint32_t)banks[bank]<<16|0x2140u)||c.kind!=NBA_CODEC_READ||!c.instruction_end)return 9;
        saved=s;
        for(i=0;i<256;i++){if(nba_setup_sound_prefix_accept(&s,(uint8_t)i)||memcmp(&s,&saved,sizeof(s)))return 10;refusals++;}
        count++;
    }
    {
        const uint16_t address[]={0x53,0x637,0x62a,0x62b,0x635,0x7cd,0x7ce,0x7cf,0x62c,0x742};
        const uint32_t pc[]={0x80a1be,0x809fb6,0x80a146,0x80a995,0x80a16d,0x80a1c4,0x80a1ff,0x80a23a,0x80a2ce,0x80a290};
        for(i=0;i<10;i++) {
            uint64_t cpu,master;seed(&b);b.wram[address[i]]=i==9?16:1;if(i==9)b.wram[0x73a]=1;
            if(!run(&b,&s,&entry,NULL,10000,&cpu,&master)||s.boundary_pc!=pc[i]||s.stop!=(i==0?NBA_SOUND_PREFIX_RETURN:NBA_SOUND_PREFIX_UNIMPLEMENTED))return 11;
            if(i==3&&(b.wram[0x62b]!=0||b.wram[0x634]!=127))return 12;
            count++;
        }
    }
    printf("{\"passed\":true,\"source_cases\":%u,\"cloned_cycles\":%u,\"refused_responses\":%u}\n",count,clones,refusals);
    return 0;
}
