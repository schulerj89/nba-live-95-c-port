#define _CRT_SECURE_NO_WARNINGS
#include "nba_setup_header_work.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t rom[0x180000];
    uint8_t wram[0x20000];
    uint32_t wmadd;
    uint32_t written;
    uint32_t payload_written;
    uint8_t dma[16], vram[65536], cgram[512];
    uint16_t vmadd, cgadd;
    uint8_t vmain, brightness;
    uint32_t dma_bytes, dma_jobs;
    FILE *dma_trace;
    uint64_t cpu_cycle;
    bool valid;
} ProbeBus;

static uint8_t bus_read(ProbeBus *b, uint32_t address)
{
    uint8_t bank = (uint8_t)(address >> 16);
    uint16_t low = (uint16_t)address;
    if (bank == 0x7e || bank == 0x7f) return b->wram[address & 0x1ffffu];
    if ((bank & 0x40u) == 0 && low < 0x2000u) return b->wram[low];
    if (low >= 0x8000u) {
        uint32_t offset = ((uint32_t)(bank & 0x7fu) << 15) | (low & 0x7fffu);
        if (offset < sizeof(b->rom)) return b->rom[offset];
    }
    b->valid = false;
    return 0;
}

static void ppu_write(ProbeBus *b, uint16_t low, uint8_t value)
{
    uint16_t increment = (b->vmain & 3u) == 0 ? 1u : (b->vmain & 3u) == 1 ? 32u : 128u;
    switch (low) {
    case 0x2100: b->brightness=value; break;
    case 0x2115: b->vmain = value; if (value & 0x0cu) b->valid = false; break;
    case 0x2116: b->vmadd = (uint16_t)((b->vmadd & 0xff00u) | value); break;
    case 0x2117: b->vmadd = (uint16_t)((b->vmadd & 255u) | ((uint16_t)value << 8)); break;
    case 0x2118:
    case 0x2119:
        b->vram[((uint32_t)b->vmadd * 2u + (low & 1u)) & 65535u] = value;
        if ((low == 0x2119) == ((b->vmain & 128u) != 0)) b->vmadd += increment;
        break;
    case 0x2121: b->cgadd = (uint16_t)value * 2u; break;
    case 0x2122:
        b->cgram[b->cgadd] = (uint8_t)(value & ((b->cgadd & 1u) ? 127u : 255u));
        b->cgadd = (b->cgadd + 1u) & 511u; break;
    default: b->valid = false;
    }
}

static void dma_run(ProbeBus *b, uint8_t mask)
{
    uint32_t i, size = b->dma[5] | ((uint32_t)b->dma[6] << 8);
    uint16_t address = b->dma[2] | ((uint16_t)b->dma[3] << 8);
    uint32_t bank = (uint32_t)b->dma[4] << 16;
    if (mask != 2 || (b->dma[0] != 0 && b->dma[0] != 1 && b->dma[0] != 8)) { b->valid = false; return; }
    if (!size) size = 65536;
    for (i = 0; i < size; ++i) {
        uint16_t dest = 0x2100u | (uint16_t)(b->dma[1] + ((b->dma[0] & 7u) == 1 ? i & 1u : 0u));
        uint8_t value = bus_read(b, bank | address);
        if (b->dma_trace) fprintf(b->dma_trace,
            "{\"job\":%u,\"index\":%u,\"cycle\":%llu,\"source\":%u,\"address\":%u,\"value\":%u}\n",
            b->dma_jobs, i, (unsigned long long)b->cpu_cycle, bank | address, dest, value);
        ppu_write(b, dest, value);
        if (!(b->dma[0] & 8u)) ++address;
    }
    b->dma[2] = (uint8_t)address; b->dma[3] = (uint8_t)(address >> 8);
    b->dma[5] = b->dma[6] = 0;
    b->dma_bytes += size; ++b->dma_jobs;
}

static void bus_write(ProbeBus *b, uint32_t address, uint8_t value)
{
    uint8_t bank = (uint8_t)(address >> 16);
    uint16_t low = (uint16_t)address;
    if (bank == 0x7e || bank == 0x7f) b->wram[address & 0x1ffffu] = value;
    else if ((bank & 0x40u) == 0 && low < 0x2000u) b->wram[low] = value;
    else if ((bank & 0x40u) == 0 && low >= 0x2180u && low <= 0x2183u) {
        switch (low) {
        case 0x2180:
            if (b->written >= 0x20000u) { b->valid = false; return; }
            if (b->wmadd >= 0x12000) ++b->payload_written;
            b->wram[b->wmadd] = value;
            b->wmadd = (b->wmadd + 1u) & 0x1ffffu;
            ++b->written;
            break;
        case 0x2181: b->wmadd = (b->wmadd & 0x1ff00u) | value; break;
        case 0x2182: b->wmadd = (b->wmadd & 0x100ffu) | ((uint32_t)value << 8); break;
        case 0x2183: b->wmadd = (b->wmadd & 0xffffu) | ((uint32_t)(value & 1u) << 16); break;
        }
    } else if ((bank & 0x40u) == 0 && low >= 0x4310 && low <= 0x431f) b->dma[low - 0x4310] = value;
    else if ((bank & 0x40u) == 0 && low == 0x420b) dma_run(b, value);
    else if ((bank & 0x40u) == 0 && ((low >= 0x2115 && low <= 0x2122) || low==0x2100)) ppu_write(b, low, value);
    else b->valid = false;
}

