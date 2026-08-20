/* Offline harness: resume the captured APU snapshot and render the Game Setup
 * music to a WAV, so the SPC700/DSP core can be checked without the game loop.
 *
 *   spc_render <spc_ram.bin> <spc_dsp.bin> <pc> <a> <x> <y> <sp> <psw> <seconds> <out.wav>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nba_spc.h"
extern unsigned nba_spc_dbg_timer_reads, nba_spc_dbg_dsp_writes, nba_spc_dbg_timer_ticks;

static void put32(FILE *f, unsigned v) { fputc(v & 255, f); fputc((v >> 8) & 255, f); fputc((v >> 16) & 255, f); fputc((v >> 24) & 255, f); }
static void put16(FILE *f, unsigned v) { fputc(v & 255, f); fputc((v >> 8) & 255, f); }

int main(int argc, char **argv) {
    if (argc < 11) { fprintf(stderr, "usage: spc_render ram dsp pc a x y sp psw seconds out.wav\n"); return 1; }

    static unsigned char ram[NBA_SPC_RAM_SIZE];
    static unsigned char dsp[NBA_SPC_DSP_REGS];
    FILE *f = fopen(argv[1], "rb");
    if (!f || fread(ram, 1, sizeof(ram), f) != sizeof(ram)) { fprintf(stderr, "bad ram\n"); return 1; }
    fclose(f);
    f = fopen(argv[2], "rb");
    if (!f || fread(dsp, 1, sizeof(dsp), f) != sizeof(dsp)) { fprintf(stderr, "bad dsp\n"); return 1; }
    fclose(f);

    static NbaSpc spc;
    if (!nba_spc_load(&spc, ram, sizeof(ram), dsp, sizeof(dsp),
                      (unsigned short)strtol(argv[3], 0, 0), (unsigned char)atoi(argv[4]),
                      (unsigned char)atoi(argv[5]), (unsigned char)atoi(argv[6]),
                      (unsigned char)atoi(argv[7]), (unsigned char)atoi(argv[8]))) {
        fprintf(stderr, "load failed\n"); return 1;
    }

    double seconds = atof(argv[9]);
    int frames = (int)(seconds * NBA_SPC_SAMPLE_RATE);
    short *buf = (short *)malloc((size_t)frames * 4);
    if (!buf) return 1;
    if (argc > 11) {
        /* sample the SPC700 PC to see whether the driver is advancing */
        int hist_n = 0;
        unsigned short pcs[64];
        for (int i = 0; i < 64; ++i) {
            nba_spc_render(&spc, buf, NBA_SPC_SAMPLE_RATE / 64);
            pcs[hist_n++] = spc.pc;
        }
        printf("PC samples:");
        for (int i = 0; i < hist_n; ++i) printf(" %04X", pcs[i]);
        printf("\n");
        for (int i = 0; i < 8; ++i)
            printf("  v%d pitch=%5d srcn=%3d envx=%3d adsr=%02X%02X gain=%02X active=%d\n",
                   i, spc.dsp[i*16+2] | ((spc.dsp[i*16+3] & 0x3F) << 8), spc.dsp[i*16+4],
                   spc.dsp[i*16+8], spc.dsp[i*16+5], spc.dsp[i*16+6], spc.dsp[i*16+7],
                   spc.voice[i].active);
        printf("  KON=%02X KOF=%02X FLG=%02X MVOL=%d/%d\n",
               spc.dsp[0x4C], spc.dsp[0x5C], spc.dsp[0x6C],
               (signed char)spc.dsp[0x0C], (signed char)spc.dsp[0x1C]);
        printf("timer reads=%u dsp writes=%u timer ticks=%u\n",
               nba_spc_dbg_timer_reads, nba_spc_dbg_dsp_writes, nba_spc_dbg_timer_ticks);
        printf("timer targets %02X %02X %02X, F1=%02X, ports F4-F7 %02X %02X %02X %02X\n",
               spc.timer_target[0], spc.timer_target[1], spc.timer_target[2],
               spc.ram[0xF1], spc.ram[0xF4], spc.ram[0xF5], spc.ram[0xF6], spc.ram[0xF7]);
    }
    nba_spc_render(&spc, buf, frames);

    FILE *w = fopen(argv[10], "wb");
    if (!w) { fprintf(stderr, "cannot open output\n"); return 1; }
    unsigned data = (unsigned)frames * 4;
    fwrite("RIFF", 1, 4, w); put32(w, 36 + data); fwrite("WAVE", 1, 4, w);
    fwrite("fmt ", 1, 4, w); put32(w, 16); put16(w, 1); put16(w, 2);
    put32(w, NBA_SPC_SAMPLE_RATE); put32(w, NBA_SPC_SAMPLE_RATE * 4); put16(w, 4); put16(w, 16);
    fwrite("data", 1, 4, w); put32(w, data);
    fwrite(buf, 1, data, w);
    fclose(w);

    long long sum = 0; short peak = 0;
    for (int i = 0; i < frames * 2; ++i) { int v = buf[i]; if (v < 0) v = -v; sum += v; if (v > peak) peak = (short)v; }
    printf("rendered %d frames (%.2fs) peak=%d meanAbs=%.1f -> %s\n",
           frames, seconds, peak, (double)sum / (frames * 2), argv[10]);
    free(buf);
    return 0;
}
