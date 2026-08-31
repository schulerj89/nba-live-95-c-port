#define _CRT_SECURE_NO_WARNINGS
#include "nba_setup_sound_init.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { uint8_t rom[0x180000], wram[0x20000]; bool valid; } Bus;

static uint8_t read_bus(Bus *b, uint32_t address)
{
    uint8_t bank = (uint8_t)(address >> 16); uint16_t low = (uint16_t)address;
    if (bank == 0x7e || bank == 0x7f) return b->wram[address & 0x1ffffu];
    if ((bank & 0x40u) == 0 && low < 0x2000u) return b->wram[low];
    if (low >= 0x8000u) {
        uint32_t offset = ((uint32_t)(bank & 0x7fu) << 15) | (low & 0x7fffu);
        if (offset < sizeof(b->rom)) return b->rom[offset];
    }
    b->valid = false; return 0;
}

static void write_bus(Bus *b, uint32_t address, uint8_t value)
{
    uint8_t bank = (uint8_t)(address >> 16); uint16_t low = (uint16_t)address;
    if (bank == 0x7e || bank == 0x7f) b->wram[address & 0x1ffffu] = value;
    else if ((bank & 0x40u) == 0 && low < 0x2000u) b->wram[low] = value;
    else b->valid = false;
}

static bool file_read(const char *name, void *buffer, size_t size)
{
    FILE *f = fopen(name, "rb"); bool ok;
    if (!f) return false;
    ok = fread(buffer, 1, size, f) == size && fgetc(f) == EOF;
    fclose(f); return ok;
}

static bool number(const char **cursor, uint32_t maximum, uint32_t *out, bool last)
{
    const char *p = *cursor; uint32_t result = 0;
    if (*p < '0' || *p > '9') return false;
    do { uint32_t digit = (uint32_t)(*p++ - '0');
        if (result > maximum / 10u || (result == maximum / 10u && digit > maximum % 10u)) return false;
        result = result * 10u + digit;
    } while (*p >= '0' && *p <= '9');
    if (last ? *p != 0 : *p != ',') return false;
    *cursor = p + (last ? 0 : 1); *out = result; return true;
}

static bool entry_parse(const char *text, NbaCodecWorkEntry *entry)
{
    uint32_t fields[6]; unsigned i;
    for (i = 0; i < 6; ++i) if (!number(&text, i < 4 ? 65535u : 255u, &fields[i], i == 5)) return false;
    entry->value=(uint16_t)fields[0];entry->symbol=(uint16_t)fields[1];entry->stream_cursor=(uint16_t)fields[2];
    entry->stack_pointer=(uint16_t)fields[3];entry->data_bank=(uint8_t)fields[4];entry->status=(uint8_t)fields[5];
    return true;
}

static bool run(Bus *bus, NbaSetupSoundInit *s, const NbaCodecWorkEntry *entry, FILE *trace,
                uint64_t limit, uint64_t *cpu, uint64_t *master)
{
    NbaCodecBusCycle c; uint64_t prior = 0;
    *cpu = *master = 0;
    if (!nba_setup_sound_init_begin(s, entry, limit)) return false;
    while (nba_setup_sound_init_peek(s, &c)) {
        if (prior != s->bus.instructions) {
            const NbaCodecWorkEntry *r=&s->bus.registers;
            if (trace) fprintf(trace,"{\"kind\":\"instruction\",\"pc\":%u,\"cycle\":%llu,\"master\":%llu,\"a\":%u,\"x\":%u,\"y\":%u,\"sp\":%u,\"db\":%u,\"ps\":%u}\n",c.source_pc,*cpu+1,*master,r->value,r->symbol,r->stream_cursor,r->stack_pointer,r->data_bank,r->status);
            prior=s->bus.instructions;
        }
        if (s->stop == NBA_SOUND_INIT_SPC_RESPONSE) break;
        ++*cpu; *master += c.master_clocks;
        if (trace) fprintf(trace,"{\"kind\":\"bus\",\"access\":%u,\"pc\":%u,\"cycle\":%llu,\"master\":%llu,\"address\":%u,\"value\":%u,\"end\":%u}\n",(unsigned)c.kind,c.source_pc,*cpu,*master,c.address,c.kind==NBA_CODEC_READ?read_bus(bus,c.address):c.value,c.instruction_end?1:0);
        if (c.kind==NBA_CODEC_WRITE) write_bus(bus,c.address,c.value);
        if (!nba_setup_sound_init_accept(s,c.kind==NBA_CODEC_READ?read_bus(bus,c.address):0)) return false;
    }
    return bus->valid;
}

