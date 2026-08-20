# Game Setup audio — ROM facts and implementation


The screen genuinely has a sequenced track. Polling the DSP every frame across
the settled screen (`tools/mesen_dsp_activity.lua`) shows **315 pitch/sample
changes and 233 note re-triggers over 700 frames**, across voices using sample
numbers 0, 4, 8, 13, 19, 20, 21, 23. It is not a held chord and not a stream.

The whole CPU-to-driver interface is four bytes. The ports are mirrored across
banks `$00`-`$3F` and `$80`-`$BF`; hooking every mirror shows that Setup is
CPU-driven, averaging roughly 142 writes per frame. The final capture detects
`$80:E600` and snapshots SPC RAM/DSP/CPU state on its brightness-1 frame, then
records 30 seconds containing
**102,445 writes** with `spc.cycle` timestamps. The binary asset is control data,
not rendered PCM.

### What is built

`src/nba_spc.c` is the in-game SPC700 + S-DSP core. Asset IDs 88-91 hold SPC
RAM, DSP registers, SPC CPU state, and the cycle-timed `$2140-$2143` trace.
`nba_audio_play_setup_spc` resumes the driver and synthesizes 32 kHz stereo PCM
in memory. Mesen's `spc.cycle` uses 2.048 MHz half-cycle units, so extraction
normalizes each delta by two into the core's 1.024 MHz domain; the DSP then
emits one sample per 32 SPC cycles. No Setup WAV is present in the asset pack.

The supported proof path is `nba95_port.exe --headless --setup-only
--dump-audio <output.wav>`; `tools/test_setup_transition.py` locks its format,
duration, peak range, eighth-second onset/RMS profile, coarse spectral bands,
channel energy, and stereo correlation. A whole-file hash is avoided because
fresh Mesen captures can begin at a slightly different SPC phase while
producing the same musical output.

Working so far:

- the SPC700 core executes the real driver, cycling through `$048B`, `$04DA`,
  `$044A`, `$0497`, `$044D`, `$0548`
- the DSP decodes BRR from the sample directory at `$0200` and produces clean
  tonal output (harmonics at 605/1210/1816 Hz, 97% of energy below 5 kHz)
- timers tick at the right rates — 1156 ticks/second across T0/T1/T2, matching
  500 + 432 + 182 Hz for targets `$10`/`$2C`/`$94`
- the driver polls the timer 38,340 times a second, so its wait loop is live

Two things had to be fixed to get that far. `$F4`-`$F7` are the CPU-facing
output latches: what the SPC reads back is whatever the 65816 last wrote, so
SPC writes must not change the read value. Without that the snapshot resumes
mid-handshake and the driver spins forever at `$0443`
(`MOV A,$F4` / `BNE $0443`). And a timer target of 0 means 256, not 0.

### The divergence, and what it actually was

A differential PC trace settled it. `tools/mesen_spc_trace.lua` captures an APU
snapshot and logs the next 60,000 SPC700 PCs from that exact instant. A retired
diagnostic harness produced the corresponding core trace so the streams could
be diffed; it is not part of the supported build.

The first divergence was at instruction 370, at `$048D`. The code there is:

```
048B  E4 FD     MOV A,$FD      ; timer 0 output
048D  D0 01     BNE $0490      ; no tick -> fall through to the RET
048F  6F        RET
0490  C4 73     MOV $73,A      ; process the tick
```

But the timers were not the problem: `$0490` executes **119 times in both**
traces, and `$0548`, `$04DA`, `$0495`, `$0497`, `$0499` all execute exactly 952
times in both. The tick path was already correct.

The real signal was the PC histogram: Mesen executes **106 distinct addresses,
this core only 29**. The first address Mesen reaches that the core never does
is `$0451`, entered from:

```
044A  3F 8B 04  CALL $048B     ; run one tick
044D  E4 F4     MOV A,$F4      ; read APU port 0
044F  F0 F9     BEQ $044A      ; loop while the port is empty
0451  ...                      ; a command arrived - process it
```

**There is no bad opcode.** The driver idles in that loop until the 65816 hands
it a command, and this core was faithfully idling because nothing was feeding
the ports.

The reason that was missed earlier is a capture bug of mine: the APU ports are
mirrored across banks `$00`-`$3F` and `$80`-`$BF`, and the first version of
`tools/mesen_apu_ports.lua` hooked only `$00`/`$80`. That reported 24 writes.
Hooking every mirror reports **71,065** over the same span — about 142 writes
per frame, in groups of `port1/port2/port3 = params; port0 = $0B; port0 = $00`.

So the Game Setup music is **CPU-driven**: the SPC700 driver is a playback
engine and the 65816 sequences it, several commands per frame.

### Validation history

`tools/spc_replay_main.c` was the diagnostic harness used to feed the recorded
port stream back into the core. It is retained for investigation but is not
part of the supported build.

With the real command stream the core produces music, not a held tone —
envelope std 688 across 0.125s windows versus 27 before, RMS ranging 278-2498.
Comparing its DSP registers against Mesen's own per-frame log:

| | match |
|---|---|
| voice pitch | 1840/2880 (63.9%) |
| voice sample number | 1814/2880 (63.0%) |

At the start the earlier frame-stamped replay was essentially exact, but it
drifted because writes were distributed evenly inside each video frame. The
runtime path replaces that approximation with the new SPC-cycle timestamps;
the DSP emits one sample every 32 SPC cycles, so commands land at the correct
audio sample.

### Remaining audio work

The authentic next step would be a direct C port of the 65816 sequencer at
`$80:A9E3`, `$80:AA7B`, and `$80:AACD`. The current implementation is already
ROM-driver/BRR synthesis rather than a music recording, but its CPU-side
command decisions are replayed from cycle-timed control data in the asset pack.
