/* Replay captured owned `$85:9A37-$A7C7` through production dispatch.
 * Host labels/ownership caches are deliberate binding sentinels, not WRAM. */
#include "nba_assets.h"
#include "nba_tipoff.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { NATIVE_INPUT_WORDS = 47, INPUT_WORDS = 50 };

static int32_t fixed(uint16_t fraction, uint16_t integer) {
    return (int32_t)(int16_t)integer * 256 + (fraction >> 8);
}

static uint16_t integer(int32_t value) {
    int32_t floor = value >= 0 ? value / 256 : -((-value + 255) / 256);
    return (uint16_t)(int16_t)floor;
}

static void emit(uint16_t value) { printf("%04x ", value); }

int main(int argc, char **argv) {
    NbaAssetPack assets;
    if (argc != 2 || !nba_assets_load(&assets, argv[1])) return 2;
    unsigned raw[INPUT_WORDS];
    for (;;) {
        for (unsigned i = 0; i < INPUT_WORDS; ++i) {
            if (scanf_s("%x", &raw[i]) != 1) {
                nba_assets_free(&assets);
                return i ? 3 : 0;
            }
            if (raw[i] > 0xFFFFu) return 4;
        }
        if (raw[0] >= NBA_GAMEPLAY_ACTOR_COUNT) return 5;
        NbaSession session;
        NbaTipoff state;
        memset(&session, 0, sizeof(session));
        memset(&state, 0, sizeof(state));
        state.assets = &assets;
        state.session = &session;
        state.possession_actor = (int8_t)raw[0];
        state.rim_raw_094a = (uint16_t)raw[1];
        state.rim_raw_0970 = (uint16_t)raw[2];
        state.attached_ball_state_raw_09f6 = (uint16_t)raw[3];
        state.dead_ball_raw_0968 = (uint16_t)raw[4];
        state.rim_raw_0962 = (uint16_t)raw[5];
        state.ball_activity_raw = (uint16_t)raw[6];
        state.camera_side_group_raw = (uint8_t)raw[7];
        state.ball.x_fp = fixed((uint16_t)raw[8], (uint16_t)raw[9]);
        state.ball.y_fp = fixed((uint16_t)raw[10], (uint16_t)raw[11]);
        state.ball.z_fp = fixed((uint16_t)raw[12], (uint16_t)raw[13]);
        state.ball.velocity_x = (int16_t)(uint16_t)raw[14];
        state.ball.velocity_y = (int16_t)(uint16_t)raw[15];
        state.ball.velocity_z = (int16_t)(uint16_t)raw[16];
        NbaTipoffActor *owner = &state.actors[raw[0]];
        owner->x_fp = fixed((uint16_t)raw[17], (uint16_t)raw[18]);
        owner->y_fp = fixed((uint16_t)raw[19], (uint16_t)raw[20]);
        owner->z_fp = fixed((uint16_t)raw[21], (uint16_t)raw[22]);
        owner->upper_animation_resource_raw_2a = (uint16_t)raw[23];
        owner->lower_animation_resource_raw_2c = (uint16_t)raw[24];
        owner->animation_resources_valid = true;
        owner->actor_status_raw_28 = (uint16_t)raw[25];
        owner->rom_upper_animation_phase_raw_3a = (uint16_t)raw[26];
        owner->upper_animation_phase_raw = (uint16_t)raw[26];
        owner->control_mode = (uint8_t)raw[27];
        owner->free_throw_launch_half_raw_a8 = (uint16_t)raw[28];
        owner->movement_direction = (uint8_t)raw[29];
        owner->requested_direction = (uint8_t)raw[30];
        owner->direction = (uint8_t)raw[31];
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
            state.actors[i].controller_assignment_raw =
                (int8_t)(int16_t)(uint16_t)raw[32 + i];
        state.rim_impact_raw_13e5 = (uint16_t)raw[42];
        state.rim_raw_13e7 = (uint16_t)raw[43];
        state.live_state_raw = (uint16_t)raw[44];
        state.shot_previous_actor_x_raw_0922 = (uint16_t)raw[45];
        state.ball_previous_z_raw_0924 = (uint16_t)raw[46];
        state.ball.state = (uint8_t)raw[NATIVE_INPUT_WORDS];
        state.ball.owner_actor = (int8_t)(int16_t)(uint16_t)raw[48];
        state.handler_actor = (uint8_t)raw[49];
        state.camera.subject_pointer_0940 =
            (uint16_t)(0x34EBu + raw[0] * 0x100u);
        (void)nba_tipoff_replay_ball_driver_entry(&state);
        emit((uint16_t)(int16_t)state.possession_actor);
        emit(state.rim_raw_094a);
        emit(state.rim_raw_0970);
        emit(state.attached_ball_state_raw_09f6);
        emit(state.dead_ball_raw_0968);
        emit(state.rim_raw_0962);
        emit(state.ball_activity_raw);
        emit((uint16_t)((state.ball.x_fp & 255) << 8));
        emit(integer(state.ball.x_fp));
        emit((uint16_t)((state.ball.y_fp & 255) << 8));
        emit(integer(state.ball.y_fp));
        emit((uint16_t)((state.ball.z_fp & 255) << 8));
        emit(integer(state.ball.z_fp));
        emit((uint16_t)state.ball.velocity_x);
        emit((uint16_t)state.ball.velocity_y);
        emit((uint16_t)state.ball.velocity_z);
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
            emit((uint16_t)(int16_t)state.actors[i].controller_assignment_raw);
        emit(state.rim_impact_raw_13e5);
        emit(state.rim_raw_13e7);
        emit(state.live_state_raw);
        emit(state.shot_previous_actor_x_raw_0922);
        emit(state.ball_previous_z_raw_0924);
        emit(state.ball.state);
        emit((uint16_t)(int16_t)state.ball.owner_actor);
        printf("%04x\n", state.handler_actor);
    }
}