static bool entry_arguments(const char *text, NbaCodecWorkEntry *entry, uint16_t *queue_cursor)
{
    uint32_t values[7] = {0};
    unsigned i;
    for (i = 0; i < 7; ++i) {
        uint32_t limit = i < 4 || i == 6 ? 0xffffu : 0xffu;
        if (*text < '0' || *text > '9') return false;
        while (*text >= '0' && *text <= '9') {
            uint32_t digit = (uint32_t)(*text++ - '0');
            if (values[i] > (limit - digit) / 10u) return false;
            values[i] = values[i] * 10u + digit;
        }
        if (i < 6) { if (*text++ != ',') return false; }
        else if (*text != 0) return false;
    }
    entry->value = (uint16_t)values[0]; entry->symbol = (uint16_t)values[1];
    entry->stream_cursor = (uint16_t)values[2]; entry->stack_pointer = (uint16_t)values[3];
    entry->data_bank = (uint8_t)values[4]; entry->status = (uint8_t)values[5];
    *queue_cursor = (uint16_t)values[6];
    return true;
}

static void initial_bus(ProbeBus *bus, const NbaCodecWorkEntry *entry, uint16_t cursor)
{
    uint16_t graphics = (uint16_t)(bus->rom[0xb8c6] & 0xf0u) << 8;
    uint16_t map = (uint16_t)(bus->rom[0xb8c4] & 0xfcu) << 8;
    uint16_t header = (uint16_t)(bus->rom[0xb8c7] & 15u) << 12;
    /* Source caller operands: $81:B8C2 PPU layout decoded by $80:E95B
     * ($E986/$E9B8/$E9CF), selector34 from $81:D00A/$D00D. No snapshot input. */
    memset((uint8_t *)bus + offsetof(ProbeBus,wram), 0, sizeof(*bus) - offsetof(ProbeBus,wram));
    bus->wram[0x7e6a]=(uint8_t)graphics; bus->wram[0x7e6b]=(uint8_t)(graphics>>8);
    bus->wram[0x7e6c]=(uint8_t)map; bus->wram[0x7e6d]=(uint8_t)(map>>8);
    bus->wram[0x7e6e]=(uint8_t)header; bus->wram[0x7e6f]=(uint8_t)(header>>8);
    bus->wram[0x7e66]=0; bus->wram[0x7e67]=(uint8_t)((bus->rom[0xb8c6]&15u)<<4);
    memset(bus->wram+0x15d9,0xff,10);
    bus->wram[0x16b1] = 34;
    bus->wram[0x35] = bus->wram[0x37] = (uint8_t)cursor;
    bus->wram[0x36] = bus->wram[0x38] = (uint8_t)(cursor >> 8);
    bus->wram[0x561] = 0xc2; bus->wram[0x562] = 0x8f;
    bus->wram[(uint16_t)(entry->stack_pointer + 1u)] = 0x17;
    bus->wram[(uint16_t)(entry->stack_pointer + 2u)] = 0xd0;
    bus->wram[(uint16_t)(entry->stack_pointer + 3u)] = 0x81;
    bus->vmain = 0x80;
    bus->valid = true;
}

