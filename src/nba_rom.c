#include "nba_rom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool nba_rom_load_file(NbaRom *rom, const char *filepath) {
    if (!rom || !filepath) return false;
    memset(rom, 0, sizeof(NbaRom));

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        printf("[ROM] Unable to open ROM file: %s\n", filepath);
        return false;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz < NBA_ROM_MIN_SIZE) {
        printf("[ROM] File too small (%ld bytes, minimum %d bytes)\n", sz, NBA_ROM_MIN_SIZE);
        fclose(f);
        return false;
    }

    rom->data = (uint8_t *)malloc((size_t)sz);
    if (!rom->data) {
        printf("[ROM] Out of memory allocating %ld bytes\n", sz);
        fclose(f);
        return false;
    }

    size_t read_bytes = fread(rom->data, 1, (size_t)sz, f);
    fclose(f);

    if (read_bytes != (size_t)sz) {
        printf("[ROM] Read error (%zu of %ld bytes read)\n", read_bytes, sz);
        free(rom->data);
        rom->data = NULL;
        return false;
    }

    rom->size = (size_t)sz;
    rom->is_headered = ((sz % 1024) == 512);

    size_t header_offset = rom->is_headered ? 512 : 0;
    size_t snes_header = header_offset + 0x7FC0;

    if (snes_header + 64 <= rom->size) {
        memcpy(rom->title, &rom->data[snes_header], 21);
        rom->title[21] = '\0';
        rom->reset_vector = (uint16_t)(rom->data[header_offset + 0x7FFC] |
                                      (rom->data[header_offset + 0x7FFD] << 8));
        printf("[ROM] Loaded successfully: \"%s\" (Reset: 0x%04X, Headered: %s, Size: %zu KiB)\n",
               rom->title, rom->reset_vector, rom->is_headered ? "Yes" : "No", rom->size / 1024);
    }

    rom->is_loaded = true;
    return true;
}

void nba_rom_free(NbaRom *rom) {
    if (rom && rom->data) {
        free(rom->data);
        rom->data = NULL;
        rom->size = 0;
        rom->is_loaded = false;
    }
}

uint32_t nba_rom_snes_to_offset(const NbaRom *rom, uint8_t bank, uint16_t addr) {
    uint32_t base = rom->is_headered ? 512 : 0;
    uint32_t raw_offset = (uint32_t)((bank & 0x7F) * 0x8000 + (addr & 0x7FFF));
    return base + raw_offset;
}

uint8_t nba_rom_read8(const NbaRom *rom, uint8_t bank, uint16_t addr) {
    if (!rom || !rom->data) return 0xFF;
    uint32_t off = nba_rom_snes_to_offset(rom, bank, addr);
    if (off >= rom->size) return 0xFF;
    return rom->data[off];
}

uint16_t nba_rom_read16(const NbaRom *rom, uint8_t bank, uint16_t addr) {
    uint8_t lo = nba_rom_read8(rom, bank, addr);
    uint8_t hi = nba_rom_read8(rom, bank, (uint16_t)(addr + 1));
    return (uint16_t)(lo | (hi << 8));
}

const uint8_t *nba_rom_ptr(const NbaRom *rom, uint8_t bank, uint16_t addr) {
    if (!rom || !rom->data) return NULL;
    uint32_t off = nba_rom_snes_to_offset(rom, bank, addr);
    if (off >= rom->size) return NULL;
    return &rom->data[off];
}
