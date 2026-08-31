# Setup scheduler investigation — paused checkpoint

Investigation stopped at the user's request to finish up and commit current
work. No production scheduler edit or new timing workaround was made. The
existing Rules reentry and partial-DMA failures remain **FAIL**. The new
bounded native capture completed successfully before stopping. No isolated
scheduler implementation/probe or independent audit of this new contract
was completed; the proposal below is not an accepted runtime fix.

## Source-backed contract established

Original ROM, `gameplay100-closure-ghidra` bank80 listing, and the previously
generated `phase-probe/reference/bank80.c` were inspected directly.

- `$80:86B0-$86BE` preserves A/bank, loads the16-bit WRAM counter `$0564`,
  and spins until the current counter differs. It does **not** mean “wait
  one host frame.” A caller entering after the counter increment waits for
  the following increment.
- NMI enters `$80:815A`, uses `$05CB` as its reentry guard, publishes PPU
  state and queued resources, then increments `$0564` at `$80:84A8` only
  when `$059C=0`. The callback at `$05C2` and audio work execute after that
  increment and before `$80:859B` returns from interrupt. A blocked main
  caller resumes after the actual interrupt completes, not at the instant
  of increment.
- The NMI DMA ring uses direct-page `$35` read cursor and `$37` write
  cursor, eight-byte records at `$0100+cursor`, wrapping at `$01FF`.
  `$39` is the byte-like publication budget, initialized to `$1518` or
  `$16A8` according to `$07F0`. The NMI charges explicit per-job overhead
  and transfer length and leaves work queued when the budget is exhausted.
  Existing listings contain mixed-width decoding errors in some less-used
  record branches; those branches are not yet accepted translations.
- `$80:8A02` chooses immediate palette DMA from the sign of `$0562`, or
  queues palette fields `$0568/$056A/$056C/$056E`. `$80:8BA1` chooses
  immediate VRAM DMA from the sign of16-bit `$0561`, otherwise queues
  chunks of at most `$1000` bytes via `$8BD0-$8C2A`. These are distinct
  control tests and should not be replaced by an assumed generic flag.
- `$80:EEC6` clears the header destination, publishes its palette, then
  calls `$86B0` at `$EF1A`. The resource work can cross a native NMI before
  the counter load. This accounts for the observed extra epoch on the
  first opening without any visit-number rule.

## Fresh native evidence

New source files:
`tools/capture_setup_scheduler.py` and `tools/mesen_setup_scheduler.lua`.
The script supplies only the already-established natural controller pulses,
with labels rebased after actual Simulation/3min normalization. It observes
routine entries, loaded wait epochs, NMI boundaries, queue cursors/budget,
DMA submissions, master clock, CPU instruction cycles and selected full
WRAM snapshots. No CPU/RAM/PPU/ROM injection or timing manipulation occurs.

Capture directory:
`.analysis/setup-scheduler-20260830/native-v1/`.
The manifest attests the copied runner/script, canonical ROM, private Mesen,
initial settings and raw output. The launcher passes a private process
environment, validates Lua's actual output/home, empty initial saves,
persisted settings, explicit exit0 and the native completion guard.
`scheduler.jsonl` retains the chronological observations; this is raw
diagnostic evidence, not a production resource or a compact parity fixture.

| Header invocation | Entry label / scanline / epoch | Before wait | After wait |
| --- | --- | --- | --- |
| First Rules opening |541 /198 /71|542 /2 /72|543 /247 /73|
| First Main return |883 /160 /15|883 /212 /15|884 /246 /16|
| Repeated Rules opening |1171 /172 /71|1171 /224 /71|1172 /245 /72|
| Repeated Main return |1513 /39 /15|1513 /91 /15|1514 /245 /16|

These reproduce the prior producer trace's actual CPU-cycle values and
scanlines, and newly expose the changing counter. First entry to pre-wait
takes90,134 master clocks; repeated Rules takes70,526. The first interval
contains NMI execution. The corresponding CPU-cycle counts are3435 and440;
CPU instruction-cycle count alone omits DMA/stall time and cannot be used
as a master-clock work budget. The second return interval takes70,566
master clocks despite the same440 CPU cycles, so even the no-NMI path needs
bus/refresh phase accounting before claiming exact cycle prediction.

The capture includes four required header invocations and reaches the normal
Rules handler twice. Broad diagnostic hook tags beyond the source-backed
core above have not yet all been re-decoded or independently audited. No
claim is made that every collected diagnostic field is a complete queue or
bus-phase oracle. The retained full WRAM and immutable script make this
limitation inspectable.

## Smallest proposed portable mechanism, not yet implemented

Use a resource-task continuation with an explicit native frame-epoch counter
and waits that remember the epoch loaded at their call site. A publication
event processes the native queue/budget and existing callbacks in native
order, then resumes eligible continuations after interrupt work. Resource
producers must choose immediate versus queued publication from their actual
state and account for their real work before entering a wait.

This event contract can avoid emulating arbitrary instructions, but it
still needs an independently derived cost/phase model for the particular
translated producers, decompressor, direct DMA and intervening NMI/audio
work. It must predict which side of the increment each wait reaches. Merely
feeding captured entry times to it would test the wait contract, not solve
production scheduling. No empirical “second visit skips a frame,” added
fade/delay, or offset-fitting rule is justified by this evidence.

Next work, intentionally not started after the stop request: validate every
selected hook and raw event schema; derive producer work from decoded
instructions and actual transfer/decompression operands; build a strict
isolated wait/queue probe; compare all first/repeated opening and return
epochs; then independently audit before any runtime integration. The
unresolved25-byte partial clear and second-opening timing failures remain
recorded in `docs/rules-reentry-resource-audit.md`.

Reproduction, when work resumes:

```powershell
python tools/capture_setup_scheduler.py --output '<new-directory>' --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc'
```

Keep raw captures, snapshots, private executables and ROM-derived resources
under ignored `.analysis`; commit only the two source tools and this report.