static int self_test(ProbeBus *bus)
{
    NbaCodecWorkEntry entry={0,0,0,0x1fef,0x80,4};
    NbaSetupHeaderWork work,copy;
    NbaCodecBusCycle event,again;
    uint64_t hashes[2]={0},costs[4]={0};
    uint8_t selector_zero[512];
    unsigned variant,cases=0;
    for (variant=0;variant<3;++variant) {
        NbaCodecWorkEntry bad=entry;bad.status|=(uint8_t)(8u<<variant);
        if (nba_setup_header_work_begin(&work,&bad,10000)) { fprintf(stderr,"header self-test failed at source line %u\n",(unsigned)__LINE__); return 3; }
        ++cases;
    }
    if (nba_setup_header_work_begin(NULL,&entry,1) || nba_setup_header_work_begin(&work,NULL,1) ||
        nba_setup_header_work_begin(&work,&entry,0)) { fprintf(stderr,"header self-test failed at source line %u\n",(unsigned)__LINE__); return 3; }
    ++cases;
    for (variant=0;variant<4;++variant) {
        uint64_t hash=1469598103934665603ull,cpu=0;
        initial_bus(bus,&entry,0);
        if (variant>=2) bus->wram[0x16b1]=(uint8_t)(variant==2?0:35);
        if (!nba_setup_header_work_begin(&work,&entry,1000000)) { fprintf(stderr,"header self-test failed at source line %u\n",(unsigned)__LINE__); return 3; }
        while (nba_setup_header_work_peek(&work,&event)) {
            uint8_t value=0;
            if (variant==1) {
                memcpy(&copy,&work,sizeof(work));memset(&work,0xa5,sizeof(work));memcpy(&work,&copy,sizeof(work));
                if (!nba_setup_header_work_peek(&work,&again) || again.kind!=event.kind ||
                    again.address!=event.address || again.value!=event.value || again.source_pc!=event.source_pc) { fprintf(stderr,"header self-test failed at source line %u\n",(unsigned)__LINE__); return 3; }
            }
            bus->cpu_cycle=++cpu;
            if (event.kind==NBA_CODEC_READ) value=bus_read(bus,event.address);
            if (event.kind==NBA_CODEC_WRITE) bus_write(bus,event.address,event.value);
            hash=(hash ^ event.address)*1099511628211ull;
            hash=(hash ^ event.source_pc ^ event.value ^ value ^ event.master_clocks ^ event.kind)*1099511628211ull;
            if (!bus->valid || !nba_setup_header_work_accept(&work,value)) { fprintf(stderr,"header self-test failed at source line %u\n",(unsigned)__LINE__); return 3; }
        }
        if (work.bus.status!=NBA_CODEC_WORK_DONE ||
            work.bus.registers.stack_pointer!=entry.stack_pointer) { fprintf(stderr,"header self-test failed at source line %u\n",(unsigned)__LINE__); return 3; }
        if (memcmp(bus->wram+0x15d9,"\0\0\0\0\0\0\0\0\0\0",10) || bus->brightness!=0x8f) return 3;
        if (variant<2) hashes[variant]=hash;
        if (variant==2) memcpy(selector_zero,bus->cgram,sizeof(selector_zero));
        if (variant==3 && memcmp(selector_zero,bus->cgram,sizeof(selector_zero))) { fprintf(stderr,"header self-test failed at source line %u\n",(unsigned)__LINE__); return 3; }
        costs[variant]=cpu;++cases;
    }
    if (hashes[0]!=hashes[1] || costs[0]!=costs[1] || costs[3]!=costs[2]+8u) { fprintf(stderr,"header self-test failed at source line %u\n",(unsigned)__LINE__); return 3; }
    ++cases;
    for (variant=0;variant<2;++variant) {
        initial_bus(bus,&entry,0);
        if (variant==1) bus->wram[0x562]=15; /* queued fill is explicitly outside this component */
        if (!nba_setup_header_work_begin(&work,&entry,variant?1000000:50)) { fprintf(stderr,"header self-test failed at source line %u\n",(unsigned)__LINE__); return 3; }
        while (nba_setup_header_work_peek(&work,&event)) {
            if (event.kind==NBA_CODEC_WRITE) bus_write(bus,event.address,event.value);
            if (!nba_setup_header_work_accept(&work,event.kind==NBA_CODEC_READ?bus_read(bus,event.address):0)) { fprintf(stderr,"header self-test failed at source line %u\n",(unsigned)__LINE__); return 3; }
        }
        if (work.bus.status!=(variant?NBA_CODEC_WORK_UNSUPPORTED:NBA_CODEC_WORK_LIMIT)) { fprintf(stderr,"header self-test failed at source line %u\n",(unsigned)__LINE__); return 3; }
        if (!variant && work.bus.instructions!=50) { fprintf(stderr,"header self-test failed at source line %u\n",(unsigned)__LINE__); return 3; }
        ++cases;
    }
    printf("PASS: %u header continuation contract cases\n",cases);
    return 0;
}

