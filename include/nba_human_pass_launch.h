#ifndef NBA_HUMAN_PASS_LAUNCH_H
#define NBA_HUMAN_PASS_LAUNCH_H
#include "nba_types.h"

typedef struct {
    uint16_t cc, ce, d0;
    uint16_t remainder_0806, remainder_high_0808, divisor_080a, divisor_high_080c;
    uint16_t quotient_080e, quotient_high_0810;
    uint16_t product_low_0820, cross_high_0822, sign_0824, count_085a;
} NbaHumanPassLaunchMath;

typedef struct { uint16_t a, x, y; } NbaHumanPassLaunchRegisters;
/* Binary16 native contracts. Results model A/X/Y, not the complete CPU.
 * F78B truncatesY to8 bits. F8D9 returns remainder inX, magnitude quotient high inY.
 * Both expose their actual DP/WRAM arithmetic scratch effects. */
NbaHumanPassLaunchRegisters nba_human_pass_launch_multiply(
    NbaHumanPassLaunchMath *, NbaHumanPassLaunchRegisters);
NbaHumanPassLaunchRegisters nba_human_pass_launch_divide(
    NbaHumanPassLaunchMath *, NbaHumanPassLaunchRegisters);

typedef struct {
    uint16_t x, y, z, velocity_x, velocity_y;
    uint16_t flags_28, upper_30, delay_5a, mode_5e, timer_60;
    uint16_t band_62, behavior_64, group_6e, flags_7e, family_c0;
} NbaHumanPassLaunchActor;

typedef struct {
    /* Original9C6F,9C93,9CB7, each six3-word rows. Third words are preserved
     * as table data but are not read by99C4. No invented seventh row. */
    uint16_t family[3][18];
} NbaHumanPassLaunchTables;

typedef struct {
    NbaHumanPassLaunchActor actors[10];
    uint16_t source_index, receiver_index;
    uint16_t ball_x, ball_y, ball_z, ball_velocity_x, ball_velocity_y, ball_velocity_z;
    uint16_t pointer_92, duration_b2, profile_e0, profile_e2;
    uint16_t ball_pointer_0910, profile_0914, profile_0916;
    uint16_t live_0936, offense_093a, owner_093e, released_094a;
    NbaHumanPassLaunchMath math;
} NbaHumanPassLaunchState;

/* Complete99C4-9BB0 memory contract, including clamp children9BB1/9BFB,
 * step9C45, signed arithmetic and conditional9846 normalization. Typed
 * actor indices must resolve the original96/8E pointers; aliasing is kept.
 * Valid table domain is bands0,6,12,18,24,30. Invalid input is unchanged.
 * The ten PEI scratch words correspond to C locals and remain unchanged. */
bool nba_human_pass_launch(const NbaHumanPassLaunchTables *, NbaHumanPassLaunchState *);
#endif
