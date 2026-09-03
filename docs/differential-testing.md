# Strict gameplay differential testing

Implemented 2026-08-28: **phase 1, a partial-state baseline gate and real
actor-sweep trace comparison**. It is not yet synchronized whole-game lockstep.
The first implementation trial is retained in Git history. The ball
initializer now has a same-entry, full-WRAM bounded comparison and runtime
binding. Its correction removes one of the default baseline mismatches
(82->81); the historical phase1 results below are retained as before-evidence.
The native ROM runs in Mesen; the C probe runs the actual port update function.
They produce separate bounded traces, then an offline comparator reports the
first differing checkpoint. It does not currently pause both programs together.

## Run

From the repository root, build the port and probe, then choose a NEW output
directory. A stale capture is never silently reused.

```powershell
./build.ps1 -RomPath 'F:/Games/SNES/NBA Live 95 (USA).sfc'
./tools/build_vector_probe.ps1 -Name differential_runtime_probe
python tools/run_differential.py --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --pack build/nba95_assets.pak --output .analysis/differential-new-run --sweeps 12
```

The default preserves the ROM's CPU-versus-human setup, as requested. Add
`--controllers cpu-vs-cpu` for an optional CPU-only diagnostic. The port still
runs CPU-only; default-mode controller mismatch is therefore explicitly
reported, not patched out or mistaken for an AI defect.

Mesen.exe must be on PATH, or supplied with `--mesen`. The runner launches it
headlessly and terminates after the bounded capture. No desktop automation is
needed. The pack/ROM identity is checked by the existing port loader first.
Rebuild the probe whenever the C sources or shared field schema change.

Outputs:

- `rom.jsonl`, `port.jsonl`: baseline plus one begin/end pair per actual sweep.
- `report.json`: first mismatch, raw values, WRAM addresses, observed ROM PCs,
  and separate video-frame timing differences.
- `baseline.wram`: complete 128-KiB native starting WRAM for later diagnosis and
  bootstrap work. This is evidence, not an asset and not loaded by the port.
- `run.json`: ROM, pack, executable, script, schema and trace hashes; Git state;
  explicitly disclosed launch setup. Logs and completion/error sentinels are
  retained alongside it.

## Exact contract and boundaries

`tools/differential_fields.def` is the shared C/Python schema: **449 unsigned
16-bit words**, comprising 49 global/ball words and 40 words for each of ten
actors. All required keys must exist. No common-key intersection, sentinel
filtering, coordinate rounding, frame-offset search, or state repair is used.
The C camera no-team byte `FF` is explicitly represented as native word `FFFF`;
this is a representation mapping, not a skipped sentinel.

| Checkpoint | Native reference | C reference |
|---|---|---|
| `baseline` | First `$87:A47A` on-court draw entry | Immediately after `nba_tipoff_init` |
| `actors.begin` | `$87:8EFB`, before initializing the ten-actor sweep context | Actual entry to the due `cpu_update_all_actors` sweep |
| `actors.end` | `$87:8F95`, after ten actors and before the next global/ball calls | Actual exit from that same sweep |

These are candidate correspondence boundaries, not a claim that initialization
and every operation between them already match. The C wrapper currently does
additional work in a different order. Missing/reordered/duplicated checkpoints
are errors; we do not fabricate begin/end events from even-numbered frames.

Compared fields include RNG; camera coordinates/mode; game and shot clocks;
ownership, receiver and pass state; tip/live/inbound state; play dispatch words;
free-throw state, attempts, aim axes, accumulator, rating step and start tick;
ball coordinates/velocity; all ten actors' coordinates/velocity, controller,
status, animation resources/phases/locks, facing, modes, assignments and selected
timers. The schema names every included address.

Native coordinate fractional words are kept intact. The C 24.8 coordinates
are expanded into integer/fraction words; a native fractional low byte that
C cannot represent causes a mismatch, not a tolerance-based pass.

The actor `+$60` word is deliberately NOT mapped yet: its C
representations are split across mode-dependent timers. Scores, full team
contexts, roster statistics, the remaining clock/foul state, controller latches, full
camera state, renderer/PPU/APU state and other RAM are not fully represented.
An equal projection must never be called whole-game equivalence.

NMI may split a native sweep across two video frames. Comparisons use exact
logical checkpoint order; different outer-frame coordinates are reported
separately, never corrected with a fitted offset. Timing parity is unproven
even if projected state matches.

The native `writers` map records the CPU PC observed by Mesen's write callback.
The report calls it `last_write_observed_pc`, not the exact store instruction.
Ghidra confirms that this Mesen version can report the PC AFTER the store:
observed `$86:E331` corresponds to `STA $16,Y` at `$86:E32E`. Resolve that
observed PC against disassembly before attributing a mismatch to a statement.
It identifies a candidate writer, not necessarily the root cause; an earlier
bad input or an incorrectly mapped boundary can cause the same mismatch.

## Launch setup and limits

These scenarios use Chicago (3) / Orlando (18) and neutral inputs. The native
default preserves one human assignment; optional CPU-only mode verifies there
are none. Both capture implementations apply neutral commands during gameplay;
the `inputs` array records those requested masks, not a proof that every native
input edge/repeat latch has been synchronized. Arbitrary input movies are not
supported yet.

The native menu driver selects Exhibition and writes the team selections at
`$82:8553` before commit. Only in optional CPU-only mode does it set all five
Player Setup selections to neutral value 1 at their actual consumer
`$86:E285`, before the first draw. It asserts the selected teams and requested
controller mode at baseline; CPU-only requires all ten controller words `FFFF`.
No gameplay memory is written after that baseline. RNG, coordinates, velocities
and other runtime state are never overwritten to make a comparison pass.

