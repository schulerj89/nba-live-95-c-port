#ifndef NBA_TIPOFF_H
#define NBA_TIPOFF_H

#include "nba_assets.h"
#include "nba_renderer.h"
#include "nba_session.h"
#include "nba_gameplay_debugger.h"

/* ROM routines correlated with live Mesen execution. */
#define SNES_ADDR_TIPOFF_PLAYER_FORMATION 0x86DDA7
#define SNES_ADDR_TIPOFF_BALL_INIT        0x86E054
#define SNES_ADDR_TIPOFF_JUMP_ANIMATION   0x86ECF4
#define SNES_ADDR_TIPOFF_CONTACT          0x86CF49
#define SNES_ADDR_TIPOFF_POSSESSION       0x86D3F9

#define NBA_TIPOFF_BALL_APPEAR_FRAME 140
#define NBA_TIPOFF_TOSS_FRAME       145
#define NBA_TIPOFF_JUMP_FRAME       156
#define NBA_TIPOFF_CONTACT_FRAME    198
#define NBA_TIPOFF_POSSESSION_FRAME 200
#define NBA_TIPOFF_BREAK_FRAME      220

typedef enum {
    NBA_TIPOFF_FORMATION = 0,
    NBA_TIPOFF_JUMP_BALL,
    NBA_TIPOFF_POSSESSION,
    NBA_TIPOFF_LIVE
} NbaTipoffPhase;

typedef struct {
    int32_t x_fp, y_fp, z_fp;
    int16_t velocity_x, velocity_y, velocity_z;
    int16_t target_x, target_y;
    uint8_t direction;
    uint8_t animation_state;
    uint8_t control_mode;
    uint8_t assignment_actor;
    uint16_t reaction_threshold;
    uint16_t action_state;
    bool visible;
} NbaTipoffActor;

typedef struct {
    int32_t x_fp, y_fp, z_fp;
    int16_t velocity_x, velocity_y, velocity_z;
    int8_t owner_actor;
    uint8_t state;
} NbaTipoffBall;

typedef struct {
    const NbaAssetPack *assets;
    NbaSession *session;
    int frame;
    NbaTipoffPhase phase;
    NbaTipoffActor actors[NBA_GAMEPLAY_ACTOR_COUNT];
    NbaTipoffBall ball;
    int16_t camera_x, camera_y;
    int8_t possession_actor;
    int8_t possession_team;
    uint16_t play_code;
    uint32_t simulation_tick;
    bool cpu_vs_cpu;
    bool is_initialized;
} NbaTipoff;

bool nba_tipoff_init(NbaTipoff *tipoff, const NbaAssetPack *assets,
                     NbaSession *session);
void nba_tipoff_update(NbaTipoff *tipoff, const NbaInput *input);
void nba_tipoff_render(const NbaTipoff *tipoff, NbaRenderer *renderer);
void nba_tipoff_capture_telemetry(const NbaTipoff *tipoff,
                                  const NbaInput *input,
                                  NbaGameplayTelemetry *telemetry);

#endif
