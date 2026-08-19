#include "nba_rom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Offset/Address/Size: 0x007FC0 | $00:FFC0 | size: 0x40
 * Purpose: Loads, parses, and validates the SNES LoROM cartridge binary from disk.
 */
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
    size_t snes_header = header_offset + SNES_LOROM_HEADER_OFFSET;

    if (snes_header + 64 <= rom->size) {
        memcpy(rom->title, &rom->data[snes_header], 21);
        rom->title[21] = '\0';
        rom->reset_vector = (uint16_t)(rom->data[header_offset + SNES_LOROM_RESET_VECTOR_OFF] |
                                      (rom->data[header_offset + SNES_LOROM_RESET_VECTOR_OFF + 1] << 8));
        printf("[ROM] Loaded successfully: \"%s\" (Reset: 0x%04X, Headered: %s, Size: %zu KiB)\n",
               rom->title, rom->reset_vector, rom->is_headered ? "Yes" : "No", rom->size / 1024);
    }

    rom->is_loaded = true;
    return true;
}

/**
 * Offset/Address/Size: N/A | Host Memory | size: N/A
 * Purpose: Releases allocated ROM memory buffer and resets state.
 */
void nba_rom_free(NbaRom *rom) {
    if (rom && rom->data) {
        free(rom->data);
        rom->data = NULL;
        rom->size = 0;
        rom->is_loaded = false;
    }
}

/**
 * Offset/Address/Size: 0x000000 | LoROM Memory Map ($00..$FF:$8000..$FFFF) | size: 0x180000
 * Purpose: Translates a 24-bit SNES LoROM bank:address tuple into a linear file byte offset.
 */
uint32_t nba_rom_snes_to_offset(const NbaRom *rom, uint8_t bank, uint16_t addr) {
    uint32_t base = (rom && rom->is_headered) ? 512 : 0;
    uint32_t raw_offset = (uint32_t)((bank & 0x7F) * SNES_LOROM_BANK_SIZE + (addr & 0x7FFF));
    return base + raw_offset;
}

/**
 * Offset/Address/Size: N/A | 8-bit LoROM Reader | size: 1 byte
 * Purpose: Reads an 8-bit unsigned integer from SNES LoROM memory address space.
 */
uint8_t nba_rom_read8(const NbaRom *rom, uint8_t bank, uint16_t addr) {
    if (!rom || !rom->data) return 0xFF;
    uint32_t off = nba_rom_snes_to_offset(rom, bank, addr);
    if (off >= rom->size) return 0xFF;
    return rom->data[off];
}

/**
 * Offset/Address/Size: N/A | 16-bit LoROM Little-Endian Reader | size: 2 bytes
 * Purpose: Reads a 16-bit little-endian unsigned integer from SNES LoROM memory address space.
 */
uint16_t nba_rom_read16(const NbaRom *rom, uint8_t bank, uint16_t addr) {
    uint8_t lo = nba_rom_read8(rom, bank, addr);
    uint8_t hi = nba_rom_read8(rom, bank, (uint16_t)(addr + 1));
    return (uint16_t)(lo | (hi << 8));
}

/**
 * Offset/Address/Size: N/A | Memory Pointer Resolver | size: N/A
 * Purpose: Resolves a direct host pointer to a given SNES LoROM memory address.
 */
const uint8_t *nba_rom_ptr(const NbaRom *rom, uint8_t bank, uint16_t addr) {
    if (!rom || !rom->data) return NULL;
    uint32_t off = nba_rom_snes_to_offset(rom, bank, addr);
    if (off >= rom->size) return NULL;
    return &rom->data[off];
}