Why the optional CPU-only check matters: the older generic capture driver cleared transient
controller words near the roster load. `$86:E285/$E2D2` subsequently reads
`$166D + pad*2`; branches at `$E29C/$E2D9` skip assignment for neutral value 1.
Otherwise `$E32E` can assign actor `+$16` again. An initial test found that human
assignment, which the user confirmed was intentional. Default mode preserves
it. The optional CPU-only mode configures the actual selection input and
verifies the outcome. The existing routine-vector driver has not been changed;
historical captures should be classified by their recorded controller state,
not automatically treated as CPU-only because a flag was requested.

All other setup defaults, full starting state, emulator RAM/persistence settings
and RNG history are **not synchronized yet**. The native first-draw boundary
and C post-init boundary can also differ in render preparation. Therefore a
baseline mismatch is a blocker to a fair trajectory comparison, not proof that
each differing C field is independently wrong.

## Results and exit status

| Result | Meaning | Exit |
|---|---|---:|
| `INITIAL_STATE_MISMATCH` | Starting projected state differs; do not diagnose later trajectory as equivalent inputs | 1 |
| `DIVERGENCE` | Baseline matched, but a later projected checkpoint differs | 1 |
| `INVALID_CAPTURE` | Incomplete, malformed, wrong setup or failed capture | 2 |
| `PROJECTION_MATCH` | Standalone comparator only: selected state matches at all supplied checkpoints | 0 |
| `PROJECTION_MATCH_CONTEXT_UNPROVEN` | Runner: selected state matches, but full launch/state context is still unproven | 2 |

The runner deliberately cannot produce an overall equivalence PASS in phase 1.
The standalone comparator is a trace-analysis utility, not a source-authenticity
certificate. Use the fresh runner and retain its hashes for ROM evidence.

Fresh 25-sweep CPU-only run
`.analysis/differential-release-final-cpu`: 51 trace rows on each side,
449 words per row, **62 differences at baseline** and zero matching
checkpoints. All ten native controllers stay `FFFF`. Examples include RNG
native `0000` vs C `9146`, game clock `A8C0` vs `2A30`, shot clock `05A0` vs
`0000`, and play code `0001` vs `0000`. The first native actor sweep is at
relative video frame 25 while the first C sweep is at frame 2. The harness
reports that cadence mismatch without fitting an offset.

The probe was rebuilt against the final ball-history/edge-contract and
generic-contact-gate objects. The report and both projected traces are byte-
identical to `.analysis/differential-native-edges-final-cpu` in this short
window, but the fresh full native WRAM snapshot differs outside the projection.
That is not a whole-state match. `run.json` and `report.json` retain
commands, ROM/tool/executable/trace hashes and the 50 checkpoint timing
differences. Neither initial-state repair nor a permissive PASS was added.

The matching default run
`.analysis/differential-human-ft-20260829-human-v2` preserves one native human
assignment and reports **63 baseline differences**. The single additional
difference is actor 0 controller `0000` in the ROM versus `FFFF` in the current
CPU-only C runtime. The bounded human free-throw helper added in this checkpoint
does not create a complete menu-to-game human ownership path. Both runs remain
`INITIAL_STATE_MISMATCH`; their later rows are diagnostic only.

Historical v5/v6/v7 runs used the 442-word schema and reported 81/82 baseline
differences plus 21,776 differing bytes outside the projection. They remain
useful artifacts but are superseded for current field counts and baseline
values. A reproducible complete starting context is still pending.

## Tests and next increment

`python tools/test_differential.py` tests first-difference selection, malformed
records, missing/extra fields/checkpoints, real sentinel values, fractional low
bits, neutral input typing, duplicate keys, observed writer metadata, disclosed
NMI frame differences, and CLI exit/report behavior. These are synthetic unit
tests, NOT ROM-equivalence evidence.

`differential_observer_probe.c` runs observed and unobserved C simulations for
2,000 updates and compares the entire game/session state each update. It checks
1,000 actual paired sweeps on the current build without changing gameplay.
Both tests are part of `build.ps1 -Test`. The native edge-contract work also
has separate multi-team, court and 63,800-frame tip-flow checks; their current
release results are recorded in `STATUS.md`. A passing C-only regression
never overrides this harness's initial-state failure. No coverage credit is
added for implementing the harness.

Next, in order:

1. Establish a reproducible native starting context and the same logical start
   point, full configuration, controller mode, roster/order,
   RNG state and relevant latches. Implement a test-only snapshot bootstrap
   for a complete mapped dependency set; validate round-trip equality. Keep a
   separate natural-start test so import does not conceal initialization bugs.
2. Resolve the first baseline differences with Ghidra/recomp, then align the
   scheduler and exact native update order. Do not mask differences or choose
   convenient frame offsets. Retain a failing reproduction for each gap.
3. Expand checkpoint/state coverage to actor dispatch, ball integration,
   acquisition, camera update and clocks. This localizes the first differing
   phase after a genuinely matching baseline.
4. Add deterministic input movies and initial-state/configuration hashes;
   broaden scenarios and seeds. Only then claim a bounded behavioral-
   equivalence PASS, stating the exact represented state and time interval.

Existing per-routine Mesen replay tests remain valuable: after a divergence is
localized, they can distinguish a bad translated helper from a bad caller,
state mapping or schedule. They complement this harness; they do not replace it.
