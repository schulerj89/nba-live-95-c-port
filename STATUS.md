# Project status

Last updated 2026-08-25. This is a current-state handoff, not a milestone log.
Use Git history for older checkpoints and run
`python tools/progress.py --write docs/progress.md` for live measurements.

## Current state

The playable path runs from the Nintendo license through Player Setup,
starting-lineup cards, tip-off, and early CPU-vs-CPU gameplay. The port has
court/camera movement, ten-player composition, possession, passing, shooting,
bounces, and initial foul scaffolding. User-controlled gameplay is not in
scope yet.

Current measured coverage:

| metric | bytes | % of observed executed code |
|---|---:|---:|
| observed executed code | 27,901 | 100.0% |
| documented by ROM-address provenance | 8,492 | 30.4% |
| verified against live ROM calls | 1,040 | 3.73% |

These values are generated, not estimated. The detailed per-bank report is
`docs/progress.md`; the authoritative verified list and evidence paths are in
`docs/verified-routines.json`.

## Verified gameplay checkpoint

Sixteen routines currently pass emulator-ground-truth replay. The most important
recent slices are:

- `$86:E4A7-$E592`: mode-11 owner/dribble gates, proximity selection, facing,
  and unlatched pose 9/11 selection.
- `$86:BAA2-$BB14`: player catch state and CPU-owner mode installation.
- `$85:F347-$F3BA`: target distance/direction calculation.
- `$85:A82C-$AB16`: native actor velocity damping, acceleration, boost, and
  cap behavior across 2,000 captured calls.
- `$85:B402-$B4B8`: predictive arrival, steering, and the coupled velocity
  application across 1,000 captured calls.
- `$85:B4B9-$B5FE`: cutter cadence and pass-receiver priority/order across
  1,187 captured calls and both stable selector exits.
- `$85:B60B-$B677`: CPU pass-receiver candidate rejection and acceptance.
- `$85:B734-$B820`: CPU mode-11 shot policy and its ordered RNG consumption.
- `$85:F5E4-$F727`: opponent lane obstruction used by the cutter and mode-11
  shot branches, including its half-open rectangle edges.

The velocity replay exposed a ROM-specific negative damping bias: for this
routine, `-128` contributes `-7`, not normal C truncation's `-8`. The port now
matches all captured outputs. The 63,800-frame CPU-vs-CPU regression and its
visual anchors pass at this checkpoint.

The latest CPU decision-chain increment adds 151 observed-executed verified
bytes, raising ground-truth coverage from 3.19% to 3.73% (+0.54 percentage
points). Its three replays cover cutter timer decrement/reload, pass selection
and no-selection, shot acceptance/rejection, and every captured RNG transition
with zero mismatches. The cutter capture did not produce a `$09A2` value change;
that rare write is protected by direct self-tests and the long integration
regression but remains a useful target for a longer ROM capture.

Do not infer that a surrounding routine is verified from one verified slice.
Only ranges present in `docs/verified-routines.json` count as ground-truth
verified.

## Evidence rules

1. **Observed executed code** is the union of Mesen `exec_*.txt` captures in
   `.analysis/**`. Current captures emphasize gameplay, so title/menu execution
   is underrepresented in the denominator.
2. **Documented code** is the intersection of those addresses with
   `$XX:XXXX` provenance comments in `src/*.c`.
3. **Verified code** must have live Mesen entry/exit vectors replayed through
   the compiled C implementation with zero output mismatches, then be entered
   in `docs/verified-routines.json`.
4. **Regression coverage** (trace hashes, long simulations, and screenshots)
   protects integration behavior but does not by itself make a ROM routine
   ground-truth verified.

See `tools/README.md` for capture, replay, Ghidra, and regression commands.

## Active gaps and next work

- Capture enough calls to verify the rare latched owner/CPU pose branch at
  `$86:E4F5-$E544`; the existing six-call sample is insufficient.
- Continue converting small post-tip CPU decision/animation slices from the
  recomp and Ghidra, re-verifying after each increment.
- Expand ball physics, scoring, and CPU decisions beyond the early-gameplay
  paths currently exercised; keep fouls scaffolded until their native branches
  are captured and verified.
- Add full-session execution coverage so title and menu code participate in
  the denominator.
- Use `docs/progress.md` to select larger undocumented regions only when they
  advance the active gameplay path; raw byte count alone is not priority.

## Resume checklist

1. Confirm `main` is clean and current.
2. Regenerate `docs/progress.md` rather than copying numbers into a new note.
3. Read the active gap above and the corresponding Ghidra listing/recomp code.
4. Capture live vectors, implement the smallest complete branch, and replay
   every vector with zero mismatches.
5. Run the relevant subsystem test plus `tools/test_cpu_gameplay.py` for
   gameplay changes, inspect visual anchors, then update the verified ledger.
6. Commit and push each verified checkpoint to `main`.
