#define _CRT_SECURE_NO_WARNINGS
#include "nba_setup_codec_work.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t rom[0x180000];
    uint8_t wram[0x20000];
    uint32_t wmadd;
    uint32_t written;
    bool valid;
} ProbeBus;

static bool hex_address(const char *text, uint32_t *value)
{
    uint32_t result = 0;
    size_t i;
    if (strlen(text) != 6) return false;
    for (i = 0; i < 6; ++i) {
        unsigned char ch = (unsigned char)text[i];
        uint32_t digit;
        if (ch >= '0' && ch <= '9') digit = ch - '0';
        else if (ch >= 'a' && ch <= 'f') digit = ch - 'a' + 10u;
        else if (ch >= 'A' && ch <= 'F') digit = ch - 'A' + 10u;
        else return false;
        result = (result << 4) | digit;
    }
    *value = result;
    return true;
}

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
            b->wram[b->wmadd] = value;
            b->wmadd = (b->wmadd + 1u) & 0x1ffffu;
            ++b->written;
            break;
        case 0x2181: b->wmadd = (b->wmadd & 0x1ff00u) | value; break;
        case 0x2182: b->wmadd = (b->wmadd & 0x100ffu) | ((uint32_t)value << 8); break;
        case 0x2183: b->wmadd = (b->wmadd & 0xffffu) | ((uint32_t)(value & 1u) << 16); break;
        }
    } else b->valid = false;
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

static void synthetic_bus(ProbeBus *bus)
{
    /* One dictionary symbol AB, a literal C, an escaped FE, then termination.
     * These input bytes are synthetic tests, never native evidence. */
    static const uint8_t stream[] = {0x46,0xfb,0,0,4,0xfe,1,0xfd,0x41,0x42,
                                     0xfd,0x43,0xfe,0xfe,0xfe,0};
    memset(bus, 0, sizeof(*bus));
    memcpy(bus->rom + 0x170000, stream, sizeof(stream));
    bus->wram[0x0d] = 0x80; bus->wram[0x0e] = 0xae;
    bus->wram[0x11] = 0x20; bus->wram[0x12] = 0x7f;
    bus->wram[0x561] = 0xc2; bus->wram[0x562] = 0x8f;
    bus->valid = true;
}

