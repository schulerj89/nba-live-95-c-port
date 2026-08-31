#ifndef NBA_BOOTSTRAP_OAM_PREFIX_H
#define NBA_BOOTSTRAP_OAM_PREFIX_H
#include "nba_bootstrap_nmi.h"

/* Source-derived continuation of the frozen first-NMI owner. The new fields
 * describe only the completed 2103 write; no prior OAM history or 213E value
 * is reconstructed from a capture. */
typedef struct {
    NbaBootstrapNmi prefix;
    bool continuation_active;
    uint8_t source_stage, bus_index, loaded_value;
    bool oam_priority_rotation;
    uint16_t oam_ram_address, oam_internal_address;
} NbaBootstrapOamPrefix;

/* Normal ROM-only power-on. Stops at80:818E before the first unowned213E PPU
 * status read. There is no controller, PPU-status or DSP response parameter. */
bool nba_bootstrap_oam_prefix_power_on(NbaBootstrapOamPrefix *state,
    const uint8_t *rom,size_t rom_size,NbaBootstrapProfile profile);
bool nba_bootstrap_oam_prefix_step(NbaBootstrapOamPrefix *state,
    NbaBootstrapObserver observer,void *context);
#endif
