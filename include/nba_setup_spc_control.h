#ifndef NBA_SETUP_SPC_CONTROL_H
#define NBA_SETUP_SPC_CONTROL_H
#include "nba_setup_spc_resident.h"
typedef struct {
    uint8_t stage0,stage1,previous_stage1,stage2,output,target;
    bool enabled,globally_enabled;
} NbaSetupSpcControlTimer;
typedef struct {
    NbaSetupSpcControlTimer timer[3];
    uint8_t staged_cpu_input[4];
    bool pending_cpu_input_update;
    bool rom_enabled,aram_write_enabled;
} NbaSetupSpcControl;
/* Commit ONE real $F1 write after the external owner has advanced that bus
 * cycle, including timers/DSP/input visibility. No clock is charged here.
 * All fields are carried hardware state; this API has no snapshot/reset seed.
 * The debugger-write-veto path is outside the original-game contract. */
bool nba_setup_spc_control_commit(NbaSetupSpcControl *hw,NbaSetupSpcResidentBus *bus,uint8_t value);
/* IPL overlay selects ROM reads only. Underlying ARAM remains separate. */
bool nba_setup_spc_control_ipl_visible(const NbaSetupSpcControl *hw,uint16_t address);
#endif