static int self_test(ProbeBus *bus)
{
    NbaCodecWorkEntry entry = {0x1234, 0x5678, 0x9abc, 0x1fef, 0x80, 1};
    NbaSetupCodecWork work, copied;
    NbaCodecBusCycle event, again;
    uint64_t cost[4] = {0};
    unsigned variant, cases = 0;
    for (variant = 0; variant < 3; ++variant) {
        NbaCodecWorkEntry invalid = entry;
        invalid.status |= (uint8_t)(8u << variant);
        if (nba_setup_codec_work_begin(&work, &invalid, 1000)) return 3;
        ++cases;
    }
    if (nba_setup_codec_work_begin(&work, &entry, 0) ||
        nba_setup_codec_work_begin(NULL, &entry, 1) ||
        nba_setup_codec_work_begin(&work, NULL, 1)) return 3;
    ++cases;
    for (variant = 0; variant < 4; ++variant) {
        uint64_t cycles = 0;
        synthetic_bus(bus);
        if (variant == 3) bus->wram[0x562] = 0x0f; /* other empty-queue branch */
        if (!nba_setup_codec_work_begin(&work, &entry, 10000)) return 3;
        while (nba_setup_codec_work_peek(&work, &event)) {
            uint8_t value = 0;
            if (variant == 1) {
                /* Relocate the complete continuation after every bus cycle,
                 * including between the read and write of an RMW instruction. */
                memcpy(&copied, &work, sizeof(work));
                memset(&work, 0xa5, sizeof(work));
                memcpy(&work, &copied, sizeof(work));
                if (!nba_setup_codec_work_peek(&work, &again) ||
                    again.kind != event.kind || again.address != event.address ||
                    again.value != event.value || again.instruction_end != event.instruction_end) return 3;
            }
            if (variant == 2 && event.kind == NBA_CODEC_READ && event.address == 0xae800b)
                bus->rom[0x17000b] = 0x44; /* resolve data at the pending read */
            if (event.kind == NBA_CODEC_READ) value = bus_read(bus, event.address);
            if (event.kind == NBA_CODEC_WRITE) bus_write(bus, event.address, event.value);
            ++cycles;
            if (!bus->valid || !nba_setup_codec_work_accept(&work, value)) return 3;
        }
        cost[variant] = cycles;
        if (work.status != NBA_CODEC_WORK_DONE || bus->written != 4 || work.output_size != 4 ||
            work.registers.stack_pointer != entry.stack_pointer || work.registers.status != entry.status ||
            memcmp(bus->wram + 0x12000, variant == 2 ? "ABD\xfe" : "ABC\xfe", 4) != 0) return 3;
        ++cases;
    }
    if (cost[0] != cost[1] || cost[0] != cost[2] || cost[0] != cost[3] + 2u) return 3;
    ++cases;
    synthetic_bus(bus);
    if (!nba_setup_codec_work_begin(&work, &entry, 1)) return 3;
    while (nba_setup_codec_work_peek(&work, &event)) {
        if (event.kind == NBA_CODEC_WRITE) bus_write(bus, event.address, event.value);
        if (!nba_setup_codec_work_accept(&work, event.kind == NBA_CODEC_READ ? bus_read(bus, event.address) : 0)) return 3;
    }
    if (work.status != NBA_CODEC_WORK_LIMIT || work.instructions != 1 || bus->written != 0) return 3;
    ++cases;
    for (variant = 0; variant < 2; ++variant) {
        synthetic_bus(bus);
        if (variant == 0) bus->wram[0x37] = 8; /* unsupported nonempty queue */
        else bus->rom[0x170000] = 0x30; /* FB30 is not silently approximated */
        if (!nba_setup_codec_work_begin(&work, &entry, 10000)) return 3;
        while (nba_setup_codec_work_peek(&work, &event)) {
            if (event.kind == NBA_CODEC_WRITE) bus_write(bus, event.address, event.value);
            if (!nba_setup_codec_work_accept(&work, event.kind == NBA_CODEC_READ ? bus_read(bus, event.address) : 0)) return 3;
        }
        if (work.status != NBA_CODEC_WORK_UNSUPPORTED || bus->written != 0) return 3;
        ++cases;
    }
    printf("PASS: %u codec continuation contract cases\n", cases);
    return 0;
}