static int self_test(Bus *bus)
{
    NbaSetupSoundInit s,saved;NbaCodecBusCycle pending;uint64_t cpu,master,reference_cpu=0,reference_master=0;
    const uint8_t banks[]={0x80,0x82,0x7e,0x7f};unsigned i,j;
    NbaCodecWorkEntry entry={0x1234,0x5678,0x9abc,0x1ff0,0x80,0x01};
    for(i=0;i<sizeof(banks);++i) {
        memset(bus->wram,0xa5,sizeof(bus->wram));bus->valid=true;
        bus->wram[0x53]=0;bus->wram[0x33]=0x82;bus->wram[0x34]=0x45;entry.data_bank=banks[i];
        if(!run(bus,&s,&entry,NULL,10000,&cpu,&master)||s.stop!=NBA_SOUND_INIT_SPC_RESPONSE||s.boundary_pc!=0x80aacd)return 1;
        if(!nba_setup_sound_init_peek(&s,&pending)||pending.address!=0x802140||!pending.instruction_end)return 2;
        if(bus->wram[0x53]!=254||bus->wram[0x5a]!=0x82||bus->wram[0x5b]!=0x45)return 3;
        for(j=0x62a;j<=0x7ed;++j) {
            uint8_t expected=(j==0x7cd||j==0x7ce||j==0x7cf||j==0x749)?255:(j==0x62d)?128:(j==0x62f)?127:0;
            if(bus->wram[j]!=expected)return 4;
        }
        if(bus->wram[0x629]!=0xa5||bus->wram[0x7ee]!=0xa5)return 5;
        saved=s;if(nba_setup_sound_init_accept(&s,0)||nba_setup_sound_init_accept(&s,255)||memcmp(&saved,&s,sizeof(s)))return 6;
        if(i==0){reference_cpu=cpu;reference_master=master;}
        if(i==1&&(cpu!=reference_cpu||master!=reference_master))return 7;
    }
    entry.data_bank=0x80;entry.status=0x09;if(nba_setup_sound_init_begin(&s,&entry,1000))return 8;entry.status=1;
    if(nba_setup_sound_init_begin(NULL,&entry,1000)||nba_setup_sound_init_begin(&s,NULL,1000)||nba_setup_sound_init_begin(&s,&entry,0))return 9;
    bus->valid=true;if(!run(bus,&s,&entry,NULL,5,&cpu,&master)||s.bus.status!=NBA_CODEC_WORK_LIMIT)return 10;
    puts("PASS: source clear bounds, derived bank/sentinels, nested guard underflow, four DB paths, immutable unresolved SPC read, limit and invalid entry");return 0;
}

int main(int argc,char **argv)
{
    static Bus bus; NbaSetupSoundInit s;NbaCodecWorkEntry entry;FILE *trace,*out;uint64_t cpu,master;
    if(argc==3&&!strcmp(argv[1],"--self-test")) {
        if(!file_read(argv[2],bus.rom,sizeof(bus.rom)))return 2;
        return self_test(&bus);
    }
    /* Explicit isolated differential input; never used by a normal game run. */
    if(argc!=7||!entry_parse(argv[3],&entry)||!file_read(argv[1],bus.rom,sizeof(bus.rom))||!file_read(argv[2],bus.wram,sizeof(bus.wram)))return 2;
    if(strcmp(argv[6],"isolated-component-differential"))return 2;
    trace=fopen(argv[4],"wb");if(!trace)return 2;bus.valid=true;
    if(!run(&bus,&s,&entry,trace,10000,&cpu,&master)){fclose(trace);return 3;}fclose(trace);
    out=fopen(argv[5],"wb");if(!out)return 2;
    if(fwrite(bus.wram,1,sizeof(bus.wram),out)!=sizeof(bus.wram)){fclose(out);return 3;}fclose(out);
    printf("{\"schema\":1,\"stop\":%u,\"boundary_pc\":%u,\"cycles\":%llu,\"master\":%llu,\"instructions\":%llu,\"status\":%u,\"a\":%u,\"x\":%u,\"y\":%u,\"sp\":%u,\"db\":%u,\"ps\":%u}\n",(unsigned)s.stop,s.boundary_pc,cpu,master,s.bus.instructions,(unsigned)s.bus.status,s.bus.registers.value,s.bus.registers.symbol,s.bus.registers.stream_cursor,s.bus.registers.stack_pointer,s.bus.registers.data_bank,s.bus.registers.status);
    return 0;
}
