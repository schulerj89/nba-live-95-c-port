/* Replay the captured 65816 -> APU port stream into the SPC700/DSP core and
 * render the result to a WAV.
 *
 * The Game Setup music is CPU-driven: the 65816 issues ~142 writes to
 * $2140-$2143 every frame and the sound driver only advances when it sees
 * them. Feeding the recorded stream back in is how the core gets validated
 * end to end before the CPU-side sequencer is ported.
 *
 *   spc_replay <spc_ram.bin> <spc_dsp.bin> <pc> <a> <x> <y> <sp> <psw>
 *              <apu_ports.txt> <startFrame> <seconds> <out.wav>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nba_spc.h"

#define MAX_WRITES 200000

typedef struct { int frame; int port; unsigned char value; } PortWrite;

static PortWrite writes[MAX_WRITES];
static int write_count = 0;

static void put32(FILE *f, unsigned v) { fputc(v & 255, f); fputc((v >> 8) & 255, f); fputc((v >> 16) & 255, f); fputc((v >> 24) & 255, f); }
static void put16(FILE *f, unsigned v) { fputc(v & 255, f); fputc((v >> 8) & 255, f); }

int main(int argc, char **argv) {
    if (argc < 13) {
        fprintf(stderr, "usage: spc_replay ram dsp pc a x y sp psw ports.txt startFrame seconds out.wav\n");
        return 1;
    }

    static unsigned char ram[NBA_SPC_RAM_SIZE];
    static unsigned char dsp[NBA_SPC_DSP_REGS];
    FILE *f = fopen(argv[1], "rb");
    if (!f || fread(ram, 1, sizeof(ram), f) != sizeof(ram)) { fprintf(stderr, "bad ram\n"); return 1; }
    fclose(f);
    f = fopen(argv[2], "rb");
    if (!f || fread(dsp, 1, sizeof(dsp), f) != sizeof(dsp)) { fprintf(stderr, "bad dsp\n"); return 1; }
    fclose(f);

    FILE *pf = fopen(argv[9], "r");
    if (!pf) { fprintf(stderr, "cannot open port log\n"); return 1; }
    char line[256];
    while (fgets(line, sizeof(line), pf)) {
        if (line[0] == '#') continue;
        int fr, port; unsigned val;
        if (sscanf(line, "%d %d %x", &fr, &port, &val) == 3 && write_count < MAX_WRITES) {
            writes[write_count].frame = fr;
            writes[write_count].port = port;
            writes[write_count].value = (unsigned char)val;
            write_count++;
        }
    }
    fclose(pf);

    int start_frame = atoi(argv[10]);
    double seconds = atof(argv[11]);

    static NbaSpc spc;
    if (!nba_spc_load(&spc, ram, sizeof(ram), dsp, sizeof(dsp),
                      (unsigned short)strtol(argv[3], 0, 0), (unsigned char)atoi(argv[4]),
                      (unsigned char)atoi(argv[5]), (unsigned char)atoi(argv[6]),
                      (unsigned char)atoi(argv[7]), (unsigned char)atoi(argv[8]))) {
        fprintf(stderr, "load failed\n"); return 1;
    }

    int total_frames = (int)(seconds * 60.0);
    int samples_per_frame = NBA_SPC_SAMPLE_RATE / 60;
    int total_samples = total_frames * samples_per_frame;
    short *buf = (short *)malloc((size_t)total_samples * 4);
    if (!buf) return 1;

    int wi = 0;
    while (wi < write_count && writes[wi].frame < start_frame) wi++;

    FILE *dsplog = fopen(".analysis/setup_capture/replay_dsp_activity.txt", "wb");
    int applied = 0;
    short *cur = buf;
    for (int fr = 0; fr < total_frames; ++fr) {
        int video_frame = start_frame + fr;

        /* collect this frame's writes */
        int first = wi;
        while (wi < write_count && writes[wi].frame == video_frame) wi++;
        int n = wi - first;

        if (n == 0) {
            nba_spc_render(&spc, cur, samples_per_frame);
        } else {
            /* spread the writes across the frame so the driver, which polls
             * $F4 in a tight loop, sees each command before the next arrives */
            int chunk = samples_per_frame / n;
            if (chunk < 1) chunk = 1;
            int done = 0;
            for (int k = 0; k < n; ++k) {
                nba_spc_write_port(&spc, writes[first + k].port, writes[first + k].value);
                applied++;
                int want = (k == n - 1) ? (samples_per_frame - done) : chunk;
                if (done + want > samples_per_frame) want = samples_per_frame - done;
                if (want > 0) { nba_spc_render(&spc, cur + done * 2, want); done += want; }
            }
            if (done < samples_per_frame) {
                nba_spc_render(&spc, cur + done * 2, samples_per_frame - done);
            }
        }
        if (dsplog) {
            fprintf(dsplog, "%d", video_frame);
            for (int v = 0; v < 8; ++v) {
                int pitch = spc.dsp[v*16+2] | ((spc.dsp[v*16+3] & 0x3F) << 8);
                fprintf(dsplog, " %d:%d/%d/%d", v, pitch, spc.dsp[v*16+8], spc.dsp[v*16+4]);
            }
            fprintf(dsplog, "\n");
        }
        cur += samples_per_frame * 2;
    }

    if (dsplog) fclose(dsplog);
    FILE *w = fopen(argv[12], "wb");
    if (!w) { fprintf(stderr, "cannot open output\n"); return 1; }
    unsigned data = (unsigned)total_samples * 4;
    fwrite("RIFF", 1, 4, w); put32(w, 36 + data); fwrite("WAVE", 1, 4, w);
    fwrite("fmt ", 1, 4, w); put32(w, 16); put16(w, 1); put16(w, 2);
    put32(w, NBA_SPC_SAMPLE_RATE); put32(w, NBA_SPC_SAMPLE_RATE * 4); put16(w, 4); put16(w, 16);
    fwrite("data", 1, 4, w); put32(w, data);
    fwrite(buf, 1, data, w);
    fclose(w);

    long long sum = 0; short peak = 0;
    for (int i = 0; i < total_samples * 2; ++i) {
        int v = buf[i]; if (v < 0) v = -v;
        sum += v; if (v > peak) peak = (short)v;
    }
    printf("replayed %d port writes over %d frames (%.2fs): peak=%d meanAbs=%.1f -> %s\n",
           applied, total_frames, seconds, peak, (double)sum / (total_samples * 2), argv[12]);
    free(buf);
    return 0;
}