int main(int argc, char **argv)
{
    static ProbeBus bus;
    NbaSetupCodecWork work;
    NbaCodecWorkEntry entry = {0, 0, 0, 0x1fef, 0x80, 0x04};
    NbaCodecBusCycle c, again;
    FILE *f;
    FILE *trace = NULL;
    uint32_t source;
    uint16_t queue_cursor = 0;
    uint64_t cycles = 0, master = 0, slow = 0, instructions = 0;
    uint32_t per_pc[0x10000] = {0};
    uint32_t pc;
    bool first = true;
    if (argc == 2 && strcmp(argv[1], "--self-test") == 0) return self_test(&bus);
    if ((argc != 4 && argc != 5 && argc != 6) || !hex_address(argv[2], &source)) {
        fprintf(stderr, "usage: setup_codec_work_probe ROM SIX_HEX_SOURCE OUTPUT_WRAM [TRACE_JSONL [A,X,Y,SP,DB,PS,QUEUE_CURSOR]]\n");
        return 2;
    }
    f = fopen(argv[1], "rb");
    if (f == NULL) return 2;
    if (fread(bus.rom, 1, sizeof(bus.rom), f) != sizeof(bus.rom) || fgetc(f) != EOF) {
        fclose(f); return 2;
    }
    fclose(f);
    if (argc == 6 && !entry_arguments(argv[5], &entry, &queue_cursor)) return 2;
    if (argc >= 5) { trace = fopen(argv[4], "wb"); if (trace == NULL) return 2; }
    /* These are ordinary typed call operands, not captured state. The default
     * initial registers have no resource-dependent effect on work. */
    bus.wram[0x0c] = (uint8_t)source;
    bus.wram[0x0d] = (uint8_t)(source >> 8);
    bus.wram[0x0e] = (uint8_t)(source >> 16);
    bus.wram[0x10] = 0;
    bus.wram[0x11] = 0x20;
    bus.wram[0x12] = 0x7f;
    bus.wram[0x35] = bus.wram[0x37] = (uint8_t)queue_cursor;
    bus.wram[0x36] = bus.wram[0x38] = (uint8_t)(queue_cursor >> 8);
    bus.wram[0x561] = 0xc2;
    bus.wram[0x562] = 0x8f; /* source immediate-dispatch callback, empty queue */
    bus.valid = true;
    if (!nba_setup_codec_work_begin(&work, &entry, 1000000u)) return 2;
    while (nba_setup_codec_work_peek(&work, &c)) {
        uint8_t value = 0;
        /* Peek must be side-effect free, including between a RMW read/write. */
        if (!nba_setup_codec_work_peek(&work, &again) ||
            c.source_pc != again.source_pc || c.address != again.address ||
            c.value != again.value || c.kind != again.kind ||
            c.instruction_end != again.instruction_end) return 3;
        if (c.kind == NBA_CODEC_READ) value = bus_read(&bus, c.address);
        else if (c.kind == NBA_CODEC_WRITE) bus_write(&bus, c.address, c.value);
        ++cycles;
        master += c.master_clocks;
        if (c.master_clocks == 8) ++slow;
        if (c.instruction_end) { ++instructions; ++per_pc[(uint16_t)c.source_pc]; }
        if (trace != NULL && work.pending_index == 0) fprintf(trace,
            "{\"kind\":\"instruction\",\"cycle\":%llu,\"master\":%llu,\"pc\":%u,\"a\":%u,\"x\":%u,\"y\":%u,\"ps\":%u,\"db\":%u,\"sp\":%u}\n",
            (unsigned long long)cycles, (unsigned long long)(master - c.master_clocks), c.source_pc, work.registers.value,
            work.registers.symbol, work.registers.stream_cursor, work.registers.status,
            work.registers.data_bank, work.registers.stack_pointer);
        if (trace != NULL && c.kind == NBA_CODEC_WRITE) fprintf(trace,
            "{\"kind\":\"write\",\"cycle\":%llu,\"pc\":%u,\"address\":%u,\"value\":%u}\n",
            (unsigned long long)cycles, c.source_pc, c.address, c.value);
        if (!bus.valid || !nba_setup_codec_work_accept(&work, value)) break;
    }
    if (trace != NULL && fclose(trace) != 0) return 2;
    f = fopen(argv[3], "wb");
    if (f == NULL) return 2;
    if (fwrite(bus.wram, 1, sizeof(bus.wram), f) != sizeof(bus.wram) || fclose(f) != 0) return 2;
    printf("{\"schema\":1,\"status\":%u,\"bus_valid\":%s,\"source\":%u,\"cycles\":%llu,\"master\":%llu,\"slow\":%llu,\"instructions\":%llu,\"output_bytes\":%u,\"declared_bytes\":%u,\"cursor\":%u,\"sp\":%u,\"wmadd\":%u,\"counts\":{",
           (unsigned)work.status, bus.valid ? "true" : "false", source,
           (unsigned long long)cycles, (unsigned long long)master,
           (unsigned long long)slow, (unsigned long long)instructions,
           bus.written, work.output_size, work.registers.stream_cursor,
           work.registers.stack_pointer, bus.wmadd);
    for (pc = 0; pc < 0x10000u; ++pc) if (per_pc[pc] != 0) {
        printf("%s\"%06X\":%u", first ? "" : ",", pc | 0x800000u, per_pc[pc]);
        first = false;
    }
    printf("}}\n");
    return work.status == NBA_CODEC_WORK_DONE && bus.valid ? 0 : 1;
}
