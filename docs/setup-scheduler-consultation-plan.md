# Bounded scheduler consultation, 2026-08-31

The single authorized read-only Max consultation is complete. It resolved
the actual FB30 resource's byte decoding and component work accounting.
It did **not** demonstrate an end-to-end predictor of the four target epochs.
The remaining dependency is source-derived NMI audio/SPC continuation with
carried state and acknowledgement timing. Queue/wait leaf parity cannot close it.

Consultant task: `01a05634-5316-78c0-bb36-f9cdfd3b562e`.
No files, original-game state, native fixtures, or production source were
modified by the consultant. The messages in that task retain the full diagnosis.

## First implementation boundary: codecs and source work

These are work checksums, not constants to insert as atomic delays. The scope
is `$80:C62B` entry through `$80:C682` entry, excluding final RTL, refresh,
DMA and interrupts, with FastROM, D=0 and the observed empty immediate queue.

| Resource | Format | Output bytes | CPU cycles | Intrinsic master clocks |
| --- | --- | ---: | ---: | ---: |
| `$AE:A0AF` | FB30 | 960 | 115554 | 736896 |
| `$AE:C446` | FB46 | 960 | 34826 | 218142 |
| `$AE:D153` | FB46 | 2054 | 94738 | 602298 |
| `$A6:C5FC` | FB46 | 6336 | 246864 | 1556550 |
| `$AF:97AA` | FB46 | 838 | 32091 | 202290 |

The FB30 output SHA256 is
`0b5c2dafc1a19c277807c8d7c81425e8fb0c23623f694ee9574452f1492b3bd7`.
A separate semantic bitreader and bounded in-memory ROM execution agree with
all960 native bytes at `$7F:2000..23BF` in scheduler capture
`.analysis/native-interrupt-tail-v2/interrupt_26_{entry,exit}.wram`.
The entry/exit file hashes are
`b973ea9a99ce03adc9741b9f3669de9f4de5f4d152bbeb169013ea2b9cb54514` and
`fc4dcdaa6ea68dd3dcf1182fcb8cf2f0d790a67631c3a5d3be54665336c0a9c7`.

Source identities (canonical original ROM as documented elsewhere):

- `$80:BE6B..C5AA`,1856bytes:
  `e6ebb2723eed4c6564a6b289c247f4998176ff7c06d21f6a09c9ce8d15aea0cc`.
- `$80:C62B..C682`,88bytes:
  `ad561c575d19757d4e2fa41d6c267514e5957195eebb33c69b843807c9d3bc98`.
- `$AE:A0AF..A37D`,719bytes including lookahead:
  `c8e1e58135576ccb0f9b4de0d60d48565a9ecaf528c4dd53c70f06b8b19af340`.

FB30's escape is89hex; MSB bitstream starts at source+6. A leading1 plus two
bits codes0..3; k>=1 zeros before1 gives width=k+2 and value
`(1<<width)-4 + next_width_bits`. Code counts for lengths1..10 are
`[0,1,0,1,3,15,18,24,38,52]`, with152 cyclic ranks among unused byte symbols.
This resource has888 literal tokens, repeat-last runs31/15/11/15, then the
escape/zero/end flag. Native termination peeks at the1bit without consuming
it: retain native cursor and prefetch semantics, not merely equivalent output.

All four FB30 and sixteen FB46 conservation equations match. They use observed
NMI intervals and are not end-to-end predictions. Only two FB46 no-NMI forward
endpoints were independently predicted. The generic legacy decoder was not
promoted; its old10-million-step failure is not a cost oracle.

## Producer and clock continuation

The five codecs account for524073CPU cycles of the common550560 backdrop work.
Derive the remaining26487 routing/transform/helper/call cycles from source;
do not insert a residual delay. Header work adds440CPU cycles, two fixed-source
4096-byte fills and14palette bytes.

Use typed source continuations with real pointers, bit/rank/loop state and
completed effects. Emit ordered bus classes, source blocks and instruction
completion points. Preserve codebook construction, cyclic symbol search,
fast-table construction, refill and RLE work. A compact decoder proves bytes
but omits native work. Neither captured instruction playback nor a generic
opcode dispatcher is the native C implementation requested here.

Carry master-clock/PPU phase, refresh, memory speed, DMA alignment/progress,
pending NMI and producer/handler continuations across every page and dwell.
Resolve hardware reads at their source bus access. The vector JML, interrupt
entry and RTI add19CPU/142intrinsic master clocks omitted by handler hooks.
DMA has its own transfer/alignment cost. There is no proven phase-erasing
boundary; `$81:D010` returns after a variable NMI tail. `$80:86B0` preserves
caller M: header compares16bits, `$8959` compares8bits. Resume only after RTI.

## Open dependency: NMI audio and SPC acknowledgements

`$80:A137` must advance from normal initialization on every preceding NMI,
including ordinary menu dwell. Carry tempo `$0643/$0645`, voices, sequence
pointers and commands. Here tempo adds436 modulo531;630frames advances153,
matching the first/repeat state difference. Recorded DSP-event playback does
not provide CPU-side work or synchronous port responses.

First/repeat Rules audio costs255404/232674master clocks and controller
72430/72916. Audio difference22730 minus controller offset486 equals the
entire22244NMI difference. Only34extra failed polls account for1428intrinsic
clocks; successful checks also differ. A fixed tail or port-latency-only
adapter cannot explain it.

Whether the uploaded SPC consumer's timer/clock/port execution can be reduced
to a bounded continuation predicting acknowledgements is still unproved.
If source inspection cannot close it, capture natural SPC state plus timed
CPU/SPC writes/responses for validation. Never use those snapshots as runtime
initial state. Unexecuted streaming-upload polling paths need not be added.

## Acceptance boundary and original behavior

Separate source-work proof from native validation reports. Record ROM/input/
routine identities, M/X/D/DB/speed preconditions, source branches, ordered
effects and bus totals. Native reports attest capture identity and completion,
then show component matches and remaining differences. Production consumes
canonical resource bytes and live state, never visit numbers, recorded phases,
NMI durations, port traces or fitted offsets.

After the actual producers/audio/hardware are wired, unchanged natural input
must yield loaded epochs72/15/71/15 and after-wait73/16/72/16. Repeated Rules
loads only782master clocks before NMI, so an unproved audio approximation is
unsafe. Rerun full state/RGB/VRAM and preserve the known Custom-return
25-byte/30-write discrepancy. Add a separately captured natural idle variation
to falsify fitting.

Preserve original bugs and quirks with evidence comments beside the code:
caller-width waits, publication then epoch then controller/audio then RTI,
and split DMA must not be normalized to make a test pass. Confidence is high
for component work and the measured tail split; the missing audio/SPC model
and complete phase prediction remain unaccepted.
