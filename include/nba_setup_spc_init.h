#ifndef NBA_SETUP_SPC_INIT_H
#define NBA_SETUP_SPC_INIT_H
#include "nba_setup_spc_resident.h"
typedef NbaSetupSpcResident NbaSetupSpcInit;
typedef struct { NbaSetupSpcResidentWork bus; bool control_publication; } NbaSetupSpcInitWork;
/* Source entries0380 (before CLRP, any P) and0387 (after external F1 owner,
 * P=0). Normal-speed, writable ARAM; uploaded source remains caller-owned. */
bool nba_setup_spc_init_begin(NbaSetupSpcInit *s,uint16_t pc,uint8_t a,uint8_t x,uint8_t y,uint8_t sp,uint8_t ps);
NbaSetupSpcInitWork nba_setup_spc_init_peek(const NbaSetupSpcInit *s);
/* F1 control publication and DSP data access refuse all mutation. */
bool nba_setup_spc_init_accept(NbaSetupSpcInit *s,NbaSetupSpcResidentBus *bus);
#endif
