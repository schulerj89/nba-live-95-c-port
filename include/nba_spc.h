#ifndef NBA_SPC_H
#define NBA_SPC_H

#include "nba_types.h"

/**
 * SPC700 + S-DSP core.
 *
 * NBA Live '95 does not stream a finished song. The 65816 side uploads a sound
 * driver, a sequence and a bank of BRR samples into SPC RAM through the APU
 * ports $2140-$2143, and the resident SPC700 driver sequences those short
 * samples across the DSP's eight voices. The Game Setup screen issues its
 * commands from $80:A9E3, $80:AA7B and $80:AACD.
 *
 * Rather than replay a recording, this core resumes the ROM's own driver from
 * a captured APU snapshot (64 KiB SPC RAM, the 128 DSP registers and the
 * SPC700 register file) and runs it, so the music is sequenced from the game's
 * sample bank exactly as the hardware does it.
 *
 * The SPC700 runs at 1.024 MHz and the DSP emits one stereo sample every 32
 * SPC cycles, giving 32000 Hz output.
 */

#define NBA_SPC_RAM_SIZE     0x10000
#define NBA_SPC_DSP_REGS     0x80
#define NBA_SPC_CLOCK_HZ     1024000
#define NBA_SPC_SAMPLE_RATE  32000
#define NBA_SPC_CYCLES_PER_SAMPLE (NBA_SPC_CLOCK_HZ / NBA_SPC_SAMPLE_RATE) /* 32 */

/* DSP register indices (per-voice registers are (voice << 4) | index) */
#define NBA_DSP_VOL_L   0x00
#define NBA_DSP_VOL_R   0x01
#define NBA_DSP_PITCH_L 0x02
#define NBA_DSP_PITCH_H 0x03
#define NBA_DSP_SRCN    0x04
#define NBA_DSP_ADSR1   0x05
#define NBA_DSP_ADSR2   0x06
#define NBA_DSP_GAIN    0x07
#define NBA_DSP_ENVX    0x08
#define NBA_DSP_OUTX    0x09

#define NBA_DSP_MVOL_L  0x0C
#define NBA_DSP_MVOL_R  0x1C
#define NBA_DSP_EVOL_L  0x2C
#define NBA_DSP_EVOL_R  0x3C
#define NBA_DSP_KON     0x4C
#define NBA_DSP_KOF     0x5C
#define NBA_DSP_FLG     0x6C
#define NBA_DSP_ENDX    0x7C
#define NBA_DSP_EFB     0x0D
#define NBA_DSP_PMON    0x2D
#define NBA_DSP_NON     0x3D
#define NBA_DSP_EON     0x4D
#define NBA_DSP_DIR     0x5D
#define NBA_DSP_ESA     0x6D
#define NBA_DSP_EDL     0x7D

typedef enum {
    NBA_ENV_ATTACK = 0,
    NBA_ENV_DECAY,
    NBA_ENV_SUSTAIN,
    NBA_ENV_RELEASE
} NbaEnvPhase;

typedef struct {
    /* BRR decode state. Blocks decode 16 samples at a time into a 32-entry
     * ring; the 4 taps the interpolator needs can straddle a block boundary,
     * so reads and writes are tracked as absolute sample counters. */
    uint16_t brr_addr;       /* address of the block being decoded */
    int16_t  ring[32];
    int      ring_write;     /* samples decoded so far  */
    int      ring_read;      /* samples consumed so far */
    int      last1, last2;   /* BRR filter history, persists across blocks */
    bool     reached_end;

    uint16_t interp_pos;     /* 12-bit fractional pitch counter */

    /* envelope */
    NbaEnvPhase phase;
    int      env;            /* 0..2047 */
    int      env_counter;    /* samples since the last envelope step */

    bool     kon_delay_active;
    int      kon_delay;
    bool     active;
    int16_t  out;
} NbaSpcVoice;

typedef struct {
    /* SPC700 register file */
    uint8_t  a, x, y, sp;
    uint16_t pc;
    uint8_t  psw;

    uint8_t  ram[NBA_SPC_RAM_SIZE];
    uint8_t  dsp[NBA_SPC_DSP_REGS];
    uint8_t  dsp_addr;

    /* timers: T0/T1 tick at 8 kHz, T2 at 64 kHz */
    uint8_t  timer_target[3];
    int      timer_counter[3];
    uint8_t  timer_out[3];
    int      timer_div[3];
    bool     timer_enabled[3];

    bool     ipl_enabled;

    /* DSP */
    NbaSpcVoice voice[8];
    uint32_t noise_lfsr;
    int      cycles_to_sample;

    bool     is_loaded;
} NbaSpc;

/**
 * Resume the ROM's sound driver from a captured APU snapshot.
 * `ram` is 64 KiB of SPC RAM, `dsp` the 128 DSP registers.
 */
bool nba_spc_load(NbaSpc *spc,
                  const void *ram, size_t ram_size,
                  const void *dsp, size_t dsp_size,
                  uint16_t pc, uint8_t a, uint8_t x, uint8_t y,
                  uint8_t sp, uint8_t psw);

/**
 * Deliver one 65816 write to an APU port ($2140-$2143 on the CPU side, which
 * the SPC700 reads at $F4-$F7). The music is driven by the CPU: on the Game
 * Setup screen the 65816 issues roughly 142 port writes per frame, and the
 * driver only advances the song when it sees them.
 */
void nba_spc_write_port(NbaSpc *spc, int port, uint8_t value);

/** Render `frames` stereo samples at 32000 Hz into `out` (2 shorts per frame). */
void nba_spc_render(NbaSpc *spc, int16_t *out, int frames);

/** Deterministic opcode, timer, port, BRR, and envelope regression vectors. */
bool nba_spc_self_test(void);

#endif /* NBA_SPC_H */
