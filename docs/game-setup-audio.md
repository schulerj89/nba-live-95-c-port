# Game Setup audio — ROM facts and implementation

Game Setup uses a sequenced SNES track, not streamed PCM. Mesen observes 315
pitch/sample changes and 233 note re-triggers over 700 settled frames, using
BRR sample numbers 0, 4, 8, 13, 19, 20, 21, and 23.

## ROM control chain

Ghidra identifies the 65816 side of the chain:

- `$80:A9E3` waits for `$2140`, then writes the command parameters.
- `$80:AA7B` issues command 6, waits for the echo, and clears the handshake.
- `$80:AACD` queues per-voice parameters and command `$0B`.

The APU ports are mirrored through banks `$00`-`$3F` and `$80`-`$BF`. The
brightness-1 capture at `$80:E600` contains 102,445 cycle-timed writes to
`$2140-$2143` over 1,800 frames. The resident SPC700 program translates them
into 19,928 writes through `$F2/$F3` to the S-DSP.

Both streams are packed. Asset 91 preserves the Ghidra-facing CPU command
evidence; asset 93 is the exact downstream S-DSP program used for playback.
The latter avoids accumulating timing drift in the port's independent SPC700
CPU implementation while preserving the behavior those ROM routines caused.

## Runtime synthesis

`nba_audio_play_setup_dsp` loads asset 88's 64 KiB SPC RAM/BRR bank, asset 89's
initial DSP registers, asset 90's handoff state, and asset 93's cycle-timed DSP
writes. `src/nba_spc.c` then decodes, interpolates, envelopes, pans, and mixes
the ROM BRR instruments into 32 kHz stereo PCM at runtime.

This is an audio engine, not a recording:

- no Setup WAV or Mesen PCM is stored in the asset pack;
- note/sample/pitch changes remain structured control events;
- instrument data remains the ROM's BRR bank;
- the S-DSP uses the hardware 512-entry Gaussian coefficient ROM and restores
  the BRR decoder's low bit before interpolation.

Mesen reports `spc.cycle` in 2.048 MHz half-cycle units. Extraction divides
the timestamps by two for the 1.024 MHz SPC domain, and the DSP emits one
sample per 32 cycles.

## Regression evidence

The supported dump path is:

```text
nba95_port.exe --headless --setup-only --rom <rom> --assets <pack> \
  --dump-audio <output.wav>
```

`tools/test_setup_transition.py` verifies:

- asset 91 contains 102,445 ordered APU writes;
- asset 93 contains 19,928 ordered DSP writes and no RIFF data;
- output format, duration, peak, spectral bands, stereo energy, and 125 ms
  onset/RMS windows;
- the normalized onset profile remains correlated with a compact fingerprint
  from the independent Mesen reference WAV (the raw WAV is ignored).

The prior APU/SPC replay scored 0.619 against the first ten seconds of that
Mesen onset fingerprint. The downstream DSP-driven runtime scores 0.892. This
test specifically rejects a return to the timing-drifted Setup sequence.
