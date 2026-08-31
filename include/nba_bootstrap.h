#ifndef NBA_BOOTSTRAP_H
#define NBA_BOOTSTRAP_H
#include <stddef.h>
#include "nba_setup_codec_work.h"
#include "nba_setup_spc_init.h"
#include "nba_setup_spc_control.h"

/* Persistent game-level owner: NEVER put inside the cleared NbaGame.scene union.
 * S1 profile is the pinned reference's NTSC, normal speed, zero power-on RAM,
 * default +40Hz SPC setting. This is an explicit software comparison profile,
 * not a claim all SNES oscillators have identical frequency/power-on contents. */
typedef enum { NBA_BOOT_PROFILE_NTSC_ZERO_32040 = 1 } NbaBootstrapProfile;
typedef enum {
    NBA_BOOT_RUNNING, NBA_BOOT_CPU_SOURCE, NBA_BOOT_SPC_SOURCE,
    NBA_BOOT_DSP_READ, NBA_BOOT_HARDWARE, NBA_BOOT_BAD_SOURCE
} NbaBootstrapStatus;
typedef struct {
    NbaSetupCodecWork work;
    uint16_t pointer;
    uint8_t pointer_bank, read_bank, program_bank;
    uint8_t indirect[10], rmw_width, change_kind, carry_in;
    bool emulation;
    uint32_t boundary_pc;
} NbaBootstrapCpu;
typedef struct {
    uint64_t master, spc_ticks, spc_steps, refreshes;
    uint16_t hclock, scanline, refresh_position;
    bool odd_frame;
    /* DSP remains reset: writes disabled by FLG E0. Its exact later state and
     * first F3 read are unresolved; never return a canned register value. */
    uint64_t dsp_unresolved_steps;
} NbaBootstrapClock;
typedef enum { NBA_BOOT_EVENT_CPU, NBA_BOOT_EVENT_SPC, NBA_BOOT_EVENT_F1,
               NBA_BOOT_EVENT_RESIDENT_ENTRY, NBA_BOOT_EVENT_STOP,
               NBA_BOOT_EVENT_CPU_ENTRY, NBA_BOOT_EVENT_SPC_ENTRY } NbaBootstrapEventKind;
typedef struct {
    NbaBootstrapEventKind kind;
    uint64_t master, spc_ticks, sample_master;
    uint32_t pc, address;
    uint8_t value, bus_kind;
    bool instruction_end;
} NbaBootstrapEvent;
typedef void (*NbaBootstrapObserver)(void *context,const NbaBootstrapEvent *event);
typedef struct {
    NbaBootstrapCpu cpu;
    NbaSetupSpcInit spc;
    NbaSetupSpcResidentBus spc_bus;
    NbaSetupSpcControl control;
    NbaBootstrapClock clock;
    uint8_t wram[131072], io[0x2400];
    const uint8_t *rom;
    size_t rom_size;
    uint64_t cpu_cycles, upload_writes;
    uint32_t boundary_pc;
    NbaBootstrapStatus status;
    bool fast_rom, resident, f1_completed;
} NbaBootstrap;

/* No entry-register, ARAM snapshot, port response or phase input is accepted.
 * Caller retains immutable ROM. Reset is whole-console cold power-on only. */
bool nba_bootstrap_power_on(NbaBootstrap *state,const uint8_t *rom,size_t rom_size,
                            NbaBootstrapProfile profile);
/* Advance one concrete CPU bus cycle, including source-produced SPC work due
 * at actual port access. Pending unsupported hardware/source refuses progress.
 * Observer is diagnostic output only; it must not mutate this owner. */
bool nba_bootstrap_step(NbaBootstrap *state,NbaBootstrapObserver observer,void *context);

#endif
