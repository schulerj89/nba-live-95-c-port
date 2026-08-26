/* Replay live `$85:9A6A-$A7C7` ownerless-ball entries through production C. */
#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nba_tipoff.h"

#define WRAM_SIZE 0x4B00u

static uint16_t word(const uint8_t *raw, unsigned address) {
    return (uint16_t)(raw[address] | ((uint16_t)raw[address + 1u] << 8));
}

static int32_t fixed_position(const uint8_t *raw, unsigned fraction,
                              unsigned integer) {
    return (int32_t)(int16_t)word(raw, integer) * 256 +
           (uint8_t)(word(raw, fraction) >> 8);
}

static void print_state(const NbaTipoff *s) {
    const NbaTipoffBall *b = &s->ball;
    int16_t x = (int16_t)(b->x_fp >= 0 ? b->x_fp / 256 :
        -(((-b->x_fp) + 255) / 256));
    int16_t y = (int16_t)(b->y_fp >= 0 ? b->y_fp / 256 :
        -(((-b->y_fp) + 255) / 256));
    int16_t z = (int16_t)(b->z_fp >= 0 ? b->z_fp / 256 :
        -(((-b->z_fp) + 255) / 256));
    printf("%04x %04x %04x %04x %04x %04x %04x %04x %04x "
           "%04x %04x %04x %04x %04x %04x %04x %04x %04x "
           "%04x %04x %04x %04x %04x %04x %04x %04x %04x "
           "%04x %04x\n",
           (unsigned)((b->x_fp & 0xff) << 8), (uint16_t)x,
           (unsigned)((b->y_fp & 0xff) << 8), (uint16_t)y,
           (unsigned)((b->z_fp & 0xff) << 8), (uint16_t)z,
           (uint16_t)b->velocity_x, (uint16_t)b->velocity_y,
           (uint16_t)b->velocity_z, s->live_state_raw,
           s->ball_activity_raw, s->rim_raw_094a, s->shot_value_raw,
           s->rim_raw_0962, s->rim_raw_096a, s->rim_raw_097c,
           s->rim_raw_096e, s->rim_raw_0970, s->rim_raw_0920,
           s->free_throw_resolution_raw_0972,
           (uint16_t)s->pass_actor_raw, (uint16_t)s->pass_aux_raw,
           (uint16_t)s->pass_receiver_raw, s->inbound_transfer_raw,
           s->rim_impact_raw_13e5, s->rim_raw_13e7, s->rng.state,
           s->session->score[0], s->session->score[1]);
}

int main(void) {
    uint8_t raw[WRAM_SIZE];
    _setmode(_fileno(stdin), _O_BINARY);
    while (fread(raw, 1u, sizeof(raw), stdin) == sizeof(raw)) {
        NbaSession session;
        NbaTipoff s;
        memset(&session, 0, sizeof(session));
        memset(&s, 0, sizeof(s));
        s.session = &session;
        s.ball.x_fp = fixed_position(raw, 0x3EEDu, 0x3EEFu);
        s.ball.y_fp = fixed_position(raw, 0x3EF1u, 0x3EF3u);
        s.ball.z_fp = fixed_position(raw, 0x3EF5u, 0x3EF7u);
        s.ball.velocity_x = (int16_t)word(raw, 0x3EF9u);
        s.ball.velocity_y = (int16_t)word(raw, 0x3EFBu);
        s.ball.velocity_z = (int16_t)word(raw, 0x3EFDu);
        s.ball.owner_actor = -1;
        s.ball.state = NBA_BALL_BOUNCE;
        s.simulation_tick = 0u;
        s.rng.state = word(raw, 0x07F6u);
        s.live_state_raw = word(raw, 0x0936u);
        s.camera_side_group_raw = (uint8_t)word(raw, 0x093Au);
        s.offense_side = s.camera_side_group_raw == 0u ? 0u : 1u;
        s.pass_actor_raw = (int16_t)word(raw, 0x0942u);
        s.pass_aux_raw = (int16_t)word(raw, 0x0944u);
        s.pass_receiver_raw = (int16_t)word(raw, 0x0946u);
        s.ball_activity_raw = word(raw, 0x0948u);
        s.rim_raw_094a = word(raw, 0x094Au);
        s.shot_value_raw = word(raw, 0x094Cu);
        s.inbound_state_raw = word(raw, 0x0952u);
        s.inbound_actor_raw = word(raw, 0x0954u);
        s.inbound_layout_raw = (int16_t)word(raw, 0x0956u);
        s.rim_raw_0962 = word(raw, 0x0962u);
        s.dead_ball_raw_0968 = word(raw, 0x0968u);
        s.rim_raw_096a = word(raw, 0x096Au);
        s.rim_raw_096e = word(raw, 0x096Eu);
        s.rim_raw_0970 = word(raw, 0x0970u);
        s.free_throw_resolution_raw_0972 = word(raw, 0x0972u);
        s.fouls.free_throw_state_raw_0978 = word(raw, 0x0978u);
        s.rim_raw_097c = word(raw, 0x097Cu);
        s.rim_raw_0920 = word(raw, 0x0920u);
        s.rim_raw_092c = word(raw, 0x092Cu);
        s.free_throw_flight_timer_raw_0930 = word(raw, 0x0930u);
        s.inbound_transfer_raw = word(raw, 0x09B8u);
        s.shot_inner_veto_raw = word(raw, 0x09F8u) != 0u;
        s.rim_force_raw_1866 = word(raw, 0x1866u);
        s.rim_impact_raw_13e5 = word(raw, 0x13E5u);
        s.rim_raw_13e7 = word(raw, 0x13E7u);
        s.leading_side_raw_1403 = word(raw, 0x1403u);
        s.left_lead_change_count_raw_1405 = word(raw, 0x1405u);
        s.right_lead_change_count_raw_1407 = word(raw, 0x1407u);
        s.rim_effect.gate_raw_3f33 = word(raw, 0x3F33u);
        s.rim_effect.resource_raw_4015 = word(raw, 0x4015u);
        s.rim_effect.effect_raw_401b = word(raw, 0x401Bu);
        s.rim_effect.frame_raw_4025 = word(raw, 0x4025u);
        s.rim_effect.timer_raw_402d = word(raw, 0x402Du);
        s.rim_effect.reference_y_raw_3ff3 = (int16_t)word(raw, 0x3FF3u);
        session.score[0] = word(raw, 0x4711u);
        session.score[1] = word(raw, 0x4791u);
        (void)nba_tipoff_replay_ownerless_ball_entry(&s);
        print_state(&s);
    }
    return ferror(stdin) ? 1 : 0;
}
