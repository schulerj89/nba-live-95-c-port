# Native backdrop interrupt work attribution

Date: 2026-08-31. New diagnostic work after the scheduler primitive freeze;
none of the frozen primitive, verifier, builder or documentation files were
modified. This is observed source-work attribution, not a production timing
fix or a completed audio/controller implementation.

The natural journey now has46 backdrop NMI intervals with67247 instruction
observations,9044 hardware reads, and92 full128KiB WRAM entry/exit snapshots.
All7102 scheduler events and old field values/timestamps equal the previous
native-scheduler-v3 capture. The private Mesen launch completed exit0 with
the normal capture sentinel and a separate interrupt completion sentinel.

The new bank80 instruction callback starts at `$80:815A` and ends before
`$80:859B` RTI. It records PC, M/X flags, A/X/Y, D/DB/S, master clock,
CPU cycle count, epoch and phase. Source/ROM hashes, initial settings,
private home/save directory and observed environment are attested by the
capture manifest. Original natural controller input is supplied by an
unchanged, separately copied/attested scheduler script. No state or timing
injection occurs.

## Exact cost split

The boundaries are native caller/return points: controller `$84C8` to
`$84CC`, callback `$8553` to `$8556`, audio `$857A` to `$857D`, and NMI
`$815A` to `$859B`. They include each stage's actual call/return overhead.

| Natural backdrop | NMIs | Entry-to-epoch master | Controller master | Callback master | Audio master | Total logged NMI master |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
|First Rules|11|33528|72430|924|255404|375808|
|First Main return|12|41512|77694|1008|298390|433268|
|Repeated Rules|11|33528|72916|924|232674|353564|
|Repeated Main return|12|41552|77262|1008|215810|350296|

For the two Rules calls, audio decreases22730 master clocks and controller
work increases486, exactly accounting for the22244 total difference.
Every Rules entry-to-epoch interval is3048 master clocks. Both callback
totals are924. The larger variable component is the audio-driver work;
these sums are outcomes of native execution, not constants to put in C.

The native `$FFEA` vector targets `$8156: JML $80815A`. Logged NMI
boundaries omit8 CPU cycles of interrupt entry,4 of that JML, and7 of RTI.
Subtracting logged NMI work **and these19 CPU cycles per interrupt** from
backdrop-to-header CPU cycle differences yields exactly550560 producer
cycles in all four calls. This19 must never be multiplied by a single bus
speed and called exact master time. Full bus/refresh phase is still needed.

## Audio and controller hardware observations

`$80:CB8F` polls `$4212` for automatic controller reading to complete, then
reads the controller registers. `$80:A137` calls the sound driver, whose
handshake helpers include `$AA7B/$AA7E` and `$AAE6/$AAE9` busy loops. Their
original behavior is preserved; polling variation is not a bug to remove.

| Rules visit | `$4212` reads | `$2140` reads | Nonzero `$2140` reads |
| --- | ---: | ---: | ---: |
|First|225|1481|356|
|Repeated|234|1429|242|

All442 captured `$AA7B` entries and436 `$AAE6` entries use **DB=$82** while
executing in PB=$80. The first tail observer hooked only banks00/80, so its
SPC read list was incomplete. Tail-v2 adds the `$82` mirror. All instruction
and boundary files, all92 WRAM snapshots, and all2641 old hardware reads
are unchanged; the new mirror adds6403 actual SPC reads. Tail-v1 remains
valid for those explicitly preserved observations, not for an assertion
that audio performed no hardware reads.

The hardware callback records the observed bus address/value and current
CPU PC. That PC may already point past fetched operands; use the surrounding
instruction records and timestamps to identify the owning instruction.
Do not relabel every callback PC as an instruction-entry boundary.

## Artifacts and reproduction

All paths below are relative to the isolated scheduler worktree
`C:/Users/joshs/Projects/nba-live-95-c-port/.analysis/worktrees/completion-scheduler`.

| Artifact | SHA256 |
| --- | --- |
|.analysis/native-interrupt-tail-v2/manifest.json|7dc7312302a40001f75de5deba73bf73ff1b3406de4b5055186aecda0e2224c0|
|.analysis/native-interrupt-tail-v2/interrupt_instructions.jsonl|433500271b991f5d9db5d0ace2b1b604d5ba7c92a1f7ee778122eddfa5ffd156|
|.analysis/native-interrupt-tail-v2/interrupt_boundaries.jsonl|1f4cb283b2fbd80fc3abf66cb4be2c14b7de7815de768388d60f13f0b25c67c8|
|.analysis/native-interrupt-tail-v2/interrupt_bus.jsonl|983f4f00668022cecd76666428a3f5a3947e9e02e5c9a31ad2aa24eee2595f28|
|.analysis/interrupt-tail-attribution-v1.json|6d6ac5eb50c5bc65764ea6ec5649555618df3fd1177bf79551dbd13a19319591|

The manifest attests each `interrupt_01_entry.wram` through
`interrupt_46_exit.wram` and the copied scripts. These remain ignored
private evidence; they are not production resource assets.

```powershell
python tools/capture_setup_interrupt_tail.py --output .analysis/native-interrupt-tail-new --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --mesen 'C:/Users/joshs/AppData/Local/Microsoft/WinGet/Packages/SourMesen.Mesen2_Microsoft.Winget.Source_8wekyb3d8bbwe/Mesen.exe'
python tools/analyze_setup_interrupt_tail.py --native .analysis/native-interrupt-tail-new --scheduler .analysis/native-scheduler-v3 --previous-tail .analysis/native-interrupt-tail-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --report .analysis/interrupt-tail-attribution-new.json
```

Stop on any unsuccessful native command. Output directories/reports must
be new. The attribution tool validates capture manifests/bytes, instruction
and event ordering,46 complete NMI boundary sets, source vector bytes,
unchanged old observations and snapshot identities. Its interval sums are
computed from raw clocks/cycles, not a table of fitted timings.

## Limits and next check

Instruction observation is limited to bank80. All recorded direct JSL/JML
targets are `$80:CB8F`, `$80:CE33`, and `$80:9FA1`; no adjacent instruction
CPU-cycle gap exceeds12. This is useful continuity evidence, not proof of
a general instruction decoder or full arbitrary interrupt closure. Captured
WRAM is not a complete SPC/APU internal-state capture. Hardware **reads**
are logged; writes are currently visible only through native instruction
operands/registers. The data can drive a bounded translation test, but
captured bus responses cannot become a production command/timing playback.

The next implementation contract must derive producer execution, controller
read completion and live CPU/SPC handshake timing from runtime state. The
current `nba_audio.c` Setup track plays recorded DSP events and does not
produce native `$80:A137` instruction/handshake work. Replacing that timing
with these observed totals would retain the same capture-schedule defect.
The consultant and owner should use these exact traces to identify the
smallest source-derived continuation and any missing SPC state capture.
Repeated Rules entry remains uncorrected and the game remains incomplete.
