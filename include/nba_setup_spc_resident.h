#ifndef NBA_SETUP_SPC_RESIDENT_H
#define NBA_SETUP_SPC_RESIDENT_H
#include <stdbool.h>
#include <stdint.h>
/* Uploaded NBA95 resident source only, not an SPC opcode interpreter.
 * Caller owns ARAM, clock phase and visibility of CPU port writes. begin never
 * seeds them. Normal upload/reset, timer and DSP execution remain unresolved. */
typedef struct {
    uint8_t aram[65536];
    uint8_t cpu_to_spc[4], spc_to_cpu[4];
    uint8_t dsp_address;
} NbaSetupSpcResidentBus;
typedef enum {
    NBA_SPC_FETCH=1, NBA_SPC_READ, NBA_SPC_WRITE, NBA_SPC_IDLE,
    NBA_SPC_TIMER, NBA_SPC_DSP, NBA_SPC_UNSUPPORTED, NBA_SPC_INVALID
} NbaSetupSpcResidentKind;
typedef struct {
    NbaSetupSpcResidentKind kind;
    uint16_t pc, address;
    uint8_t value;
    bool instruction_end;
} NbaSetupSpcResidentWork;
typedef struct {
    uint16_t pc, temporary;
    uint8_t a,x,y,sp,ps,phase;
    uint64_t cycles, instructions;
    bool valid;
} NbaSetupSpcResident;
/* Entry PCs 0441/0443/0447/044D/0613 only, PS.P=0. Caller must provide
 * normal-speed SPC hardware with ARAM writes enabled and canonical upload.
 * Register/ARAM prestates are allowed only for isolated component tests. */
bool nba_setup_spc_resident_begin(NbaSetupSpcResident *s, uint16_t pc,
    uint8_t a,uint8_t x,uint8_t y,uint8_t sp,uint8_t ps);
NbaSetupSpcResidentWork nba_setup_spc_resident_peek(const NbaSetupSpcResident *s);
/* One intrinsic SPC machine cycle, never master-clock prediction. Reads
 * sample the live bus here; timer/DSP/unsupported operations refuse mutation. */
bool nba_setup_spc_resident_accept(NbaSetupSpcResident *s,NbaSetupSpcResidentBus *bus);
/* Publish an ALREADY VISIBLE input latch. This is not a 65816 write adapter:
 * the scheduler must first derive cross-clock visibility from hardware state. */
bool nba_setup_spc_resident_visible_input(NbaSetupSpcResidentBus *bus,unsigned port,uint8_t value);
#endif
