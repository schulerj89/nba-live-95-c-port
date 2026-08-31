/* Independent endpoint projection against native C682 registers. The frozen
 * probe's bus adapter is reused; native WRAM and expected exits are never input.
 * This is an audit tool only, outside the production source manifest. */
#define main frozen_probe_main
#include "../build/codec-audit-v1/source/tools/setup_codec_work_probe.c"
#undef main

int main(int argc, char **argv)
{
    static ProbeBus bus;
    NbaCodecWorkEntry entry;
    NbaSetupCodecWork work;
    NbaCodecBusCycle cycle;
    uint32_t source;
    uint16_t cursor;
    FILE *rom;
    if (argc != 4 || !hex_address(argv[2], &source) ||
        !entry_arguments(argv[3], &entry, &cursor)) return 2;
    rom = fopen(argv[1], "rb");
    if (!rom) return 2;
    if (fread(bus.rom, 1, sizeof(bus.rom), rom) != sizeof(bus.rom) || fgetc(rom) != EOF) return 2;
    fclose(rom);
    bus.wram[0x0c] = (uint8_t)source;
    bus.wram[0x0d] = (uint8_t)(source >> 8);
    bus.wram[0x0e] = (uint8_t)(source >> 16);
    bus.wram[0x11] = 0x20; bus.wram[0x12] = 0x7f;
    bus.wram[0x35] = bus.wram[0x37] = (uint8_t)cursor;
    bus.wram[0x36] = bus.wram[0x38] = (uint8_t)(cursor >> 8);
    bus.wram[0x561] = 0xc2; bus.wram[0x562] = 0x8f;
    bus.valid = true;
    if (!nba_setup_codec_work_begin(&work, &entry, 1000000)) return 2;
    while (nba_setup_codec_work_peek(&work, &cycle)) {
        uint8_t value = 0;
        if (cycle.kind == NBA_CODEC_READ) value = bus_read(&bus, cycle.address);
        if (cycle.kind == NBA_CODEC_WRITE) bus_write(&bus, cycle.address, cycle.value);
        if (!bus.valid || !nba_setup_codec_work_accept(&work, value)) return 3;
    }
    if (work.status != NBA_CODEC_WORK_DONE) return 3;
    printf("{\"a\":%u,\"x\":%u,\"y\":%u,\"sp\":%u,\"db\":%u,\"ps\":%u}\n",
           work.registers.value, work.registers.symbol, work.registers.stream_cursor,
           work.registers.stack_pointer, work.registers.data_bank, work.registers.status);
    return 0;
}
