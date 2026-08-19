#ifndef NBA_SNES_H
#define NBA_SNES_H

#include "nba_types.h"

typedef struct {
    /* PPU Registers */
    uint8_t inidisp;     /* $2100: Brightness & forced blanking */
    uint8_t obsel;       /* $2101: Sprite configuration */
    uint16_t oamadd;     /* $2102/$2103: OAM word address */
    uint8_t bgmode;      /* $2105: BG mode & priority */
    uint8_t bg1sc;       /* $2107: BG1 screen base & size */
    uint8_t bg2sc;       /* $2108: BG2 screen base & size */
    uint8_t bg3sc;       /* $2109: BG3 screen base & size */
    uint8_t bg4sc;       /* $210A: BG4 screen base & size */
    uint8_t bg12nba;     /* $210B: BG1/BG2 character data base */
    uint8_t bg34nba;     /* $210C: BG3/BG4 character data base */
    uint8_t vmain;       /* $2115: VRAM transfer increment */
    uint16_t vmadd;      /* $2116/$2117: VRAM word address */
    uint8_t cgadd;       /* $2121: CGRAM color index */
    uint8_t tm;          /* $212C: Main screen layer enable */
    uint8_t ts;          /* $212D: Sub screen layer enable */

    /* CPU / Joypad Registers */
    uint8_t nmitimen;    /* $4200: Interrupt / Joypad read enable */
    uint8_t wrio;        /* $4201: Programmable I/O */
    uint8_t memsel;      /* $420D: FastROM control */
    uint16_t joy1;       /* $4218/$4219: Controller 1 */
    uint16_t joy2;       /* $421A/$421B: Controller 2 */

    /* Arithmetic / Multiplier registers ($4202..$420B) */
    uint8_t wrmpya;
    uint8_t wrmpyb;
    uint16_t wrdiv;
    uint8_t wrdvdd;
    uint16_t rddiv;
    uint16_t rdmpy;

    /* Memory buffers */
    uint8_t vram[0x10000];   /* 64 KiB VRAM */
    uint16_t cgram[256];     /* 256 color palette entries (BGR555) */
    uint8_t oam[544];        /* 544 bytes OAM */
    uint8_t wram[0x20000];   /* 128 KiB WRAM */

    /* Direct Page & CPU state */
    uint16_t direct_page;
    uint16_t stack_pointer;
    bool fast_rom_enabled;
    bool screen_enabled;
    uint8_t brightness;
} NbaSnes;

void nba_snes_init(NbaSnes *snes);
void nba_snes_boot_reset(NbaSnes *snes);
void nba_snes_set_joypad(NbaSnes *snes, uint16_t buttons);
void nba_snes_write_cgram(NbaSnes *snes, uint8_t color_index, uint16_t bgr555);
void nba_snes_write_vram_tile8(NbaSnes *snes, uint16_t tile_index, const uint8_t *tile_data, int bpp);

#endif /* NBA_SNES_H */
