# Game Setup audio — ROM facts and implementation

Game Setup uses a sequenced SNES track, not streamed PCM. Its streamed S-DSP
directory is at ARAM `$0200` and contains 30 BRR sources (`SRCN $00-$1D`).

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

The transition-origin ARAM snapshot still contains the outgoing title bank.
During forced blank the ROM uploads Setup's new directory and BRR payloads.
Asset 88 therefore stores the 64 KiB ARAM bank captured immediately before the
first Setup `KON`, when all 30 directory entries are valid. Asset 89 preserves
the initial DSP state and asset 93 supplies the cycle-timed writes.

`nba_audio_play_setup_dsp` decodes, interpolates, envelopes, pans, and mixes
those BRR instruments into 32 kHz stereo PCM at runtime. Assets 94-123 expose
the same 30 sources as individually auditionable WAV views in the F11 debugger;
their metadata records `SRCN`, BRR start, and loop addresses. These debugger
views are not used by Setup playback.

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
- F11 assets 94-123 match all 30 `$0200` directory start/loop pointers;
- output format, duration, peak, spectral bands, stereo energy, and 125 ms
  onset/RMS windows;
- the normalized onset profile remains correlated with a compact fingerprint
  from the independent Mesen reference WAV (the raw WAV is ignored).

The prior APU/SPC replay scored 0.619 against the first ten seconds of that
Mesen onset fingerprint. DSP replay with the stale pre-upload title bank scored
0.892. Using the first-`KON` Setup bank scores 0.978 with normalized envelope
error 0.060. The regression requires at least 0.97 correlation and rejects both
the timing-drifted sequence and the wrong-bank shortcut.
