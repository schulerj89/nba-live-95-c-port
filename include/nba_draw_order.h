#ifndef NBA_DRAW_ORDER_H
#define NBA_DRAW_ORDER_H
#include <stdbool.h>
#include <stdint.h>

/* Persistent source ownership: order is WRAM7E44..7E5B; depth[i] is
 * (34EB+100*i)+68 for ten actors, the ball, and the basket record. */
typedef struct {
    uint16_t order[12];
    uint16_t depth[12];
} NbaDrawOrder;
typedef struct {
    uint16_t x[12], y[12];
    uint16_t camera_y;
} NbaDrawOrderInput;

/* 80:FBE9..FBFE writes identity pointers ONLY. Carried depths survive.
 * The original new-game caller is86:DA89, not every frame/period. */
bool nba_draw_order_initialize(NbaDrawOrder *state);
/* 87:A3B6..A3CE depth projection only. Excludes +6A projection/culling,
 * indicators, OAM, CPU scratch/registers, interrupts and elapsed time.
 * Caller supplies the actual current XY/camera at the scheduled boundary.
 * Basket X comes from its existing presentation owner; Y originates in
 * 86:DBC2 but remains an explicit runtime input, never inferred here. */
bool nba_draw_order_project(NbaDrawOrder *state, const NbaDrawOrderInput *input);
/* 80:FC80..FCA1: exactly11 reverse adjacent comparisons, not fullFBFF sort.
 * Requires a bijection of the original12 pointers; rejects without mutation. */
bool nba_draw_order_pass(NbaDrawOrder *state);
/* Convenience for a caller owning both boundaries; same exclusions as above.
 * Inputs and output remain unchanged on refusal. */
bool nba_draw_order_update(NbaDrawOrder *state, const NbaDrawOrderInput *input);
#endif
