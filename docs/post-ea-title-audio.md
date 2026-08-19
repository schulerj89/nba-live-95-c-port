# Post-EA title: Ghidra/audio notes

## What the original ROM does

The SNES version does not stream a finished song. `$82:AC0E` selects the post-EA
resource list at `$82:A9D1/$82:AB69`. `$82:ABE0` walks that list and
`$82:ABCE -> $80:9829` transfers the selected sequence, instrument, and BRR
sample resources through APU ports `$2140-$2143` into SPC RAM. The resident
SPC700 sound driver sequences those short samples on the SNES audio voices.

The title visuals are synchronized by cue values delivered to CPU RAM `$064A`.
`$80:E381` consumes and clears each cue. The following blocks perform the
observed build:

- `$80:E391`: cue 1/7, N stage
- `$80:E3DB`: cue 2/8, B stage
- `$80:E432`: cue 3/9, A stage
- `$80:E489`: cue 4/10/11, LIVE stage
- `$80:E4E0`: cue 5, 95 stage
- `$80:E548`: cue 6, enable the lights around 95
- `$80:E8D9`: every eight frames, rotate three CGRAM colors used by the lights

`$87:8000/$87:80CB` belong to the timeout-to-attract transition. They do not
construct the initial title letters.

## What the current C port does

Asset 15 is currently one 20-second, 22,050 Hz, mono PCM WAV (882,078 bytes)
captured from Mesen's final SPC mix. It preserves the audible song but does not
yet reproduce the ROM's sample bank, sequencer, or music-cue channel.

The extra entries shown by the port's F11 debugger are 52 standalone WAV previews
made by decoding candidate BRR offsets. They are useful for identifying source
instruments/effects, but the F11 list is not a playlist and the port does not
currently assemble those entries into the title song.

The renderer currently crossfades three captured title keyframes. A faithful
implementation should replace that with the six `$064A`-driven construction
stages above and time credits from the same music cue/event stream.