int main(int argc, char **argv)
{
    static ProbeBus bus;
    NbaSetupHeaderWork work;
    NbaCodecWorkEntry entry = {0, 0, 0, 0x1fef, 0x80, 0x04};
    NbaCodecBusCycle c, again;
    FILE *f;
    FILE *trace = NULL;
    uint16_t queue_cursor = 0;
    uint64_t cycles = 0, master = 0, slow = 0, instructions = 0;
    uint32_t per_pc[0x10000] = {0};
    uint32_t pc;
    bool first = true;
    if (argc==3 && strcmp(argv[1],"--self-test")==0) {
        f=fopen(argv[2],"rb");if (!f) return 2;
        if (fread(bus.rom,1,sizeof(bus.rom),f)!=sizeof(bus.rom) || fgetc(f)!=EOF) { fclose(f);return 2; }
        fclose(f);return self_test(&bus);
    }
    if (argc != 3 && argc != 4 && argc != 5) {
        fprintf(stderr, "usage: setup_header_work_probe ROM OUTPUT_WRAM [TRACE_JSONL [A,X,Y,SP,DB,PS,QUEUE_CURSOR]]\n");
        return 2;
    }
    f = fopen(argv[1], "rb");
    if (f == NULL) return 2;
    if (fread(bus.rom, 1, sizeof(bus.rom), f) != sizeof(bus.rom) || fgetc(f) != EOF) {
        fclose(f); return 2;
    }
    fclose(f);
    if (argc == 5 && !entry_arguments(argv[4], &entry, &queue_cursor)) return 2;
    if (argc >= 4) { trace = fopen(argv[3], "wb"); if (trace == NULL) return 2; }
    initial_bus(&bus, &entry, queue_cursor);
    if (argc >= 4) {
        char path[4096];
        if (snprintf(path, sizeof(path), "%s.dma.jsonl", argv[3]) >= sizeof(path)) return 2;
        bus.dma_trace = fopen(path, "wb"); if (!bus.dma_trace) return 2;
    }
    bus.valid = true;
    if (!nba_setup_header_work_begin(&work, &entry, 1000000u)) return 2;
    while (nba_setup_header_work_peek(&work, &c)) {
        const NbaSetupCodecWork *current = &work.bus;
        bus.cpu_cycle = cycles + 1;
        uint8_t value = 0;
        /* Peek must be side-effect free, including between a RMW read/write. */
        if (!nba_setup_header_work_peek(&work, &again) ||
            c.source_pc != again.source_pc || c.address != again.address ||
            c.value != again.value || c.kind != again.kind ||
            c.instruction_end != again.instruction_end) return 3;
        if (c.kind == NBA_CODEC_READ) value = bus_read(&bus, c.address);
        else if (c.kind == NBA_CODEC_WRITE) bus_write(&bus, c.address, c.value);
        ++cycles;
        master += c.master_clocks;
        if (c.master_clocks == 8) ++slow;
        if (c.instruction_end) { ++instructions; ++per_pc[(uint16_t)c.source_pc]; }
        if (trace != NULL && current->pending_index == 0) fprintf(trace,
            "{\"kind\":\"instruction\",\"cycle\":%llu,\"master\":%llu,\"pc\":%u,\"a\":%u,\"x\":%u,\"y\":%u,\"ps\":%u,\"db\":%u,\"sp\":%u}\n",
            (unsigned long long)cycles, (unsigned long long)(master - c.master_clocks), c.source_pc, current->registers.value,
            current->registers.symbol, current->registers.stream_cursor, current->registers.status,
            current->registers.data_bank, current->registers.stack_pointer);
        if (trace != NULL && c.kind == NBA_CODEC_WRITE) fprintf(trace,
            "{\"kind\":\"write\",\"cycle\":%llu,\"pc\":%u,\"address\":%u,\"value\":%u}\n",
            (unsigned long long)cycles, c.source_pc, c.address, c.value);
        if (!bus.valid || !nba_setup_header_work_accept(&work, value)) break;
    }
    if (bus.dma_trace && fclose(bus.dma_trace) != 0) return 2;
    if (trace != NULL && fclose(trace) != 0) return 2;
    f = fopen(argv[2], "wb");
    if (f == NULL) return 2;
    if (fwrite(bus.wram, 1, sizeof(bus.wram), f) != sizeof(bus.wram) || fclose(f) != 0) return 2;
    printf("{\"schema\":1,\"status\":%u,\"bus_valid\":%s,\"return_pc\":%u,\"cycles\":%llu,\"master\":%llu,\"slow\":%llu,\"instructions\":%llu,\"dma_bytes\":%u,\"dma_jobs\":%u,\"cursor\":%u,\"sp\":%u,\"wmadd\":%u,\"counts\":{",
           (unsigned)work.bus.status, bus.valid ? "true" : "false", 0x80ef1a,
           (unsigned long long)cycles, (unsigned long long)master,
           (unsigned long long)slow, (unsigned long long)instructions,
           bus.dma_bytes, bus.dma_jobs, work.bus.registers.stream_cursor,
           work.bus.registers.stack_pointer, bus.wmadd);
    for (pc = 0; pc < 0x10000u; ++pc) if (per_pc[pc] != 0) {
        printf("%s\"%06X\":%u", first ? "" : ",", pc | 0x800000u, per_pc[pc]);
        first = false;
    }
    printf("}}\n");
    return work.bus.status == NBA_CODEC_WORK_DONE && bus.valid ? 0 : 1;
}
