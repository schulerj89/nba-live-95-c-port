#include "nba_snes.h"
#include <stdio.h>
#include <string.h>

void nba_snes_init(NbaSnes *snes) {
    if (!snes) return;
    memset(snes, 0, sizeof(NbaSnes));
    nba_snes_boot_reset(snes);
}

void nba_snes_boot_reset(NbaSnes *snes) {
    if (!snes) return;

    /* Replicates 65816 boot sequence from 80:8020 */
    printf("[SNES] Executing hardware reset lifecycle (LoROM $80:8020)...\n");

    /* 1. CPU stack & direct page */
    snes->stack_pointer = 0x1FFF;
    snes->direct_page = 0x0000;

    /* 2. Clear NMITIMEN and set WRIO */
    snes->nmitimen = 0x00; /* $4200 */
    snes->wrio = 0xFF;     /* $4201 */

    /* 3. Clear arithmetic & timer registers ($4202..$420B) */
    snes->wrmpya = 0;
    snes->wrmpyb = 0;
    snes->wrdiv = 0;
    snes->wrdvdd = 0;
    snes->rddiv = 0;
    snes->rdmpy = 0;

    /* 4. FastROM enable ($420D = 0x01) */
    snes->memsel = 0x01;
    snes->fast_rom_enabled = true;

    /* 5. Force blanking with max brightness ($2100 = 0x8F) */
    snes->inidisp = 0x8F;
    snes->brightness = 0x0F;
    snes->screen_enabled = false;

    /* 6. Clear Sprite & OAM registers ($2101..$2104) */
    snes->obsel = 0x00;
    snes->oamadd = 0x0000;
    memset(snes->oam, 0, sizeof(snes->oam));

    /* 7. Clear BG mode and tilemap pointers ($2105..$210C) */
    snes->bgmode = 0x00;
    snes->bg1sc = 0x00;
    snes->bg2sc = 0x00;
    snes->bg3sc = 0x00;
    snes->bg4sc = 0x00;
    snes->bg12nba = 0x00;
    snes->bg34nba = 0x00;

    /* 8. Setup VRAM auto-increment ($2115 = 0x80) */
    snes->vmain = 0x80;
    snes->vmadd = 0x0000;

    /* 9. Clear memories */
    memset(snes->vram, 0, sizeof(snes->vram));
    memset(snes->cgram, 0, sizeof(snes->cgram));
    memset(snes->wram, 0, sizeof(snes->wram));

    /* 10. Screen layer configuration ($212C, $212D) */
    snes->tm = 0x11; /* BG1 + OBJ default */
    snes->ts = 0x00;

    /* 11. Enable NMI & Joypad auto-polling ($4200 = 0x81) */
    snes->nmitimen = 0x81;

    printf("[SNES] Subsystems initialized: FastROM=%d, VMAIN=0x%02X, NMITIMEN=0x%02X\n",
           snes->fast_rom_enabled, snes->vmain, snes->nmitimen);
}

void nba_snes_set_joypad(NbaSnes *snes, uint16_t buttons) {
    if (!snes) return;
    snes->joy1 = buttons;
}

void nba_snes_write_cgram(NbaSnes *snes, uint8_t color_index, uint16_t bgr555) {
    if (!snes) return;
    snes->cgram[color_index] = bgr555;
}

void nba_snes_write_vram_tile8(NbaSnes *snes, uint16_t tile_index, const uint8_t *tile_data, int bpp) {
    if (!snes || !tile_data) return;

    size_t bytes_per_tile = (size_t)(bpp * 8);
    size_t offset = (size_t)tile_index * bytes_per_tile;
    if (offset + bytes_per_tile <= sizeof(snes->vram)) {
        memcpy(&snes->vram[offset], tile_data, bytes_per_tile);
    }
}
