#ifndef NBA_ROM_H
#define NBA_ROM_H

#include "nba_types.h"

#define NBA_ROM_EXPECTED_SHA256 "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
#define NBA_ROM_MIN_SIZE (1024 * 1024)

typedef struct {
    uint8_t *data;
    size_t size;
    bool is_headered;
    char title[22];
    uint16_t reset_vector;
    bool is_loaded;
} NbaRom;

bool nba_rom_load_file(NbaRom *rom, const char *filepath);
void nba_rom_free(NbaRom *rom);

uint32_t nba_rom_snes_to_offset(const NbaRom *rom, uint8_t bank, uint16_t addr);
uint8_t nba_rom_read8(const NbaRom *rom, uint8_t bank, uint16_t addr);
uint16_t nba_rom_read16(const NbaRom *rom, uint8_t bank, uint16_t addr);
const uint8_t *nba_rom_ptr(const NbaRom *rom, uint8_t bank, uint16_t addr);

#endif /* NBA_ROM_H */
